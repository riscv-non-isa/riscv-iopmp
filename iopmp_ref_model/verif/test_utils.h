/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Gull Ahmed (gull.ahmed@10xengineers.ai)
//         Yazan Hussnain (yazan.hussain@10xengineers.ai)
// Date: October 21, 2024
// Description:
// This header file defines macros, and function prototypes
// for the Input/Output Physical Memory Protection (IOPMP) Test file.
***************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "iopmp.h"

#define SRCMD_EN     0x00
#define SRCMD_ENH    0x04
#define SRCMD_R      0x08
#define SRCMD_RH     0x0C
#define SRCMD_W      0x10
#define SRCMD_WH     0x14
#define SRCMD_X      0x18
#define SRCMD_XH     0x1C
#define SRCMD_PERM   0x00
#define SRCMD_PERMH  0x04

#define ENTRY_ADDR      0x00
#define ENTRY_ADDRH     0x04
#define ENTRY_CFG       0x08
#define ENTRY_USER_CFG  0x0C

// Permissions
#define R 0x01
#define W 0x02
#define X 0x04

// Address Mode
#define OFF   0x00
#define TOR   0x08
#define NA4   0x10
#define NAPOT 0x18

// Interrupt Suppression
#define SIRE 0x20
#define SIWE 0x40
#define SIXE 0x80

// Error Suppression
#define SERE 0x100
#define SEWE 0x200
#define SEXE 0x400

extern int test_num;
extern int8_t *memory;
extern uint64_t bus_error;

extern int create_memory(uint8_t mem_gb);
extern uint8_t read_memory(uint64_t addr, uint8_t size, uint64_t *data);
extern void configure_srcmd_n(iopmp_dev_t *iopmp, uint8_t srcmd_reg, uint16_t srcmd_idx, reg_intf_dw data, uint8_t num_bytes);
extern void configure_mdcfg_n(iopmp_dev_t *iopmp, uint8_t md_idx, reg_intf_dw data, uint8_t num_bytes);
extern void configure_entry_n(iopmp_dev_t *iopmp, uint8_t entry_reg, uint64_t entry_idx, reg_intf_dw data, uint8_t num_bytes);
extern reg_intf_dw read_entry_n(iopmp_dev_t *iopmp, uint8_t entry_reg, uint64_t entry_idx, uint8_t num_bytes);
extern void receiver_port(uint16_t rrid, uint64_t addr, uint32_t length, uint32_t size, perm_type_e perm, bool is_amo, iopmp_trans_req_t *iopmp_trans_req);
extern int error_record_chk(iopmp_dev_t *iopmp, uint8_t err_type, uint8_t perm, uint64_t addr, bool err_rcd);
extern void set_hwcfg0_enable(iopmp_dev_t *iopmp);

// Test Macros: Define macros for IOPMP testing framework
#define START_TEST(TEST_DESC)                           \
    test_num++;                                         \
    printf("Test %02d : %-61s : ", test_num, TEST_DESC)

// Start the test only when CONDITION is true, otherwise skip the test
#define START_TEST_IF(CONDITION, TEST_DESC, TEST)       \
    START_TEST(TEST_DESC);                              \
    if (CONDITION) {                                    \
        TEST                                            \
    } else {                                            \
        SKIP_TEST();                                    \
    }

// Fails test If condition is true.
#define FAIL_IF(CONDITION)                                                        \
    if (CONDITION) {                                                              \
        printf("Test %02d : \x1B[31mFAIL. Line %d\x1B[0m\n", test_num, __LINE__); \
        return -1;                                                                \
    }

// Must be used at the end of test.
#define END_TEST() \
    { printf("\x1B[32mPASS\x1B[0m\n"); }

// Must be used at the end of test.
#define SKIP_TEST() \
    { printf("\x1B[33mSKIP\x1B[0m\n"); }

// Transaction Check Macro: Verifies transaction response against expected values
#define CHECK_IOPMP_TRANS(iopmp, RSP_STATUS, ERR_TYPE)                                                                      \
    FAIL_IF((iopmp_trans_rsp.rrid != iopmp_trans_req.rrid));                                                                \
    FAIL_IF((iopmp_trans_rsp.status != (RSP_STATUS)));                                                                      \
    err_info_temp.raw = read_register(iopmp, ERR_INFO_OFFSET, 4);                                                           \
    if (iopmp_trans_rsp.status == IOPMP_ERROR) {                                                                            \
        FAIL_IF((iopmp_trans_rsp.violation != 1));                                                                          \
        FAIL_IF((err_info_temp.v != 1));                                                                                    \
        FAIL_IF((err_info_temp.ttype != iopmp_trans_req.perm));                                                             \
        FAIL_IF((err_info_temp.etype != (ERR_TYPE)));                                                                       \
        FAIL_IF((read_register(iopmp, ERR_REQADDR_OFFSET, 4) != (uint32_t)((iopmp_trans_req.addr >> 2) & 0xFFFFFFFF)));     \
        FAIL_IF((read_register(iopmp, ERR_REQADDRH_OFFSET, 4) != (uint32_t)((iopmp_trans_req.addr >> 34) & 0xFFFFFFFF)));   \
        if (bus_error == 0) { FAIL_IF((err_info_temp.msi_werr != 0)); }                                                     \
        else { FAIL_IF((err_info_temp.msi_werr != 1)); }                                                                    \
    } else {                                                                                                                \
        FAIL_IF((iopmp_trans_rsp.violation != 0));                                                                          \
        FAIL_IF((err_info_temp.v != 0));                                                                                    \
    }
