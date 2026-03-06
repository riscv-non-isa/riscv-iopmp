/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author:  Gull Ahmed (gull.ahmed@10xengineers.ai)
// Date: October 21, 2024
// Description: This file contains all the tests that are used to test
// full model.
// Max Supported RRIDs: 65536
// Max Supported MDs: 63
***************************************************************************/

#include "iopmp.h"
#include "config.h"
#include "test_utils.h"

// Declarations
iopmp_trans_req_t iopmp_trans_req;
iopmp_trans_rsp_t iopmp_trans_rsp;
err_info_t err_info_temp;

int main()
{
    // Create IOPMP instance
    iopmp_dev_t iopmp = {0};
    iopmp_cfg_t cfg = {0}, cfg_saved;
    uint8_t intrpt;

    FAIL_IF(create_memory(1) < 0)

    // Configure your IOPMP when reset
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
    cfg.sps_en = true;
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

    START_TEST("Test entry granularity and encoding of ENTRY_ADDR(H)");
    uint32_t entry_addr, entry_addrh;
    cfg.granularity = MIN_GRANULARITY;  // G=0
    reset_iopmp(&iopmp, &cfg);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 5678 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);
    entry_addr  = read_entry_n(&iopmp, ENTRY_ADDR,  1, 4);
    entry_addrh = read_entry_n(&iopmp, ENTRY_ADDRH, 1, 4);
    FAIL_IF((entry_addr != (5678 >> 2)));
    FAIL_IF((entry_addrh != 0));
    cfg.granularity = (1ULL << (1+2));  // G=1
    reset_iopmp(&iopmp, &cfg);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 5678 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);  // Will be OFF because G=1
    entry_addr  = read_entry_n(&iopmp, ENTRY_ADDR,  1, 4);
    entry_addrh = read_entry_n(&iopmp, ENTRY_ADDRH, 1, 4);
    FAIL_IF((entry_addr != ((5678 >> 2) & ~0x1)));  // OFF. Bits [G-1:0] read as all zeros
    FAIL_IF((entry_addrh != 0));
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | R), 4);
    entry_addr  = read_entry_n(&iopmp, ENTRY_ADDR,  1, 4);
    entry_addrh = read_entry_n(&iopmp, ENTRY_ADDRH, 1, 4);
    FAIL_IF((entry_addr != ((5678 >> 2) & ~0x1)));  // TOR. Bits [G-1:0] read as all zeros
    FAIL_IF((entry_addrh != 0));
    cfg.granularity = (1ULL << (35+2)); // G=35
    reset_iopmp(&iopmp, &cfg);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, (0xFF), 4);
    configure_entry_n(&iopmp, ENTRY_ADDRH, 1, (0xF2000000FF) >> 32, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);  // Will be OFF because G=35
    entry_addr  = read_entry_n(&iopmp, ENTRY_ADDR,  1, 4);
    entry_addrh = read_entry_n(&iopmp, ENTRY_ADDRH, 1, 4);
    FAIL_IF((entry_addr != (0)));       // OFF. Bits [G-1:0] read as all zeros
    FAIL_IF((entry_addrh != (0xF0)));   // OFF. Bits [G-1:0] read as all zeros
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | R), 4);
    entry_addr  = read_entry_n(&iopmp, ENTRY_ADDR,  1, 4);
    entry_addrh = read_entry_n(&iopmp, ENTRY_ADDRH, 1, 4);
    FAIL_IF((entry_addr != (0)));       // TOR. Bits [G-1:0] read as all zeros
    FAIL_IF((entry_addrh != (0xF0)));   // TOR. Bits [G-1:0] read as all zeros
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4);
    entry_addr  = read_entry_n(&iopmp, ENTRY_ADDR,  1, 4);
    entry_addrh = read_entry_n(&iopmp, ENTRY_ADDRH, 1, 4);
    FAIL_IF((entry_addr != (0xFFFFFFFF)));  // NAPOT. Bits [G-2:0] read as all ones
    FAIL_IF((entry_addrh != (0xF3)));       // NAPOT. Bits [G-2:0] read as all ones
    END_TEST();

    // Reset granularity for following tests
    cfg.granularity = MIN_GRANULARITY;

#if (SRC_ENFORCEMENT_EN == 0)
    START_TEST("Test OFF - Read Access permissions");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (OFF | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 0, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test OFF - Write Access permissions");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (OFF | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 0, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test OFF - Instruction Fetch permissions");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (OFF | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 0, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test OFF - UNKNOWN RRID ERROR");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (OFF | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(70, 364, 0, 0, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, UNKNOWN_RRID);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en,
                  "Test TOR - Partial hit on a priority rule error",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4); // SRCMD_EN[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_R, 2, 0x10, 4);  // SRCMD_R[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);              // MDCFG[3].t contains 2
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, PARTIAL_HIT_ON_PRIORITY);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en, "Test TOR - 4Byte Read Access",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4); // SRCMD_EN[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_R, 2, 0x10, 4);  // SRCMD_R[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);              // MDCFG[3].t contains 2
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en && iopmp.reg_file.hwcfg2.sps_en,
                  "Test TOR - 4Byte Read Access with SRCMD_R not set",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4); // SRCMD_EN[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);              // MDCFG[3].t contains 2
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en && !iopmp.reg_file.hwcfg2.sps_en,
                  "Test TOR - 4Byte Read Access, SRCMD_R not set, SPS disabled",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);                                                // SRCMD_EN[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);                                                             // MDCFG[3].t contains 2
    write_register(&iopmp, HWCFG0_OFFSET, read_register(&iopmp, HWCFG0_OFFSET, 4) & 0xFFFFFFDF, 4); // Disabling SPS extension
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en, "Test TOR - 4Byte AMO Write Access",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4); // SRCMD_EN[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_R, 2, 0x10, 4);  // SRCMD_R[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_W, 2, 0x10, 4);  // SRCMD_W[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);              // MDCFG[3].t contains 2
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | W | R), 4); // IOPMP_ENTRY[1] contains ENTRY_CFG - (TOR with read and write permissions)
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en,
                  "Test TOR - 4Byte Non-AMO Write Access",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4); // SRCMD_EN[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_W, 2, 0x10, 4);  // SRCMD_W[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);              // MDCFG[3].t contains 2
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | W), 4); // IOPMP_ENTRY[1] contains ENTRY_CFG - (TOR with write permissions)
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 2, WRITE_ACCESS, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg0.tor_en, "Test TOR - 4Byte Write Access",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4); // SRCMD_EN[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_R, 2, 0x10, 4);  // SRCMD_R[2] is associated with MD[3]
    configure_srcmd_n(&iopmp, SRCMD_W, 2, 0x10, 4);  // SRCMD_W[2] is associated with MD[3]
    configure_mdcfg_n(&iopmp, 3, 2, 4);              // MDCFG[3].t contains 2
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 368 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (TOR | W), 4); // IOPMP_ENTRY[1] contains ENTRY_CFG - (TOR with write permissions)
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 4Byte Read Access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte No Read Access error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.sps_en,
                  "Test NA4 - 4Byte No SPS Read Access error",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x00, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 4Byte Write Access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte Non-AMO Write Access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | W), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte No Write Access error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.sps_en,
                  "Test NA4 - 4Byte No SPS Write Access error",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x00, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 4Byte Execute Access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | X | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - 4Byte No Execute Access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.sps_en,
                  "Test NA4 - 4Byte No SPS.X, Execute Access",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 32, 0x00, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | X | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 2, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NA4 - 8Byte Access error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 364, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, PARTIAL_HIT_ON_PRIORITY);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NA4 - For exact 4 Byte error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 364 >> 2, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NA4 | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 368, 0, 0, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte read access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte read access error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, READ_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte write access error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_WRITE_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte write access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte Non-AMO write access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | W), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte Instruction access error");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test NAPOT - 8 Byte Instruction access");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.non_prio_en,
                  "Test NAPOT - 8 Byte Instruction access for non-priority Entry",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 31, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 31, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 17, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 74, 4); // (300 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x20, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 32, 0x20, 4);
    configure_mdcfg_n(&iopmp, 4, 25, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 18, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 18, (NAPOT), 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 20, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 20, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test NAPOT - 8 Byte Instruction access when xinr=1");
    cfg.xinr = true;
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    iopmp_trans_req.perm = READ_ACCESS; // Since xinr=1, we expect ttype="Read access" in CHECK_IOPMP_TRANS()
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_READ_ACCESS);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    cfg.xinr = false;
    END_TEST();

    START_TEST_IF(iopmp.imp_mdlck, "Test MDLCK, updating locked srcmd_en field",
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDLCK_OFFSET, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.imp_mdlck, "Test MDLCK, updating unlocked srcmd_en field",
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDLCK_OFFSET, 0x8, 4);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test MDCFG_LCK, updating locked MDCFG field");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDCFGLCK_OFFSET, 0x8, 4); // MD[0]-MD[3] are locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test MDCFG_LCK, updating unlocked MDCFG field");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDCFGLCK_OFFSET, 0x4, 4); // MD[0]-MD[1] are locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test Entry_LCK, updating locked ENTRY field");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x8, 4); // ENTRY[0]-ENTRY[3] are locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test Entry_LCK, updating unlocked ENTRY field");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x8, 4); // ENTRY[0]-ENTRY[3] are locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 5, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 4, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 4, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test SRCMD_EN lock bit, updating locked SRCMD Table");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x1, 4); // SRCMD_EN[2] lock bit is set
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test SRCMD_EN lock bit, updating unlocked SRCMD Table");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 1, 0x1, 4); // SRCMD_EN[1] lock bit is set
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 5, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 4, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 4, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.imp_mdlck, "Test MDLCK register lock bit",
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDLCK_OFFSET, 0x8, 4);  // MD[2] is locked
    write_register(&iopmp, MDLCK_OFFSET, 0x1, 4);  // Locking MDLCK register
    write_register(&iopmp, MDLCK_OFFSET, 0x10, 4); // Trying to lock MD[3] but it shouldn't be locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test MDCFG_LCK register lock bit");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDCFGLCK_OFFSET, 0x8, 4); // MD[0]-MD[3] are locked
    write_register(&iopmp, MDCFGLCK_OFFSET, 0x1, 4); // MDCFGLCK is locked
    write_register(&iopmp, MDCFGLCK_OFFSET, 0x4, 4); // Updating locked MD's MD[0]-MD[1] are locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("Test Entry_LCK register lock bit");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x8, 4); // ENTRY[0]-ENTRY[3] are locked
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x1, 4); // ENTRYLCK is locked
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x2, 4); // ENTRY[0] is locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.mfr_en, "Test MFR Extension",
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x8, 4); // ENTRY[0]-ENTRY[3] are locked
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x1, 4); // ENTRYLCK is locked
    write_register(&iopmp, ENTRYLCK_OFFSET, 0x2, 4); // ENTRY[0] is locked
    configure_srcmd_n(&iopmp, SRCMD_EN, 2, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 2, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    err_mfr_t err_mfr_temp;
    err_mfr_temp.raw = read_register(&iopmp, ERR_MFR_OFFSET, 4);
    FAIL_IF((err_mfr_temp.svw != 0));
    FAIL_IF((err_mfr_temp.svs != 0));
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    err_mfr_temp.raw = read_register(&iopmp, ERR_MFR_OFFSET, 4);
    FAIL_IF((err_mfr_temp.svi != 0));
    FAIL_IF((err_mfr_temp.svs != 1));
    FAIL_IF((err_mfr_temp.svw != 4));
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.imp_mdlck, "Test MDLCK, updating locked srcmd_enh field",
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDLCKH_OFFSET, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, NOT_HIT_ANY_RULE);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.imp_mdlck, "Test MDLCK, updating unlocked srcmd_enh field",
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, MDLCKH_OFFSET, 0x2, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg2.peis, "Test Interrupt Suppression is Enabled",
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x2, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);                // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (SIXE | NAPOT | R), 4); // Address Mode is NAPOT, with read permission and exe suppression
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt == 1)); // Interrupt is suppressed
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test Interrupt Suppression is disabled");
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x2, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt == 0)); // Interrupt is not suppressed
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.pees, "Test Error Suppression is Enabled",
    // Receiver Port Signals
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x4, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);                // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (SEXE | NAPOT | R), 4); // Address Mode is NAPOT, with read permission and exe suppression
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_SUCCESS));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != USER));
    error_record_chk(&iopmp, NO_ERROR, INSTR_FETCH, 360, 0);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg2.pees,
                  "Test Error Suppression is Enabled but rs is zero",
    // Receiver Port Signals
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);                // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (SEXE | NAPOT | R), 4); // Address Mode is NAPOT, with read permission and exe suppression
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_SUCCESS));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != USER));
    error_record_chk(&iopmp, NO_ERROR, INSTR_FETCH, 360, 0);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test Error Suppression is disabled");
    // Receiver Port Signals
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);         // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4); // Address Mode is NAPOT, with read permission and exe suppression
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_ERROR));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != 0));
    error_record_chk(&iopmp, ILLEGAL_INSTR_FETCH, INSTR_FETCH, 360, 1);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.peis && iopmp.reg_file.hwcfg2.pees,
                  "Test Interrupt and Error Suppression is Enabled",
    // Receiver Port Signals
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x6, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);                       // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (SEXE | SIXE | NAPOT | R), 4); // Address Mode is NAPOT, with read permission and exe suppression
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt == 1));
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_SUCCESS));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    FAIL_IF((iopmp_trans_rsp.user != USER));
    error_record_chk(&iopmp, ILLEGAL_INSTR_FETCH, INSTR_FETCH, 360, 0);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST("Test Interrupt and Error Suppression is disabled");
    // Receiver Port Signals
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x2, 4);
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);         // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4); // Address Mode is NAPOT, with read permission and exe suppression
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((intrpt != 1));
    FAIL_IF((iopmp_trans_rsp.status != IOPMP_ERROR));
    FAIL_IF((iopmp_trans_rsp.rrid != 2));
    error_record_chk(&iopmp, ILLEGAL_INSTR_FETCH, INSTR_FETCH, 360, 1);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();

    START_TEST("RRIDSCP read/write behaviors");
    cfg_saved = cfg;
    cfg.stall_en = true;
    cfg.imp_rridscp = false;
    reset_iopmp(&iopmp, &cfg);
    rridscp_t rridscp_rb;
    // If RRIDSCP is not implemented, it always returns zero. Software can test
    // if it is implemented by writing a zero and then reading it back.
    write_register(&iopmp, RRIDSCP_OFFSET, 0, 4);
    rridscp_rb.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_rb.raw != 0));

    cfg.imp_rridscp = true;
    cfg.rridscp_unselectable[cfg.rrid_num - 1] = true;  // Choose RRID(rrid_num-1) as unselectable
    reset_iopmp(&iopmp, &cfg);
    // Any IOPMP implementing RRIDSCP should not return a zero in RRIDSCP.stat in this case.
    write_register(&iopmp, RRIDSCP_OFFSET, 0, 4);
    rridscp_rb.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_rb.raw == 0));

    // If an out-of-bounds unimplemented RRID is written into RRIDSCP.rrid,
    // the written RRID is ignored by the model.
    rridscp_rb.rrid = cfg.rrid_num;
    write_register(&iopmp, RRIDSCP_OFFSET, rridscp_rb.raw, 4);
    rridscp_rb.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_rb.rrid == cfg.rrid_num));

    // If an unselectable RRID is written into RRIDSCP.rrid, the model returns RRIDSCP.stat = 3.
    rridscp_rb.rrid = cfg.rrid_num - 1;
    write_register(&iopmp, RRIDSCP_OFFSET, rridscp_rb.raw, 4);
    rridscp_rb.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_rb.rrid != (cfg.rrid_num - 1)));
    FAIL_IF((rridscp_rb.stat != RRIDSCP_STAT_INVALID_RRID));

    // Query valid RRID
    rridscp_rb.rrid = 0;
    write_register(&iopmp, RRIDSCP_OFFSET, rridscp_rb.raw, 4);
    rridscp_rb.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_rb.rrid != 0));
    FAIL_IF((rridscp_rb.stat != RRIDSCP_STAT_RRID_NOT_STALLED));

    // Reset configuration
    cfg = cfg_saved;
    END_TEST();

    START_TEST_IF(iopmp.reg_file.hwcfg2.stall_en && iopmp.imp_rridscp, "Stall MD Feature",
    cfg.imp_stall_buffer = true;
    cfg.stall_buffer_size = 32;
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 5, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 5, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    write_register(&iopmp, MDSTALL_OFFSET, 0x10, 4);
    write_register(&iopmp, RRIDSCP_OFFSET, 5, 4);
    receiver_port(5, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.rrid_stalled != 1));
    rridscp_t rridscp_temp;
    rridscp_temp.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_temp.stat != 1));
    FAIL_IF((iopmp_trans_rsp.rrid != 5));
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg2.stall_en && iopmp.imp_rridscp,
                  "Faulting Stalled Transactions Feature",
    cfg.imp_stall_buffer = false;
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_EN, 5, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_X, 5, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | X), 4);
    set_hwcfg0_enable(&iopmp);
    write_register(&iopmp, MDSTALL_OFFSET, 0x10, 4);
    write_register(&iopmp, RRIDSCP_OFFSET, 5, 4);
    receiver_port(5, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.rrid_stalled == 1));
    rridscp_t rridscp_temp;
    rridscp_temp.raw = read_register(&iopmp, RRIDSCP_OFFSET, 4);
    FAIL_IF((rridscp_temp.stat != 1));
    FAIL_IF((iopmp_trans_rsp.rrid != 5));
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, STALLED_TRANSACTION);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    // Reset configuration
    cfg.imp_stall_buffer = true;
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg3.rrid_transl_en, "Test Cascading IOPMP Feature",
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 32, 0x10, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 32, 0x10, 4);
    configure_mdcfg_n(&iopmp, 3, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    FAIL_IF((iopmp_trans_rsp.rrid_transl != iopmp.reg_file.hwcfg3.rrid_transl));
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg2.msi_en, "Test MSI Write error",
    uint64_t read_data;
    reset_iopmp(&iopmp, &cfg);
    bus_error = 0x8000;
    write_register(&iopmp, ERR_CFG_OFFSET, 0x8F0A, 4);
    if (iopmp.reg_file.hwcfg0.addrh_en) {
        write_register(&iopmp, ERR_MSIADDR_OFFSET, 0x8000, 4);
    } else {
        write_register(&iopmp, ERR_MSIADDR_OFFSET, (0x8000 >> 2), 4);
    }
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    bus_error = 0;
    read_memory(0x8000, 4, &read_data);
    FAIL_IF(intrpt == 1);
    FAIL_IF(read_data == 0x8F); // Interrupt is not suppressed
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)

    START_TEST_IF(iopmp.reg_file.hwcfg2.msi_en, "Test MSI",
    uint64_t read_data;
    reset_iopmp(&iopmp, &cfg);
    write_register(&iopmp, ERR_CFG_OFFSET, 0x8F0A, 4);
    if (iopmp.reg_file.hwcfg0.addrh_en) {
        write_register(&iopmp, ERR_MSIADDR_OFFSET, 0x8000, 4);
    } else {
        write_register(&iopmp, ERR_MSIADDR_OFFSET, (0x8000 >> 2), 4);
    }
    configure_srcmd_n(&iopmp, SRCMD_ENH, 2, 0x1, 4);
    configure_srcmd_n(&iopmp, SRCMD_XH, 2, 0x1, 4);
    configure_mdcfg_n(&iopmp, 31, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 1, 90, 4);
    configure_entry_n(&iopmp, ENTRY_CFG, 1, (NAPOT | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(2, 360, 0, 3, INSTR_FETCH, 0, &iopmp_trans_req);
    // Requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    read_memory(0x8000, 4, &read_data);
    FAIL_IF(intrpt == 1);
    FAIL_IF(read_data != 0x8F); // Interrupt is not suppressed
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_ERROR, ILLEGAL_INSTR_FETCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();)
#endif

    free(memory);

#if (SRC_ENFORCEMENT_EN)
    START_TEST("Test SourceEnforcement Enable Feature");
    reset_iopmp(&iopmp, &cfg);
    configure_srcmd_n(&iopmp, SRCMD_EN, 0, 0x02, 4);
    configure_srcmd_n(&iopmp, SRCMD_R, 0, 0x02, 4);
    configure_srcmd_n(&iopmp, SRCMD_W, 0, 0x02, 4);
    configure_mdcfg_n(&iopmp, 0, 2, 4);
    configure_entry_n(&iopmp, ENTRY_ADDR, 0, 90, 4); // (364 >> 2) and keeping lsb 0
    configure_entry_n(&iopmp, ENTRY_CFG, 0, (NAPOT | W | R), 4);
    set_hwcfg0_enable(&iopmp);
    receiver_port(32, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    receiver_port(12, 360, 0, 3, WRITE_ACCESS, 1, &iopmp_trans_req);
    // requestor Port Signals
    iopmp_validate_access(&iopmp, &iopmp_trans_req, &iopmp_trans_rsp, &intrpt);
    CHECK_IOPMP_TRANS(&iopmp, IOPMP_SUCCESS, ENTRY_MATCH);
    write_register(&iopmp, ERR_INFO_OFFSET, 0, 4);
    END_TEST();
#endif

    return 0;
}
