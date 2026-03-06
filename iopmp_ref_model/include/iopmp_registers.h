/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Authors: Mazhar Ali (mazhar.ali@10xengineers.ai)
//          Gull Ahmed (gull.ahmed@10xengineers.ai)
// Date: October 21, 2024
// Description: This header file provides a comprehensive definition of
// the IOPMP MMAP registers, designed to support flexibility and modularity
// in various IOPMP configurations. It defines all necessary registers while
// allowing customization by including or excluding specific register
// definitions based on the IOPMP model being implemented. Additionally,
// it separates the entry table definition from the remaining structure
// in which the rest of the registers are defined, as the entry offset
// is implementation-dependent.
***************************************************************************/

#include "config.h"

#ifndef IOPMP_REGS
#define IOPMP_REGS

// Offset to fields
#define VERSION_OFFSET        0x00
#define IMPLEMENTATION_OFFSET 0x04
#define HWCFG0_OFFSET         0x08
#define HWCFG1_OFFSET         0x0C
#define HWCFG2_OFFSET         0x10
#define HWCFG3_OFFSET         0x14
#define ENTRYOFFSET_OFFSET    0x2C
#define MDSTALL_OFFSET        0x30
#define MDSTALLH_OFFSET       0x34
#define RRIDSCP_OFFSET        0x38
#define MDLCK_OFFSET          0x40
#define MDLCKH_OFFSET         0x44
#define MDCFGLCK_OFFSET       0x48
#define ENTRYLCK_OFFSET       0x4C
#define ERR_CFG_OFFSET        0x60
#define ERR_INFO_OFFSET       0x64
#define ERR_REQADDR_OFFSET    0x68
#define ERR_REQADDRH_OFFSET   0x6C
#define ERR_REQID_OFFSET      0x70
#define ERR_MFR_OFFSET        0x74
#define ERR_MSIADDR_OFFSET    0x78
#define ERR_MSIADDRH_OFFSET   0x7C

#define ERR_USER0_OFFSET      0x80
#define ERR_USER1_OFFSET      0x84
#define ERR_USER2_OFFSET      0x88
#define ERR_USER3_OFFSET      0x8C
#define ERR_USER4_OFFSET      0x90
#define ERR_USER5_OFFSET      0x94
#define ERR_USER6_OFFSET      0x98
#define ERR_USER7_OFFSET      0x9C

#define MDCFG_TABLE_BASE_OFFSET 0x0800
#define SRCMD_TABLE_BASE_OFFSET 0x1000

#define SRCMD_WH_OFFSET       0x1014

#define SRCMD_REG_STRIDE      32
#define ENTRY_REG_STRIDE      16

#define IOPMP_MAX_MD_NUM      63
#define IOPMP_MAX_RRID_NUM    65535
#define IOPMP_MAX_ENTRY_NUM   65535

#if (REG_INTF_BUS_WIDTH == 4)
    typedef uint32_t reg_intf_dw;
#elif (REG_INTF_BUS_WIDTH == 8)
    typedef uint64_t reg_intf_dw;
#endif

#define ALIGNUP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))

// VERSION register is a read-only register reporting
// IOPMP comfiguration information of the instance:
// 1. vendor ID
// 2. Specification Version
typedef union {
    struct {
        uint32_t vendor  : 24;              // The vendor ID
        uint32_t specver : 8;               // The specification version
    };
    uint32_t raw;
} version_t;

// IMPLEMENTATION register is a read-only register reporting
// implementation ID specific to the instance
typedef union {
    struct {
        uint32_t impid;                     // The user-defined implementation ID.
    };
    uint32_t raw;
} implementation_t;

// HWCFG0 register is one of hardware configuration registers reporting
// features supported by IOPMP. each bit if bot clear indicates
// presence of that feature in IOPMP
typedef union {
    struct {
        uint32_t enable             : 1;    // Indicate if the IOPMP checks transactions by default.
                                            // If it is implemented, it should be initial to 0 and sticky to 1.
                                            // If it is not implemented, it should be wired to 1.
        uint32_t HWCFG2_en          : 1;    // Indicate if HWCFG2 is implemented.
        uint32_t HWCFG3_en          : 1;    // Indicate if HWCFG3 is implemented.
        uint32_t rsv                : 20;   // Must be zero on write, reserved for future.
        uint32_t no_err_rec         : 1;    // Indicate if error record is not implemented.
        uint32_t md_num             : 6;    // Indicate the supported number of MD in the instance
        uint32_t addrh_en           : 1;    // Indicate if ENTRY_ADDRH(i) and ERR_REQADDRH are available.
        uint32_t tor_en             : 1;    // Indicate if TOR is supported
    };
    uint32_t raw;
} hwcfg0_t;

// HWCFG1 register is one of hardware configuration
// registers reporting features supported by IOPMP.
typedef union {
    struct {
        uint32_t rrid_num  : 16;            // Indicate the supported number of RRID in the instance
        uint32_t entry_num : 16;            // Indicate the supported number of entries in the instance, which should be
                                            // larger than zero.
    };
    uint32_t raw;
} hwcfg1_t;

// HWCFG2 register is one of hardware configuration registers indicating the
// extended configurations of current IOPMP instance for extension.
typedef union {
    struct {
        uint32_t prio_entry    : 16;        // Indicate the number of entries matched with priority. These rules should
                                            // be placed in the lowest order. Within these rules, the lower order has a
                                            // higher priority.
        uint32_t prio_ent_prog : 1;         // A write-1-clear bit is sticky to 0 and indicates if HWCFG2.prio_entry
                                            // is programmable. Reset to 1 if the implementation supports programmable
                                            // prio_entry, otherwise, wired to 0.
        uint32_t non_prio_en   : 1;         // Indicates whether the IOPMP supports non-priority entries.
        uint32_t rsv           : 8;         // Must be zero on write, reserved for future.
        uint32_t msi_en        : 1;         // Indicate if the IOPMP implements MSI extension.
        uint32_t peis          : 1;         // Indicate if the IOPMP implements interrupt suppression per entry,
                                            // including fields sire and siwe in ENTRY_CFG(i), i = 0…HWCFG1.entry_num-1.
        uint32_t pees          : 1;         // Indicate if the IOPMP implements the error suppression per entry,
                                            // including fields sere and sewe in ENTRY_CFG(i), i = 0…HWCFG1.entry_num-1.
        uint32_t sps_en        : 1;         // Indicates secondary permission settings are supported; which are
                                            // SRCMD_R/RH(s) and SRCMD_W/WH(s) registers, s = 0…HWCFG1.rrid_num - 1.
        uint32_t stall_en      : 1;         // Indicate if the IOPMP implements stall-related features, which are MDSTALL,
                                            // MDSTALLH, and RRIDSCP registers.
        uint32_t mfr_en        : 1;         // Indicate if the IOPMP implements Multi Faults Record, that is ERR_MFR and
                                            // ERR_INFO.svc.
    };
    uint32_t raw;
} hwcfg2_t;

// HWCFG3 register is one of hardware configuration registers indicating the
// extended configurations of current IOPMP instance for application note.
typedef union {
    struct {
        uint32_t mdcfg_fmt        : 2;      // Indicate the MDCFG format
                                            //  -> 0x0: Format 0. MDCFG table is implemented.
                                            //  -> 0x1: Format 1. No MDCFG table. HWCFG3.md_entry_num is fixed.
                                            //  -> 0x2: Format 2. No MDCFG table. HWCFG3.md_entry_num is programmable.
                                            //  -> 0x3: reserved.
        uint32_t srcmd_fmt        : 2;      // Indicate the SRCMD Table format
                                            //  -> 0x0: Format 0 (baseline). SRCMD_EN(s) and SRCMD_ENH(s) are available.
                                            //  -> 0x1: Format 1. Exclusive Format. No SRCMD Table. RRID i associates
                                            //     exclusively with memory domain i, i = 0…HWCFG0.md_num-1.
                                            //  -> 0x2: Format 2. MD-indexed Format. SRCMD_PERM(m) and SRCMD_PERMH(m)
                                            //     are available.
                                            //  -> 0x3: reserved.
        uint32_t md_entry_num     : 7;      // When HWCFG3.mdcfg_fmt =
                                            //  -> 0x0: must be zero
                                            //  -> 0x1 or 0x2: md_entry_num indicates each memory domain has exactly
                                            //    (md_entry_num + 1) entries
                                            // md_entry_num is locked if HWCFG0.enable is 1.
        uint32_t xinr             : 1;      // Indicates whether the IOPMP treats the instruction fetch accesses as data
                                            // read accesses. All fields related to instruction fetch can be fixed to 0,
                                            // or unavailable.
        uint32_t no_x             : 1;      // For HWCFG2.xinr=0, when no_x=1, the IOPMP denies all instruction fetch
                                            // transactions; otherwise, it depends on the x-bit in ENTRY_CFG(i). For
                                            // xinr=1, no_x has no effect.
        uint32_t no_w             : 1;      // Indicate if the IOPMP always denies write accesses as if no rule matched.
        uint32_t rrid_transl_en   : 1;      // Indicate the if tagging a new RRID on the initiator port is supported
        uint32_t rrid_transl_prog : 1;      // A write-1-clear bit that is sticky to 0. Indicates if the rrid_transl
                                            // field is programmable. Supported only for rrid_transl_en=1, otherwise,
                                            // wired to 0.
        uint32_t rrid_transl      : 16;     // The RRID tagged to outgoing transactions. Supported only for
                                            // rrid_transl_en=1. It is writable only when rrid_transl_prog=1.
    };
    uint32_t raw;
} hwcfg3_t;

// ENTRYOFFSET register ndicates the internal address
// offsets of each table.
typedef union {
    struct {
        uint32_t offset;                    // Indicate the offset address of the IOPMP array from the base
                                            // of an IOPMP instance, a.k.a. the address of VERSION.
                                            // Note: the offset is a signed number. That is, the IOPMP array
                                            // can be placed in front of VERSION.
    };
    uint32_t raw;
} entryoffset_t;

// MDSTALL is an optional register and used to support
// atomicity issue while programming the IOPMP, as the IOPMP
// rule may not be updated in a single transaction.
typedef union {
    struct {
        uint32_t exempt : 1;                    // Stall transactions with exempt selected MDs, or Stall selected MDs.
        uint32_t md     : 31;                   // Writting md[i]=1 selects MD i; reading md[i] = 1 means MD i selected.
    };
    struct {
        uint32_t is_busy : 1;                   // After the last writing of MDSTALL (included) plus any following writing RRIDSCP, 1
                                                // indicates that all requested stalls take effect; otherwise, 0. After the last writing
                                                // MDSTALLH (if any) and then MDSTALL by zero, 0 indicates that all transactions have
                                                // been resumed; otherwise, 1.
    };
    uint32_t raw;
} mdstall_t;

// MDSTALLH is an optional register implemented along with MDSTALL
// to support upto 63 memory domains (MDs) while programming the IOPMP
typedef union {
    struct {
        uint32_t mdh : 32;                  // Writting mdh[i]=1 selects MD i+31;
                                            // reading mdh[i] = 1 means MD i+31 selected.
    };
    uint32_t raw;
} mdstallh_t;

// RRIDSCP is an optional register and used to support
// atomicity issue while programming the IOPMP, as the IOPMP
// rule may not be updated in a single transaction.
typedef union {
    struct {
        uint32_t rrid : 16;                 // Stall transactions with exempt selected MDs, or Stall selected MDs.
        uint32_t rsv  : 14;                 // Reserved for future use.
        uint32_t op   : 2;                  // 0x0: query
                                            // 0x1: stall transactions associated with selected RRID
                                            // 0x2: don’t stall transactions associated with selected RRID
                                            // 0x3: reserved
    };
    struct {
        uint32_t rsv1 : 30;                 // Stat is ready-only and located at 31:30
        uint32_t stat : 2;                  // 0: RRIDSCP not implemented
                                            // 1: transactions associated with selected RRID are stalled
                                            // 2: transactions associated with selected RRID are not stalled
                                            // 3: unimplemented or unselectable RRID
    };
    uint32_t raw;
} rridscp_t;

#define RRIDSCP_OP_QUERY                0
#define RRIDSCP_OP_STALL                1
#define RRIDSCP_OP_DONT_STALL           2
#define RRIDSCP_OP_RESERVED             3
#define RRIDSCP_STAT_NOT_IMPL           0
#define RRIDSCP_STAT_RRID_STALLED       1
#define RRIDSCP_STAT_RRID_NOT_STALLED   2
#define RRIDSCP_STAT_INVALID_RRID       3

// MDLCK is an optional register with a bitmap field to
// indicate which MDs are locked in SRCMD table.
typedef union {
    struct {
        uint32_t l  : 1;                    // Lock bit to MDLCK and MDLCKH register.
        uint32_t md : 31;                   // md[j] = 1, indicates MD j is locked
                                            // for all source memory domain table entries.
    };
    uint32_t raw;
} mdlck_t;

// MDLCKH is an optional register implemented along
// with MDLCK to support upto 63 memory domains (MDs),
typedef union {
    struct {
        uint32_t mdh;                       // md[j] = 1, indicates MD j+31 is locked
                                            // for all source memory domain table entries.
    };
    uint32_t raw;
} mdlckh_t;

// MDCFGLCK is the lock register to MDCFG table.
typedef union {
    struct {
        uint32_t l   : 1;                   // ock bit to MDCFGLCK register.
        uint32_t f   : 6;                   // Indicate the number of locked MDCFG entries
                                            // MDCFG(i) is locked for i < f.
        uint32_t rsv : 25;                  // Reserved for future use
    };
    uint32_t raw;
} mdcfglck_t;

// ENTRYLCK is the lock register to Entry table.
typedef union {
    struct {
        uint32_t l   : 1;                   // ock bit to ENTRYLCK register.
        uint32_t f   : 16;                  // Indicate the number of locked IOPMP entries
                                            // NTRY_ADDR(i), ENTRY_ADDRH(i), ENTRY_CFG(i), and
                                            // ENTRY_USER_CFG(i) are locked for i < f.
        uint32_t rsv : 15;                  // Reserved for future use
    };
    uint32_t raw;
} entrylck_t;

// ERR_CFG is a read/write WARL register used to
// configure the global error reporting behavior on an
// IOPMP violation.
typedef union {
    struct {
        uint32_t l       : 1;              // Lock fields to ERR_CFG register
        uint32_t ie      : 1;              // Enable the interrupt of the IOPMP
        uint32_t rs      : 1;              // To suppress an error response on an IOPMP rule violation.
                                           // • 0x0: respond an implementation-dependent error, such as a bus error
                                           // • 0x1: respond a success with a pre-defined value to the requestor instead of an error
        uint32_t msi_sel : 1;              // It indicates whether the IOPMP triggers interrupt by MSI or wired interrupt
                                           // • 0x0: the IOPMP triggers interrupt by wired interrupt
                                           // • 0x1: the IOPMP triggers interrupt by MSI
        uint32_t stall_violation_en  : 1;  // It indicates whether the IOPMP faults stalled transactions
        uint32_t rsv1    : 3;              // reserved for future use
        uint32_t msidata : 11;             // The data to trigger MSI

        uint32_t rsv2    : 13;             // reserved for future use
    };
    uint32_t raw;
} err_cfg_t;

// ERR_INFO captures more detailed error information.
typedef union {
    struct {
        uint32_t v     : 1;                 // Indicate if the illegal capture recorder register has a
                                            // valid content and will keep the content until the bit is cleared

        uint32_t ttype : 2;                 // Indicates the transaction type
                                            // 0x00 = reserved
                                            // 0x01 = read access
                                            // 0x02 = write access
                                            // 0x03 = instruction fetch

        uint32_t msi_werr  : 1;             // It’s asserted when IOPMP-originated MSI has failed.

        uint32_t etype : 4;                 // Indicates the type of violation
                                            // 0x00 = no error
                                            // 0x01 = illegal read access
                                            // 0x02 = illegal write access/AMO
                                            // 0x03 = illegal instruction fetch
                                            // 0x04 = partial hit on a priority rule
                                            // 0x05 = not hit any rule
                                            // 0x06 = unknown RRID
                                            // 0x07 = error due to a stalled transaction
                                            // 0x08 ~ 0x0D = N/A, reserved for future
                                            // 0x0E ~ 0x0F = user-defined error

        uint32_t svc   : 1;                 // Indicate there is a subsequent violation caught in ERR_MFR.
                                            // Implemented only for HWCFG2.mfr_en=1,

        uint32_t rsv   : 23;                // reserved for future use
    };
    uint32_t raw;
} err_info_t;

// ERR_REQADDR indicate the errored request address.
typedef union {
    struct {
        uint32_t addr;                      // Indicate the errored address[33:2]
    };
    uint32_t raw;
} err_reqaddr_t;

// ERR_REQADDRH indicate the errored request address.
typedef union {
    struct {
        uint32_t addrh;                     // Indicate the errored address[65:34]
    };
    uint32_t raw;
} err_reqaddrh_t;

// ERR_REQID indicates the errored RRID and entry index.
typedef union {
    struct {
        uint32_t rrid : 16;                 // Indicate the errored RRID
        uint32_t eid  : 16;                 // Indicates the index pointing to the entry that catches
                                            // the violation. If no entry is hit, i.e., etype=0x05,
                                            // the value of this field is invalid.
                                            // If the field is not implemented, it should be wired to 0xffff.
    };
    uint32_t raw;
} err_reqid_t;

// ERR_MFR is an optional register. If Multi-Faults Record Extension
// is enabled (HWCFG0.mfr_en=1),ERR_MFR can be used to retrieve
// which RRIDs make subsequent violations.
typedef union {
    struct {
        uint32_t svw : 16;                  // Subsequent violations in the window indexed by svi

        uint32_t svi : 12;                  // Window’s index to search subsequent violations.
                                            // When read, svi moves forward until one subsequent violation
                                            // is found or svi has been rounded back to the same value

        uint32_t rsv : 3;                   // reserved for future use

        uint32_t svs : 1;                   // the status of this window’s content:
                                            // 0x0 = no subsequent violation found
                                            // 0xq = subsequent violation found
    };
    uint32_t raw;
} err_mfr_t;

// MSI Data Address register
typedef union {
    struct {
        uint32_t msiaddr;                      // Indicate the msi address[33:2]
    };
    uint32_t raw;
} err_msiaddr_t;

// MSI Data Address register
typedef union {
    struct {
        uint32_t msiaddrh;                     // Indicate the msi address[65:34]
    };
    uint32_t raw;
} err_msiaddrh_t;

// ERR_USER are optional registers (0, 1,... 8) to provide users to
// define their own error capture information
typedef union {
    struct {
        uint32_t user : 32;                 // Indicate the errored address[65:34]
    };
    uint32_t raw;
} err_user_t;

// MDCFG table is a lookup to specify the number of IOPMP entries
// that is associated with each MD. number of MDCFG registers is equal
// to HWCFG0.md_num, all MDCFG registers are readable and writable
typedef union {
    struct {
        uint32_t t   : 16;                  // Indicate the top range of memory domain m.
                                            // An IOPMP entry with index j belongs to MD m
        uint32_t rsv : 16;                  // REserved for future use
    };
    uint32_t raw;
} mdcfg_t;

// SRCMD_EN register (0, .... , HWCFG1.rrid_num-1) is a specific register
// for each source (RRID) and indicates which MDs this source maps to
typedef union {
    struct {
        uint32_t l  : 1;                    // A sticky lock bit. When set, locks SRCMD_EN(s), SRCMD_ENH(s),
                                            // SRCMD_R(s), SRCMD_RH(s), SRCMD_W(s),

        uint32_t md : 31;                   // md[m] = 1 indicates MD m is associated with RRID s.
    };
    uint32_t raw;
} srcmd_en_t;

// SRCMD_ENH register (0, .... , HWCFG1.rrid_num-1) is a specific register
// for each source (RRID) and indicates which MDs this source maps to
typedef union {
    struct {
        uint32_t mdh : 32;                  // mdh[m]=1 indicates MD m+31 is associated with RRID;
    };
    uint32_t raw;
} srcmd_enh_t;

// SRCMD_R register (0, .... , HWCFG1.rrid_num-1) is an optional
// specific register for each source (RRID) and indicates which MDs
// has read permissions.
typedef union {
    struct {
        uint32_t rsv : 1;                   // Reserved for future use
        uint32_t md  : 31;                  // md[m] = 1 indicates RRID s has read permission to
                                            // the corresponding MD m
    };
    uint32_t raw;
} srcmd_r_t;

// SRCMD_RH register (0, .... , HWCFG1.rrid_num-1) is an optional
// specific register for each source (RRID) and indicates which MDs
// has read permissions.
typedef union {
    struct {
        uint32_t mdh : 32;                  // md[m] = 1 indicates RRID s has read permission to
                                            // the corresponding MD m+31
    };
    uint32_t raw;
} srcmd_rh_t;

// SRCMD_W register (0, .... , HWCFG1.rrid_num-1) is an optional
// specific register for each source (RRID) and indicates which MDs
// has write permissions.
typedef union {
    struct {
        uint32_t rsv : 1;                   // Reserved for future use
        uint32_t md  : 31;                  // md[m] = 1 indicates RRID s has write permission to
                                            // the corresponding MD m
    };
    uint32_t raw;
} srcmd_w_t;

// SRCMD_WH register (0, .... , HWCFG1.rrid_num-1) is an optional
// specific register for each source (RRID) and indicates which MDs
// has write permissions.
typedef union {
    struct {
        uint32_t mdh : 32;                  // md[m] = 1 indicates RRID s has write permission to
                                            // the corresponding MD m+31
    };
    uint32_t raw;
} srcmd_wh_t;

// SRCMD_X register (0, .... , HWCFG1.rrid_num-1) is an optional
// specific register for each source (RRID) and indicates which MDs
// has instruction fetch permissions.
typedef union {
    struct {
        uint32_t rsv : 1;                   // Reserved for future use
        uint32_t md  : 31;                  // md[m] = 1 indicates RRID s has instruction fetch permission to
                                            // the corresponding MD m
    };
    uint32_t raw;
} srcmd_x_t;

// SRCMD_XH register (0, .... , HWCFG1.rrid_num-1) is an optional
// specific register for each source (RRID) and indicates which MDs
// has instruction fetch permissions.
typedef union {
    struct {
        uint32_t mdh : 32;                  // md[m] = 1 indicates RRID s has instruction fetch permission to
                                            // the corresponding MD m+31
    };
    uint32_t raw;
} srcmd_xh_t;

typedef union {
    struct {
        uint32_t perm  : 32;
    };
    uint32_t raw;
} srcmd_perm_t;

typedef union {
    struct {
        uint32_t permh  : 32;
    };
    uint32_t raw;
} srcmd_permh_t;

// SRCMD Table contains HWCFG1.rrid_num-1 groups of registers
typedef union {
    struct {
        srcmd_en_t  srcmd_en;
        srcmd_enh_t srcmd_enh;
        srcmd_r_t   srcmd_r;
        srcmd_rh_t  srcmd_rh;
        srcmd_w_t   srcmd_w;
        srcmd_wh_t  srcmd_wh;
        srcmd_x_t   srcmd_x;
        srcmd_xh_t  srcmd_xh;
    };
    struct {
        srcmd_perm_t  srcmd_perm;
        srcmd_permh_t srcmd_permh;
        uint32_t      rsvd1[6];
    };
} srcmd_table_t;

// ENTRY_ADDR registers (0, ..... HWCFG1.entry_num-1) holds physical address
// of protected memory region
typedef union {
    struct {
        uint32_t addr : 32;                 // The physical address[33:2] of protected memory region.
    };
    uint32_t raw;
} entry_addr_t;

// ENTRY_ADDRH register (0, ..... HWCFG1.entry_num-1) holds physical address
// of protected memory region.
// it is implemented to support wider physical addresses However, an IOPMP
// can only manage a segment of space, so an implementation would have a certain
// number of the most significant bits that are the same among all entries.
// These bits are allowed to be hardwired.
typedef union {
    struct {
        uint32_t addrh : 32;                // The physical address[65:43] of protected memory region.
    };
    uint32_t raw;
} entry_addrh_t;

// ENTRY_CFG register (0, ..... HWCFG1.entry_num-1) holds permissions
// related to protected meomory region (IOPMP entry)
// These entries are used to validate the requested permissions.
typedef union {
    struct {
        uint32_t r    : 1;                  // The read permission to protected memory region

        uint32_t w    : 1;                  // The write permission to the protected memory region

        uint32_t x    : 1;                  // The instruction fetch permission to the protected memory region.
                                            // Optional field, if unimplemented, write any read the same value
                                            // as r field.

        uint32_t a    : 2;                  // The address mode of the IOPMP entry
                                            // 0x0: OFF
                                            // 0x1: TOR
                                            // 0x2: NA4
                                            // 0x3: NAPOT

        uint32_t sire : 1;                  // Suppress interrupt for an illegal read access caught by the entry.
        uint32_t siwe : 1;                  // Suppress interrupt for an illegal write access/AMO caught by the entry.
        uint32_t sixe : 1;                  // Suppress interrupt on an illegal instruction fetch caught by the entry.
        uint32_t sere : 1;                  // Supress the (bus) error on an illegal read access caught by the entry
                                            // * 0x0: respond an error if ERR_CFG.rs is 0x0.
                                            // * 0x1: do not respond an error. User to define the behavior,
                                            //        e.g., respond a success with an implementation-dependent value to
                                            //        the initiator.
        uint32_t sewe : 1;                  // Supress the (bus) error on an illegal write access caught by the entry
                                            // * 0x0: respond an error if ERR_CFG.rs is 0x0.
                                            // * 0x1: do not respond an error. User to define the behavior,
                                            //        e.g., respond a success if response is needed
        uint32_t sexe : 1;                  // Suppress the (bus) error on an illegal instruction fetch caught by the
                                            // entry:
                                            // * 0x0: the response by ERR_CFG.rxe
                                            // * 0x1: do not respond an error. User to define the behavior,
                                            //        e.g., respond a success with an implementation-dependent value to
                                            //        the requestor.
        uint32_t rsv  : 21;                 // Must be zero on write, reserved for future
    };
    uint32_t raw;
} entry_cfg_t;

// ENTRY_USER_CFG implementation defined registers (0, ..... HWCFG1.entry_num-1)
// that allows users to define their own additional IOPMP check
// rules beside the rules defined in ENTRY_CFG.
typedef union {
    struct {
        uint32_t im;                        // reserved for future use
    };
    uint32_t raw;
} entry_user_cfg_t;

// IOPMP Entry Table contains HWCFG1.rrid_num-1 groups of register
typedef struct {
    entry_addr_t     entry_addr;
    entry_addrh_t    entry_addrh;
    entry_cfg_t      entry_cfg;
    entry_user_cfg_t entry_user_cfg;
} entry_table_t;

// IOPMP Paked register map
typedef union {
    struct __attribute__((__packed__)) {
        version_t        version;
        implementation_t implementation;
        hwcfg0_t         hwcfg0;
        hwcfg1_t         hwcfg1;
        hwcfg2_t         hwcfg2;
        hwcfg3_t         hwcfg3;
        uint32_t         reserved0[5];
        entryoffset_t    entryoffset;
        mdstall_t        mdstall;
        mdstallh_t       mdstallh;
        rridscp_t        rridscp;
        uint32_t         reserved1[1];
        mdlck_t          mdlck;
        mdlckh_t         mdlckh;
        mdcfglck_t       mdcfglck;
        entrylck_t       entrylck;
        uint32_t         reserved2[4];
        err_cfg_t        err_cfg;
        err_info_t       err_info;
        err_reqaddr_t    err_reqaddr;
        err_reqaddrh_t   err_reqaddrh;
        err_reqid_t      err_reqid;
        err_mfr_t        err_mfr;
        err_msiaddr_t    err_msiaddr;
        err_msiaddrh_t   err_msiaddrh;
        err_user_t       err_user[8];
        uint32_t         reserved4[472];
        mdcfg_t          mdcfg[IOPMP_MAX_MD_NUM];
        uint32_t         reserved5[(SRCMD_TABLE_BASE_OFFSET - (MDCFG_TABLE_BASE_OFFSET + (IOPMP_MAX_MD_NUM * 4))) / 4];
        srcmd_table_t    srcmd_table[IOPMP_MAX_RRID_NUM];
    };
    uint32_t regs4[(SRCMD_TABLE_BASE_OFFSET + (ALIGNUP(IOPMP_MAX_RRID_NUM, sizeof(uint32_t)) * SRCMD_REG_STRIDE)) / sizeof(uint32_t)];
    uint64_t regs8[(SRCMD_TABLE_BASE_OFFSET + (ALIGNUP(IOPMP_MAX_RRID_NUM, sizeof(uint32_t)) * SRCMD_REG_STRIDE)) / sizeof(uint64_t)];
} iopmp_regs_t;

typedef union {
    struct __attribute__((__packed__)) {
        entry_table_t    entry_table[IOPMP_MAX_ENTRY_NUM];
    };
    uint32_t regs4[(ALIGNUP(IOPMP_MAX_ENTRY_NUM, sizeof(uint32_t)) * ENTRY_REG_STRIDE) / sizeof(uint32_t)];
    uint64_t regs8[(ALIGNUP(IOPMP_MAX_ENTRY_NUM, sizeof(uint32_t)) * ENTRY_REG_STRIDE) / sizeof(uint64_t)];
} iopmp_entries_t;

// Number of subsequent violation record windows to accommodate all RRIDs.
#define NUM_SVW         (ALIGNUP(IOPMP_MAX_RRID_NUM, 16) / 16)

typedef struct {
    err_mfr_t sv[NUM_SVW];
} err_mfrs_t;

#endif