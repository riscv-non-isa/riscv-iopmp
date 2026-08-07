/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Yazan Hussnain (yazan.hussain@10xengineers.ai)
//         Gull Ahmed (gull.ahmed@10xengineers.ai)
// Date: October 21, 2024
// Description: IOPMP Request Response Structure
// This header file defines data structures and enumerations used in the
// Input/Output Physical Memory Protection (IOPMP) for handling request
// and response transactions. It includes the enumeration of permission
// types (read, write, and instruction fetch) and status codes, as well
// as detailed request and response structures for transactions. These
// definitions help to manage access control, track transaction details,
// and capture any errors that occur during access attempts.
***************************************************************************/

#include "iopmp.h"

#ifndef __IOPMP_REQ_RSP_H__
#define __IOPMP_REQ_RSP_H__

// Enumerates transaction types (read, write, instruction fetch)
typedef enum {
    READ_ACCESS  = 1,  // Read permission
    WRITE_ACCESS = 2,  // Write permission
    INSTR_FETCH  = 3   // Instruction fetch permission
} perm_type_e;

// Structure for IOPMP transaction requests
typedef struct __attribute__((__packed__)) {
    uint16_t    rrid;      // Requester ID
    uint64_t    addr;      // Target address for the transaction
    uint32_t    length;    // Length of the transaction
    uint32_t    size;      // Size of each access in the transaction
    perm_type_e perm;      // Type of permission requested
    bool        is_amo;    // Indicates the AMO Access
} iopmp_trans_req_t;

// Enumerates status results for IOPMP transactions
typedef enum {
    IOPMP_SUCCESS = 0,  // Transaction successful
    IOPMP_ERROR   = 1   // Transaction encountered an error
} status_e;

// Structure for IOPMP transaction responses
typedef struct __attribute__((__packed__)) {
    uint32_t rrid;          // Requester ID
    uint8_t  user;          // User mode indicator
    uint8_t  rrid_stalled;  // Requester ID stall status
    uint8_t  rrid_stalled_no_available_buffer;  // Requester ID stall status due to no available stall buffer
    uint16_t rrid_transl;   // The RRID tagged to outgoing transactions
    status_e status;        // Transaction status (success or error)
    uint8_t  violation;     // The checker failed this transaction, even if
                            // error suppression answered it with IOPMP_SUCCESS
} iopmp_trans_rsp_t;

#endif
