/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Yazan Hussnain (yazan.hussain@10xengineers.ai)
//         Gull Ahmed (gull.ahmed@10xengineers.ai)
// Date: October 21, 2024
// Description:
// This file implements the IOPMP (I/O Physical Memory Protection)
// Permission Check for RISC-V systems. It contains functions to compute
// and validate address ranges, match transaction requests against IOPMP
// entries, and enforce access permissions (Read, Write, Execute) based
// on the IOPMP configuration and hardware restrictions. The IOPMP
// protects memory regions for peripheral accesses, ensuring only
// authorized transactions occur within specified address ranges and
// permissions.
//
// The main functions in this file include:
// - iopmpAddrRange: Computes address ranges for IOPMP entries based on
//   entry configurations, supporting modes such as NA4, TOR, and NAPOT.
// - iopmpMatchAddr: Matches transaction request addresses against an
//   IOPMP entry range, with priority handling.
// - iopmpCheckPerms: Verifies permissions against the IOPMP entry
//   configuration and Requestor Role ID (RRID) to allow or deny access.
// - iopmpRuleAnalyzer: Analyzes IOPMP rules to determine if a transaction
//   matches an entry and has the required permissions, considering
//   priority and configuration-specific conditions.
//
***************************************************************************/

#include "iopmp.h"

/**
  * @brief Computes the address range based on the IOPMP entry configuration.
  *
  * @param iopmp The IOPMP instance.
  * @param startAddr Pointer to store the first address of the range, in 4-byte units
  * @param lastAddr Pointer to store the last address of the range, in 4-byte units
  * @param prev_iopmpaddr Previous IOPMP address (used in TOR mode)
  * @param iopmpaddr Current IOPMP address
  * @param iopmpcfg IOPMP entry configuration
  * @return 0 on success, 1 if the IOPMP entry is disabled or its range is empty
 **/
static int iopmpAddrRange(iopmp_dev_t *iopmp, uint64_t *startAddr, uint64_t *lastAddr, uint64_t prev_iopmpaddr, uint64_t iopmpaddr, entry_cfg_t iopmpcfg) {
    uint64_t napot_mask;
    // Check if IOPMP entry is OFF
    if (iopmpcfg.a == IOPMP_OFF) { return 1; }

    uint8_t G = get_granularity_G(iopmp);

    switch (iopmpcfg.a) {
        case IOPMP_NA4:  // Address range covers a single 4-byte address
            *startAddr = iopmpaddr;
            *lastAddr  = iopmpaddr;
            break;

        case IOPMP_TOR: // Address range specified by top-of-range mode
            /* Bits [G-1:0] do not affect the TOR address-matching logic */
            if (G >= 1) {
                uint64_t G_mask = gen_granularity_tor_mask(G);
                prev_iopmpaddr &= ~G_mask;
                iopmpaddr      &= ~G_mask;
            }
            /* Empty range, no match */
            if (iopmpaddr <= prev_iopmpaddr) {
                return 1;
            }
            *startAddr = prev_iopmpaddr;
            *lastAddr  = iopmpaddr - 1;
            break;

        default:  // Assume NAPOT (Naturally Aligned Power-of-Two) mode
            /* When G>=2 and the mode is NAPOT, bits [G-2:0] read as all ones */
            if (G >= 2) {
                uint64_t G_mask = gen_granularity_napot_mask(G);
                iopmpaddr |= G_mask;
            }
            napot_mask = iopmpaddr ^ (iopmpaddr + 1);
            *startAddr = iopmpaddr & ~napot_mask;
            *lastAddr  = *startAddr + napot_mask;
            break;
    }

    return 0;
}

/**
  * @brief Determine the matching status of transaction request address against IOPMP entry range.
  *        All addresses are in 4-byte units.
  *
  * @param trans_start First address of the transaction
  * @param trans_last Last address of the transaction
  * @param lo First address of the IOPMP range
  * @param hi Last address of the IOPMP range
  * @return iopmpMatchStatus_t Status of the match:
  *         - ENTRY_MATCH: Entry matches all bytes of a transaction
  *         - ENTRY_PARTIAL_MATCH: Entry matches partial bytes of a transation
  *         - ENTRY_NOTMATCH: Entry doesn't cover any byte of the transaction
 **/
static iopmpMatchStatus_t iopmpMatchAddr(uint64_t trans_start,
                                         uint64_t trans_last,
                                         uint64_t lo, uint64_t hi) {
    // Check if transaction falls outside the IOPMP entry range
    if (trans_last < lo || trans_start > hi) { return ENTRY_NOTMATCH; } // No match, transaction outside range

    // Determine if there's a full match
    if (trans_start >= lo && trans_last <= hi) { return ENTRY_MATCH; }

    return ENTRY_PARTIAL_MATCH;
}

/**
  * @brief Checks IOPMP permissions based on request and configuration of a fully matching entry
  *
  * @param iopmp The IOPMP instance.
  * @param rrid Requestor Role ID
  * @param req_perm Requested permission type (Read, Write, Execute)
  * @param iopmpcfg IOPMP configuration for the entry
  * @param md Respective Memory Domain
  * @param is_amo Indicates the AMO Access
  * @param sie Output 0 if matched entry doesn't suppress the interrupt, otherwise output 1
  * @param see Output 0 if matched entry doesn't suppress the bus error, otherwise output 1
  * @return - true if entry grants transation permission
  *         - false if entry doesn't grant transation permission
 **/
static bool iopmpCheckPerms(iopmp_dev_t *iopmp, uint16_t rrid, perm_type_e req_perm,
                            entry_cfg_t iopmpcfg, uint8_t md, bool is_amo,
                            bool *sie, bool *see) {
    // Common permission checks
    bool read_allowed    = false;
    bool write_allowed   = false;
    bool execute_allowed = false;

    switch (iopmp->reg_file.hwcfg3.srcmd_fmt) {
    case 0: {
        uint64_t srcmd_r, srcmd_w, srcmd_x;
        uint8_t  srcmd_r_bit, srcmd_w_bit, srcmd_x_bit;
        bool sps_en = iopmp->reg_file.hwcfg2.sps_en; // Software privilege separation enable
        srcmd_r     = CONCAT32(iopmp->reg_file.srcmd_table[rrid].srcmd_rh.raw, iopmp->reg_file.srcmd_table[rrid].srcmd_r.raw);
        srcmd_w     = CONCAT32(iopmp->reg_file.srcmd_table[rrid].srcmd_wh.raw, iopmp->reg_file.srcmd_table[rrid].srcmd_w.raw);
        srcmd_x     = CONCAT32(iopmp->reg_file.srcmd_table[rrid].srcmd_xh.raw, iopmp->reg_file.srcmd_table[rrid].srcmd_x.raw);
        srcmd_r_bit = GET_BIT(srcmd_r, (md + 1));
        srcmd_w_bit = GET_BIT(srcmd_w, (md + 1));
        srcmd_x_bit = GET_BIT(srcmd_x, (md + 1));

        read_allowed    = sps_en ? (iopmpcfg.r & srcmd_r_bit) : iopmpcfg.r;
        write_allowed   = sps_en ? (iopmpcfg.w & srcmd_w_bit & ((iopmpcfg.r & srcmd_r_bit) | !is_amo)) : (iopmpcfg.w & (iopmpcfg.r | !is_amo));
        execute_allowed = sps_en ? (iopmpcfg.x & srcmd_x_bit) : iopmpcfg.x;
        break;
    }
    case 1:
        read_allowed    = iopmpcfg.r;
        write_allowed   = (iopmpcfg.w & (iopmpcfg.r | !is_amo));
        execute_allowed = iopmpcfg.x;
        break;
    case 2: {
        uint64_t srcmd_perm;
        uint8_t  srcmd_perm_r, srcmd_perm_w;
        srcmd_perm   = CONCAT32(iopmp->reg_file.srcmd_table[md].srcmd_permh.raw, iopmp->reg_file.srcmd_table[md].srcmd_perm.raw);
        srcmd_perm_r = GET_BIT(srcmd_perm, (rrid * 2));
        srcmd_perm_w = GET_BIT(srcmd_perm, ((rrid * 2) + 1));

        read_allowed    = iopmpcfg.r || srcmd_perm_r;
        write_allowed   = ((iopmpcfg.w || srcmd_perm_w) & ((iopmpcfg.r || srcmd_perm_r) | !is_amo));
        execute_allowed = (iopmpcfg.x || srcmd_perm_r);
        break;
    }
    default:
        break;
    }

    // Handle requested permission type
    switch (req_perm) {
    case READ_ACCESS:
        if (!read_allowed) {
            *sie = iopmp->reg_file.hwcfg2.peis ? iopmpcfg.sire : false;
            *see = iopmp->reg_file.hwcfg2.pees ? iopmpcfg.sere : false;
        }
        return read_allowed;

    case WRITE_ACCESS:
        if (!write_allowed) {
            *sie = iopmp->reg_file.hwcfg2.peis ? iopmpcfg.siwe : false;
            *see = iopmp->reg_file.hwcfg2.pees ? iopmpcfg.sewe : false;
        }
        return write_allowed;

    case INSTR_FETCH:
        if (!iopmp->reg_file.hwcfg3.xinr) {
            if (!execute_allowed) {
                *sie = iopmp->reg_file.hwcfg2.peis ? iopmpcfg.sixe : false;
                *see = iopmp->reg_file.hwcfg2.pees ? iopmpcfg.sexe : false;
            }
            return execute_allowed;
        }
        return false;   // Instruction fetch check not implemented

    default:
        return false;   // Default case for invalid permission request
    }
}

/**
  * @brief Analyze the matching status and permissions of a transaction request
  *
  * @param iopmp The IOPMP instance.
  * @param input All the information the analyzer needs.
  * @param output Address matching status and permission grant result.
 **/
void iopmpRuleAnalyzer(iopmp_dev_t *iopmp,
                       iopmp_rule_analyzer_input_t *input,
                       iopmp_rule_analyzer_output_t *output)
{
    uint64_t start_addr, last_addr;

    // Set up the address range; if range setup fails, return not matched
    if (iopmpAddrRange(iopmp, &start_addr, &last_addr, input->prev_iopmpaddr,
                       input->iopmpaddr, input->iopmpcfg)) {
        output->match_status = ENTRY_NOTMATCH;
        return;
    }

    // Match transaction address to the IOPMP address range in 4-byte units.
    // The byte address of an entry, bits 65:0, does not fit in 64 bits.
    output->match_status = iopmpMatchAddr(input->trans_start >> 2,
                                          (input->trans_end - 1) >> 2,
                                          start_addr, last_addr);
    if (output->match_status == ENTRY_NOTMATCH ||
        output->match_status == ENTRY_PARTIAL_MATCH) {
        // It's unnecessary to check entry permissions in these two cases
        return;
    }

    // iopmpMatchAddr() returns ENTRY_MATCH. Further checks entry permissions
    output->grant_perm = iopmpCheckPerms(iopmp, input->rrid, input->perm,
                                         input->iopmpcfg, input->md,
                                         input->is_amo,
                                         &output->sie, &output->see);
}
