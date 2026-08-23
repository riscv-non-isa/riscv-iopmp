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

int main(void)
{
    IOPMP_t iopmp = {0};
    enum iopmp_error ret;
    uintptr_t addr;
    uint16_t val_u16;
    uint32_t val_u32;
    int val_int;
    bool val_bool;

    FAIL_IF(create_memory(1) < 0)

    // Configure and reset IOPMP device
    cfg.vendor = 1;
    cfg.specver = 1;
    cfg.impid = 0;
    cfg.no_err_rec = false;
    cfg.md_num = 63;
    cfg.addrh_en = true;
    cfg.tor_en = true;
    cfg.rrid_num = 64;
    cfg.entry_num = 512;
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

    reset_iopmp(&iopmp_dev, &cfg);

    // Read the registers
    hwcfg0_t hwcfg0;
    hwcfg0.raw = read_register(&iopmp_dev, HWCFG0_OFFSET, 4);
    hwcfg1_t hwcfg1;
    hwcfg1.raw = read_register(&iopmp_dev, HWCFG1_OFFSET, 4);
    hwcfg2_t hwcfg2;
    if (hwcfg0.HWCFG2_en) {
        hwcfg2.raw = read_register(&iopmp_dev, HWCFG2_OFFSET, 4);
    } else {
        hwcfg2.raw = 0;
    }
    hwcfg3_t hwcfg3;
    if (hwcfg0.HWCFG3_en) {
        hwcfg3.raw = read_register(&iopmp_dev, HWCFG3_OFFSET, 4);
    } else {
        hwcfg3.raw = 0;
    }

    /* Call libiopmp API to initialize this IOPMP instance */
    START_TEST("Initialize IOPMP");
    ret = iopmp_init(&iopmp, 0, IOPMP_SRCMD_FMT_0, IOPMP_MDCFG_FMT_0,
                     IOPMP_IMPID_NOT_SPECIFIED);
    FAIL_IF(ret != IOPMP_OK);
    END_TEST();

    /* Start unit tests */

    START_TEST("Get major version of libiopmp");
    val_int = libiopmp_major_version();
    FAIL_IF(val_int != LIBIOPMP_VERSION_MAJOR);
    END_TEST();

    START_TEST("Get minor version of libiopmp");
    val_int = libiopmp_minor_version();
    FAIL_IF(val_int != LIBIOPMP_VERSION_MINOR);
    END_TEST();

    START_TEST("Get extra version of libiopmp");
    val_int = libiopmp_extra_version();
    FAIL_IF(val_int != LIBIOPMP_VERSION_EXTRA);
    END_TEST();

    START_TEST("Check given version is equal to the version of libiopmp");
    val_bool = libiopmp_check_version(LIBIOPMP_VERSION_MAJOR,
                                      LIBIOPMP_VERSION_MINOR,
                                      LIBIOPMP_VERSION_EXTRA);
    FAIL_IF(val_bool != false);
    END_TEST();

    START_TEST("Check given major version is greater than version of libiopmp");
    val_bool = libiopmp_check_version(LIBIOPMP_VERSION_MAJOR + 1,
                                      LIBIOPMP_VERSION_MINOR,
                                      LIBIOPMP_VERSION_EXTRA);
    FAIL_IF(val_bool != true);
    END_TEST();

    START_TEST("Check given minor version is greater than version of libiopmp");
    val_bool = libiopmp_check_version(LIBIOPMP_VERSION_MAJOR,
                                      LIBIOPMP_VERSION_MINOR + 1,
                                      LIBIOPMP_VERSION_EXTRA);
    FAIL_IF(val_bool != true);
    END_TEST();

    START_TEST("Check given extra version is greater than version of libiopmp");
    val_bool = libiopmp_check_version(LIBIOPMP_VERSION_MAJOR,
                                      LIBIOPMP_VERSION_MINOR,
                                      LIBIOPMP_VERSION_EXTRA + 1);
    FAIL_IF(val_bool != true);
    END_TEST();

    START_TEST("Check IOPMP has been initialized by libiopmp");
    val_bool = iopmp_is_initialized(&iopmp);
    FAIL_IF(val_bool != true);
    END_TEST();

    START_TEST("Get base address of IOPMP");
    addr = iopmp_get_base_addr(&iopmp);
    FAIL_IF(addr != (uintptr_t)0);
    END_TEST();

    START_TEST("Check MDCFG table format");
    uint32_t mdcfg_fmt = iopmp_get_mdcfg_fmt(&iopmp);
    FAIL_IF(mdcfg_fmt != (uint32_t)hwcfg3.mdcfg_fmt);
    END_TEST();

    START_TEST("Check SRCMD table format");
    uint32_t srcmd_fmt = iopmp_get_srcmd_fmt(&iopmp);
    FAIL_IF(srcmd_fmt != (uint32_t)hwcfg3.srcmd_fmt);
    END_TEST();

    START_TEST("Check IOPMP supports TOR or not");
    val_bool = iopmp_get_support_tor(&iopmp);
    FAIL_IF(val_bool != hwcfg0.tor_en);
    END_TEST();

    START_TEST("Check IOPMP supports SPS extension or not");
    val_bool = iopmp_get_support_sps(&iopmp);
    FAIL_IF(val_bool != hwcfg2.sps_en);
    END_TEST();

    START_TEST("Check IOPMP supports programmable prio_entry or not");
    val_bool = iopmp_get_support_programmable_prio_entry(&iopmp);
    FAIL_IF(val_bool != hwcfg2.prio_ent_prog);
    END_TEST();

    START_TEST("Check IOPMP supports tagging a new RRID or not");
    val_bool = iopmp_get_support_rrid_transl(&iopmp);
    FAIL_IF(val_bool != hwcfg3.rrid_transl_en);
    END_TEST();

    START_TEST("Check IOPMP rrid_transl is programmable or not");
    ret = iopmp_get_rrid_transl_prog(&iopmp, &val_bool);
    if (hwcfg3.rrid_transl_en) {
        FAIL_IF(ret != IOPMP_OK);
        FAIL_IF(val_bool != hwcfg3.rrid_transl_prog);
    } else {
        FAIL_IF(ret != IOPMP_ERR_NOT_SUPPORTED);
    }
    END_TEST();

    START_TEST("Check IOPMP implements the check of instruction fetch or not");
    val_bool = iopmp_get_support_chk_x(&iopmp);
    FAIL_IF(val_bool != !hwcfg3.xinr);
    END_TEST();

    START_TEST("Check IOPMP always fails on an instruction fetch or not");
    val_bool = iopmp_get_no_x(&iopmp);
    FAIL_IF(val_bool != hwcfg3.no_x);
    END_TEST();

    START_TEST("Check IOPMP always fails on an write accesses or not");
    val_bool = iopmp_get_no_w(&iopmp);
    FAIL_IF(val_bool != hwcfg3.no_w);
    END_TEST();

    START_TEST("Check IOPMP implements stall-related features or not");
    val_bool = iopmp_get_support_stall(&iopmp);
    FAIL_IF(val_bool != hwcfg2.stall_en);
    END_TEST();

    START_TEST("Check IOPMP implements interrupt suppression per entry or not");
    val_bool = iopmp_get_support_peis(&iopmp);
    FAIL_IF(val_bool != hwcfg2.peis);
    END_TEST();

    START_TEST("Check IOPMP implements the error suppression per entry or not");
    val_bool = iopmp_get_support_pees(&iopmp);
    FAIL_IF(val_bool != hwcfg2.pees);
    END_TEST();

    START_TEST("Check IOPMP implements Multi Faults Record Extension or not");
    val_bool = iopmp_get_support_mfr(&iopmp);
    FAIL_IF(val_bool != hwcfg2.mfr_en);
    END_TEST();

    START_TEST("Get the supported number of MD");
    val_u32 = iopmp_get_md_num(&iopmp);
    FAIL_IF(val_u32 != hwcfg0.md_num);
    END_TEST();

    START_TEST("Check IOPMP implements ENTRY_ADDRH(i) and ERR_MSIADDRH");
    val_bool = iopmp_get_addrh_en(&iopmp);
    FAIL_IF(val_bool != hwcfg0.addrh_en);
    END_TEST();

    START_TEST("Check IOPMP checks transactions by default");
    val_bool = iopmp_get_enable(&iopmp);
    FAIL_IF(val_bool != hwcfg0.enable);
    END_TEST();

    START_TEST("Get the supported number of RRID");
    val_u32 = iopmp_get_rrid_num(&iopmp);
    FAIL_IF(val_u32 != hwcfg1.rrid_num);
    END_TEST();

    START_TEST("Get the supported number of entries");
    val_u32 = iopmp_get_entry_num(&iopmp);
    FAIL_IF(val_u32 != hwcfg1.entry_num);
    END_TEST();

    START_TEST("Get the number of entries matched with priority");
    val_u32 = iopmp_get_prio_entry_num(&iopmp);
    FAIL_IF(val_u32 != hwcfg2.prio_entry);
    END_TEST();

    START_TEST("Get the RRID tagged to outgoing transactions");
    ret = iopmp_get_rrid_transl(&iopmp, &val_u16);
    if (hwcfg3.rrid_transl_en) {
        FAIL_IF(ret != IOPMP_OK);
        FAIL_IF(val_u16 != hwcfg3.rrid_transl);
    } else {
        FAIL_IF(ret != IOPMP_ERR_NOT_SUPPORTED);
    }
    END_TEST();

    START_TEST("Get vendor ID correctly");
    ret = iopmp_get_vendor_id(&iopmp, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 1);
    END_TEST();

    START_TEST("Get vendor ID with invalid argument");
    ret = iopmp_get_vendor_id(&iopmp, NULL);
    FAIL_IF(ret != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Get specification version correctly");
    ret = iopmp_get_specver(&iopmp, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 1);
    END_TEST();

    START_TEST("Get specification version with invalid argument");
    ret = iopmp_get_specver(&iopmp, NULL);
    FAIL_IF(ret != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Get the user-defined implementation ID correctly");
    ret = iopmp_get_impid(&iopmp, &val_u32);
    FAIL_IF(ret != IOPMP_OK);
    FAIL_IF(val_u32 != 0);
    END_TEST();

    START_TEST("Get the user-defined implementation ID with invalid argument");
    ret = iopmp_get_impid(&iopmp, NULL);
    FAIL_IF(ret != IOPMP_ERR_INVALID_PARAMETER);
    END_TEST();

    START_TEST("Get base address of IOPMP entry array");
    addr = iopmp_get_base_addr_entry_array(&iopmp);
    FAIL_IF(addr != (uintptr_t)iopmp_dev.reg_file.entryoffset.offset);
    END_TEST();

    return 0;
}
