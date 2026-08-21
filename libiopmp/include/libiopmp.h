/*
 * Copyright 2018-2026 Andes Technology Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LIBIOPMP_H__
#define __LIBIOPMP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/******************************************************************************/
/* libiopmp data structure.                                                   */
/******************************************************************************/
/**
 * Structure for an IOPMP instance, including base address, operations,
 * configurations, etc
 */
struct iopmp_instance {
    /** Base MMIO physical address of IOPMP */
    uintptr_t addr;
    /** PMP granularity */
    uint32_t granularity;
    /** Implemented bits of ENTRY_ADDR(H) */
    uint64_t entry_addr_bits;
    /** Per-instance override of libiopmp's operations, NULL for none */
    const struct iopmp_operations_override *ops_override;

    /** Base MMIO physical address of IOPMP entries */
    uintptr_t addr_entry_array;

    /** The JEDEC manufacturer ID */
    uint32_t vendor;
    /** The user-defined implementation ID */
    uint32_t impid;
    /** Indicate the supported number of RRID in the instance */
    uint16_t rrid_num;
    /** Indicate the supported number of entries in the instance */
    uint16_t entry_num;
    /** Indicate the number of entries matched with priority */
    uint16_t prio_entry_num;
    /** The RRID tagged to outgoing transactions */
    uint16_t rrid_transl;
    /** The specification version */
    uint8_t specver;
    /** Indicate the supported number of MD in the instance */
    uint8_t md_num;
    /** When mdcfg_fmt={1,2}, indicate each MD has (md_entry_num+1) entries */
    uint8_t md_entry_num;

    /**
     * MDs that MDSTALL(H) can select. Every implemented MD until
     * iopmp_probe_stall_by_md() narrows it down
     */
    uint64_t stall_md_mask;

    /** Cache of MDLCK.l */
    uint8_t mdlck_lock;
    /** Cache of MDLCK.md */
    uint64_t mdlck_md;
    /** Cache of MDCFGLCK.l */
    uint8_t mdcfglck_lock;
    /** Cache of MDCFGLCK.f */
    uint8_t mdcfglck_f;
    /** Cache of ENTRYLCK.l */
    uint8_t entrylck_lock;
    /** Cache of ENTRYLCK.f */
    uint16_t entrylck_f;

    /**
     * Cache of {ERR_MSIADDRH, ERR_MSIADDR}. If HWCFG0.addrh_en=0, this member
     * contains bits 33 to 2 of the MSI address. If HWCFG0.addrh_en=1, this
     * member contains bits 63 to 0 of the MSI address
     */
    uint64_t msiaddr64;
    /** Cache of ERR_CFG.msidata */
    uint16_t msidata;

    /** Flags */
    struct {
        /** Flag to indicate the IOPMP instance has been initialized */
        unsigned int init : 1;
        /** Flag to indicate the MDCFG format */
        unsigned int mdcfg_fmt : 2;
        /** Flag to indicate the SRCMD format */
        unsigned int srcmd_fmt : 2;
        /** Flag to indicate if the Error Capture Record is not implemented */
        unsigned int no_err_rec : 1;
        /** Flag to indicate if TOR is supported */
        unsigned int tor_en : 1;
        /** Flag to indicate SPS(secondary permission settings) is supported */
        unsigned int sps_en : 1;
        /** Flag to indicates if HWCFG2.prio_entry is programmable */
        unsigned int prio_ent_prog : 1;
        /** Flag to indicates whether the IOPMP supports non-priority entries */
        unsigned int non_prio_en : 1;
        /**
         * Flag to indicate the if tagging a new RRID on the initiator port is
         * supported
         */
        unsigned int rrid_transl_en : 1;
        /** Flag to indicate if the field HWCFG3.rrid_transl is programmable */
        unsigned int rrid_transl_prog : 1;
        /**
         * Flag to indicate if the IOPMP treats the instruction fetch accesses
         * as data read accesses
         */
        unsigned int xinr : 1;
        /**
         * Flag to indicate for xinr=0, the IOPMP with no_x=1 always fails on
         * an instruction fetch
         */
        unsigned int no_x : 1;
        /**
         * Flag to indicate if the IOPMP always fails write accesses considered
         * as no rule matched
         */
        unsigned int no_w : 1;
        /** Flag to indicate if the IOPMP implements stall-related features */
        unsigned int stall_en : 1;
        /**
         * Flag to indicate if the IOPMP implements interrupt suppression per
         * entry
         */
        unsigned int peis : 1;
        /**
         * Flag to indicate if the IOPMP implements error suppression per entry
         */
        unsigned int pees : 1;
        /**
         * Flag to indicate if the IOPMP implements MFR(Multi-Faults Record)
         * extension
         */
        unsigned int mfr_en : 1;
        /**
         * Flag to indicate if registers ENTRY_ADDRH(i) and ERR_MSIADDRH (if
         * HWCFG2.msi_en = 1) are available
         */
        unsigned int addrh_en : 1;
        /** Indicate if the IOPMP checks transactions */
        unsigned int enable : 1;
        /** Lock fields to ERR_CFG register */
        unsigned int err_cfg_lock : 1;
        /** Enable the global interrupt of the IOPMP */
        unsigned int intr_enable : 1;
        /** Suppress the global error responses of the IOPMP */
        unsigned int err_resp_suppress : 1;
        /** Flag to indicate whether the IOPMP supports MSI extension */
        unsigned int msi_en : 1;
        /** Flag to indicate whether the IOPMP triggers MSI or wired interrupt*/
        unsigned int msi_sel : 1;
        /** Flag to indicate whether the IOPMP faults stalled transactions */
        unsigned int stall_violation_en : 1;
        /** Flag to indicate if stall by RRID is supported */
        unsigned int support_stall_by_rrid : 1;
        /** Flag to indicate if IOPMP is stalling some transactions */
        unsigned int is_stalling : 1;
    };
};

/**
 * Flags to indicate an entry must be priority entry or not. Some APIs
 * which writing the entries into IOPMP will check this flag
 *   - 0b00: ignore check
 *   - 0b01: must be priority entry
 *   - 0b10: must be non-priority entry
 */
enum iopmp_prient_flags {
    /** User sets this flag to indicate the entry's priority doesn't matter */
    IOPMP_PRIENT_ANY = 0,
    /** User sets this flag to indicate the entry must be priority entry */
    IOPMP_PRIENT_PRIORITY = (1 << 0),
    /** User sets this flag to indicate the entry must be non-priority entry */
    IOPMP_PRIENT_NON_PRIORITY = (1 << 1),
};

/**
 * Structure to represent an IOPMP entry, including the physical address
 * of protected memory region, permission attributes, per-entry suppression
 * settings, priority flags, private data, etc
 */
struct iopmp_entry {
    /** Values of ENTRY_ADDR and ENTRY_ADDRH */
    union {
        struct {
            /** The physical address[33:2] of memory region */
            uint32_t addrl;
            /** The physical address[65:34] of memory region */
            uint32_t addrh;
        };
        /** The physical address[65:2] of protected memory region */
        uint64_t addr;
    };

    /** Value of ENTRY_CFG */
    union {
        struct {
            /** ENTRY_CFG.r */
            uint32_t r   : 1;
            /** ENTRY_CFG.w */
            uint32_t w   : 1;
            /** ENTRY_CFG.x */
            uint32_t x   : 1;
            /** ENTRY_CFG.a */
            uint32_t a   : 2;
            /** ENTRY_CFG.sire */
            uint32_t sire: 1;
            /** ENTRY_CFG.siwe */
            uint32_t siwe: 1;
            /** ENTRY_CFG.sixe */
            uint32_t sixe: 1;
            /** ENTRY_CFG.sere */
            uint32_t sere: 1;
            /** ENTRY_CFG.sewe */
            uint32_t sewe: 1;
            /** ENTRY_CFG.sexe */
            uint32_t sexe: 1;
            /** ENTRY_CFG.rsv */
            uint32_t rsv : 21;
        };
        /** ENTRY_CFG */
        uint32_t cfg;
    };

    /** Flag to indicate this is priority or non-priority entry */
    enum iopmp_prient_flags prient_flag;

    /**
     * Additional 64-bit data that can be used in specific model.
     *
     * For example, it can be used as SRCMD_PERM(H) in SRCMD_FMT=2, MDCFG_FMT=1
     * and HWCFG3.md_entry_num=0 (K=1). In this configuration, each MD has
     * exactly single entry. User can set SRCMD_PERM(H) and entry in single
     * entry API call.
     *
     * \note Only iopmp_set_entries_with_md_permission() writes it;
     *       iopmp_set_entries() leaves SRCMD_PERM(H) alone
     */
    uint64_t private_data;
};

/** Indicated the transaction type of the first captured violation */
enum iopmp_errinfo_ttype {
    /** Reserved */
    IOPMP_ERRINFO_TTYPE_RSVD       = 0x00,
    /** Read access */
    IOPMP_ERRINFO_TTYPE_READ       = 0x01,
    /** Write access/AMO */
    IOPMP_ERRINFO_TTYPE_WRITE      = 0x02,
    /** Instruction fetch */
    IOPMP_ERRINFO_TTYPE_INST_FETCH = 0x03,
};

/** Indicated the type of violation */
enum iopmp_errinfo_etype {
    /** No error */
    IOPMP_ERRINFO_ETYPE_NONE         = 0x00,
    /** Illegal read access */
    IOPMP_ERRINFO_ETYPE_READ         = 0x01,
    /** Illegal write access/AMO */
    IOPMP_ERRINFO_ETYPE_WRITE        = 0x02,
    /** Illegal instruction fetch */
    IOPMP_ERRINFO_ETYPE_INST_FETCH   = 0x03,
    /** Partial hit on a priority rule */
    IOPMP_ERRINFO_ETYPE_PART_HIT     = 0x04,
    /** Not hit any rule */
    IOPMP_ERRINFO_ETYPE_NOT_HIT      = 0x05,
    /** Unknown RRID */
    IOPMP_ERRINFO_ETYPE_UNKNOWN_RRID = 0x06,
    /** Error due to a stalled transaction */
    IOPMP_ERRINFO_ETYPE_STALL        = 0x07,
    /** N/A, reserved for future */
    IOPMP_ERRINFO_ETYPE_RESERVED_0   = 0x08,
    /** N/A, reserved for future */
    IOPMP_ERRINFO_ETYPE_RESERVED_1   = 0x09,
    /** N/A, reserved for future */
    IOPMP_ERRINFO_ETYPE_RESERVED_2   = 0x0A,
    /** N/A, reserved for future */
    IOPMP_ERRINFO_ETYPE_RESERVED_3   = 0x0B,
    /** N/A, reserved for future */
    IOPMP_ERRINFO_ETYPE_RESERVED_4   = 0x0C,
    /** N/A, reserved for future */
    IOPMP_ERRINFO_ETYPE_RESERVED_5   = 0x0D,
    /** User-defined error */
    IOPMP_ERRINFO_ETYPE_USER_DEF_0   = 0x0E,
    /** User-defined error */
    IOPMP_ERRINFO_ETYPE_USER_DEF_1   = 0x0F,
};

/** Structure represents an IOPMP error report */
struct iopmp_err_report {
    /** Errored address[65:2] */
    uint64_t addr;
    /** Errored RRID */
    uint32_t rrid;
    /** Indicates the index pointing to the entry that catches the violation */
    uint32_t eid;
    /** Indicated the transaction type of the first captured violation */
    enum iopmp_errinfo_ttype ttype;
    /** Indicated the type of violation */
    enum iopmp_errinfo_etype etype;
    /** Indicate the write access to trigger an IOPMP originated MSI failed */
    bool msi_werr;
    /** Indicate there is a subsequent violation caught in ERR_MFR */
    bool svc;
};

typedef struct iopmp_instance IOPMP_t;

typedef struct iopmp_entry IOPMP_Entry_t;

typedef struct iopmp_err_report IOPMP_ERR_REPORT_t;

/** Maximum supported RRID when srcmd_fmt=2 */
#define IOPMP_MAX_RRID_SRCMD_FMT_2  32

/**
 * \brief Configuration used in srcmd_fmt=2 to set SRCMD_PERM(H)
 *
 * \note User should call the following macros or APIs to update this structure:
 *       - iopmp_set_srcmd_perm_cfg() to update single RRID
 *       - iopmp_set_srcmd_perm_cfg_nocheck() to update single RRID
 *       - IOPMP_SRCMD_PERM_CFG_SET_DIRECT() to directly set multiple RRIDs
 */
struct iopmp_srcmd_perm_config {
/** Bit position of SRCMD_PERM.r for each RRID */
#define IOPMP_SRCMD_PERM_R      (1 << 0)
/** Bit position of SRCMD_PERM.w for each RRID */
#define IOPMP_SRCMD_PERM_W      (1 << 1)
/** Bit mask of SRCMD_PERM for each RRID */
#define IOPMP_SRCMD_PERM_MASK   (IOPMP_SRCMD_PERM_W | IOPMP_SRCMD_PERM_R)

    /**
     * Bit mask to indicate which RRIDs' permission bits should be configured.
     * For example, if we are going to configure RRID(0)'s bits, the bit 0 and
     * bit 1 of this member will be set to 1
     */
    uint64_t srcmd_perm_mask;

    /**
     * Bit mask to indicate the desired permissions for configured RRIDs.
     * For example, if we are going to configure RRID(0)'s bits, the bit 0
     * indicates whether RRID(0) has read permission on this MD, while the bit 1
     * indicates whether RRID(0) has write permission on this MD
     */
    uint64_t srcmd_perm_val;
};
typedef struct iopmp_srcmd_perm_config IOPMP_SRCMD_PERM_CFG_t;

/**
 * \brief Macro used to directly set members in struct iopmp_srcmd_perm_config
 *
 * \param[in] cfg       pointer to struct iopmp_srcmd_perm_config
 * \param[in] mask      Desired value of srcmd_perm_mask
 * \param[in] val       Desired value of srcmd_perm_val
 */
#define IOPMP_SRCMD_PERM_CFG_SET_DIRECT(cfg, mask, val) \
    do {                                                \
        IOPMP_SRCMD_PERM_CFG_t *cfg_p = (cfg);          \
        cfg_p->srcmd_perm_mask = mask;                  \
        cfg_p->srcmd_perm_val  = val;                   \
    } while (0);

/******************************************************************************/
/* Supported IOPMP implementation ID                                          */
/******************************************************************************/
/** Enumerate implementation ID of IOPMP */
enum iopmp_impid {
    /** The implementation ID of IOPMP is not specified */
    IOPMP_IMPID_NOT_SPECIFIED = 0xFFFFFFFF,
};

/******************************************************************************/
/* MDCFG_FMT and SRCMD_FMT and models                                         */
/******************************************************************************/
/** Enumerate the SRCMD table format */
enum iopmp_srcmd_fmt {
    /** Format 0. SRCMD_EN(s) and SRCMD_ENH(s) are available */
    IOPMP_SRCMD_FMT_0,
    /** Format 1. No SRCMD Table */
    IOPMP_SRCMD_FMT_1,
    /** Format 2. SRCMD_PERM(m) and SRCMD_PERMH(m) are available */
    IOPMP_SRCMD_FMT_2,
    /** Reserved */
    IOPMP_SRCMD_FMT_RESERVED,
    /** Maximum number of SRCMD table formats */
    IOPMP_SRCMD_FMT_MAX,
};

/** Enumerate the MDCFG table format */
enum iopmp_mdcfg_fmt {
    /** Format 0. MDCFG Table is implemented */
    IOPMP_MDCFG_FMT_0,
    /** Format 1. No MDCFG Table. HWCFG3.md_entry_num is fixed */
    IOPMP_MDCFG_FMT_1,
    /** Format 2. No MDCFG Table. HWCFG3.md_entry_num is programmable */
    IOPMP_MDCFG_FMT_2,
    /** Reserved */
    IOPMP_MDCFG_FMT_RESERVED,
    /** Maximum number of MDCFG table formats */
    IOPMP_MDCFG_FMT_MAX,
};

/** Enumerate well-defined IOPMP models */
enum iopmp_model {
    /** srcmd_fmt = 0 and mdcfg_fmt = 0 */
    IOPMP_MODEL_FULL        = 0,
    /** srcmd_fmt = 0 and mdcfg_fmt = 1 */
    IOPMP_MODEL_RAPID_K     = 1,
    /** srcmd_fmt = 0 and mdcfg_fmt = 2 */
    IOPMP_MODEL_DYNAMIC_K   = 2,
    /** srcmd_fmt = 0 and mdcfg_fmt = 3 (reserved) */
    IOPMP_MODEL_RESERVED_3  = 3,
    /** srcmd_fmt = 1 and mdcfg_fmt = 0 */
    IOPMP_MODEL_ISOLATION   = 4,
    /** srcmd_fmt = 1 and mdcfg_fmt = 1 */
    IOPMP_MODEL_COMPACT_K   = 5,
    /** srcmd_fmt = 1 and mdcfg_fmt = 2 */
    IOPMP_MODEL_6           = 6,
    /** srcmd_fmt = 1 and mdcfg_fmt = 3 (reserved) */
    IOPMP_MODEL_RESERVED_7  = 7,
    /** srcmd_fmt = 2 and mdcfg_fmt = 0 */
    IOPMP_MODEL_8           = 8,
    /** srcmd_fmt = 2 and mdcfg_fmt = 1 */
    IOPMP_MODEL_9           = 9,
    /** srcmd_fmt = 2 and mdcfg_fmt = 2 */
    IOPMP_MODEL_RESERVED_10 = 10,
    /** srcmd_fmt = 2 and mdcfg_fmt = 3 (reserved) */
    IOPMP_MODEL_RESERVED_11 = 11,
    /** srcmd_fmt = 3 and mdcfg_fmt = 0 (reserved) */
    IOPMP_MODEL_RESERVED_12 = 12,
    /** srcmd_fmt = 3 and mdcfg_fmt = 1 (reserved) */
    IOPMP_MODEL_RESERVED_13 = 13,
    /** srcmd_fmt = 3 and mdcfg_fmt = 2 (reserved) */
    IOPMP_MODEL_RESERVED_14 = 14,
    /** srcmd_fmt = 3 and mdcfg_fmt = 3 (reserved) */
    IOPMP_MODEL_RESERVED_15 = 15
};

/** The operations of RRIDSCP.op field */
enum iopmp_rridscp_op {
    /** Query */
    IOPMP_RRIDSCP_OP_QUERY = 0,
    /** Stall transactions associated with selected RRID */
    IOPMP_RRIDSCP_OP_STALL = 1,
    /** Don’t stall transactions associated with selected RRID */
    IOPMP_RRIDSCP_OP_DONT_STALL = 2,
    /** Reserved */
    IOPMP_RRIDSCP_OP_RESERVED = 3
};

/** The states of RRIDSCP.stat field */
enum iopmp_rridscp_stat {
    /** RRIDSCP is not implemented */
    IOPMP_RRIDSCP_STAT_NOT_IMPL = 0,
    /** Transactions associated with selected RRID are stalled */
    IOPMP_RRIDSCP_STAT_STALLED = 1,
    /** Transactions associated with selected RRID are not stalled */
    IOPMP_RRIDSCP_STAT_NOT_STALLED = 2,
    /** Unimplemented or unselectable RRID */
    IOPMP_RRIDSCP_STAT_ERR_RRID = 3
};

/******************************************************************************/
/* The flags used when calling iopmp_encode_entry()                           */
/******************************************************************************/
/** The flags used when calling iopmp_encode_entry() */
enum iopmp_entry_flags {
    /** ENTRY_CFG.r = 1 */
    IOPMP_ENTRY_R = (1UL << 0),
    /** ENTRY_CFG.w = 1 */
    IOPMP_ENTRY_W = (1UL << 1),
    /** ENTRY_CFG.x = 1 */
    IOPMP_ENTRY_X = (1UL << 2),
    /** ENTRY_CFG.r = 1 and ENTRY_CFG.w = 1 */
    IOPMP_ENTRY_RW = (IOPMP_ENTRY_R | IOPMP_ENTRY_W),
    /** ENTRY_CFG.r = 1 and ENTRY_CFG.x = 1 */
    IOPMP_ENTRY_RX = (IOPMP_ENTRY_R | IOPMP_ENTRY_X),
    /** ENTRY_CFG.r = 1, ENTRY_CFG.w = 1, and ENTRY_CFG.x = 1 */
    IOPMP_ENTRY_RWX = (IOPMP_ENTRY_R | IOPMP_ENTRY_W | IOPMP_ENTRY_X),

    /** ENTRY_CFG.a = OFF */
    IOPMP_ENTRY_A_OFF = (0UL << 3),
    /** ENTRY_CFG.a = TOR */
    IOPMP_ENTRY_A_TOR = (1UL << 3),
    /** ENTRY_CFG.a = NA4 */
    IOPMP_ENTRY_A_NA4 = (2UL << 3),
    /** ENTRY_CFG.a = NAPOT */
    IOPMP_ENTRY_A_NAPOT = (3UL << 3),
    /** Bit mask of ENTRY_CFG.a field */
    IOPMP_ENTRY_A_MASK = (3UL << 3),

    /** ENTRY_CFG.sire = 1 */
    IOPMP_ENTRY_SIRE = (1UL << 5),
    /** ENTRY_CFG.siwe = 1 */
    IOPMP_ENTRY_SIWE = (1UL << 6),
    /** ENTRY_CFG.sixe = 1 */
    IOPMP_ENTRY_SIXE = (1UL << 7),
    /** Bit mask of ENTRY_CFG.sire, ENTRY_CFG.siwe, and ENTRY_CFG.sixe */
    IOPMP_ENTRY_SIE_MASK = (7UL << 5),
    /** ENTRY_CFG.sere = 1 */
    IOPMP_ENTRY_SERE = (1UL << 8),
    /** ENTRY_CFG.sewe = 1 */
    IOPMP_ENTRY_SEWE = (1UL << 9),
    /** ENTRY_CFG.sexe = 1 */
    IOPMP_ENTRY_SEXE = (1UL << 10),
    /** Bit mask of ENTRY_CFG.sere, ENTRY_CFG.sewe, and ENTRY_CFG.sexe */
    IOPMP_ENTRY_SEE_MASK = (7UL << 8),

    /** [Software flag] Forcefully set ENTRY_CFG.a = OFF */
    IOPMP_ENTRY_FORCE_OFF = (1UL << 27),
    /** [Software flag] Forcefully set ENTRY_CFG(0).a = TOR */
    IOPMP_ENTRY_FIRST_TOR = (1UL << 28),
    /** [Software flag] Forcefully set ENTRY_CFG.a = TOR */
    IOPMP_ENTRY_FORCE_TOR = (1UL << 29),

    /** [Software flag] Flag to indicate this entry is priority entry */
    IOPMP_ENTRY_PRIO = (1UL << 30),
    /** [Software flag] Flag to indicate this entry is non-priority entry */
    IOPMP_ENTRY_NON_PRIO = (1UL << 31),

    /** Bit mask of all software flags */
    IOPMP_ENTRY_SW_FLAGS_MASK = (IOPMP_ENTRY_FORCE_OFF | IOPMP_ENTRY_FIRST_TOR |
                                 IOPMP_ENTRY_FORCE_TOR | IOPMP_ENTRY_PRIO |
                                 IOPMP_ENTRY_NON_PRIO),
};

/******************************************************************************/
/* API Error codes                                                            */
/******************************************************************************/
/** The libiopmp API error Code */
enum iopmp_error {
    /** Success */
    IOPMP_OK                    =  0,
    /** The operation is not supported by this IOPMP */
    IOPMP_ERR_NOT_SUPPORTED     = -1,
    /** The given index is out-of-bounds */
    IOPMP_ERR_OUT_OF_BOUNDS     = -2,
    /** The register is locked */
    IOPMP_ERR_REG_IS_LOCKED     = -3,
    /** The operation is not allowed */
    IOPMP_ERR_NOT_ALLOWED       = -4,
    /** The result does not exist */
    IOPMP_ERR_NOT_EXIST         = -5,
    /** The resource is not available */
    IOPMP_ERR_NOT_AVAILABLE     = -6,
    /** The given parameter is invalid */
    IOPMP_ERR_INVALID_PARAMETER = -7,
    /** The given priority is invalid */
    IOPMP_ERR_INVALID_PRIORITY  = -8,
    /** The desired value written into WARL field does not match actual value */
    IOPMP_ERR_ILLEGAL_VALUE     = -9,
};

/******************************************************************************/
/* Helper macros and functions to get libiopmp version information            */
/******************************************************************************/
/** Major version of libiopmp release version */
#define LIBIOPMP_VERSION_MAJOR          0
/** Minor version of libiopmp release version */
#define LIBIOPMP_VERSION_MINOR          1
/** Extra version of libiopmp release version */
#define LIBIOPMP_VERSION_EXTRA          0

/** The bit position of the major version encoded in the IOPMP version number */
#define LIBIOPMP_VERSION_MAJOR_SHIFT    16
/** The bit mask of the major version encoded in the IOPMP version number */
#define LIBIOPMP_VERSION_MAJOR_MASK     0xffff

/** The bit position of the minor version encoded in the IOPMP version number */
#define LIBIOPMP_VERSION_MINOR_SHIFT    8
/** The bit mask of the minor version encoded in the IOPMP version number */
#define LIBIOPMP_VERSION_MINOR_MASK     0xff

/** The bit position of the extra version encoded in the IOPMP version number */
#define LIBIOPMP_VERSION_EXTRA_SHIFT    0
/** The bit mask of the extra version encoded in the IOPMP version number */
#define LIBIOPMP_VERSION_EXTRA_MASK     0xff

/**
 * \brief The macro to construct the IOPMP version number
 *
 * \param[in] __major           The major version
 * \param[in] __minor           The minor version
 * \param[in] __extra           The extra version
 *
 * \return IOPMP version number
 */
#define LIBIOPMP_VERSION(__major, __minor, __extra) \
((((__major) & LIBIOPMP_VERSION_MAJOR_MASK) << LIBIOPMP_VERSION_MAJOR_SHIFT) | \
 (((__minor) & LIBIOPMP_VERSION_MINOR_MASK) << LIBIOPMP_VERSION_MINOR_SHIFT) | \
 (((__extra) & LIBIOPMP_VERSION_EXTRA_MASK) << LIBIOPMP_VERSION_EXTRA_SHIFT))

/**
 * \brief Get major version of libiopmp
 *
 * \return The major version of libiopmp
 */
int libiopmp_major_version(void);

/**
 * \brief Get minor version of libiopmp
 *
 * \return The minor version of libiopmp
 */
int libiopmp_minor_version(void);

/**
 * \brief Get extra version of libiopmp
 *
 * \return The extra version of libiopmp
 */
int libiopmp_extra_version(void);

/**
 * \brief Check given version with libiopmp
 *
 * \param[in] major             The major version
 * \param[in] minor             The minor version
 * \param[in] extra             The extra version
 *
 * \retval 1 if given version is greater than version of libiopmp
 * \retval 0 if given version is less than or equal to version of libiopmp
 */
bool libiopmp_check_version(int major, int minor, int extra);

/******************************************************************************/
/* Helper macros to get/set local variables                                   */
/******************************************************************************/
/**
 * \brief Check if the IOPMP has been initialized by libiopmp
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if the IOPMP has been initialized by libiopmp
 * \retval 0 if the IOPMP hasn't been initialized by libiopmp
 */
static inline bool iopmp_is_initialized(IOPMP_t *iopmp)
{
    return (iopmp != NULL) && (iopmp->init != 0U);
}

/**
 * \brief Get the base physical address of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return The base physical address of the IOPMP
 */
static inline uintptr_t iopmp_get_base_addr(IOPMP_t *iopmp)
{
    return iopmp->addr;
}

/**
 * \brief Get the base physical address of the IOPMP entry array
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return The base physical address of the IOPMP entry array
 */
static inline uintptr_t iopmp_get_base_addr_entry_array(IOPMP_t *iopmp)
{
    return iopmp->addr_entry_array;
}

/**
 * \brief Get the granularity of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return The granularity of the IOPMP
 */
static inline uint32_t iopmp_get_granularity(IOPMP_t *iopmp)
{
    return iopmp->granularity;
}

/**
 * \brief Get HWCFG3.mdcfg_fmt of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return HWCFG3.mdcfg_fmt of the IOPMP
 */
static inline enum iopmp_mdcfg_fmt iopmp_get_mdcfg_fmt(IOPMP_t *iopmp)
{
    return iopmp->mdcfg_fmt;
}

/**
 * \brief Get HWCFG3.srcmd_fmt of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return HWCFG3.srcmd_fmt of the IOPMP
 */
static inline enum iopmp_srcmd_fmt iopmp_get_srcmd_fmt(IOPMP_t *iopmp)
{
    return iopmp->srcmd_fmt;
}

/**
 * \brief Get HWCFG0.tor_en of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \retval 1 if HWCFG0.tor_en = 1
 * \retval 0 if HWCFG0.tor_en = 0
 */
static inline bool iopmp_get_support_tor(IOPMP_t *iopmp)
{
    return iopmp->tor_en;
}

/**
 * \brief Check if the IOPMP supports SPS extension
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG2.sps_en = 1
 * \retval 0 if HWCFG2.sps_en = 0
 */
static inline bool iopmp_get_support_sps(IOPMP_t *iopmp)
{
    return iopmp->sps_en;
}

/**
 * \brief Check if HWCFG2.prio_entry is programmable
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG2.prio_ent_prog = 1
 * \retval 0 if HWCFG2.prio_ent_prog = 0
 */
static inline bool iopmp_get_support_programmable_prio_entry(IOPMP_t *iopmp)
{
    return iopmp->prio_ent_prog;
}

/**
 * \brief Check if tagging a new RRID on the initiator port is supported
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG3.rrid_transl_en = 1
 * \retval 0 if HWCFG3.rrid_transl_en = 0
 */
static inline bool iopmp_get_support_rrid_transl(IOPMP_t *iopmp)
{
    return iopmp->rrid_transl_en;
}

/**
 * \brief Check if the IOPMP implements the check of an instruction fetch
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG3.xinr = 0
 * \retval 0 if HWCFG3.xinr = 1
 */
static inline bool iopmp_get_support_chk_x(IOPMP_t *iopmp)
{
    return iopmp->xinr == 0U;
}

/**
 * \brief Check if the IOPMP always fails on an instruction fetch
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG3.no_x = 1
 * \retval 0 if HWCFG3.no_x = 0
 */
static inline bool iopmp_get_no_x(IOPMP_t *iopmp)
{
    return iopmp->no_x;
}

/**
 * \brief Check if the IOPMP always fails on write accesses considered as
 * as no rule matched
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG3.no_w = 1
 * \retval 0 if HWCFG3.no_w = 0
 */
static inline bool iopmp_get_no_w(IOPMP_t *iopmp)
{
    return iopmp->no_w;
}

/**
 * \brief Check if the IOPMP implements stall-related features
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG2.stall_en = 1
 * \retval 0 if HWCFG2.stall_en = 0
 *
 * \note HWCFG2.stall_en covers MDSTALL(H), RRIDSCP and
 *       ERR_CFG.stall_violation_en together. Use
 *       iopmp_get_support_stall_by_rrid() and iopmp_probe_stall_by_md() to
 *       narrow it down
 */
static inline bool iopmp_get_support_stall(IOPMP_t *iopmp)
{
    return iopmp->stall_en;
}

/**
 * \brief Check if the IOPMP implements interrupt suppression per entry
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG2.peis = 1
 * \retval 0 if HWCFG2.peis = 0
 */
static inline bool iopmp_get_support_peis(IOPMP_t *iopmp)
{
    return iopmp->peis;
}

/**
 * \brief Check if the IOPMP implements the error suppression per entry
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG2.pees = 1
 * \retval 0 if HWCFG2.pees = 0
 */
static inline bool iopmp_get_support_pees(IOPMP_t *iopmp)
{
    return iopmp->pees;
}

/**
 * \brief Check if the IOPMP implements the Multi-Faults Record Extension
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG2.mfr_en = 1
 * \retval 0 if HWCFG2.mfr_en = 0
 */
static inline bool iopmp_get_support_mfr(IOPMP_t *iopmp)
{
    return iopmp->mfr_en;
}

/**
 * \brief Get the supported number of MD in the IOPMP instance
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return HWCFG0.md_num
 */
static inline uint32_t iopmp_get_md_num(IOPMP_t *iopmp)
{
    return iopmp->md_num;
}

/**
 * \brief Check if ENTRY_ADDRH(i) and ERR_MSIADDRH (if HWCFG2.msi_en = 1) are
 * available
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG0.addrh_en = 1
 * \retval 0 if HWCFG0.addrh_en = 0
 */
static inline uint32_t iopmp_get_addrh_en(IOPMP_t *iopmp)
{
    return iopmp->addrh_en;
}

/**
 * \brief Check if the IOPMP checks transactions
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if HWCFG0.enable = 1
 * \retval 0 if HWCFG0.enable = 0
 */
static inline bool iopmp_get_enable(IOPMP_t *iopmp)
{
    return iopmp->enable;
}

/**
 * \brief Get the supported number of RRID in the IOPMP instance
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return HWCFG1.rrid_num
 */
static inline uint32_t iopmp_get_rrid_num(IOPMP_t *iopmp)
{
    return iopmp->rrid_num;
}

/**
 * \brief Get the supported number of entries in the IOPMP instance
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return HWCFG1.entry_num
 */
static inline uint32_t iopmp_get_entry_num(IOPMP_t *iopmp)
{
    return iopmp->entry_num;
}

/**
 * \brief Get the number of entries matched with priority
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return HWCFG2.prio_entry
 */
static inline uint16_t iopmp_get_prio_entry_num(IOPMP_t *iopmp)
{
    return iopmp->prio_entry_num;
}

/**
 * \brief Check if the IOPMP implements stall-related features of RRIDSCP
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if RRIDSCP is implemented
 * \retval 0 if RRIDSCP is not implemented
 */
static inline bool iopmp_get_support_stall_by_rrid(IOPMP_t *iopmp)
{
    return iopmp->support_stall_by_rrid;
}

/**
 * \brief Check if the ERR_CFG register has been locked
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if ERR_CFG.l = 1
 * \retval 0 if ERR_CFG.l = 0
 */
static inline bool iopmp_is_err_cfg_locked(IOPMP_t *iopmp)
{
    return iopmp->err_cfg_lock;
}

/**
 * \brief Check if the interrupt of the IOPMP rule violation has been enabled
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if ERR_CFG.ie = 1
 * \retval 0 if ERR_CFG.ie = 0
 */
static inline bool iopmp_get_global_intr(IOPMP_t *iopmp)
{
    return iopmp->intr_enable;
}

/**
 * \brief Check if the IOPMP suppresses error response on a rule violation
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if ERR_CFG.rs = 1
 * \retval 0 if ERR_CFG.rs = 0
 */
static inline bool iopmp_get_global_err_resp(IOPMP_t *iopmp)
{
    return iopmp->err_resp_suppress;
}

/**
 * \brief Check if the IOPMP faults stalled transactions
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if ERR_CFG.stall_violation_en = 1
 * \retval 0 if ERR_CFG.stall_violation_en = 0
 */
static inline bool iopmp_get_stall_violation_en(IOPMP_t *iopmp)
{
    return iopmp->stall_violation_en;
}

/**
 * \brief Check if the IOPMP triggers interrupt by MSI
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if ERR_CFG.msi_sel = 1
 * \retval 0 if ERR_CFG.msi_sel = 0
 */
static inline bool iopmp_get_msi_sel(IOPMP_t *iopmp)
{
    return iopmp->msi_sel;
}

/**
 * \brief Check if MDLCK register has been locked
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if MDLCK.l = 1
 * \retval 0 if MDLCK.l = 0
 */
static inline bool iopmp_is_mdlck_locked(IOPMP_t *iopmp)
{
    return iopmp->mdlck_lock;
}

/**
 * \brief Check if ENTRYLCK register has been locked
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if ENTRYLCK.l = 1
 * \retval 0 if ENTRYLCK.l = 0
 */
static inline bool iopmp_is_entrylck_locked(IOPMP_t *iopmp)
{
    return iopmp->entrylck_lock;
}

/**
 * \brief Get the number of locked IOPMP entries
 *
 * \param[in] iopmp             The IOPMP instance to be got
 *
 * \return ENTRYLCK.f
 */
static inline uint32_t iopmp_get_locked_entry_num(IOPMP_t *iopmp)
{
    return iopmp->entrylck_f;
}

/**
 * \brief Get the errored address from the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \return Errored address[65:2]
 */
static inline uint64_t iopmp_err_report_get_addr(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->addr;
}

/**
 * \brief Get the errored RRID from the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \return Errored RRID
 */
static inline uint32_t iopmp_err_report_get_rrid(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->rrid;
}

/**
 * \brief Get the index pointing to the entry that catches the violation from
 * the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \return The index pointing to the entry that catches the violation
 */
static inline uint32_t iopmp_err_report_get_eid(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->eid;
}

/**
 * \brief Check if the type of violation is "not hit any rule" in the error
 * report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \retval 1 if the type of violation is "not hit any rule"
 * \retval 0 if the type of violation is not "not hit any rule"
 */
static inline bool iopmp_err_report_is_no_hit(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->etype == IOPMP_ERRINFO_ETYPE_NOT_HIT;
}

/**
 * \brief Check if the type of violation is "partial hit on a priority rule" in
 * the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \retval 1 if the type of violation is "partial hit on a priority rule"
 * \retval 0 if the type of violation is not "partial hit on a priority rule"
 */
static inline bool iopmp_err_report_is_part_hit(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->etype == IOPMP_ERRINFO_ETYPE_PART_HIT;
}

/**
 * \brief Get the transaction type from the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \return The transaction type
 */
static inline enum iopmp_errinfo_ttype
iopmp_err_report_get_ttype(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->ttype;
}

/**
 * \brief Check if the write access to trigger an IOPMP originated MSI has
 * failed in the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \retval 1 if the write access to trigger an IOPMP originated MSI has failed
 * \retval 0 if the write access to trigger an IOPMP originated MSI hasn't
 *         failed
 */
static inline bool iopmp_err_report_get_msi_werr(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->msi_werr;
}

/**
 * \brief Get the type of violation from the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \return The type of violation
 */
static inline enum iopmp_errinfo_etype
iopmp_err_report_get_etype(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->etype;
}

/**
 * \brief Get ERR_INFO.svc from the error report
 *
 * \param[in] err_report        The pointer to the error report
 *
 * \retval 1 if there is a subsequent violation caught in ERR_MFR
 * \retval 0 if there is no subsequent violation
 */
static inline bool iopmp_err_report_get_svc(IOPMP_ERR_REPORT_t *err_report)
{
    return err_report->svc;
}

/**
 * \brief Get the physical address[65:2] of protected memory region from the
 * IOPMP entry structure
 *
 * \param[in] entry             The pointer to the IOPMP entry structure
 *
 * \return The physical address[65:2] of protected memory region
 */
static inline uint64_t iopmp_entry_get_addr(IOPMP_Entry_t *entry)
{
    return entry->addr;
}

/**
 * \brief Get the permissions and attributes of protected memory region from the
 * IOPMP entry structure
 *
 * \param[in] entry             The pointer to the IOPMP entry structure
 *
 * \return The permissions and attributes of protected memory region
 */
static inline uint32_t iopmp_entry_get_cfg(IOPMP_Entry_t *entry)
{
    return entry->cfg;
}

/******************************************************************************/
/* API for IOPMP                                                              */
/******************************************************************************/
/**
 * \brief Initialize the IOPMP instance. Read the intial states and prepare the
 * IOPMP driver operations
 *
 * \param[in] iopmp             The IOPMP instance to be initialized
 * \param[in] addr              The base memory-mapped address of the IOPMP
 * \param[in] srcmd_fmt         The SRCMD_FMT of this IOPMP instance
 * \param[in] mdcfg_fmt         The MDCFG_FMT of this IOPMP instance
 * \param[in] impid             The implementation ID of this IOPMP instance
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if some features are not supported
 */
enum iopmp_error iopmp_init(IOPMP_t *iopmp, uintptr_t addr, uint8_t srcmd_fmt,
                            uint8_t mdcfg_fmt, uint32_t impid);

/**
 * \brief Get the vendor ID of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] vendor           Pointer to integer to store vendor ID
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p vendor is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support get the vendor
 *         ID of IOPMP
 */
enum iopmp_error iopmp_get_vendor_id(IOPMP_t *iopmp, uint32_t *vendor);

/**
 * \brief Get the specification version of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] specver          Pointer to integer to store specification
 *                              version
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p specver is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support get the
 *         specification version of the IOPMP
 */
enum iopmp_error iopmp_get_specver(IOPMP_t *iopmp, uint32_t *specver);

/**
 * \brief Get the implementation ID of the IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] impid            Pointer to integer to store implementation ID
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p impid is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support get the
 *         implementation ID of the IOPMP
 */
enum iopmp_error iopmp_get_impid(IOPMP_t *iopmp, uint32_t *impid);

/**
 * \brief Lock number of priority entry if the IOPMP HWCFG2.prio_ent_prog=1
 *
 * \param[in] iopmp             The IOPMP instance
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support non-priority
 *         entries
 */
enum iopmp_error iopmp_lock_prio_entry_num(IOPMP_t *iopmp);

/**
 * \brief Lock the RRID tagged to outgoing transactions if the IOPMP
 * HWCFG3.rrid_transl_prog=1
 *
 * \param[in] iopmp             The IOPMP instance
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support lock the RRID
 *         tagged to outgoing transactions
 */
enum iopmp_error iopmp_lock_rrid_transl(IOPMP_t *iopmp);

/**
 * \brief Enable the IOPMP checker
 *
 * \param[in] iopmp             The IOPMP instance to be set
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support enable the
 *         checker
 */
enum iopmp_error iopmp_set_enable(IOPMP_t *iopmp);

/**
 * \brief Set the number of entries matched with priority
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] num_entry     Input the number of entries to be matched with
 *                              priority. Output WARL value.
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support non-priority
 *         entries
 * \retval IOPMP_ERR_REG_IS_LOCKED if HWCFG2.prio_ent_prog is 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p num_entry is NULL
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p num_entry does not match
 *         the actual value. The actual value is output via \p num_entry
 */
enum iopmp_error iopmp_set_prio_entry_num(IOPMP_t *iopmp, uint16_t *num_entry);

/**
 * \brief Check if HWCFG3.rrid_transl is programmable
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] rrid_transl_prog Output true if HWCFG3.rrid_transl is
 *                              programmable. Otherwise output false.
 *
 * \retval IOPMP_OK if HWCFG3.rrid_transl_en is 1
 * \retval IOPMP_ERR_NOT_SUPPORTED if HWCFG3.rrid_transl_en is 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p rrid_transl_prog is NULL
 */
enum iopmp_error iopmp_get_rrid_transl_prog(IOPMP_t *iopmp,
                                            bool *rrid_transl_prog);

/**
 * \brief Get the RRID tagged to outgoing transactions
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] rrid_transl      Output the RRID tagged to outgoing transactions
 *
 * \retval IOPMP_OK if HWCFG3.rrid_transl_en is 1
 * \retval IOPMP_ERR_NOT_SUPPORTED if HWCFG3.rrid_transl_en is 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p rrid_transl is NULL
 */
enum iopmp_error iopmp_get_rrid_transl(IOPMP_t *iopmp, uint16_t *rrid_transl);

/**
 * \brief Set the RRID tagged to outgoing transactions
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] rrid_transl   Input the RRID tagged to outgoing transactions.
 *                              Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if HWCFG3.rrid_transl_en is 0
 * \retval IOPMP_ERR_REG_IS_LOCKED if HWCFG3.rrid_transl_prog is 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p rrid_transl is NULL
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p rrid_transl does not match
 *         the actual value. The actual value is output via \p rrid_transl
 */
enum iopmp_error iopmp_set_rrid_transl(IOPMP_t *iopmp, uint16_t *rrid_transl);

/**
 * \brief Stall the transactions related to given MD bitmap and poll the stall
 * status until stalling takes effect if necessary
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] mds           Input the MD bitmap to be stalled. Output WARL
 *                              value
 * \param[in] exempt            Stall transactions with exempt selected MDs
 * \param[in] polling           Set true to poll the stall status until stalling
 *                              takes effect
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value. The actual value is output via \p mds
 * \retval IOPMP_ERR_NOT_ALLOWED if MDSTALL has already been written and
 *         libiopmp expects user resumes the transactions first
 *
 * \note An MD that MDSTALL(H) can not select is rejected before the write, so
 *       the IOPMP is left untouched. iopmp_probe_stall_by_md() teaches
 *       libiopmp which MDs those are; until it runs, every implemented MD is
 *       assumed selectable and the read-back catches the rest
 */
enum iopmp_error iopmp_stall_transactions_by_mds(IOPMP_t *iopmp, uint64_t *mds,
                                                 bool exempt, bool polling);

/**
 * \brief Resume the stalled transactions previously stalled, and poll the
 * resume status until resuming takes effect if necessary
 *
 * \param[in] iopmp             The IOPMP instance to be resumed
 * \param[in] polling           Set true to poll the resume status until
 *                              resuming takes effect
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall or
 *         resuming of stall
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value
 * \retval IOPMP_ERR_NOT_ALLOWED if there was no transactions being stalled
 */
enum iopmp_error iopmp_resume_transactions(IOPMP_t *iopmp, bool polling);

/**
 * \brief Check if the requested stall transactions takes effect
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[in] polling           Set true to poll the stall status until stalling
 *                              takes effect
 * \param[out] stalled          Set to 1 if the stall has taken effect, or to 0
 *                              if it has not taken effect yet
 *
 * \retval IOPMP_OK if \p stalled has been written
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p stalled is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall
 * \retval IOPMP_ERR_NOT_EXIST if \p iopmp did not stall any transactions by
 *         iopmp_stall_transactions_by_mds()
 */
enum iopmp_error iopmp_transactions_are_stalled(IOPMP_t *iopmp, bool polling,
                                                bool *stalled);

/**
 * \brief Check if the requested resume transactions takes effect
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[in] polling           Set true to poll the resume status until
 *                              resuming takes effect
 * \param[out] resumed          Set to 1 if the resuming has taken effect, or to
 *                              0 if it has not taken effect yet
 *
 * \retval IOPMP_OK if \p resumed has been written
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p resumed is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall
 * \retval IOPMP_ERR_NOT_EXIST if \p iopmp did not resume any transactions by
 *         iopmp_resume_transactions()
 */
enum iopmp_error iopmp_transactions_are_resumed(IOPMP_t *iopmp, bool polling,
                                                bool *resumed);

/**
 * \brief Detect which MDs can be selected by MDSTALL and MDSTALLH
 *
 * An IOPMP need not make every implemented MD selectable, so this writes all
 * ones into MDSTALL.md and MDSTALLH.mdh, reads them back, and resumes.
 *
 * \param[in] iopmp             The IOPMP instance to be probed
 * \param[out] mds              Bitmap of the MDs that MDSTALL(H) can select
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if HWCFG2.stall_en is 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 * \retval IOPMP_ERR_NOT_ALLOWED if \p iopmp is already stalling transactions
 *
 * \note The probe stalls the transactions of the selected MDs for as long as
 *       it runs. Call it while that is acceptable, typically before
 *       HWCFG0.enable is set. If ERR_CFG.stall_violation_en is 1, the stalled
 *       transactions fault rather than wait.
 */
enum iopmp_error iopmp_probe_stall_by_md(IOPMP_t *iopmp, uint64_t *mds);

/**
 * \brief Select or deselect the transactions with specific RRIDs to stall
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] rrid          Input the RRID to be stalled. Output WARL value
 * \param[in] select            Set true select or false to deselect
 * \param[out] stat             The pointer to store enum iopmp_rridscp_stat
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p rrid is NULL or invalid
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall by RRID
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p rrid does not match the
 *         actual value. The actual value is output via \p rrid
 *
 * \note Although this function returns IOPMP_OK, the caller must check \p stat
 *       to determine the state of the operation.
 * \note After RRIDSCP is written, the action to stall desired transactions may
 *       not take effect immediately in some implementations. To determine
 *       whether the action takes effect, one can call
 *       iopmp_transactions_are_stalled().
 */
enum iopmp_error iopmp_stall_cherry_pick_rrid(IOPMP_t *iopmp, uint32_t *rrid,
                                              bool select,
                                              enum iopmp_rridscp_stat *stat);

/**
 * \brief Query the stall status of given RRID
 *
 * \param[in] iopmp             The IOPMP instance to be queried
 * \param[in,out] rrid          Input the RRID to be queried. Output WARL value
 * \param[out] stat             The pointer to store enum iopmp_rridscp_stat
 *
 * \retval Positive value for stall status of \p rrid
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p rrid or \p stat is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall by RRID or
 *         querying of stall
 *
 * \note Although this function returns IOPMP_OK, the caller must check \p stat
 *       to determine the state of the operation.
 * \note After RRIDSCP is written, the action to stall desired transactions may
 *       not take effect immediately in some implementations. To determine
 *       whether the action takes effect, one can call
 *       iopmp_transactions_are_stalled().
 */
enum iopmp_error iopmp_query_stall_stat_by_rrid(IOPMP_t *iopmp, uint32_t *rrid,
                                                enum iopmp_rridscp_stat *stat);

/**
 * \brief Get locked MDs and MDLCK.l
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[out] mds              Pointer to integer to store locked MD bitmap
 * \param[out] mdlck_lock       Pointer to integer to store MDLCK.l
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds or \p mdlck_lock is NULL
 */
enum iopmp_error iopmp_get_locked_md(IOPMP_t *iopmp, uint64_t *mds,
                                     bool *mdlck_lock);

/**
 * \brief Lock MDs
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] mds           Input the MD bitmap to be locked. Output WARL
 *                              value
 * \param[in] mdlck_lock        Set 1 to lock MDLCK and MDLCKH registers
 *
 * \retval IOPMP_OK if successes or both \p mds and \p mdlck_lock are 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p mds is NULL
 * \retval IOPMP_ERR_REG_IS_LOCKED if MDLCK and MDLCKH have already been locked
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p mds is out-of-bounds
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written value does not match the
 *         actual value. The actual values are output in \p mds
 *
 * \note If MDLCK.l has already been set to 1, this API always expects
 *       \p mdlck_lock be 1.
 */
enum iopmp_error iopmp_lock_md(IOPMP_t *iopmp, uint64_t *mds, bool mdlck_lock);

/**
 * \brief Lock MDCFG(0) ~ MDCFG(md_num - 1)
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] md_num        Input the number of MD to be locked. Output WARL
 *                              value
 * \param[in] lock              Set 1 to lock MDCFGLCK register
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p md_num is NULL
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if value of \p md_num is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if MDCFGLCK has already been locked
 * \retval IOPMP_ERR_NOT_ALLOWED if \p md_num is not monotonically increased
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement MDCFG table
 *         format 0
 */
enum iopmp_error iopmp_lock_mdcfg(IOPMP_t *iopmp, uint32_t *md_num, bool lock);

/**
 * \brief Check if MDCFGLCK was locked
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[out] locked           The pointer to an integer to store MDCFGLCK.l
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p locked is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement MDCFG table
 *         format 0
 */
enum iopmp_error iopmp_is_mdcfglck_locked(IOPMP_t *iopmp, bool *locked);

/**
 * \brief Get number of MDs whose MDCFG were locked by MDCFGLCK
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[out] md_num           The pointer to an integer to store MDCFGLCK.f
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p md_num is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement MDCFG table
 *         format 0
 */
enum iopmp_error iopmp_get_locked_mdcfg_num(IOPMP_t *iopmp, uint32_t *md_num);

/**
 * \brief Lock ENTRY_ADDR[0 ~ (entry_num-1)], ENTRY_ADDRH[0 ~ (entry_num-1)],
 * ENTRY_CFG[0 ~ (entry_num-1)], and ENTRY_USER_CFG[0 ~ (entry_num-1)]
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] entry_num     Input the number of entry to be locked. Output
 *                              WARL value
 * \param[in] lock              Set 1 to lock ENTRYLCK register
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p entry_num is NULL
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if value of \p entry_num is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if ENTRYLCK has already been locked
 * \retval IOPMP_ERR_NOT_ALLOWED if value of \p entry_num is not monotonically
 *         increased
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p entry_num does not match
 *         the actual value. The actual value is output via \p entry_num
 */
enum iopmp_error iopmp_lock_entries(IOPMP_t *iopmp, uint32_t *entry_num,
                                    bool lock);
/**
 * \brief Lock fields of ERR_CFG register
 *
 * \param[in] iopmp             The IOPMP instance to be set
 *
 * \return IOPMP_OK
 */
enum iopmp_error iopmp_lock_err_cfg(IOPMP_t *iopmp);

/**
 * \brief Enable/Disable global interrupt
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] enable            1(enable) or 0(disable)
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_REG_IS_LOCKED if ERR_CFG register is locked
 */
enum iopmp_error iopmp_set_global_intr(IOPMP_t *iopmp, bool enable);

/**
 * \brief Suppress/express global error responses
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] suppress      Input 1(suppress) or 0(express). Output WARL
 *                              value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p suppress is NULL
 * \retval IOPMP_ERR_REG_IS_LOCKED if ERR_CFG register is locked
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p suppress does not match
 *         the actual value. The actual value is output via \p suppress
 *
 * \note ERR_CFG.rs is WARL and has no capability bit in HWCFG, so an
 *       implementation that hardwires it is reported this way rather than as
 *       IOPMP_ERR_NOT_SUPPORTED: \p suppress comes back holding the value the
 *       hardware is fixed at
 */
enum iopmp_error iopmp_set_global_err_resp(IOPMP_t *iopmp, bool *suppress);

/**
 * \brief Enable/disable IOPMP trigger message-signaled interrupts on errors
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] enable        True to enable or false to disable. Output WARL
 *                              value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support MSI
 * \retval IOPMP_ERR_REG_IS_LOCKED if ERR_CFG.l=1
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written value do not match the actual
 *         value
 */
enum iopmp_error iopmp_set_msi_sel(IOPMP_t *iopmp, bool *enable);

/**
 * \brief Get the address to trigger message-signaled interrupts
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] msiaddr64        Pointer to 64-bit integer to store MSI address
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support MSI
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p msiaddr64 is NULL
 *
 * \note If HWCFG0.addrh_en=0, the \p msiaddr64 contains bits 33 to 2 of the MSI
 *       address
 * \note If HWCFG0.addrh_en=1, the \p msiaddr64 contains bits 63 to 0 of the MSI
 *       address
 */
enum iopmp_error iopmp_get_msi_addr(IOPMP_t *iopmp, uint64_t *msiaddr64);

/**
 * \brief Get the data to trigger message-signaled interrupts
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] msidata          Pointer to 16-bit integer to store MSI data
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support MSI
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p msidata is NULL
 */
enum iopmp_error iopmp_get_msi_data(IOPMP_t *iopmp, uint16_t *msidata);

/**
 * \brief Set address and data of message-signaled interrupts
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] msiaddr64     Input 64-bit MSI address. Output WARL value
 * \param[in,out] msidata       Input 11-bit MSI data. Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p msiaddr64 or \p msidata is
 *         NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support MSI, or
 *         \p msiaddr64 has high-32 bit but IOPMP does not support addrh_en,
 *         or \p msidata is out-of-bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if ERR_CFG.l=1
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written values do not match the
 *         actual values. The actual values are output in \p msiaddr64 and
 *         \p msidata
 */
enum iopmp_error iopmp_set_msi_info(IOPMP_t *iopmp, uint64_t *msiaddr64,
                                    uint16_t *msidata);

/**
 * \brief Check if there is an MSI write error and clear the flag
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[out] msi_werr         The pointer to flag
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p msi_werr is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support MSI
 */
enum iopmp_error iopmp_get_and_clear_msi_werr(IOPMP_t *iopmp, bool *msi_werr);

/**
 * \brief Enable or disable the IOPMP faults stalled transactions
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] enable        Input 1 to enable, 0 to disable. Output WARL
 *                              value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p enable is NULL
 * \retval IOPMP_ERR_ILLEGAL_VALUE if \p enable can't be written into \p iopmp
 */
enum iopmp_error iopmp_set_stall_violation_en(IOPMP_t *iopmp, bool *enable);

/**
 * \brief Invalidate the error record by clearing ERR_INFO.v bit
 *
 * \param[in] iopmp             The IOPMP instance to be invalidated
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support clear error
 *         interrupt pending bit
 */
enum iopmp_error iopmp_invalidate_error(IOPMP_t *iopmp);

/**
 * \brief Capture an IOPMP error information
 *
 * \param[in] iopmp             The IOPMP instance to be captured
 * \param[out] err_report       The pointer to IOPMP error report structure
 * \param[in] invalidate        Flag to clear V bit after reading error report
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p err_report is NULL
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support capture error
 * \retval IOPMP_ERR_NOT_EXIST if there is no an pending error
 */
enum iopmp_error iopmp_capture_error(IOPMP_t *iopmp,
                                     IOPMP_ERR_REPORT_t *err_report,
                                     bool invalidate);

/**
 * \brief Get subsequent violation window, if IOPMP supports MFR extension
 *
 * \param[in] iopmp             The IOPMP instance to be allocated
 * \param[in,out] svi           When calling, user can specify start index of
 *                              search windows. When this function returns with
 *                              IOPMP_OK, svi indicates the index of window
 *                              which has subsequent violation
 * \param[out] svw              When this function returns with IOPMP_OK, svw
 *                              indicates the content of window which has
 *                              subsequent violation
 *
 * \retval IOPMP_OK if at least one subsequent violation is found
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support MFR extension
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p svi or \p svw is NULL
 * \retval IOPMP_ERR_NOT_EXIST if there is no any subsequent violation
 */
enum iopmp_error iopmp_mfr_get_sv_window(IOPMP_t *iopmp, uint16_t *svi,
                                         uint16_t *svw);

/**
 * \brief Lock SRCMD_EN(rrid), SRCMD_ENH(rrid), SRCMD_R(rrid), SRCMD_RH(rrid),
 * SRCMD_W(rrid), and SRCMD_WH(rrid) if any
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] rrid              The RRID to be locked
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp SRCMD_FMT!=0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 *
 * \note This operation is only supported by SRCMD_FMT=0
 */
enum iopmp_error iopmp_lock_srcmd_table_fmt_0(IOPMP_t *iopmp, uint32_t rrid);

/**
 * \brief Check if SRCMD_EN(rrid), SRCMD_ENH(rrid), SRCMD_R(rrid),
 * SRCMD_RH(rrid), SRCMD_W(rrid), and SRCMD_WH(rrid) if any, have been locked
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] rrid              The RRID to be got
 * \param[out] locked           The pointer to an integer to store SRCMD_EN.l
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp SRCMD_FMT!=0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p locked is NULL
 *
 * \note This operation is only supported by SRCMD_FMT=0
 */
enum iopmp_error iopmp_is_srcmd_table_fmt_0_locked(IOPMP_t *iopmp,
                                                   uint32_t rrid,
                                                   bool *locked);

/**
 * \brief Lock SRCMD_PERM(mdidx) and SRCMD_PERMH(mdidx)
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] mdidx             The index of MD to be locked
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp SRCMD_FMT!=2
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if MDLCK has been locked
 *
 * \note This operation is only supported by SRCMD_FMT=2
 */
enum iopmp_error iopmp_lock_srcmd_table_fmt_2(IOPMP_t *iopmp, uint32_t mdidx);

/**
 * \brief Check if SRCMD_PERM(mdidx) and SRCMD_PERMH(mdidx) have been locked
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] mdidx             The index of MD to be got
 * \param[out] locked           The pointer to an integer to store SRCMD_EN.l
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp SRCMD_FMT!=2
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p locked is NULL
 *
 * \note This operation is only supported by SRCMD_FMT=2
 */
enum iopmp_error iopmp_is_srcmd_table_fmt_2_locked(IOPMP_t *iopmp,
                                                   uint32_t mdidx,
                                                   bool *locked);

/**
 * \brief Get the associated MD bitmap and lock bit of given RRID
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] rrid              The RRID to be got
 * \param[out] mds              The pointer to an integer to store SRCMD_EN.md
 * \param[out] lock             The pointer to an integer to store SRCMD_EN.l
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p lock or \p md is NULL
 */
enum iopmp_error iopmp_get_rrid_md_association(IOPMP_t *iopmp, uint32_t rrid,
                                               uint64_t *mds, bool *lock);

/**
 * \brief Associate/Disassociate the given RRID with the given MD bitmap
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to be associated with \p rrid
 * \param[in] mds_clr           The desired MDs to be disassociated with \p rrid
 * \param[out] mds              The pointer to an integer to store WARL value of
 *                              SRCMD_EN.md after setting
 * \param[in] lock              Set 1 to lock SRCMD_EN[rrid]
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp SRCMD_FMT!=0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mds_set or \p mds_clr
 *         is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 * \retval IOPMP_ERR_REG_IS_LOCKED if SRCMD_EN[rrid] has been locked or some or
 *         MDs are locked by MDLCK
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value
 */
enum iopmp_error iopmp_set_rrid_md_association(IOPMP_t *iopmp, uint32_t rrid,
                                               uint64_t mds_set,
                                               uint64_t mds_clr,
                                               uint64_t *mds,
                                               bool lock);

/**
 * \brief (srcmd_fmt=2 only) Set single RRID's r/w permissions to MD
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] rrid              The RRID to be set
 * \param[in] mdidx             The desired MD to be given permission
 * \param[in,out] r             Set true to give the read permission to \p rrid
 *                              Output WARL value
 * \param[in,out] w             Set true to give the write permission to \p rrid
 *                              Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SRCMD table
 *         format 2
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mdidx is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if MD( \p mdidx ) has been locked by MDLCK
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p r or \p w do not match the
 *         actual values
 */
enum iopmp_error iopmp_set_md_permission(IOPMP_t *iopmp, uint32_t rrid,
                                         uint32_t mdidx, bool *r, bool *w);

/**
 * \brief (srcmd_fmt=2 only) Set multiple RRID's r/w permissions to MD
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] mdidx             The desired MD to be given permission
 * \param[in] cfg               The configuration structure for SRCMD format 2
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SRCMD table
 *         format 2
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p cfg is NULL
 * \retval IOPMP_ERR_REG_IS_LOCKED if MD( \p mdidx ) has been locked by MDLCK
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written permissions in \p cfg do not
 *         match the actual values
 */
enum iopmp_error iopmp_set_md_permission_multi(IOPMP_t *iopmp, uint32_t mdidx,
                                               IOPMP_SRCMD_PERM_CFG_t *cfg);

/**
 * \brief Helper function used to set struct iopmp_srcmd_perm_config
 *
 * \param[in] cfg               Pointer to struct iopmp_srcmd_perm_config
 * \param[in] rrid              Desired RRID to be set
 * \param[in] r                 Set true to give RRID read permission; false to
 *                              clear read permission
 * \param[in] w                 Set true to give RRID write permission; false to
 *                              clear write permission
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p cfg is NULL
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 */
enum iopmp_error iopmp_set_srcmd_perm_cfg(IOPMP_SRCMD_PERM_CFG_t *cfg,
                                          uint32_t rrid, bool r, bool w);

/**
 * \brief Helper function used to set struct iopmp_srcmd_perm_config. This is
 * similar to iopmp_set_srcmd_perm_cfg() but there are no checks on cfg and RRID
 *
 * \param[in] cfg               Pointer to struct iopmp_srcmd_perm_config
 * \param[in] rrid              Desired RRID to be set
 * \param[in] r                 Set true to give RRID read permission; false to
 *                              clear read permission
 * \param[in] w                 Set true to give RRID write permission; false to
 *                              clear write permission
 */
void iopmp_set_srcmd_perm_cfg_nocheck(IOPMP_SRCMD_PERM_CFG_t *cfg,
                                      uint32_t rrid, bool r, bool w);

/**
 * \brief (SPS only) Set RRID's read permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to set permission to \p rrid
 * \param[in] mds_clr           The desired MDs to clear permission to \p rrid
 * \param[out] mds              The pointer to an integer to store WARL value of
 *                              SRCMD_R.md after setting
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mds is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if register has been locked by SRCMD_EN.l
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 */
enum iopmp_error iopmp_sps_set_rrid_md_read(IOPMP_t *iopmp, uint32_t rrid,
                                            uint64_t mds_set,
                                            uint64_t mds_clr,
                                            uint64_t *mds);

/**
 * \brief (SPS only) Get RRID's read permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be checked
 * \param[out] mds              Pointer to variable to output permission
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 */
enum iopmp_error iopmp_sps_get_rrid_md_read(IOPMP_t *iopmp, uint32_t rrid,
                                            uint64_t *mds);

/**
 * \brief (SPS only) Set RRID's write permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to set permission to \p rrid
 * \param[in] mds_clr           The desired MDs to clear permission to \p rrid
 * \param[out] mds              The pointer to an integer to store WARL value of
 *                              SRCMD_W.md after setting
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mds is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if register has been locked by SRCMD_EN.l
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 */
enum iopmp_error iopmp_sps_set_rrid_md_write(IOPMP_t *iopmp, uint32_t rrid,
                                             uint64_t mds_set,
                                             uint64_t mds_clr,
                                             uint64_t *mds);

/**
 * \brief (SPS only) Get RRID's write permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be checked
 * \param[out] mds              Pointer to variable to output permission
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 */
enum iopmp_error iopmp_sps_get_rrid_md_write(IOPMP_t *iopmp, uint32_t rrid,
                                             uint64_t *mds);

/**
 * \brief (SPS only) Set RRID's instruction fetch permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to set permission to \p rrid
 * \param[in] mds_clr           The desired MDs to clear permission to \p rrid
 * \param[out] mds              The pointer to an integer to store WARL value of
 *                              SRCMD_X.md after setting
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mds is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if register has been locked by SRCMD_EN.l
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 */
enum iopmp_error iopmp_sps_set_rrid_md_insn_fetch(IOPMP_t *iopmp, uint32_t rrid,
                                                  uint64_t mds_set,
                                                  uint64_t mds_clr,
                                                  uint64_t *mds);

/**
 * \brief (SPS only) Get RRID's instruction fetch permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be checked
 * \param[out] mds              Pointer to variable to output permission
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 */
enum iopmp_error iopmp_sps_get_rrid_md_insn_fetch(IOPMP_t *iopmp, uint32_t rrid,
                                                  uint64_t *mds);

/**
 * \brief (SPS only) Set RRID's read/write/instruction fetch permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set_r         The desired MDs to set R permission to \p rrid
 * \param[in] mds_clr_r         The desired MDs to clear R permission to \p rrid
 * \param[in] mds_set_w         The desired MDs to set W permission to \p rrid
 * \param[in] mds_clr_w         The desired MDs to clear W permission to \p rrid
 * \param[in] mds_set_x         The desired MDs to set X permission to \p rrid
 * \param[in] mds_clr_x         The desired MDs to clear X permission to \p rrid
 * \param[out] mds_r            The pointer to an integer to store WARL value of
 *                              SRCMD_R.md after setting
 * \param[out] mds_w            The pointer to an integer to store WARL value of
 *                              SRCMD_W.md after setting
 * \param[out] mds_x            The pointer to an integer to store WARL value of
 *                              SRCMD_X.md after setting
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mds_r or \p mds_w is
 *         out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if register has been locked by SRCMD_EN.l
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds_r or \p mds_w does
 *         not match the actual values
 */
enum iopmp_error iopmp_sps_set_rrid_md_rwx(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t mds_set_r,
                                           uint64_t mds_clr_r,
                                           uint64_t mds_set_w,
                                           uint64_t mds_clr_w,
                                           uint64_t mds_set_x,
                                           uint64_t mds_clr_x,
                                           uint64_t *mds_r,
                                           uint64_t *mds_w,
                                           uint64_t *mds_x);

/**
 * \brief (SPS only) Get RRID's read/write/instruction fetch permission to
 * multiple MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[out] mds_r            Pointer to variable to output read permission
 * \param[out] mds_w            Pointer to variable to output write permission
 * \param[out] mds_x            Pointer to variable to output instruction fetch
 *                              permission
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not implement SPS extension
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds_r or \p mds_w is NULL
 */
enum iopmp_error iopmp_sps_get_rrid_md_rwx(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds_r, uint64_t *mds_w,
                                           uint64_t *mds_x);

/**
 * \brief Get start index and number of the entries belong to MD[mdidx]
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] mdidx             The index of target MD
 * \param[out] entry_idx_start  The pointer to an integer to return start index
 * \param[out] num_entry        The pointer to an integer to return number of
 *                              entry
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_idx_start or
 *         \p num_entry is NULL
 */
enum iopmp_error iopmp_get_md_entry_association(IOPMP_t *iopmp, uint32_t mdidx,
                                                uint32_t *entry_idx_start,
                                                uint32_t *num_entry);

/**
 * \brief Associate given entries with given multiple MDs
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] mdidx_start       The start index of target MDs
 * \param[in,out] num_entries   Input the number of entries to be associated.
 *                              Output actual number of entries be associated.
 * \param[in] md_num            The number of target MDs to be set
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_ALLOWED if \p iopmp MDCFG format is not 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p num_entries is NULL
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx_start or \p md_num is out
 *         of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if MDCFG of given \p mdidx_start has been
 *         locked by MDCFGLCK.f
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p num_entries does not
 *         match the actual value. The actual value is output in \p num_entries
 *
 * \note This function must be called only when IOPMP MDCFG format is 0
 */
enum iopmp_error iopmp_set_md_entry_association_multi(IOPMP_t *iopmp,
                                                      uint32_t mdidx_start,
                                                      uint32_t *num_entries,
                                                      uint32_t md_num);

/**
 * \brief Associate given entries with given MD(mdidx)
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] mdidx             The index of target MD
 * \param[in,out] num_entry     Input the number of entries to be associated.
 *                              Output actual number of entries be associated.
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_ALLOWED if \p iopmp MDCFG format is not 0
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p num_entry is NULL
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx or \p num_entry is out of
 *         bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if MDCFG of given \p mdidx has been locked
 *         by MDCFGLCK.f
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p num_entry does not match
 *         the actual value. The actual value is output in \p num_entry
 *
 * \note This function must be called only when IOPMP MDCFG format is 0
 */
static inline
enum iopmp_error iopmp_set_md_entry_association(IOPMP_t *iopmp, uint32_t mdidx,
                                                uint32_t *num_entry)
{
    return iopmp_set_md_entry_association_multi(iopmp, mdidx, num_entry, 1);
}

/**
 * \brief Get value of HWCFG3.md_entry_num if IOPMP model is xxx-K
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[out] md_entry_num     The pointer to an integer to return value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p md_entry_num is NULL
 */
enum iopmp_error iopmp_get_md_entry_num(IOPMP_t *iopmp, uint32_t *md_entry_num);

/**
 * \brief Program value of HWCFG3.md_entry_num
 *
 * \param[in] iopmp             The IOPMP instance to be programmed
 * \param[in,out] md_entry_num  Input the drsired value of md_entry_num. Output
 *                              WARL value.
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_ALLOWED if IOPMP MDCFG format is not 2
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p md_entry_num is NULL
 * \retval IOPMP_ERR_REG_IS_LOCKED if IOPMP has been enabled
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p md_entry_num is out-of-bounds
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p md_entry_num does not
 *         match the actual value. The actual value is output in \p md_entry_num
 *
 * \note This function must be called only when IOPMP's MDCFG_FMT=2
 */
enum iopmp_error iopmp_set_md_entry_num(IOPMP_t *iopmp, uint32_t *md_entry_num);

/**
 * \brief Encode IOPMP entry from given memory region and flags
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[out] entries          The array of entry to be output
 * \param[in] num_entry         Number of entries in \p entries
 * \param[in] addr              Address of the memory region
 * \param[in] size              Size of the memory region
 * \param[in] flags             Flags of the entry for this memory region
 * \param[in] private_data      Private data that can be used in specific model
 *
 * \retval 1 if successes and the memory region is encoded as NAPOT entry or as
 *         TOR entry 0
 * \retval 2 if successes and the memory region is encoded as two TOR entries
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entries is NULL or
 *         \p num_entry is 0 or \p size is 0 or \p addr or \p size is not
 *         aligned to IOPMP granularity
 * \retval IOPMP_ERR_NOT_SUPPORTED if memory region should be encoded as TOR
 *         entry, but \p iopmp does not support TOR entry; or \p flags contain
 *         unsupported hardware features such as per-entry interrupt/error
 *         suppression
 * \retval IOPMP_ERR_NOT_ALLOWED if memory region should be encoded as TOR, but
 *         only one entry in given \p entries ; or if given \p flags include
 *         IOPMP_ENTRY_FIRST_TOR but the \p addr is not 0
 *
 * \note Caller is responsible for providing the permission bits and per-entry
 *       interrupt/error suppression bits via \p flags parameter.
 * \note The address-matching mode of the entry will be determined by this API.
 *       Caller doesn't need to provide the address matching mode via \p flags
 *       parameter, such as IOPMP_ENTRY_A_TOR or IOPMP_ENTRY_A_NAPOT. Caller can
 *       check the address-matching mode by entry->a field after returning from
 *       this API.
 * \note If caller wants to encode the the entry as "OFF" address-matching mode,
 *       caller must provide IOPMP_ENTRY_FORCE_OFF via \p flags parameter.
 * \note In general, a TOR region will be encoded into two entries. However, the
 *       specification permits the PMP entry 0 having TOR address-matching mode.
 *       In this case, caller must provide IOPMP_ENTRY_FIRST_TOR via \p flags
 *       parameter. The API returns 1 whereas the entry is encoded as TOR
 *       address-matching mode.
 * \note If caller wants to encode TOR entries on an NAPOT-able region, caller
 *       must provide IOPMP_ENTRY_FORCE_TOR via \p flags parameter.
 * \note If caller wants to specify the entry is whether a priority entry or a
 *       non-priority entry, caller can provide IOPMP_ENTRY_PRIO or
 *       IOPMP_ENTRY_NON_PRIO via \p flags parameter. The iopmp_set_entries()
 *       and similar APIs will check the priority on the entry. If the caller
 *       provides neither of them, the iopmp_set_entries() and similar APIs
 *       won't check the priority on the entry.
 * \note Currently, the \p private_data is used in a specific model with SRCMD
 *       format 2 and MDCFG format 1 and HWCFG3.md_entry_num=0 configurations.
 *       In this case, the \p private_data encodes {SRCMD_PERM(H) | SRCMD_PERM}
 *       for the entry associated with a single MD.
 */
enum iopmp_error iopmp_encode_entry(IOPMP_t *iopmp, struct iopmp_entry *entries,
                                    uint32_t num_entry, uint64_t addr,
                                    uint64_t size,
                                    enum iopmp_entry_flags flags,
                                    uint64_t private_data);

/**
 * \brief Set IOPMP entries together with the MD permissions that go with them
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] entry_array       The pointer to the entry array
 * \param[in] idx_start         The global index of the first entry
 * \param[in] num_entry         The number of entries to be set
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp is not SRCMD_FMT=2 &
 *         MDCFG_FMT=1 with HWCFG3.md_entry_num=0 (K=1)
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p entry_array is NULL or
 *         \p num_entry is zero
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if the range exceeds the implemented
 *         entries, or reaches past the MD each of them belongs to
 * \retval IOPMP_ERR_INVALID_PRIORITY if an entry is on the wrong side of
 *         HWCFG2.prio_entry
 * \retval IOPMP_ERR_REG_IS_LOCKED if an entry has been locked by ENTRYLCK.f,
 *         or its MD by MDLCK
 * \retval IOPMP_ERR_ILLEGAL_VALUE if a written SRCMD_PERM(H) does not match
 *         the actual value
 *
 * \note At K=1 entry i is the only entry of MD i, so the permissions of that
 *       MD can be written with the entry. entry->private_data holds
 *       SRCMD_PERM(H); iopmp_set_entries() writes the entry alone and leaves
 *       SRCMD_PERM(H) to iopmp_set_md_permission()
 * \note Nothing is written unless the whole range passes every check both
 *       halves make, so a refused call leaves SRCMD_PERM(H) untouched. The
 *       permissions go through the same path as
 *       iopmp_set_md_permission_multi(), so a driver overriding that reaches
 *       them here too
 */
enum iopmp_error iopmp_set_entries_with_md_permission(
    IOPMP_t *iopmp, const struct iopmp_entry *entry_array,
    uint32_t idx_start, uint32_t num_entry);

/**
 * \brief Set the entries belong to given MD to IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] mdidx             The index of target MD
 * \param[in] entry_array       The array of entries
 * \param[in] idx_start         The local index of entry in target MD
 * \param[in] num_entry         The number of entries to be set
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx, \p idx_start or
 *         \p num_entry is out of bounds
 * \retval IOPMP_ERR_INVALID_PRIORITY if priority of entry is invalid
 * \retval IOPMP_ERR_REG_IS_LOCKED if entries from \p idx_start have been locked
 *         by ENTRYLCK.f
 */
enum iopmp_error iopmp_set_entries_to_md(IOPMP_t *iopmp, uint32_t mdidx,
                                         const struct iopmp_entry *entry_array,
                                         uint32_t idx_start,
                                         uint32_t num_entry);

/**
 * \brief Set single entry belong to given MD to IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] mdidx             The index of target MD
 * \param[in] entry             The pointer to the entry
 * \param[in] idx               The local index of entry in target MD
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx, \p idx_start or
 *         \p num_entry is out of bounds
 * \retval IOPMP_ERR_INVALID_PRIORITY if priority of entry is invalid
 * \retval IOPMP_ERR_REG_IS_LOCKED if entries from \p idx_start have been locked
 *         by ENTRYLCK.f
 */
static inline
enum iopmp_error iopmp_set_entry_to_md(IOPMP_t *iopmp, uint32_t mdidx,
                                       const struct iopmp_entry *entry,
                                       uint32_t idx)
{
    return iopmp_set_entries_to_md(iopmp, mdidx, entry, idx, 1);
}

/**
 * \brief Get the entries belong to given MD from IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be read
 * \param[in] mdidx             The index of target MD
 * \param[out] entry_array      The array of entries
 * \param[in] idx_start         The local start index of entries in target MD
 * \param[in] num_entry         The number of entries to be read
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx, \p idx_start or
 *         \p num_entry is out of bounds
 */
enum iopmp_error iopmp_get_entries_from_md(IOPMP_t *iopmp, uint32_t mdidx,
                                           struct iopmp_entry *entry_array,
                                           uint32_t idx_start,
                                           uint32_t num_entry);

/**
 * \brief Get single entry belong to given MD from IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be read
 * \param[in] mdidx             The index of target MD
 * \param[out] entry            The pointer to the entry
 * \param[in] idx               The local start index of entries in target MD
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx, \p idx_start or
 *         \p num_entry is out of bounds
 */
static inline
enum iopmp_error iopmp_get_entry_from_md(IOPMP_t *iopmp, uint32_t mdidx,
                                         struct iopmp_entry *entry,
                                         uint32_t idx)
{
    return iopmp_get_entries_from_md(iopmp, mdidx, entry, idx, 1);
}

/**
 * \brief Get the global entries from IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be read
 * \param[out] entry_array      The array of entries
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be read
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx_start or \p num_entry is out
 *         of bounds
 */
enum iopmp_error iopmp_get_entries(IOPMP_t *iopmp,
                                   struct iopmp_entry *entry_array,
                                   uint32_t idx_start, uint32_t num_entry);

/**
 * \brief Get single global entry from IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be read
 * \param[out] entry            The pointer to the entry
 * \param[in] idx               The global start index of target entries
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx_start or \p num_entry is out
 *         of bounds
 */
static inline
enum iopmp_error iopmp_get_entry(IOPMP_t *iopmp, struct iopmp_entry *entry,
                                 uint32_t idx)
{
    return iopmp_get_entries(iopmp, entry, idx, 1);
}

/**
 * \brief Set the global entries into IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] entry_array       The array of entries
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be written
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx_start or \p num_entry is out
 *         of bounds
 * \retval IOPMP_ERR_INVALID_PRIORITY if priority of entry is invalid
 * \retval IOPMP_ERR_REG_IS_LOCKED if entries from \p idx_start have been locked
 *         by ENTRYLCK.f
 */
enum iopmp_error iopmp_set_entries(IOPMP_t *iopmp,
                                   const struct iopmp_entry *entry_array,
                                   uint32_t idx_start, uint32_t num_entry);

/**
 * \brief Set single global entry into IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] entry             The pointer to the entry
 * \param[in] idx               The global start index of target entries
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p entry_array is NULL or
 *         \p num_entry is 0
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx_start or \p num_entry is out
 *         of bounds
 * \retval IOPMP_ERR_INVALID_PRIORITY if priority of entry is invalid
 * \retval IOPMP_ERR_REG_IS_LOCKED if entries from \p idx_start have been locked
 *         by ENTRYLCK.f
 */
static inline
enum iopmp_error iopmp_set_entry(IOPMP_t *iopmp,
                                 const struct iopmp_entry *entry,
                                 uint32_t idx)
{
    return iopmp_set_entries(iopmp, entry, idx, 1);
}

/**
 * \brief Clear IOPMP entries in MD
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] mdidx             The index of target MD
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p mdidx is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if entries in MD \p mdidx have been locked by
 *         ENTRYLCK.f
 */
enum iopmp_error iopmp_clear_entries_in_md(IOPMP_t *iopmp, uint32_t mdidx);

/**
 * \brief Clear IOPMP entries
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be cleared
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx_start or \p num_entry is out
 *         of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if some of entries have been locked by
 *         ENTRYLCK.f
 */
enum iopmp_error iopmp_clear_entries(IOPMP_t *iopmp, uint32_t idx_start,
                                     uint32_t num_entry);

/**
 * \brief Clear single global entry
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] idx               The global index of target entry
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx is out of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if some of entries have been locked by
 *         ENTRYLCK.f
 */
static inline enum iopmp_error iopmp_clear_entry(IOPMP_t *iopmp, uint32_t idx)
{
    return iopmp_clear_entries(iopmp, idx, 1);
}

/**
 * \brief Get the MD bitmap that given index range of IOPMP entries belong to
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be checked
 * \param[out] mds              Pointer to integer to store MD bitmap
 *
 * \retval IOPMP_OK on success
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p idx_start or \p num_entry is out
 *         of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 */
enum iopmp_error iopmp_entries_get_belong_md(IOPMP_t *iopmp, uint32_t idx_start,
                                             uint32_t num_entry, uint64_t *mds);

#endif
