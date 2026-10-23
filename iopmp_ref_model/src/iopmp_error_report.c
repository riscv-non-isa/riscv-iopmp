/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Yazan Hussnain (yazan.hussain@10xengineers.ai)
//         Gull Ahmed (gull.ahmed@10xengineers.ai)
// Date: October 24, 2024
// Description: IOPMP Error Reporting Module
// This file contains the implementation for error reporting in the
// Input/Output Physical Memory Protection (IOPMP). The `errorReport`
// function is responsible for flagging the error through ERR_INFO.v when a
// transaction request encounters an access violation or permission-related
// issue, and can trigger an interrupt if necessary. If the error capture
// record is implemented, it also stores details such as transaction type,
// error type, request ID, entry ID, and the address where the error occurred.
// Additionally, the function detects and flags subsequent violations.
***************************************************************************/

#include "iopmp.h"

/**
  * @brief Sets the corresponding bit in the error subsequent violations (SV) structure for a given RRID.
  *
  * @param iopmp The IOPMP instance.
  * @param rrid Resource Record ID (RRID) whose corresponding bit needs to be set in the SV structure.
  */
static void setRridSv(iopmp_dev_t *iopmp, uint16_t rrid) {
    iopmp->err_svs.sv[rrid/16].svw |= (1 << (rrid % 16));
}

/**
  * @brief Reports the error of a transaction request, and captures and logs
  *        its information if the error capture record is implemented.
  *
  * @param iopmp The IOPMP instance.
  * @param trans_type Type of the transaction request (read/write permissions).
  * @param error_type Specific error type encountered during the transaction.
  * @param rrid Requester ID associated with the transaction.
  * @param entry_id IOPMP entry ID where the error was encountered.
  * @param err_addr Address at which the error occurred.
  * @param gen_intrpt Generate interrupt or not.
  * @param gen_buserr Generate bus error or not.
  * @param intrpt Pointer to the variable to store wired interrupt flag.
  *               This flag is set to 1 if the following conditions are true:
  *                 - the transaction fails
  *                 - there is no pending error, that is, ERR_INFO.v = 0
  *                 - the interrupts are not suppressed
  *                 - IOPMP doesn't implement MSI extension, or MSI is not enabled
  *               This flag is set to 0 if the following conditions are true:
  *                 - this transaction fails
  *                 - there is no pending error, that is, ERR_INFO.v = 0
  *                 - the interrupts are suppressed, or IOPMP implements MSI extension
  *                   and triggers MSI instead of wired interrupt
 **/
void errorReport(iopmp_dev_t *iopmp, perm_type_e trans_type, uint8_t error_type,
                 uint16_t rrid, uint16_t entry_id, uint64_t err_addr,
                 bool gen_intrpt, bool gen_buserr, uint8_t *intrpt) {

    // Current value of ERR_INFO.v
    int err_reqinfo_v = iopmp->reg_file.err_info.v;

    // ERR_INFO.v is set when the following conditions are true:
    //   - An interrupt is triggered, or a bus error is returned
    //   - There is no pending error, that is, current ERR_INFO.v = '0'
    bool first_record = (gen_intrpt || gen_buserr) && !err_reqinfo_v;

    if (first_record) {
        iopmp->reg_file.err_info.v = 1;               // Flag the error and the pending interrupt

        // ERR_INFO.v is also the interrupt pending bit, so it is set even if
        // IOPMP doesn't implement the error capture record. Only the details
        // of the transaction are left unrecorded in that case.
        if (!iopmp->reg_file.hwcfg0.no_err_rec) {
            // Set error status and transaction details
            iopmp->reg_file.err_info.ttype = trans_type;  // Transaction type (read/write)
            iopmp->reg_file.err_info.etype = error_type;  // Specific error type

            // Capture lower and upper parts of error address. ERR_REQADDRH is
            // implemented only if HWCFG0.addrh_en is set.
            iopmp->reg_file.err_reqaddr.addr   = (uint32_t)((err_addr >> 2) & UINT32_MAX);         // Error address [33:2]
            if (iopmp->reg_file.hwcfg0.addrh_en) {
                iopmp->reg_file.err_reqaddrh.addrh = (uint32_t)((err_addr >> 34) & UINT32_MAX);    // Error address [65:34]
            }

            // Record Request ID and Entry ID details
            iopmp->reg_file.err_reqid.rrid = rrid;
            if (iopmp->imp_err_reqid_eid) {
                // One can implement the error capture record, but doesn't implement
                // the error entry index record (ERR_REQID.eid).
                // If IOPMP doesn't implement ERR_REQID.eid it won't be updated.
                iopmp->reg_file.err_reqid.eid = entry_id;
            }
        }

        generate_interrupt(iopmp, gen_intrpt, intrpt);
    }

    // If IOPMP implements Multi-Faults Record extension, IOPMP has ability to
    // record subsequent violations when the primary error capture registers
    // recorded last violation and software have not invalidated them yet.
    if (iopmp->reg_file.hwcfg2.mfr_en) {
        // Subsequent error capture occurs when the following conditions are true:
        //   - An interrupt is triggered or a bus error is returned
        //   - There is a pending error capture, that is, current ERR_INFO.v = '1'
        bool subsq_record = (gen_intrpt || gen_buserr) && err_reqinfo_v;

        if (subsq_record) {
            // Update violation window
            setRridSv(iopmp, rrid);
            iopmp->reg_file.err_info.svc = 1;
        }
    }
}
