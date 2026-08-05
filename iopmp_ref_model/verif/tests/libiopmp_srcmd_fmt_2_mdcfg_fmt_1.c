/***************************************************************************
// libiopmp tests for the SRCMD_FMT=2 & MDCFG_FMT=1 model.
//
// SRCMD_EN(H) and the SPS registers are replaced by SRCMD_PERM(H), and there
// is no physical MDCFG table: each MD owns HWCFG3.md_entry_num+1 entries. The
// K=1 form (md_entry_num=0) gives every entry its own MD, so SRCMD_PERM(H) has
// to be written together with the entry.
***************************************************************************/

#include "iopmp.h"
#include "config.h"
#include "test_utils.h"

#include "libiopmp.h"

// Create IOPMP instance
iopmp_dev_t iopmp_dev = {0};
iopmp_cfg_t cfg = {0};

/* Override libiopmp IO functions */
uint32_t io_read32(uintptr_t addr)
{
    return read_register(&iopmp_dev, addr, 4);
}

void io_write32(uintptr_t addr, uint32_t val)
{
    return write_register(&iopmp_dev, addr, val, 4);
}

/* Reset the reference model with @cfg, then initialize @iopmp on top of it.
 * Returns IOPMP_ERR_NOT_AVAILABLE if @cfg is an illegal configuration, so that
 * one never leaves the model silently holding its previous state. */
static enum iopmp_error libiopmp_setup(IOPMP_t *iopmp, iopmp_cfg_t *cfg)
{
    if (reset_iopmp(&iopmp_dev, cfg) != 0) {
        return IOPMP_ERR_NOT_AVAILABLE;
    }

    memset(iopmp, 0, sizeof(*iopmp));
    return iopmp_init(iopmp, 0, cfg->srcmd_fmt, cfg->mdcfg_fmt,
                      IOPMP_IMPID_NOT_SPECIFIED);
}

#define CFG_MD_NUM      63
#define CFG_ENTRY_NUM   512
#define CFG_RRID_NUM    32

/* SRCMD_PERM(m) and SRCMD_PERMH(m) as the reference model lays them out */
#define SRCMD_PERM_ADDR(m)  (SRCMD_TABLE_BASE_OFFSET + (m) * SRCMD_REG_STRIDE)
#define SRCMD_PERMH_ADDR(m) (SRCMD_PERM_ADDR(m) + 4)

static uint64_t read_srcmd_perm_64(uint32_t mdidx)
{
    uint64_t lo = read_register(&iopmp_dev, SRCMD_PERM_ADDR(mdidx), 4);
    uint64_t hi = read_register(&iopmp_dev, SRCMD_PERMH_ADDR(mdidx), 4);

    return (hi << 32) | lo;
}

/* SRCMD_EN(H) belongs to SRCMD_FMT=0, so libiopmp has to refuse these before
 * it reaches the NULL operations this model leaves behind. */
static int check_srcmd_fmt_0_apis_refused(IOPMP_t *iopmp)
{
    uint64_t mds = 0x1;

    if (iopmp_set_rrid_md_association(iopmp, 0, 0x1, 0, &mds, false) !=
        IOPMP_ERR_NOT_SUPPORTED) {
        return -1;
    }
    if (iopmp_lock_srcmd_table_fmt_0(iopmp, 0) != IOPMP_ERR_NOT_SUPPORTED) {
        return -1;
    }

    return 0;
}

int main(void)
{
    IOPMP_t iopmp = {0};
    struct iopmp_entry entry = {0};
    struct iopmp_entry rb_entry = {0};
    IOPMP_SRCMD_PERM_CFG_t perm_cfg;
    uint32_t val_u32, val_u32_2;
    uint64_t val_u64;
    bool val_bool, val_bool_2;

    // Configure your IOPMP when reset
    cfg.vendor = 1;
    cfg.specver = 1;
    cfg.impid = 0;
    cfg.no_err_rec = false;
    cfg.md_num = CFG_MD_NUM;
    cfg.addrh_en = true;
    cfg.tor_en = true;
    cfg.rrid_num = CFG_RRID_NUM;
    cfg.entry_num = CFG_ENTRY_NUM;
    cfg.prio_entry = 16;
    cfg.prio_ent_prog = false;
    cfg.non_prio_en = true;
    cfg.msi_en = true;
    cfg.peis = true;
    cfg.pees = true;
    /* SRCMD_FMT=2 replaces the SPS registers with SRCMD_PERM(H) */
    cfg.sps_en = false;
    cfg.stall_en = true;
    cfg.mfr_en = true;
    cfg.mdcfg_fmt = 1;
    cfg.srcmd_fmt = 2;
    cfg.md_entry_num = 3;   /* K=4 */
    cfg.xinr = false;
    cfg.no_x = false;
    cfg.no_w = false;
    cfg.rrid_transl_en = true;
    cfg.rrid_transl_prog = false;
    cfg.rrid_transl = 48;
    cfg.entryoffset = 0x2000;
    cfg.granularity = MIN_GRANULARITY;
    cfg.imp_mdlck = true;
    cfg.imp_err_reqid_eid = true;
    cfg.imp_rridscp = true;
    cfg.imp_stall_buffer = true;
    cfg.stall_buffer_size = 32;

    /* Start unit tests */

#if (SRC_ENFORCEMENT_EN == 0)
    START_TEST("Initialize SRCMD_FMT=2 & MDCFG_FMT=1");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_srcmd_fmt(&iopmp) != IOPMP_SRCMD_FMT_2);
    FAIL_IF(iopmp_get_mdcfg_fmt(&iopmp) != IOPMP_MDCFG_FMT_1);
    FAIL_IF(iopmp_get_md_entry_num(&iopmp, &val_u32) != IOPMP_OK);
    FAIL_IF(val_u32 != 3);
    END_TEST();

    START_TEST("SRCMD_FMT=2 implicitly associates every MD with every RRID");
    FAIL_IF(iopmp_get_rrid_md_association(&iopmp, 1, &val_u64,
                                          &val_bool) != IOPMP_OK);
    FAIL_IF(val_u64 != (((uint64_t)0x1 << CFG_MD_NUM) - 1));
    FAIL_IF(val_bool != true);
    END_TEST();

    START_TEST("Each MD owns md_entry_num+1 entries");
    /* MDCFG_FMT=1 has no MDCFG table: MD m owns entries [m*K, m*K+K-1] */
    for (uint32_t m = 0; m < 4; m++) {
        FAIL_IF(iopmp_get_md_entry_association(&iopmp, m, &val_u32,
                                               &val_u32_2) != IOPMP_OK);
        FAIL_IF(val_u32 != m * 4);
        FAIL_IF(val_u32_2 != 4);
    }
    END_TEST();

    START_TEST("Set and get a single RRID's permission to a MD");
    val_bool = true;    /* r */
    val_bool_2 = false; /* w */
    FAIL_IF(iopmp_set_md_permission(&iopmp, 3, 1, &val_bool,
                                    &val_bool_2) != IOPMP_OK);
    FAIL_IF(val_bool != true || val_bool_2 != false);
    /* RRID 3 occupies SRCMD_PERM(1) bits 7:6 */
    FAIL_IF(read_srcmd_perm_64(1) != ((uint64_t)0x1 << 6));
    END_TEST();

    START_TEST("Set every RRID's permission to a MD at once");
    IOPMP_SRCMD_PERM_CFG_SET_DIRECT(&perm_cfg, UINT64_MAX, 0x3);
    FAIL_IF(iopmp_set_md_permission_multi(&iopmp, 2, &perm_cfg) != IOPMP_OK);
    FAIL_IF(read_srcmd_perm_64(2) != 0x3);
    END_TEST();

    START_TEST("Locking a MD's SRCMD table sets its MDLCK bit");
    FAIL_IF(iopmp_is_srcmd_table_fmt_2_locked(&iopmp, 4, &val_bool) !=
            IOPMP_OK);
    FAIL_IF(val_bool != false);
    FAIL_IF(iopmp_lock_srcmd_table_fmt_2(&iopmp, 4) != IOPMP_OK);
    FAIL_IF(iopmp_is_srcmd_table_fmt_2_locked(&iopmp, 4, &val_bool) !=
            IOPMP_OK);
    FAIL_IF(val_bool != true);
    /* A locked SRCMD_PERM(H) no longer takes a permission */
    val_bool = true;
    val_bool_2 = true;
    FAIL_IF(iopmp_set_md_permission(&iopmp, 0, 4, &val_bool, &val_bool_2) !=
            IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Lock MDs through MDLCK");
    /* Locking MD 4's SRCMD table above already set its MDLCK bit */
    val_u64 = 0x1;
    FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, false) != IOPMP_OK);
    FAIL_IF(val_u64 != ((uint64_t)0x1 | ((uint64_t)0x1 << 4)));
    END_TEST();

    START_TEST("SRCMD_EN(H) APIs are refused");
    FAIL_IF(check_srcmd_fmt_0_apis_refused(&iopmp) != 0);
    END_TEST();

    START_TEST("There is no MDCFG table to program or lock");
    val_u32 = 1;
    FAIL_IF(iopmp_lock_mdcfg(&iopmp, &val_u32, false) !=
            IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_set_md_entry_association(&iopmp, 0, &val_u32) !=
            IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("HWCFG3.md_entry_num is fixed outside MDCFG_FMT=2");
    val_u32 = 7;
    FAIL_IF(iopmp_set_md_entry_num(&iopmp, &val_u32) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("Only K=1 takes entries carrying a permission");
    FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, &entry, 0, 1) !=
            IOPMP_ERR_NOT_SUPPORTED);
    END_TEST();

    START_TEST("K>1 writes the entry without touching SRCMD_PERM(H)");
    entry.addr = 0x80000000 >> 2;
    entry.cfg = IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R;
    entry.private_data = 0x3;   /* must be ignored when K>1 */
    FAIL_IF(read_srcmd_perm_64(8) != 0);
    FAIL_IF(iopmp_set_entry(&iopmp, &entry, 8) != IOPMP_OK);
    FAIL_IF(iopmp_get_entry(&iopmp, &rb_entry, 8) != IOPMP_OK);
    FAIL_IF(rb_entry.addr != entry.addr || rb_entry.cfg != entry.cfg);
    /* Only the K=1 override writes private_data, and it indexes by entry */
    FAIL_IF(read_srcmd_perm_64(8) != 0);
    END_TEST();

    START_TEST("K=1 gives every entry its own MD");
    cfg.md_entry_num = 0;   /* K=1 */
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_md_entry_num(&iopmp, &val_u32) != IOPMP_OK);
    FAIL_IF(val_u32 != 0);
    for (uint32_t m = 0; m < 4; m++) {
        FAIL_IF(iopmp_get_md_entry_association(&iopmp, m, &val_u32,
                                               &val_u32_2) != IOPMP_OK);
        FAIL_IF(val_u32 != m);
        FAIL_IF(val_u32_2 != 1);
    }
    END_TEST();

    START_TEST("K=1 writes SRCMD_PERM(H) together with the entry");
    entry.addr = 0x90000000 >> 2;
    entry.cfg = IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R | IOPMP_ENTRY_W;
    /* RRID 0 gets read and write, RRID 1 gets read only */
    entry.private_data = 0x3 | ((uint64_t)0x1 << 2);
    FAIL_IF(read_srcmd_perm_64(5) != 0);
    FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, &entry, 5, 1)
            != IOPMP_OK);
    FAIL_IF(read_srcmd_perm_64(5) != entry.private_data);
    FAIL_IF(iopmp_get_entry(&iopmp, &rb_entry, 5) != IOPMP_OK);
    FAIL_IF(rb_entry.addr != entry.addr || rb_entry.cfg != entry.cfg);
    /* The plain entry API leaves SRCMD_PERM(H) where it is */
    entry.private_data = 0;
    FAIL_IF(iopmp_set_entry(&iopmp, &entry, 5) != IOPMP_OK);
    FAIL_IF(read_srcmd_perm_64(5) != (0x3 | ((uint64_t)0x1 << 2)));
    END_TEST();

    START_TEST("K=1 writes SRCMD_PERM(H) for every entry of a range");
    struct iopmp_entry entries[3] = {0};
    for (uint32_t i = 0; i < 3; i++) {
        entries[i].addr = (0xA0000000 + (i << 12)) >> 2;
        entries[i].cfg = IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R;
        entries[i].private_data = 0x1 << (i << 1);
    }
    FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, entries, 16, 3)
            != IOPMP_OK);
    for (uint32_t i = 0; i < 3; i++) {
        FAIL_IF(read_srcmd_perm_64(16 + i) != entries[i].private_data);
    }
    END_TEST();

    START_TEST("A refused range leaves SRCMD_PERM(H) untouched");
    {
        struct iopmp_entry bad = {0};
        uint32_t idx = 40;

        bad.addr = 0xB0000000 >> 2;
        bad.cfg = IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R;
        bad.private_data = 0x3;

        /* Past the last MD: entry 40 exists but MD 63 and beyond do not */
        FAIL_IF(read_srcmd_perm_64(CFG_MD_NUM - 1) != 0);
        FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, &bad,
                                                     CFG_MD_NUM - 1, 2) !=
                IOPMP_ERR_OUT_OF_BOUNDS);
        FAIL_IF(read_srcmd_perm_64(CFG_MD_NUM - 1) != 0);

        /* Locked by MDLCK */
        val_u64 = (uint64_t)0x1 << idx;
        FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, false) != IOPMP_OK);
        FAIL_IF(read_srcmd_perm_64(idx) != 0);
        FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, &bad, idx, 1) !=
                IOPMP_ERR_REG_IS_LOCKED);
        FAIL_IF(read_srcmd_perm_64(idx) != 0);

        /* Locked by ENTRYLCK.f, which only iopmp_set_entries() used to catch */
        val_u32 = 45;
        FAIL_IF(iopmp_lock_entries(&iopmp, &val_u32, false) != IOPMP_OK);
        FAIL_IF(read_srcmd_perm_64(44) != 0);
        FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, &bad, 44, 1) !=
                IOPMP_ERR_REG_IS_LOCKED);
        FAIL_IF(read_srcmd_perm_64(44) != 0);

        /* A priority entry above HWCFG2.prio_entry, likewise. The priority
         * rule is checked before the lock, so this reports the priority */
        bad.prient_flag = IOPMP_PRIENT_PRIORITY;
        FAIL_IF(read_srcmd_perm_64(46) != 0);
        FAIL_IF(iopmp_set_entries_with_md_permission(&iopmp, &bad, 46, 1) !=
                IOPMP_ERR_INVALID_PRIORITY);
        FAIL_IF(read_srcmd_perm_64(46) != 0);
    }
    END_TEST();
#endif

    return 0;
}
