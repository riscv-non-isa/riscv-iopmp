#include "iopmp.h"
#include "config.h"
#include "test_utils.h"

#include "libiopmp.h"
#include "libiopmp_def.h"

// Declarations
iopmp_trans_req_t iopmp_trans_req;
iopmp_trans_rsp_t iopmp_trans_rsp;
err_info_t err_info_temp;

// Create IOPMP instance
iopmp_dev_t iopmp_dev = {0};
iopmp_cfg_t cfg = {0};
uint8_t intrpt;

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
    if (reset_iopmp(&iopmp_dev, cfg) != 0)
        return IOPMP_ERR_NOT_AVAILABLE;

    memset(iopmp, 0, sizeof(*iopmp));
    return iopmp_init(iopmp, 0, cfg->srcmd_fmt, cfg->mdcfg_fmt,
                      IOPMP_IMPID_NOT_SPECIFIED);
}

#if (SRC_ENFORCEMENT_EN == 0)
/* A per-instance generic override, for the two-instance dispatch test. Only
 * set_global_intr is filled in, so every other operation has to fall back. */
static int fake_set_global_intr_hits;

static void fake_set_global_intr(IOPMP_t *iopmp, bool enable)
{
    (void)iopmp;
    (void)enable;
    fake_set_global_intr_hits++;
}

static int fake_invalidate_error_hits;

static void fake_invalidate_error(IOPMP_t *iopmp)
{
    (void)iopmp;
    fake_invalidate_error_hits++;
}

static struct iopmp_operations_override fake_ops_override = {
    .set_global_intr = fake_set_global_intr,
    .invalidate_error = fake_invalidate_error,
};
#endif

#define CFG_MD_NUM          63
#define CFG_ENTRY_NUM       512
#define CFG_RRID_NUM        64
// Entries handed to each MD when the whole MDCFG table is programmed
#define ENTRY_PER_MD        8

/* Program MDCFG(0) ~ MDCFG(CFG_MD_NUM-1) so that every MD owns ENTRY_PER_MD
 * entries. Returns 0 on success. */
static int program_full_mdcfg_table(IOPMP_t *iopmp)
{
    uint32_t num_entries[CFG_MD_NUM];

    for (int m = 0; m < CFG_MD_NUM; m++)
        num_entries[m] = ENTRY_PER_MD;

    if (iopmp_set_md_entry_association_multi(iopmp, 0, num_entries,
                                             CFG_MD_NUM) != IOPMP_OK)
        return -1;

    for (int m = 0; m < CFG_MD_NUM; m++) {
        if (num_entries[m] != ENTRY_PER_MD)
            return -1;
    }

    return 0;
}

int main(void)
{
    IOPMP_t iopmp = {0};
    enum iopmp_error ret;
    uint32_t val_u32;
    uint64_t val_u64;
    struct iopmp_entry entries[8] = {0};

    FAIL_IF(create_memory(1) < 0)

    // Configure and reset IOPMP device
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
    cfg.sps_en= true;
    cfg.stall_en = true;
    cfg.mfr_en = true;
    cfg.mdcfg_fmt = 0;
    cfg.srcmd_fmt = 0;
    cfg.md_entry_num = 0;
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
    bool val_bool, val_bool_2;
    IOPMP_ERR_REPORT_t err_report = {0};
    IOPMP_SRCMD_PERM_CFG_t perm_cfg;
    struct iopmp_entry rb_entries[8] = {0};
    uint64_t val_u64_2, val_u64_3;
    uint32_t val_u32_2;
    uint16_t val_u16;

    START_TEST("Test OFF - Read Access permissions");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_FORCE_OFF | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single OFF entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_OFF | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 0, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test OFF - Write Access permissions");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_FORCE_OFF | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single OFF entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_OFF | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 0, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test OFF - Instruction Fetch permissions");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_FORCE_OFF | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single OFF entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_OFF | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 0, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test OFF - UNKNOWN RRID ERROR");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_FORCE_OFF | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single OFF entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_OFF | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(70, 364, 0, 0, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, UNKNOWN_RRID);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en,
                  "Test TOR - Partial hit on a priority rule error",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_R);
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_R));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, PARTIAL_HIT_ON_PRIORITY);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en, "Test TOR - 4Byte Read Access",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_R);
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_R));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en && iopmp_dev.reg_file.hwcfg2.sps_en,
                  "Test TOR - 4Byte Read Access with SRCMD_R not set",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_R);
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_R));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en && !iopmp_dev.reg_file.hwcfg2.sps_en,
                  "Test TOR - 4Byte Read Access, SRCMD_R not set, SPS disabled",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_R);
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_R));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en, "Test TOR - 4Byte AMO Write Access",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en,
                  "Test TOR - 4Byte Non-AMO Write Access",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_W, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_W));
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_W));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 2, WRITE_ACCESS, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg0.tor_en, "Test TOR - 4Byte Write Access",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0, 368,
                             IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_W, 0);
    FAIL_IF(ret != 2);  // Two TOR entries
    FAIL_IF(entries[0].addr != 0);
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_W));
    FAIL_IF(entries[1].addr != (368 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_W));
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 4Byte Read Access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte No Read Access error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, 0, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_A_NA4);
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.sps_en,
                  "Test NA4 - 4Byte No SPS Read Access error",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x0, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x0);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 4Byte Write Access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte Non-AMO Write Access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_W, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_W));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte No Write Access error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.sps_en,
                  "Test NA4 - 4Byte No SPS Write Access error",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x0, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x0);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 4Byte Execute Access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_X | IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_X | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte No Execute Access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.sps_en,
                  "Test NA4 - 4Byte No SPS.X, Execute Access",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 32, 0x0, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x0);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4,
                             IOPMP_ENTRY_X | IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_X | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 2, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 8Byte Access error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 364, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, PARTIAL_HIT_ON_PRIORITY);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - For exact 4 Byte error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 364, 4, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NA4 entry
    FAIL_IF(entries[0].addr != (364 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NA4 | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 368, 0, 0, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte read access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte read access error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, 0, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_A_NAPOT);
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte write access error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, 0, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_A_NAPOT);
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte write access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte Non-AMO write access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_W, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_W));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte Instruction access error");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, 0, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_A_NAPOT);
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte Instruction access");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.non_prio_en,
                  "Test NAPOT - 8 Byte Instruction access for non-priority Entry",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 31, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 31, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 17;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 17);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 296, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (296 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x10, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x10);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 32, 0x10, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x10);
    val_u32 = 25;
    ret = iopmp_set_md_entry_association(&iopmp, 4, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 25);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, 0, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_A_NAPOT);
    ret = iopmp_set_entry(&iopmp, entries, 18);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 20);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NAPOT - 8 Byte Instruction access when xinr=1");
    cfg.xinr = true;
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, 0, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != IOPMP_ENTRY_A_NAPOT);
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    iopmp_trans_req.perm = READ_ACCESS; // Since xinr=1, we expect ttype="Read access" in CHECK_IOPMP_TRANS()
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    cfg.xinr = false;
    END_TEST();

    START_TEST_IF(iopmp_dev.imp_mdlck, "Test MDLCK, updating locked srcmd_en field",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x8;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.imp_mdlck, "Test MDLCK, updating unlocked srcmd_en field",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x4;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x4);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test MDCFG_LCK, updating locked MDCFG field");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u32 = 4;
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test MDCFG_LCK, updating unlocked MDCFG field");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u32 = 2;
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test Entry_LCK, updating locked ENTRY field");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u32 = 4;
    ret = iopmp_lock_entries(&iopmp, &val_u32, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test Entry_LCK, updating unlocked ENTRY field");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u32 = 4;
    ret = iopmp_lock_entries(&iopmp, &val_u32, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 5;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 5);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 4);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test SRCMD_EN lock bit, updating locked SRCMD Table");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0, 0, &val_u64, true);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test SRCMD_EN lock bit, updating unlocked SRCMD Table");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 1, 0, 0, &val_u64, true);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 5;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 5);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 4);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.imp_mdlck, "Test MDLCK register lock bit",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x4;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);   // MD[2] is locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x4);
    ret = iopmp_lock_md(&iopmp, &val_u64, true);    // Locking MDLCK register
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x8;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);   // Trying to lock MD[3] but it shouldn't be locked
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test MDCFG_LCK register lock bit");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u32 = 4;
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, false);    // MD[0]-MD[3] are locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, true);     // MDCFGLCK is locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    val_u32 = 8;
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, false);    // Updating locked MD's MD[0]-MD[1] are locked
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test Entry_LCK register lock bit");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u32 = 4;
    ret = iopmp_lock_entries(&iopmp, &val_u32, false);  // ENTRY[0]-ENTRY[3] are locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    ret = iopmp_lock_entries(&iopmp, &val_u32, true);   // ENTRYLCK is locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    val_u32 = 8;
    ret = iopmp_lock_entries(&iopmp, &val_u32, false);  // Updating locked entries but ENTRYLCK was locked
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.mfr_en, "Test MFR Extension",
    // Following the previous test
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    uint16_t svi = 0;
    uint16_t svw = 0;
    ret = iopmp_mfr_get_sv_window(&iopmp, &svi, &svw);
    FAIL_IF(ret != IOPMP_ERR_NOT_EXIST);
    FAIL_IF(svi != 0);
    FAIL_IF(svw != 0);
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    ret = iopmp_mfr_get_sv_window(&iopmp, &svi, &svw);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(svi != 0);
    FAIL_IF(svw != 4);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.imp_mdlck, "Test MDLCK, updating locked srcmd_enh field",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x80000000;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);   // MD[31] is locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.imp_mdlck, "Test MDLCK, updating unlocked srcmd_enh field",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x100000000;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);   // MD[32] is locked
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x100000000);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.peis, "Test Interrupt Suppression is Enabled",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_global_intr(&iopmp, true);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_SIXE | IOPMP_ENTRY_R, 0);  // Address Mode is NAPOT, with read permission and ixe suppression
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_SIXE | IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt == 1)); // Interrupt is suppressed
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test Interrupt Suppression is disabled");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_global_intr(&iopmp, true);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt == 0)); // Interrupt is not suppressed
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.pees, "Test Error Suppression is Enabled",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_bool = true;
    ret = iopmp_set_global_err_resp(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_SEXE | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_SEXE | IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));    // Address Mode is NAPOT, with read permission and exe suppression
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_SUCCESS));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != USER));
    error_record_chk(&iopmp_dev, NO_ERROR, INSTR_FETCH, 360, 0);
    ret = iopmp_capture_error(&iopmp, &err_report, false);
    FAIL_IF(ret != IOPMP_ERR_NOT_EXIST);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.pees,
                  "Test Error Suppression is Enabled but rs is zero",
    // Receiver Port Signals
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_SEXE | IOPMP_ENTRY_R, 0);  // Address Mode is NAPOT, with read permission and exe suppression
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_SEXE | IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_SUCCESS));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != USER));
    error_record_chk(&iopmp_dev, NO_ERROR, INSTR_FETCH, 360, 0);
    ret = iopmp_capture_error(&iopmp, &err_report, false);
    FAIL_IF(ret != IOPMP_ERR_NOT_EXIST);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test Error Suppression is disabled");
    // Receiver Port Signals
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_ERROR));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != 0));
    error_record_chk(&iopmp_dev, ILLEGAL_INSTR_FETCH, INSTR_FETCH, 360, 1);
    ret = iopmp_capture_error(&iopmp, &err_report, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF((err_report.etype != IOPMP_ETYPE_INST_FETCH));
    FAIL_IF((err_report.ttype != IOPMP_TTYPE_INST_FETCH));
    FAIL_IF((err_report.addr != (360 >> 2)));
    FAIL_IF((err_report.rrid != 2));
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.peis && iopmp_dev.reg_file.hwcfg2.pees,
                  "Test Interrupt and Error Suppression is Enabled",
    // Receiver Port Signals
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_global_intr(&iopmp, true);
    FAIL_IF(ret != IOPMP_OK);
    val_bool = true;
    ret = iopmp_set_global_err_resp(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_SEXE | IOPMP_ENTRY_SIXE | IOPMP_ENTRY_R, 0);  // Address Mode is NAPOT, with read permission and ixe/exe suppression
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_SEXE | IOPMP_ENTRY_SIXE | IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt == 1));
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_SUCCESS));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != USER));
    error_record_chk(&iopmp_dev, ILLEGAL_INSTR_FETCH, INSTR_FETCH, 360, 0);
    ret = iopmp_capture_error(&iopmp, &err_report, false);
    FAIL_IF(ret != IOPMP_ERR_NOT_EXIST);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test Interrupt and Error Suppression is disabled");
    // Receiver Port Signals
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_global_intr(&iopmp, true);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt != 1));
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_ERROR));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    error_record_chk(&iopmp_dev, ILLEGAL_INSTR_FETCH, INSTR_FETCH, 360, 1);
    ret = iopmp_capture_error(&iopmp, &err_report, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF((err_report.etype != IOPMP_ETYPE_INST_FETCH));
    FAIL_IF((err_report.ttype != IOPMP_TTYPE_INST_FETCH));
    FAIL_IF((err_report.addr != (360 >> 2)));
    FAIL_IF((err_report.rrid != 2));
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.stall_en && iopmp_dev.imp_rridscp, "Stall MD Feature",
    cfg.imp_stall_buffer = true;
    cfg.stall_buffer_size = 32;
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 5, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 5, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x8;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true);
    FAIL_IF(ret != IOPMP_OK);
    uint32_t stat = 0U;
    val_u32 = 5;
    ret = iopmp_stall_cherry_pick_rrid(&iopmp, &val_u32, true, &stat);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(stat != IOPMP_RRIDSCP_STAT_STALLED);

    receiver_port(5, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.rrid_stalled != 1));
    val_u32 = 5;
    ret = iopmp_query_stall_stat_by_rrid(&iopmp, &val_u32, &stat);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(stat != IOPMP_RRIDSCP_STAT_STALLED);
    FAIL_IF((iopmp_trans_rsp.rrid != 5));
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.stall_en && iopmp_dev.imp_rridscp,
                  "Faulting Stalled Transactions Feature",
    cfg.imp_stall_buffer = false;
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    val_bool = true;
    ret = iopmp_set_stall_violation_en(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    ret = iopmp_set_rrid_md_association(&iopmp, 5, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 5, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_X, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_X));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);
    val_u64 = 0x8;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true);
    FAIL_IF(ret != IOPMP_OK);
    uint32_t stat = 0U;
    val_u32 = 5;
    ret = iopmp_stall_cherry_pick_rrid(&iopmp, &val_u32, true, &stat);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(stat != IOPMP_RRIDSCP_STAT_STALLED);

    receiver_port(5, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.rrid_stalled == 1));
    val_u32 = 5;
    ret = iopmp_query_stall_stat_by_rrid(&iopmp, &val_u32, &stat);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(stat != IOPMP_RRIDSCP_STAT_STALLED);
    FAIL_IF((iopmp_trans_rsp.rrid != 5));
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, STALLED_TRANSACTION);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    // Reset configuration
    cfg.imp_stall_buffer = true;
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg3.rrid_transl_en, "Test Cascading IOPMP Feature",
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 32, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 32, 0x8, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 3, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.rrid_transl != iopmp_dev.reg_file.hwcfg3.rrid_transl));
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.msi_en,
                  "ERR_MSIADDR alone carries an MSI address above 4G",
    uint64_t msiaddr64;
    uint16_t msidata;
    /* Without ERR_MSIADDRH, ERR_MSIADDR alone carries bits 33:2 */
    cfg.addrh_en = false;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    msiaddr64 = 0x100000000ULL;
    msidata = 0x8F;
    FAIL_IF(iopmp_set_msi_info(&iopmp, &msiaddr64, &msidata) != IOPMP_OK);
    FAIL_IF(msiaddr64 != 0x100000000ULL);
    cfg.addrh_en = true;
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.msi_en, "Test MSI Write error",
    uint64_t read_data;
    uint64_t msiaddr64;
    uint16_t msidata;
    reset_iopmp(&iopmp_dev, &cfg);
    bus_error = 0x8000;

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_global_intr(&iopmp, true);
    FAIL_IF(ret != IOPMP_OK);
    val_bool = true;
    ret = iopmp_set_msi_sel(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    msiaddr64 = 0x8000;
    msidata = 0x8F;
    ret = iopmp_set_msi_info(&iopmp, &msiaddr64, &msidata);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(msiaddr64 != 0x8000);
    FAIL_IF(msidata != 0x8F);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    bus_error = 0;
    read_memory(0x8000, 4, &read_data);
    FAIL_IF(intrpt == 1);
    FAIL_IF(read_data == 0x8F); // Interrupt is not suppressed
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp_dev.reg_file.hwcfg2.msi_en, "Test MSI",
    uint64_t read_data;
    uint64_t msiaddr64;
    uint16_t msidata;
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_global_intr(&iopmp, true);
    FAIL_IF(ret != IOPMP_OK);
    val_bool = true;
    ret = iopmp_set_msi_sel(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    msiaddr64 = 0x8000;
    msidata = 0x8F;
    ret = iopmp_set_msi_info(&iopmp, &msiaddr64, &msidata);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(msiaddr64 != 0x8000);
    FAIL_IF(msidata != 0x8F);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x80000000, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    ret = iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x80000000, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x80000000);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 31, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 1);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    read_memory(0x8000, 4, &read_data);
    FAIL_IF(intrpt == 1);
    FAIL_IF(read_data != 0x8F); // Interrupt is not suppressed
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)


    /**********************************************************************/
    /* Instance getters that report the reset state                       */
    /**********************************************************************/
    START_TEST("Get granularity detected at initialization");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_granularity(&iopmp) != iopmp_dev.granularity);
    END_TEST();

    START_TEST("Get the number of locked entries after reset");
    FAIL_IF(iopmp_get_locked_entry_num(&iopmp) != 0);
    FAIL_IF(iopmp_is_entrylck_locked(&iopmp) != false);
    END_TEST();

    START_TEST("Check stall-related features are detected");
    FAIL_IF(iopmp_get_support_stall(&iopmp) != iopmp_dev.reg_file.hwcfg2.stall_en);
    FAIL_IF(iopmp_get_support_stall_by_rrid(&iopmp) != iopmp_dev.imp_rridscp);
    END_TEST();

    START_TEST("Get ERR_CFG related states after reset");
    FAIL_IF(iopmp_is_err_cfg_locked(&iopmp) != false);
    FAIL_IF(iopmp_get_global_intr(&iopmp) != false);
    FAIL_IF(iopmp_get_global_err_resp(&iopmp) != false);
    FAIL_IF(iopmp_get_msi_sel(&iopmp) != false);
    FAIL_IF(iopmp_get_stall_violation_en(&iopmp) != false);
    END_TEST();

    START_TEST("Get MDLCK state after reset");
    FAIL_IF(iopmp_is_mdlck_locked(&iopmp) != false);
    ret = iopmp_get_locked_md(&iopmp, &val_u64, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0);
    FAIL_IF(val_bool != false);
    END_TEST();

    START_TEST("Get MDLCK state with invalid arguments");
    FAIL_IF(iopmp_get_locked_md(&iopmp, NULL, &val_bool) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_locked_md(&iopmp, &val_u64, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Get MDCFGLCK state after reset");
    ret = iopmp_is_mdcfglck_locked(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != false);
    ret = iopmp_get_locked_mdcfg_num(&iopmp, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 0);
    END_TEST();

    START_TEST("Get MDCFGLCK state with invalid arguments");
    FAIL_IF(iopmp_is_mdcfglck_locked(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_locked_mdcfg_num(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    /**********************************************************************/
    /* SRCMD table (format 0) queries and locking                         */
    /**********************************************************************/
    START_TEST("Get RRID/MD association of an untouched RRID");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    ret = iopmp_get_rrid_md_association(&iopmp, 2, &val_u64, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0);
    FAIL_IF(val_bool != false);
    END_TEST();

    START_TEST("Get RRID/MD association reflects a previous write");
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x18, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x18);
    ret = iopmp_get_rrid_md_association(&iopmp, 2, &val_u64, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x18);
    FAIL_IF(val_bool != false);
    END_TEST();

    START_TEST("Clear MDs from a RRID/MD association");
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0, 0x10, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    END_TEST();

    START_TEST("Get RRID/MD association with invalid arguments");
    FAIL_IF(iopmp_get_rrid_md_association(&iopmp, CFG_RRID_NUM, &val_u64, &val_bool) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_get_rrid_md_association(&iopmp, 2, NULL, &val_bool) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_rrid_md_association(&iopmp, 2, &val_u64, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Set RRID/MD association with invalid arguments");
    FAIL_IF(iopmp_set_rrid_md_association(&iopmp, CFG_RRID_NUM, 0x1, 0, &val_u64, false) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_set_rrid_md_association(&iopmp, 2, 0x1, 0, NULL, false) != IOPMP_ERR_INVALID_PARAMETER);
    /* MD(63) does not exist because HWCFG0.md_num is 63 */
    FAIL_IF(iopmp_set_rrid_md_association(&iopmp, 2, (uint64_t)1 << CFG_MD_NUM, 0, &val_u64, false) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_set_rrid_md_association(&iopmp, 2, 0, (uint64_t)1 << CFG_MD_NUM, &val_u64, false) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Check SRCMD_EN(rrid) lock state and lock it");
    ret = iopmp_is_srcmd_table_fmt_0_locked(&iopmp, 2, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != false);
    ret = iopmp_lock_srcmd_table_fmt_0(&iopmp, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_is_srcmd_table_fmt_0_locked(&iopmp, 2, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    END_TEST();

    START_TEST("Set RRID/MD association of a locked SRCMD_EN(rrid)");
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x1, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Lock/query SRCMD table with invalid arguments");
    FAIL_IF(iopmp_lock_srcmd_table_fmt_0(&iopmp, CFG_RRID_NUM) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_is_srcmd_table_fmt_0_locked(&iopmp, CFG_RRID_NUM, &val_bool) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_is_srcmd_table_fmt_0_locked(&iopmp, 2, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Set RRID/MD association of a MD locked by MDLCK");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u64 = 0x4;
    ret = iopmp_lock_md(&iopmp, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x4);
    FAIL_IF(iopmp_is_mdlck_locked(&iopmp) != false);
    ret = iopmp_get_locked_md(&iopmp, &val_u64, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x4);
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x4, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Lock MDLCK register and reject a later change");
    val_u64 = 0x4;
    ret = iopmp_lock_md(&iopmp, &val_u64, true);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_is_mdlck_locked(&iopmp) != true);
    /* Requesting exactly what is already locked is accepted */
    val_u64 = 0x4;
    FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, true) != IOPMP_OK);
    /* Requesting anything more is rejected */
    val_u64 = 0x8;
    FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, true) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Lock MD with invalid arguments");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_lock_md(&iopmp, NULL, false) != IOPMP_ERR_INVALID_PARAMETER);
    /* Locking nothing is a no-op that succeeds */
    val_u64 = 0;
    FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, false) != IOPMP_OK);
    /* MD(63) does not exist */
    val_u64 = (uint64_t)1 << CFG_MD_NUM;
    FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, false) != IOPMP_ERR_NOT_SUPPORTED);
    END_TEST();

    /**********************************************************************/
    /* MDCFG table (format 0)                                             */
    /**********************************************************************/
    START_TEST("Program the whole MDCFG table");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    END_TEST();

    START_TEST("Get MD/entry association of every MD");
    for (uint32_t m = 0; m < CFG_MD_NUM; m++) {
        ret = iopmp_get_md_entry_association(&iopmp, m, &val_u32, &val_u32_2);
        FAIL_IF(ret != IOPMP_OK);
        FAIL_IF(val_u32 != m * ENTRY_PER_MD);
        FAIL_IF(val_u32_2 != ENTRY_PER_MD);
    }
    END_TEST();

    START_TEST("Get MD/entry association with invalid arguments");
    FAIL_IF(iopmp_get_md_entry_association(&iopmp, CFG_MD_NUM, &val_u32, &val_u32_2) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_get_md_entry_association(&iopmp, 0, NULL, &val_u32_2) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_md_entry_association(&iopmp, 0, &val_u32, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Set MD/entry association with invalid arguments");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u32 = 4;
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, CFG_MD_NUM, &val_u32, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, &val_u32, CFG_MD_NUM + 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, NULL, 1) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Set MD/entry association beyond the number of entries");
    val_u32 = CFG_ENTRY_NUM + 1;
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, &val_u32, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Set MD/entry association of zero MD is a no-op");
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, &val_u32, 0) != IOPMP_OK);
    END_TEST();

    START_TEST("Lock MDCFG and reject a later change");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    val_u32 = 2;
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_get_locked_mdcfg_num(&iopmp, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    /* MDCFG(0) and MDCFG(1) are locked now */
    val_u32 = ENTRY_PER_MD;
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, &val_u32, 1) != IOPMP_ERR_REG_IS_LOCKED);
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 1, &val_u32, 1) != IOPMP_ERR_REG_IS_LOCKED);
    /* MDCFG(2) is still writable */
    val_u32 = ENTRY_PER_MD;
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 2, &val_u32, 1) != IOPMP_OK);
    END_TEST();

    START_TEST("MDCFGLCK.f must be monotonically increased");
    val_u32 = 1;
    FAIL_IF(iopmp_lock_mdcfg(&iopmp, &val_u32, false) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("Lock MDCFG with invalid arguments");
    FAIL_IF(iopmp_lock_mdcfg(&iopmp, NULL, false) != IOPMP_ERR_INVALID_PARAMETER);
    val_u32 = CFG_MD_NUM + 1;
    FAIL_IF(iopmp_lock_mdcfg(&iopmp, &val_u32, false) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Lock MDCFGLCK register and reject a later change");
    val_u32 = 4;
    ret = iopmp_lock_mdcfg(&iopmp, &val_u32, true);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_is_mdcfglck_locked(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    /* Asking for the very same state is still accepted */
    val_u32 = 4;
    FAIL_IF(iopmp_lock_mdcfg(&iopmp, &val_u32, true) != IOPMP_OK);
    val_u32 = 5;
    FAIL_IF(iopmp_lock_mdcfg(&iopmp, &val_u32, true) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("HWCFG3.md_entry_num APIs are rejected on MDCFG_FMT=0");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_md_entry_num(&iopmp, &val_u32) != IOPMP_ERR_NOT_SUPPORTED);
    val_u32 = 4;
    FAIL_IF(iopmp_set_md_entry_num(&iopmp, &val_u32) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    /**********************************************************************/
    /* Entry array: write, read back, and clear                           */
    /**********************************************************************/
    START_TEST("Write entries and read them back");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    ret = iopmp_encode_entry(&iopmp, &entries[0], 1, 0x1000, 0x1000,
                             IOPMP_ENTRY_RW, 0);
    FAIL_IF(ret != 1);
    ret = iopmp_encode_entry(&iopmp, &entries[1], 1, 0x8000, 0x4000,
                             IOPMP_ENTRY_RX, 0);
    FAIL_IF(ret != 1);
    ret = iopmp_set_entries(&iopmp, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_get_entries(&iopmp, rb_entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    for (int i = 0; i < 2; i++) {
        FAIL_IF(iopmp_entry_get_addr(&rb_entries[i]) != iopmp_entry_get_addr(&entries[i]));
        FAIL_IF(iopmp_entry_get_cfg(&rb_entries[i]) != iopmp_entry_get_cfg(&entries[i]));
    }
    END_TEST();

    START_TEST("Write and read back a single entry");
    ret = iopmp_encode_entry(&iopmp, &entries[2], 1, 0x20000, 0x10000,
                             IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);
    ret = iopmp_set_entry(&iopmp, &entries[2], 5);
    FAIL_IF(ret != IOPMP_OK);
    memset(&rb_entries[2], 0, sizeof(rb_entries[2]));
    ret = iopmp_get_entry(&iopmp, &rb_entries[2], 5);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_entry_get_addr(&rb_entries[2]) != iopmp_entry_get_addr(&entries[2]));
    FAIL_IF(iopmp_entry_get_cfg(&rb_entries[2]) != iopmp_entry_get_cfg(&entries[2]));
    END_TEST();

    START_TEST("Get entries with invalid arguments");
    FAIL_IF(iopmp_get_entries(&iopmp, NULL, 0, 1) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_entries(&iopmp, rb_entries, 0, 0) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_entries(&iopmp, rb_entries, CFG_ENTRY_NUM, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_get_entries(&iopmp, rb_entries, CFG_ENTRY_NUM - 1, 2) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Set entries with invalid arguments");
    FAIL_IF(iopmp_set_entries(&iopmp, NULL, 0, 1) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_set_entries(&iopmp, entries, 0, 0) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_set_entries(&iopmp, entries, CFG_ENTRY_NUM, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_set_entries(&iopmp, entries, CFG_ENTRY_NUM - 1, 2) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Reject a priority entry placed outside the priority range");
    entries[3] = entries[0];
    entries[3].prient_flag = IOPMP_PRIENT_PRIORITY;
    /* HWCFG2.prio_entry is 16, so entry 16 and above are non-priority */
    FAIL_IF(iopmp_set_entries(&iopmp, &entries[3], 16, 1) != IOPMP_ERR_INVALID_PRIORITY);
    FAIL_IF(iopmp_set_entries(&iopmp, &entries[3], 15, 1) != IOPMP_OK);
    entries[3].prient_flag = IOPMP_PRIENT_NON_PRIORITY;
    FAIL_IF(iopmp_set_entries(&iopmp, &entries[3], 15, 1) != IOPMP_ERR_INVALID_PRIORITY);
    FAIL_IF(iopmp_set_entries(&iopmp, &entries[3], 16, 1) != IOPMP_OK);
    END_TEST();

    START_TEST("Write and read back entries through a MD");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    /* MD(2) owns entry 16 ~ 23 */
    ret = iopmp_set_entries_to_md(&iopmp, 2, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_get_entries(&iopmp, rb_entries, 2 * ENTRY_PER_MD, 2);
    FAIL_IF(ret != IOPMP_OK);
    for (int i = 0; i < 2; i++) {
        FAIL_IF(iopmp_entry_get_addr(&rb_entries[i]) != iopmp_entry_get_addr(&entries[i]));
        FAIL_IF(iopmp_entry_get_cfg(&rb_entries[i]) != iopmp_entry_get_cfg(&entries[i]));
    }
    memset(rb_entries, 0, sizeof(rb_entries));
    ret = iopmp_get_entries_from_md(&iopmp, 2, rb_entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    for (int i = 0; i < 2; i++) {
        FAIL_IF(iopmp_entry_get_addr(&rb_entries[i]) != iopmp_entry_get_addr(&entries[i]));
        FAIL_IF(iopmp_entry_get_cfg(&rb_entries[i]) != iopmp_entry_get_cfg(&entries[i]));
    }
    END_TEST();

    START_TEST("Write and read back a single entry through a MD");
    ret = iopmp_set_entry_to_md(&iopmp, 3, &entries[2], 1);
    FAIL_IF(ret != IOPMP_OK);
    memset(&rb_entries[2], 0, sizeof(rb_entries[2]));
    ret = iopmp_get_entry_from_md(&iopmp, 3, &rb_entries[2], 1);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_entry_get_addr(&rb_entries[2]) != iopmp_entry_get_addr(&entries[2]));
    /* The very same entry is reachable through its global index */
    memset(&rb_entries[3], 0, sizeof(rb_entries[3]));
    ret = iopmp_get_entry(&iopmp, &rb_entries[3], 3 * ENTRY_PER_MD + 1);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_entry_get_addr(&rb_entries[3]) != iopmp_entry_get_addr(&entries[2]));
    END_TEST();

    START_TEST("Access entries of a MD with invalid arguments");
    FAIL_IF(iopmp_set_entries_to_md(&iopmp, CFG_MD_NUM, entries, 0, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_get_entries_from_md(&iopmp, CFG_MD_NUM, rb_entries, 0, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    /* A MD only owns ENTRY_PER_MD entries */
    FAIL_IF(iopmp_set_entries_to_md(&iopmp, 2, entries, 0, ENTRY_PER_MD + 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_get_entries_from_md(&iopmp, 2, rb_entries, 0, ENTRY_PER_MD + 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_set_entries_to_md(&iopmp, 2, entries, ENTRY_PER_MD, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_get_entries_from_md(&iopmp, 2, rb_entries, ENTRY_PER_MD, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Clear a single entry");
    ret = iopmp_clear_entry(&iopmp, 2 * ENTRY_PER_MD);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_get_entry(&iopmp, &rb_entries[0], 2 * ENTRY_PER_MD);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_entry_get_addr(&rb_entries[0]) != 0);
    FAIL_IF(iopmp_entry_get_cfg(&rb_entries[0]) != 0);
    END_TEST();

    START_TEST("Clear all entries owned by a MD");
    ret = iopmp_set_entries_to_md(&iopmp, 2, entries, 0, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_clear_entries_in_md(&iopmp, 2);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_get_entries_from_md(&iopmp, 2, rb_entries, 0, ENTRY_PER_MD);
    FAIL_IF(ret != IOPMP_OK);
    for (int i = 0; i < ENTRY_PER_MD; i++) {
        FAIL_IF(iopmp_entry_get_addr(&rb_entries[i]) != 0);
        FAIL_IF(iopmp_entry_get_cfg(&rb_entries[i]) != 0);
    }
    /* The neighbouring MD is untouched */
    ret = iopmp_get_entry(&iopmp, &rb_entries[0], 3 * ENTRY_PER_MD + 1);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_entry_get_addr(&rb_entries[0]) != iopmp_entry_get_addr(&entries[2]));
    END_TEST();

    START_TEST("Clear entries with invalid arguments");
    FAIL_IF(iopmp_clear_entries(&iopmp, 0, CFG_ENTRY_NUM + 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_clear_entries(&iopmp, CFG_ENTRY_NUM, 1) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_clear_entries(&iopmp, CFG_ENTRY_NUM - 1, 2) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_clear_entries_in_md(&iopmp, CFG_MD_NUM) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Lock entries and reject later writes");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    val_u32 = 4;
    ret = iopmp_lock_entries(&iopmp, &val_u32, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 4);
    FAIL_IF(iopmp_get_locked_entry_num(&iopmp) != 4);
    FAIL_IF(iopmp_set_entries(&iopmp, entries, 0, 1) != IOPMP_ERR_REG_IS_LOCKED);
    FAIL_IF(iopmp_clear_entries(&iopmp, 0, 1) != IOPMP_ERR_REG_IS_LOCKED);
    FAIL_IF(iopmp_clear_entries_in_md(&iopmp, 0) != IOPMP_ERR_REG_IS_LOCKED);
    /* Entry 4 and above are still writable */
    FAIL_IF(iopmp_set_entries(&iopmp, entries, 4, 1) != IOPMP_OK);
    FAIL_IF(iopmp_clear_entries(&iopmp, 4, 1) != IOPMP_OK);
    END_TEST();

    START_TEST("ENTRYLCK.f must be monotonically increased");
    val_u32 = 2;
    FAIL_IF(iopmp_lock_entries(&iopmp, &val_u32, false) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("Lock entries with invalid arguments");
    FAIL_IF(iopmp_lock_entries(&iopmp, NULL, false) != IOPMP_ERR_INVALID_PARAMETER);
    val_u32 = CFG_ENTRY_NUM + 1;
    FAIL_IF(iopmp_lock_entries(&iopmp, &val_u32, false) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("Lock ENTRYLCK register and reject a later change");
    val_u32 = 8;
    ret = iopmp_lock_entries(&iopmp, &val_u32, true);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_is_entrylck_locked(&iopmp) != true);
    val_u32 = 8;
    FAIL_IF(iopmp_lock_entries(&iopmp, &val_u32, true) != IOPMP_OK);
    val_u32 = 9;
    FAIL_IF(iopmp_lock_entries(&iopmp, &val_u32, true) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Report which MDs a range of entries belongs to");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    /* Entry 0 ~ 7 is exactly MD(0) */
    ret = iopmp_entries_get_belong_md(&iopmp, 0, ENTRY_PER_MD, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x1);
    /* Entry 4 ~ 11 straddles MD(0) and MD(1) */
    ret = iopmp_entries_get_belong_md(&iopmp, 4, ENTRY_PER_MD, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x3);
    /* Entry 8 ~ 31 is exactly MD(1) ~ MD(3) */
    ret = iopmp_entries_get_belong_md(&iopmp, ENTRY_PER_MD, 3 * ENTRY_PER_MD, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0xE);
    /* Entry 504 ~ 511 is beyond the last MD */
    ret = iopmp_entries_get_belong_md(&iopmp, CFG_MD_NUM * ENTRY_PER_MD,
                                      CFG_ENTRY_NUM - (CFG_MD_NUM * ENTRY_PER_MD),
                                      &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0);
    END_TEST();

    START_TEST("An empty index range belongs to no MD");
    ret = iopmp_entries_get_belong_md(&iopmp, 4, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0);
    END_TEST();

    START_TEST("A MD owning no entry is never reported as belonging");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    /* MD(0) owns entry 0 ~ 15, MD(1) owns entry 16 ~ 31, MD(2) onwards own
     * nothing. The table stays monotonic, so this is a proper setting */
    {
        uint32_t num_entries[CFG_MD_NUM] = {16, 16};
        FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, num_entries,
                                                     CFG_MD_NUM) != IOPMP_OK);
    }
    ret = iopmp_get_md_entry_association(&iopmp, 2, &val_u32, &val_u32_2);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 32);
    FAIL_IF(val_u32_2 != 0);
    /* Entry 0 ~ 31 covers MD(0) and MD(1), and no empty MD may appear */
    ret = iopmp_entries_get_belong_md(&iopmp, 0, 32, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x3);
    /* Entry 20 ~ 23 sits inside MD(1) only */
    ret = iopmp_entries_get_belong_md(&iopmp, 20, 4, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x2);
    END_TEST();

    START_TEST("An improperly set MDCFG table reports no entry, not underflow");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    /* The reference model only makes the MDCFG table monotonic once the IOPMP
     * is enabled, so MDCFG(2).t stays 0 while MDCFG(1).t is already 32 */
    val_u32 = 16;
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 0, &val_u32, 1) != IOPMP_OK);
    val_u32 = 16;
    FAIL_IF(iopmp_set_md_entry_association_multi(&iopmp, 1, &val_u32, 1) != IOPMP_OK);
    ret = iopmp_get_md_entry_association(&iopmp, 2, &val_u32, &val_u32_2);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 32);
    FAIL_IF(val_u32_2 != 0);
    ret = iopmp_entries_get_belong_md(&iopmp, 0, 32, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x3);
    END_TEST();

    START_TEST("Report belonging MDs with invalid arguments");
    FAIL_IF(iopmp_entries_get_belong_md(&iopmp, CFG_ENTRY_NUM, 1, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_entries_get_belong_md(&iopmp, CFG_ENTRY_NUM - 1, 2, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_entries_get_belong_md(&iopmp, 0, 1, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    /**********************************************************************/
    /* iopmp_encode_entry() rejections                                    */
    /**********************************************************************/
    START_TEST("Encode entry with invalid arguments");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_encode_entry(&iopmp, NULL, 1, 0x1000, 0x1000, IOPMP_ENTRY_R, 0) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_encode_entry(&iopmp, entries, 0, 0x1000, 0x1000, IOPMP_ENTRY_R, 0) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0, IOPMP_ENTRY_R, 0) != IOPMP_ERR_INVALID_PARAMETER);
    /* The granularity is 4 bytes, so a 2-byte region is not encodable */
    FAIL_IF(iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 2, IOPMP_ENTRY_R, 0) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_encode_entry(&iopmp, entries, 1, 0x1002, 4, IOPMP_ENTRY_R, 0) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Encode a TOR region needing two entries");
    /* 0x1000 ~ 0x2800 is not a NAPOT region */
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0x1000, 0x1800, IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 2);
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_OFF | IOPMP_ENTRY_R));
    FAIL_IF(entries[0].addr != (0x1000 >> 2));
    FAIL_IF(entries[1].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_R));
    FAIL_IF(entries[1].addr != (0x2800 >> 2));
    /* ... but only if the caller offers room for two entries */
    FAIL_IF(iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0x1800, IOPMP_ENTRY_R, 0) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("Encode the first TOR entry");
    ret = iopmp_encode_entry(&iopmp, entries, 1, 0, 0x1800,
                             IOPMP_ENTRY_FIRST_TOR | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_TOR | IOPMP_ENTRY_R));
    FAIL_IF(entries[0].addr != (0x1800 >> 2));
    /* The first TOR entry must start from address 0 */
    FAIL_IF(iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0x1800,
                               IOPMP_ENTRY_FIRST_TOR | IOPMP_ENTRY_R, 0) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("Encode entry carrying the priority software flags");
    ret = iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0x1000,
                             IOPMP_ENTRY_R | IOPMP_ENTRY_PRIO, 0);
    FAIL_IF(ret != 1);
    FAIL_IF(entries[0].prient_flag != IOPMP_PRIENT_PRIORITY);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0x1000,
                             IOPMP_ENTRY_R | IOPMP_ENTRY_NON_PRIO, 0);
    FAIL_IF(ret != 1);
    FAIL_IF(entries[0].prient_flag != IOPMP_PRIENT_NON_PRIORITY);
    ret = iopmp_encode_entry(&iopmp, entries, 2, 0x1000, 0x1800,
                             IOPMP_ENTRY_R | IOPMP_ENTRY_PRIO, 0);
    FAIL_IF(ret != 2);
    FAIL_IF(entries[0].prient_flag != IOPMP_PRIENT_PRIORITY);
    FAIL_IF(entries[1].prient_flag != IOPMP_PRIENT_PRIORITY);
    END_TEST();

    /**********************************************************************/
    /* ERR_CFG and the error capture record                               */
    /**********************************************************************/
    START_TEST("Program every ERR_CFG field and read it back");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_set_global_intr(&iopmp, true) != IOPMP_OK);
    FAIL_IF(iopmp_get_global_intr(&iopmp) != true);
    /* Setting the very same value again short-circuits */
    FAIL_IF(iopmp_set_global_intr(&iopmp, true) != IOPMP_OK);
    val_bool = true;
    FAIL_IF(iopmp_set_global_err_resp(&iopmp, &val_bool) != IOPMP_OK);
    FAIL_IF(val_bool != true);
    FAIL_IF(iopmp_get_global_err_resp(&iopmp) != true);
    FAIL_IF(iopmp_set_global_err_resp(&iopmp, &val_bool) != IOPMP_OK);
    val_bool = true;
    FAIL_IF(iopmp_set_msi_sel(&iopmp, &val_bool) != IOPMP_OK);
    FAIL_IF(val_bool != true);
    FAIL_IF(iopmp_get_msi_sel(&iopmp) != true);
    FAIL_IF(iopmp_set_msi_sel(&iopmp, &val_bool) != IOPMP_OK);
    val_bool = true;
    FAIL_IF(iopmp_set_stall_violation_en(&iopmp, &val_bool) != IOPMP_OK);
    FAIL_IF(val_bool != true);
    FAIL_IF(iopmp_get_stall_violation_en(&iopmp) != true);
    FAIL_IF(iopmp_set_stall_violation_en(&iopmp, &val_bool) != IOPMP_OK);
    END_TEST();

    START_TEST("Program ERR_CFG fields with invalid arguments");
    FAIL_IF(iopmp_set_global_err_resp(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_set_msi_sel(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_set_stall_violation_en(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Program the MSI address and data");
    val_u64 = 0x8000;
    val_u16 = 0x8F;
    ret = iopmp_set_msi_info(&iopmp, &val_u64, &val_u16);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8000);
    FAIL_IF(val_u16 != 0x8F);
    ret = iopmp_get_msi_addr(&iopmp, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8000);
    ret = iopmp_get_msi_data(&iopmp, &val_u16);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u16 != 0x8F);
    /* Programming the same information again short-circuits */
    FAIL_IF(iopmp_set_msi_info(&iopmp, &val_u64, &val_u16) != IOPMP_OK);
    END_TEST();

    START_TEST("Program the MSI information with invalid arguments");
    FAIL_IF(iopmp_get_msi_addr(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_msi_data(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_set_msi_info(&iopmp, NULL, &val_u16) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_set_msi_info(&iopmp, &val_u64, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    /* ERR_CFG.msidata only holds 11 bits */
    val_u64 = 0x8000;
    val_u16 = 0x800;
    FAIL_IF(iopmp_set_msi_info(&iopmp, &val_u64, &val_u16) != IOPMP_ERR_NOT_SUPPORTED);
    END_TEST();

    START_TEST("Query the MSI write error flag");
    ret = iopmp_get_and_clear_msi_werr(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != false);
    FAIL_IF(iopmp_get_and_clear_msi_werr(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Capture an error when the record is empty");
    FAIL_IF(iopmp_capture_error(&iopmp, &err_report, false) != IOPMP_ERR_NOT_EXIST);
    FAIL_IF(iopmp_capture_error(&iopmp, NULL, false) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_invalidate_error(&iopmp) != IOPMP_OK);
    END_TEST();

    START_TEST("Lock ERR_CFG and reject later changes");
    FAIL_IF(iopmp_lock_err_cfg(&iopmp) != IOPMP_OK);
    FAIL_IF(iopmp_is_err_cfg_locked(&iopmp) != true);
    /* Locking again is accepted */
    FAIL_IF(iopmp_lock_err_cfg(&iopmp) != IOPMP_OK);
    FAIL_IF(iopmp_set_global_intr(&iopmp, false) != IOPMP_ERR_REG_IS_LOCKED);
    val_bool = false;
    FAIL_IF(iopmp_set_global_err_resp(&iopmp, &val_bool) != IOPMP_ERR_REG_IS_LOCKED);
    val_bool = false;
    FAIL_IF(iopmp_set_msi_sel(&iopmp, &val_bool) != IOPMP_ERR_REG_IS_LOCKED);
    val_bool = false;
    FAIL_IF(iopmp_set_stall_violation_en(&iopmp, &val_bool) != IOPMP_ERR_REG_IS_LOCKED);
    val_u64 = 0x9000;
    val_u16 = 0x90;
    FAIL_IF(iopmp_set_msi_info(&iopmp, &val_u64, &val_u16) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Read every field of a captured error report");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(program_full_mdcfg_table(&iopmp) != 0);
    /* RRID(2) is associated with MD(3), whose entries stay all OFF, so any
     * transaction from RRID(2) hits no rule */
    ret = iopmp_set_rrid_md_association(&iopmp, 2, 0x8, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_set_enable(&iopmp) != IOPMP_OK);
    FAIL_IF(iopmp_get_enable(&iopmp) != true);
    /* Enabling again short-circuits */
    FAIL_IF(iopmp_set_enable(&iopmp) != IOPMP_OK);

    receiver_port(2, 364, 0, 0, READ_ACCESS, 0, &iopmp_trans_req);
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF(iopmp_trans_rsp.status != IOPMP_ERROR);

    ret = iopmp_capture_error(&iopmp, &err_report, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_err_report_get_addr(&err_report) != (364 >> 2));
    FAIL_IF(iopmp_err_report_get_rrid(&err_report) != 2);
    FAIL_IF(iopmp_err_report_get_ttype(&err_report) != IOPMP_TTYPE_READ);
    FAIL_IF(iopmp_err_report_get_etype(&err_report) != IOPMP_ETYPE_NOT_HIT);
    FAIL_IF(iopmp_err_report_is_no_hit(&err_report) != true);
    FAIL_IF(iopmp_err_report_is_part_hit(&err_report) != false);
    FAIL_IF(iopmp_err_report_get_msi_werr(&err_report) != false);
    FAIL_IF(iopmp_err_report_get_svc(&err_report) != false);
    /* ERR_REQID.eid is implementation defined. Cross-check it against the
     * register the reference model exposes rather than a magic number */
    val_u32 = read_register(&iopmp_dev, ERR_REQID_OFFSET, 4) >> 16;
    FAIL_IF(iopmp_err_report_get_eid(&err_report) != val_u32);
    END_TEST();

    START_TEST("Capturing an error can invalidate the record");
    /* The record is still valid because the previous capture did not clear it */
    ret = iopmp_capture_error(&iopmp, &err_report, true);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_capture_error(&iopmp, &err_report, false) != IOPMP_ERR_NOT_EXIST);
    END_TEST();

    START_TEST("Error capture APIs when no_err_rec is set");
    /* MFR and ERR_REQID.eid both depend on the error capture record */
    cfg.no_err_rec = true;
    cfg.mfr_en = false;
    cfg.imp_err_reqid_eid = false;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_support_mfr(&iopmp) != false);
    FAIL_IF(iopmp_capture_error(&iopmp, &err_report, false) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_invalidate_error(&iopmp) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_mfr_get_sv_window(&iopmp, &val_u16, &val_u16) != IOPMP_ERR_NOT_SUPPORTED);
    cfg.no_err_rec = false;
    cfg.mfr_en = true;
    cfg.imp_err_reqid_eid = true;
    END_TEST();

    /**********************************************************************/
    /* Programmable HWCFG2.prio_entry and HWCFG3.rrid_transl              */
    /**********************************************************************/
    START_TEST("Program the number of priority entries");
    cfg.prio_ent_prog = true;
    cfg.rrid_transl_prog = true;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_support_programmable_prio_entry(&iopmp) != true);
    val_u16 = 32;
    ret = iopmp_set_prio_entry_num(&iopmp, &val_u16);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u16 != 32);
    FAIL_IF(iopmp_get_prio_entry_num(&iopmp) != 32);
    FAIL_IF(iopmp_set_prio_entry_num(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Lock the number of priority entries");
    ret = iopmp_lock_prio_entry_num(&iopmp);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_get_support_programmable_prio_entry(&iopmp) != false);
    /* Locking again short-circuits */
    FAIL_IF(iopmp_lock_prio_entry_num(&iopmp) != IOPMP_OK);
    val_u16 = 8;
    FAIL_IF(iopmp_set_prio_entry_num(&iopmp, &val_u16) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("Program the RRID tagged to outgoing transactions");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    ret = iopmp_get_rrid_transl_prog(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    FAIL_IF(iopmp_get_rrid_transl_prog(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    val_u16 = 7;
    ret = iopmp_set_rrid_transl(&iopmp, &val_u16);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u16 != 7);
    ret = iopmp_get_rrid_transl(&iopmp, &val_u16);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u16 != 7);
    FAIL_IF(iopmp_set_rrid_transl(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_get_rrid_transl(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Lock the RRID tagged to outgoing transactions");
    ret = iopmp_lock_rrid_transl(&iopmp);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_get_rrid_transl_prog(&iopmp, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != false);
    /* Locking again short-circuits */
    FAIL_IF(iopmp_lock_rrid_transl(&iopmp) != IOPMP_OK);
    val_u16 = 9;
    FAIL_IF(iopmp_set_rrid_transl(&iopmp, &val_u16) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("prio_entry APIs when non-priority entries are unsupported");
    cfg.non_prio_en = false;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u16 = 32;
    FAIL_IF(iopmp_set_prio_entry_num(&iopmp, &val_u16) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_lock_prio_entry_num(&iopmp) != IOPMP_ERR_NOT_SUPPORTED);
    cfg.non_prio_en = true;
    END_TEST();

    START_TEST("rrid_transl APIs when tagging is unsupported");
    cfg.rrid_transl_en = false;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_rrid_transl_prog(&iopmp, &val_bool) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_get_rrid_transl(&iopmp, &val_u16) != IOPMP_ERR_NOT_SUPPORTED);
    val_u16 = 7;
    FAIL_IF(iopmp_set_rrid_transl(&iopmp, &val_u16) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_lock_rrid_transl(&iopmp) != IOPMP_ERR_NOT_SUPPORTED);
    cfg.rrid_transl_en = true;
    cfg.prio_ent_prog = false;
    cfg.rrid_transl_prog = false;
    END_TEST();

    /**********************************************************************/
    /* Stalling transactions                                              */
    /**********************************************************************/
    START_TEST("Stall and resume transactions by MD");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    /* Nothing is stalled yet */
    FAIL_IF(iopmp_transactions_are_stalled(&iopmp, false, &val_bool) != IOPMP_ERR_NOT_EXIST);
    FAIL_IF(iopmp_resume_transactions(&iopmp, false) != IOPMP_ERR_NOT_ALLOWED);
    val_u64 = 0x8;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true);
    FAIL_IF(ret != IOPMP_OK);
    /* The stall was requested with polling, so it has taken effect */
    val_bool = false;
    ret = iopmp_transactions_are_stalled(&iopmp, false, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    /* MDSTALL can only be written once before a resume */
    val_u64 = 0x8;
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, false) != IOPMP_ERR_NOT_ALLOWED);
    FAIL_IF(iopmp_transactions_are_resumed(&iopmp, false, &val_bool) != IOPMP_ERR_NOT_EXIST);
    FAIL_IF(iopmp_resume_transactions(&iopmp, true) != IOPMP_OK);
    val_bool = false;
    ret = iopmp_transactions_are_resumed(&iopmp, false, &val_bool);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_bool != true);
    FAIL_IF(iopmp_resume_transactions(&iopmp, false) != IOPMP_ERR_NOT_ALLOWED);
    END_TEST();

    START_TEST("Probe which MDs MDSTALL can select");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    ret = iopmp_probe_stall_by_md(&iopmp, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != (((uint64_t)1 << CFG_MD_NUM) - 1));
    /* The probe must not leave anything stalled */
    val_u64 = 0x8;
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true) != IOPMP_OK);
    FAIL_IF(iopmp_resume_transactions(&iopmp, true) != IOPMP_OK);
    END_TEST();

    START_TEST("Probe reports only the implemented MDs");
    cfg.md_num = 8;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    ret = iopmp_probe_stall_by_md(&iopmp, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0xFF);
    /* The probe result is cached, so an unselectable MD is now rejected
     * without writing MDSTALL at all */
    val_u64 = 0x100;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, false);
    FAIL_IF(ret != IOPMP_ERR_ILLEGAL_VALUE);
    FAIL_IF(val_u64 != 0);          /* none of the request was selectable */
    FAIL_IF(read_register(&iopmp_dev, MDSTALL_OFFSET, 4) != 0);
    /* ... and the rejection left nothing stalled, so a legal one still works */
    val_u64 = 0x81;
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true) != IOPMP_OK);
    FAIL_IF(val_u64 != 0x81);
    FAIL_IF(iopmp_resume_transactions(&iopmp, true) != IOPMP_OK);
    cfg.md_num = CFG_MD_NUM;
    END_TEST();

    START_TEST("Stall a partially selectable MD bitmap");
    cfg.md_num = 8;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_probe_stall_by_md(&iopmp, &val_u64) != IOPMP_OK);
    /* Bit 3 is selectable, bit 9 is not */
    val_u64 = 0x208;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, false);
    FAIL_IF(ret != IOPMP_ERR_ILLEGAL_VALUE);
    FAIL_IF(val_u64 != 0x8);        /* the selectable subset is reported */
    cfg.md_num = CFG_MD_NUM;
    END_TEST();

    START_TEST("Probe MDSTALL with invalid arguments");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_probe_stall_by_md(&iopmp, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, NULL, false, false) != IOPMP_ERR_INVALID_PARAMETER);
    /* Probing in the middle of a stall is not allowed */
    val_u64 = 0x8;
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true) != IOPMP_OK);
    FAIL_IF(iopmp_probe_stall_by_md(&iopmp, &val_u64) != IOPMP_ERR_NOT_ALLOWED);
    FAIL_IF(iopmp_resume_transactions(&iopmp, true) != IOPMP_OK);
    END_TEST();

    START_TEST("Poll the stall status with invalid arguments");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u64 = 0x8;
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true) != IOPMP_OK);
    FAIL_IF(iopmp_transactions_are_stalled(&iopmp, false, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_resume_transactions(&iopmp, true) != IOPMP_OK);
    FAIL_IF(iopmp_transactions_are_resumed(&iopmp, false, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Stall transactions with the exempt flag");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u64 = 0x8;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, true, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(iopmp_resume_transactions(&iopmp, false) != IOPMP_OK);
    END_TEST();

    START_TEST("Stall a MD above 31 so that MDSTALLH is used");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u64 = (uint64_t)1 << 40;
    ret = iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, true);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != ((uint64_t)1 << 40));
    FAIL_IF(iopmp_resume_transactions(&iopmp, true) != IOPMP_OK);
    END_TEST();

    START_TEST("Cherry pick a RRID with invalid arguments");
    uint32_t stat;
    val_u32 = CFG_RRID_NUM;
    FAIL_IF(iopmp_stall_cherry_pick_rrid(&iopmp, &val_u32, true, &stat) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_stall_cherry_pick_rrid(&iopmp, NULL, true, &stat) != IOPMP_ERR_INVALID_PARAMETER);
    val_u32 = 2;
    FAIL_IF(iopmp_stall_cherry_pick_rrid(&iopmp, &val_u32, true, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    val_u32 = 2;
    FAIL_IF(iopmp_query_stall_stat_by_rrid(&iopmp, &val_u32, &stat) != IOPMP_OK);
    FAIL_IF(stat != IOPMP_RRIDSCP_STAT_NOT_STALLED);
    END_TEST();

    START_TEST("Stall APIs when stalling is unsupported");
    /* RRIDSCP and the stall buffer both depend on the stall feature */
    cfg.stall_en = false;
    cfg.imp_rridscp = false;
    cfg.imp_stall_buffer = false;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_support_stall_by_rrid(&iopmp) != false);
    FAIL_IF(iopmp_get_support_stall(&iopmp) != false);
    val_u64 = 0x8;
    FAIL_IF(iopmp_stall_transactions_by_mds(&iopmp, &val_u64, false, false) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_resume_transactions(&iopmp, false) != IOPMP_ERR_NOT_SUPPORTED);
    val_u32 = 2;
    FAIL_IF(iopmp_stall_cherry_pick_rrid(&iopmp, &val_u32, true, &stat) != IOPMP_ERR_NOT_SUPPORTED);
    val_bool = true;
    FAIL_IF(iopmp_set_stall_violation_en(&iopmp, &val_bool) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_probe_stall_by_md(&iopmp, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    cfg.stall_en = true;
    cfg.imp_rridscp = true;
    cfg.imp_stall_buffer = true;
    END_TEST();

    /**********************************************************************/
    /* SPS extension                                                      */
    /**********************************************************************/
    START_TEST("Set and get all SPS permissions of a RRID at once");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_support_sps(&iopmp) != true);
    ret = iopmp_sps_set_rrid_md_rwx(&iopmp, 2,
                                    0x8, 0,     /* read  */
                                    0x18, 0,    /* write */
                                    0x28, 0,    /* fetch */
                                    &val_u64, &val_u64_2, &val_u64_3);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    FAIL_IF(val_u64_2 != 0x18);
    FAIL_IF(val_u64_3 != 0x28);
    ret = iopmp_sps_get_rrid_md_rwx(&iopmp, 2, &val_u64, &val_u64_2, &val_u64_3);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    FAIL_IF(val_u64_2 != 0x18);
    FAIL_IF(val_u64_3 != 0x28);
    END_TEST();

    START_TEST("Get each SPS permission of a RRID individually");
    ret = iopmp_sps_get_rrid_md_read(&iopmp, 2, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x8);
    ret = iopmp_sps_get_rrid_md_write(&iopmp, 2, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x18);
    ret = iopmp_sps_get_rrid_md_insn_fetch(&iopmp, 2, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x28);
    END_TEST();

    START_TEST("Clear SPS permissions of a RRID");
    ret = iopmp_sps_set_rrid_md_rwx(&iopmp, 2,
                                    0, 0x8,     /* read  */
                                    0, 0x10,    /* write */
                                    0, 0x20,    /* fetch */
                                    &val_u64, &val_u64_2, &val_u64_3);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0);
    FAIL_IF(val_u64_2 != 0x8);
    FAIL_IF(val_u64_3 != 0x8);
    END_TEST();

    START_TEST("SPS APIs with invalid arguments");
    FAIL_IF(iopmp_sps_get_rrid_md_read(&iopmp, CFG_RRID_NUM, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_sps_get_rrid_md_write(&iopmp, CFG_RRID_NUM, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_sps_get_rrid_md_insn_fetch(&iopmp, CFG_RRID_NUM, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_sps_get_rrid_md_read(&iopmp, 2, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_sps_set_rrid_md_read(&iopmp, CFG_RRID_NUM, 0x8, 0, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, NULL) != IOPMP_ERR_INVALID_PARAMETER);
    FAIL_IF(iopmp_sps_set_rrid_md_read(&iopmp, 2, (uint64_t)1 << CFG_MD_NUM, 0, &val_u64) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_sps_get_rrid_md_rwx(&iopmp, CFG_RRID_NUM, &val_u64, &val_u64_2, &val_u64_3) != IOPMP_ERR_OUT_OF_BOUNDS);
    FAIL_IF(iopmp_sps_set_rrid_md_rwx(&iopmp, CFG_RRID_NUM, 0x8, 0, 0x8, 0, 0x8, 0,
                                      &val_u64, &val_u64_2, &val_u64_3) != IOPMP_ERR_OUT_OF_BOUNDS);
    END_TEST();

    START_TEST("SPS APIs are rejected when a MD is locked by MDLCK");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u64 = 0x8;
    FAIL_IF(iopmp_lock_md(&iopmp, &val_u64, false) != IOPMP_OK);
    FAIL_IF(iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64) != IOPMP_ERR_REG_IS_LOCKED);
    END_TEST();

    START_TEST("SPS APIs when the extension is unsupported");
    cfg.sps_en = false;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    FAIL_IF(iopmp_get_support_sps(&iopmp) != false);
    FAIL_IF(iopmp_sps_get_rrid_md_read(&iopmp, 2, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_get_rrid_md_write(&iopmp, 2, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_get_rrid_md_insn_fetch(&iopmp, 2, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_set_rrid_md_read(&iopmp, 2, 0x8, 0, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_set_rrid_md_write(&iopmp, 2, 0x8, 0, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_set_rrid_md_insn_fetch(&iopmp, 2, 0x8, 0, &val_u64) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_get_rrid_md_rwx(&iopmp, 2, &val_u64, &val_u64_2, &val_u64_3) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_sps_set_rrid_md_rwx(&iopmp, 2, 0x8, 0, 0x8, 0, 0x8, 0,
                                      &val_u64, &val_u64_2, &val_u64_3) != IOPMP_ERR_NOT_SUPPORTED);
    cfg.sps_en = true;
    END_TEST();

    /**********************************************************************/
    /* APIs belonging to other SRCMD/MDCFG formats                        */
    /**********************************************************************/
    START_TEST("SRCMD_FMT=2 APIs are rejected on the Full Model");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_bool = true;
    val_bool_2 = true;
    FAIL_IF(iopmp_set_md_permission(&iopmp, 2, 0, &val_bool, &val_bool_2) != IOPMP_ERR_NOT_SUPPORTED);
    IOPMP_SRCMD_PERM_CFG_SET_DIRECT(&perm_cfg, 0x3, 0x1);
    FAIL_IF(iopmp_set_md_permission_multi(&iopmp, 0, &perm_cfg) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_lock_srcmd_table_fmt_2(&iopmp, 0) != IOPMP_ERR_NOT_SUPPORTED);
    FAIL_IF(iopmp_is_srcmd_table_fmt_2_locked(&iopmp, 0, &val_bool) != IOPMP_ERR_NOT_SUPPORTED);
    END_TEST();

    /**********************************************************************/
    /* Initialization rejections                                          */
    /**********************************************************************/
    START_TEST("Detect granularity when the leading entries are in use");
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    val_u32 = iopmp_get_granularity(&iopmp);
    /* Occupy entry 0 ~ 3 so that the probe has to skip over them */
    ret = iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0x1000,
                             IOPMP_ENTRY_RW, 0);
    FAIL_IF(ret != 1);
    for (uint32_t i = 0; i < 4; i++)
        FAIL_IF(iopmp_set_entry(&iopmp, entries, i) != IOPMP_OK);
    /* Re-initializing must still detect the very same granularity */
    FAIL_IF(iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                       IOPMP_IMPID_NOT_SPECIFIED) != IOPMP_OK);
    FAIL_IF(iopmp_get_granularity(&iopmp) != val_u32);
    FAIL_IF(val_u32 != iopmp_dev.granularity);
    END_TEST();

    START_TEST("Initialization fails when no entry is free to probe");
    /* A small entry array so that every entry can be occupied */
    cfg.entry_num = 8;
    cfg.prio_entry = 4;
    FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 0x1000, 0x1000,
                             IOPMP_ENTRY_RW, 0);
    FAIL_IF(ret != 1);
    for (uint32_t i = 0; i < 8; i++)
        FAIL_IF(iopmp_set_entry(&iopmp, entries, i) != IOPMP_OK);
    /* Nothing is left to probe, so libiopmp must report it rather than
     * computing a granularity out of a zero ENTRY_ADDR read-back */
    FAIL_IF(iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                       IOPMP_IMPID_NOT_SPECIFIED) != IOPMP_ERR_NOT_AVAILABLE);
    FAIL_IF(iopmp_is_initialized(&iopmp) != false);
    cfg.entry_num = CFG_ENTRY_NUM;
    cfg.prio_entry = 16;
    END_TEST();

    START_TEST("Initialize with a model no compiled-in driver claims");
    reset_iopmp(&iopmp_dev, &cfg);
    /* No entry of iopmp_drivers[] claims a reserved SRCMD_FMT */
    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_RESERVED, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_ERR_NOT_SUPPORTED);
    END_TEST();

    START_TEST("Initialize with a model this instance does not report");
    reset_iopmp(&iopmp_dev, &cfg);
    /* The isolation driver is compiled in and claims SRCMD_FMT=1/MDCFG_FMT=0,
     * but this instance reports the Full Model in HWCFG3, so its
     * iopmp_drv_init_common() must reject the mismatch */
    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_1, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_ERR_NOT_SUPPORTED);
    END_TEST();

    START_TEST("A generic override applies to one instance, not the others");
    {
        IOPMP_t iopmp_ovr;

        /* Two instances in one image: one with an override, one without */
        FAIL_IF(libiopmp_setup(&iopmp_ovr, &cfg) != IOPMP_OK);
        iopmp_ovr.ops_override = &fake_ops_override;
        FAIL_IF(libiopmp_setup(&iopmp, &cfg) != IOPMP_OK);

        /* Member filled in: the override runs and ERR_CFG.ie stays clear */
        fake_set_global_intr_hits = 0;
        FAIL_IF(iopmp_set_global_intr(&iopmp_ovr, true) != IOPMP_OK);
        FAIL_IF(fake_set_global_intr_hits != 1);
        FAIL_IF(read_register(&iopmp_dev, ERR_CFG_OFFSET, 4) & 0x2);

        /* ops_generic is NULL here, so the built-in runs and sets ERR_CFG.ie */
        FAIL_IF(iopmp_set_global_intr(&iopmp, true) != IOPMP_OK);
        FAIL_IF(fake_set_global_intr_hits != 1);
        FAIL_IF(!(read_register(&iopmp_dev, ERR_CFG_OFFSET, 4) & 0x2));

        /* Member left NULL: the override instance falls back to the built-in */
        FAIL_IF(iopmp_lock_err_cfg(&iopmp_ovr) != IOPMP_OK);
        FAIL_IF(!(read_register(&iopmp_dev, ERR_CFG_OFFSET, 4) & 0x1));
    }
    END_TEST();

    START_TEST("capture_error dispatches an invalidate_error override");
    {
        IOPMP_t iopmp_ovr;

        FAIL_IF(libiopmp_setup(&iopmp_ovr, &cfg) != IOPMP_OK);
        iopmp_ovr.ops_override = &fake_ops_override;
        /* RRID 2 is associated with a MD that owns no entry, so the access
         * below hits no rule and leaves a record behind */
        FAIL_IF(iopmp_set_rrid_md_association(&iopmp_ovr, 2, 0x8, 0, &val_u64,
                                              false) != IOPMP_OK);
        FAIL_IF(iopmp_set_enable(&iopmp_ovr) != IOPMP_OK);
        receiver_port(2, 364, 0, 0, READ_ACCESS, 0, &iopmp_trans_req);
        iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp,
                              &intrpt);
        FAIL_IF(iopmp_trans_rsp.status != IOPMP_ERROR);

        /* capture_error is the built-in here, but invalidate_error is not:
         * the built-in has to reach the override rather than clear the record
         * itself, so ERR_INFO.v survives */
        fake_invalidate_error_hits = 0;
        FAIL_IF(iopmp_capture_error(&iopmp_ovr, &err_report, true) != IOPMP_OK);
        FAIL_IF(fake_invalidate_error_hits != 1);
        FAIL_IF(!(read_register(&iopmp_dev, ERR_INFO_OFFSET, 4) & 0x1));
    }
    END_TEST();
#endif

    free(memory);

#if (SRC_ENFORCEMENT_EN)
    START_TEST("Test SourceEnforcement Enable Feature");
    reset_iopmp(&iopmp_dev, &cfg);

    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_rrid_md_association(&iopmp, 0, 0x1, 0, &val_u64, false);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x1);
    ret = iopmp_sps_set_rrid_md_read(&iopmp, 0, 0x1, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x1);
    ret = iopmp_sps_set_rrid_md_write(&iopmp, 0, 0x1, 0, &val_u64);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u64 != 0x1);
    val_u32 = 2;
    ret = iopmp_set_md_entry_association(&iopmp, 0, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 2);
    ret = iopmp_encode_entry(&iopmp, entries, 1, 360, 8,
                             IOPMP_ENTRY_W | IOPMP_ENTRY_R, 0);
    FAIL_IF(ret != 1);  // Single NAPOT entry
    FAIL_IF(entries[0].addr != (360 >> 2));
    FAIL_IF(entries[0].cfg != (IOPMP_ENTRY_A_NAPOT | IOPMP_ENTRY_W | IOPMP_ENTRY_R));
    ret = iopmp_set_entry(&iopmp, entries, 0);
    FAIL_IF(ret != IOPMP_OK);
    ret = iopmp_set_enable(&iopmp);
    FAIL_IF(ret != IOPMP_OK);

    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    receiver_port(12, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp_dev, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp_dev, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp_dev, ERR_INFO_OFFSET, 0, 4);
    END_TEST();
#endif

    return 0;
}
