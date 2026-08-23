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

#ifndef __IOPMP_DRV_COMMON_H__
#define __IOPMP_DRV_COMMON_H__

#include "libiopmp.h"
#include "libiopmp_def.h"

/* Generate 32-bit mask[h:l] */
#define GENMASK_32(h, l)                                            \
    (((~(uint32_t)0U) - ((uint32_t)1U << (l)) + 1U) &               \
     (~(uint32_t)0U >> (31U - (h))))
/* Generate 64-bit mask[h:l] */
#define GENMASK_64(h, l)                                            \
    (((~(uint64_t)0U) - ((uint64_t)1U << (l)) + 1U) &               \
     (~(uint64_t)0U >> (63U - (h))))

/*
 * Extract a specific field in given register.
 * This macros requires user defines "field"_MASK and "field"_SHIFT macros first
 *
 * For example:
 * EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_MDCFG_FMT) is expanded to be
 * (((hwcfg0) & IOPMP_HWCFG0_MDCFG_FMT_MASK) >> IOPMP_HWCFG0_MDCFG_FMT_SHIFT)
 */
#define EXTRACT_FIELD(reg, field)   \
        (((reg) & field ## _MASK) >> field ## _SHIFT)

/*
 * Make a specific field in given register.
 * This macros requires user defines "field"_MASK and "field"_SHIFT macros first
 *
 * For example:
 * MAKE_FIELD_32(top, IOPMP_MDCFG_T) is expanded to be
 * (((uint32_t)(top) << IOPMP_MDCFG_T_SHIFT) & (uint32_t)(IOPMP_MDCFG_T_MASK))
 */
#define MAKE_FIELD_32(val, field)   \
        (((uint32_t)(val) << field ## _SHIFT) & (uint32_t)(field ## _MASK))
#define MAKE_FIELD_64(val, field)   \
        (((uint64_t)(val) << field ## _SHIFT) & (uint64_t)(field ## _MASK))

#ifdef __riscv_zbb
#include <riscv_bitmanip.h>

#if __riscv_xlen == 32
/**
 * \brief Count trailing zeros in 64-bit value for RV32 using Zbb instruction
 *
 * \param[in] val           The value to search
 *
 * \return 0 ~ 64
 */
static inline unsigned iopmp_ctz_64_xlen_32(uint64_t val)
{
    unsigned num;
    uint32_t lo = val & UINT32_MAX;
    uint32_t hi = val >> 32;

    num = __riscv_ctz_32(lo);
    if (lo == 0)
        num += __riscv_ctz_32(hi);

    return num;
}

#define iopmp_ctzll(VAL)        iopmp_ctz_64_xlen_32(VAL)
#elif __riscv_xlen == 64
#define iopmp_ctzll(VAL)        __riscv_ctz_64(VAL)
#endif
#else   /* __riscv_zbb is not defined */
#define iopmp_ctzll(VAL)        __builtin_ctzll(VAL)
#endif

#ifdef ENABLE_IO_WEAK_FUNCTIONS
/* Define IO functions as weak so that application code can override them */
#define __IOPMP_IO_FUNC_ATTR    __attribute__((weak))
#else
#define __IOPMP_IO_FUNC_ATTR    static inline
#endif

/* GCOVR_EXCL_START */
__IOPMP_IO_FUNC_ATTR uint32_t io_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

__IOPMP_IO_FUNC_ATTR void io_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}
/* GCOVR_EXCL_STOP */

/**
 * \brief Detect writable bits of ENTRY_ADDR(H) and IOPMP granularity
 *
 * \param[in] iopmp             The IOPMP instance to be detected
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_AVAILABLE if no unlocked and unused entry is available
 *         to probe, so that neither value can be determined
 */
enum iopmp_error detect_entry_addr_bits(IOPMP_t *iopmp);

/**
 * \brief Assemble two 32-bit unsigned integers to 64-bit unsigned integer
 *
 * \param[in] reg0              The 32-bit unsigned integer to be high bits
 * \param[in] reg1              The 32-bit unsigned integer to be low bits
 *
 * \return (reg0 << 32 | reg1)
 */
static inline uint64_t reg_pair_to_64(uint32_t reg0, uint32_t reg1)
{
    return (uint64_t)reg0 << 32 | reg1;
}

/**
 * \brief IOPMP driver information for initialization
 *
 * \note The libiopmp iterates all these structures and finds matched one to
 *       initialize your IOPMP instance with corresponding driver
 */
struct iopmp_driver {
    /* The callback function to start initialization */
    enum iopmp_error (*init)(IOPMP_t *iopmp, uintptr_t addr);
    /* The implementation ID to be matched */
    uint32_t impid;
    /* The SRCMD_FMT to be matched */
    uint8_t srcmd_fmt;
    /* The MDCFG_FMT to be matched */
    uint8_t mdcfg_fmt;
};

/**
 * \brief IOPMP driver initialization for standard IOPMP models
 *
 * \param[in] iopmp             The IOPMP instance to be initialized
 * \param[in] addr              The base memory-mapped address of IOPMP
 * \param[in] srcmd_fmt         The SRCMD_FMT of this IOPMP instance
 * \param[in] mdcfg_fmt         The MDCFG_FMT of this IOPMP instance
 * \param[in] ops_override      The operations this hardware does differently,
 *                              or NULL to use libiopmp's own throughout
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_SUPPORTED if some features are not supported
 *
 * \note If your IOPMP follows standard RISC-V IOPMP specification, you can
 *       simply call this function. Otherwise you need to implement your own
 *       initialization function and assign the function to struct iopmp_driver.
 */
enum iopmp_error
iopmp_drv_init_common(IOPMP_t *iopmp, uintptr_t addr,
                      uint8_t srcmd_fmt, uint8_t mdcfg_fmt,
                      const struct iopmp_operations_override *ops_override);

/**
 * \brief Get the associated MD bitmap and lock bit of given RRID
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] rrid              The RRID to be got
 * \param[out] mds              The pointer to an integer to store SRCMD_EN.md
 * \param[out] lock             The pointer to an integer to store SRCMD_EN.l
 *
 * \note This operation is only supported by SRCMD_FMT=0
 */
void srcmd_fmt_0_get_association_rrid_md(IOPMP_t *iopmp, uint32_t rrid,
                                         uint64_t *mds, bool *lock);

/**
 * \brief Associate the given RRID with the given MD bitmap
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] rrid              The RRID to be set
 * \param[in,out] mds           Input the desired MDs to be associated with
 *                              \p rrid. Output WARL value
 * \param[in] lock              Set 1 to lock SRCMD_EN[rrid]
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value
 *
 * \note This operation is only supported by SRCMD_FMT=0
 */
enum iopmp_error srcmd_fmt_0_set_association_rrid_md(IOPMP_t *iopmp,
                                                     uint32_t rrid,
                                                     uint64_t *mds,
                                                     bool lock);

/**
 * \brief Lock MDs
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] mds           Input the MD bitmap to be locked. Output WARL
 *                              value
 * \param[in] mdlck_lock        Set 1 to lock MDLCK and MDLCKH registers
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written value does not match the
 *         actual value. The actual values are output in \p mds
 *
 * \note This operation is only supported by SRCMD_FMT=0 or SRCMD_FMT=2
 * \note If MDLCK.l has already been set to 1, this API always expects
 *       \p mdlck_lock be 1.
 */
enum iopmp_error srcmd_fmt_0_2_set_md_lock(IOPMP_t *iopmp, uint64_t *mds,
                                           bool lock_mdlck);

/**
 * \brief Lock SRCMD_EN(rrid), SRCMD_ENH(rrid), SRCMD_R(rrid), SRCMD_RH(rrid),
 * SRCMD_W(rrid), and SRCMD_WH(rrid) if any
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] rrid              The RRID to be locked
 * \param[in] mdidx             The index of MD to be locked
 *
 * \return IOPMP_OK
 *
 * \note This operation is only supported by SRCMD_FMT=0
 */
enum iopmp_error srcmd_fmt_0_lock_srcmd_table(IOPMP_t *iopmp, uint32_t rrid,
                                              uint32_t mdidx);

/**
 * \brief Lock SRCMD_PERM(mdidx) and SRCMD_PERMH(mdidx)
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] rrid              The RRID to be locked
 * \param[in] mdidx             The index of MD to be locked
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_REG_IS_LOCKED if MDLCK has been locked
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written value does not match the
 *         actual value. The actual values are output in \p mds
 *
 * \note This operation is only supported by SRCMD_FMT=2
 */
enum iopmp_error srcmd_fmt_2_lock_srcmd_table(IOPMP_t *iopmp, uint32_t rrid,
                                              uint32_t mdidx);

/**
 * \brief Lock MDCFG(0) ~ MDCFG(md_num - 1)
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] md_num        Input the number of MD to be locked. Output WARL
 *                              value
 * \param[in] lock              Set 1 to lock MDCFGLCK register
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p md_num does not match the
 *         actual value. The actual value is output in \p md_num
 *
 * \note This operation is only supported by MDCFG_FMT=0
 */
enum iopmp_error mdcfg_fmt_0_lock_mdcfg(IOPMP_t *iopmp, uint32_t *md_num,
                                        bool lock);

/**
 * \brief Get the value of MDCFG(mdidx).t
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] mdidx             The index of MD to be got
 * \param[out] entry_top        The pointer to an integer to store MDCFG.t
 *
 * \note This operation is only supported by MDCFG_FMT=0
 */
void mdcfg_fmt_0_get_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                                  uint32_t *entry_top);

/**
 * \brief Calculate the top entry index of given MD using md_entry_num
 *
 * \param[in] iopmp             The IOPMP instance to be got
 * \param[in] mdidx             The index of MD to be got
 * \param[out] entry_top        The pointer to an integer to store MDCFG.t
 *
 * \note This operation is only supported by MDCFG_FMT=1 or MDCFG_FMT=2
 */
void mdcfg_fmt_1_2_get_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                                    uint32_t *entry_top);

/**
 * \brief Set the value of MDCFG(mdidx).t
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] mds               The index of MD to be set
 * \param[in,out] entry_top     Input the value of MDCFG.t and output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p entry_top does not match
 *         the actual value. The actual value is output in \p entry_top
 *
 * \note This operation is only supported by MDCFG_FMT=0
 */
enum iopmp_error mdcfg_fmt_0_set_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                                              uint32_t *entry_top);

/**
 * \brief Set the value of HWCFG3.md_entry_num
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] md_entry_num  Input the value of HWCFG3.md_entry_num. Output
 *                              WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p md_entry_num does not
 *         match the actual value. The actual value is output in \p md_entry_num
 *
 * \note This operation is only supported by MDCFG_FMT=2
 */
enum iopmp_error mdcfg_fmt_2_set_md_entry_num(IOPMP_t *iopmp,
                                              uint32_t *md_entry_num);

/**
 * \brief Set single RRID's r/w permissions to MD
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
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p r or \p w do not match the
 *         actual values
 *
 * \note This operation is only supported by SRCMD_FMT=2
 */
enum iopmp_error srcmd_fmt_2_set_md_permission(IOPMP_t *iopmp, uint32_t rrid,
                                               uint32_t mdidx, bool *r,
                                               bool *w);

/**
 * \brief Set multiple RRID's r/w permissions to MD
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] mdidx             The desired MD to be given permission
 * \param[in] cfg               The configuration structure for SRCMD format 2
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written permissions in \p cfg do not
 *         match the actual values
 *
 * \note This operation is only supported by SRCMD_FMT=2
 */
enum iopmp_error
srcmd_fmt_2_set_md_permission_multi(IOPMP_t *iopmp, uint32_t mdidx,
                                    IOPMP_SRCMD_PERM_CFG_t *cfg);

/******************************************************************************/
/* Generic operations, shared by every IOPMP model                            */
/******************************************************************************/
/*
 * One implementation serves every model, so libiopmp calls these by name. A
 * driver that has to replace one installs it in iopmp->ops_override, which each
 * call site tests first. See struct iopmp_operations_override.
 */
/**
 * \brief Capture latest IOPMP error report
 *
 * \param[in] iopmp             The IOPMP instance to be captured
 * \param[out] err_report       The pointer to IOPMP error report structure
 * \param[in] invalidate        Flag to clear V bit after reading error report
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_NOT_EXIST if there is no an pending error
 */
enum iopmp_error generic_capture_error(IOPMP_t *iopmp,
                                       IOPMP_ERR_REPORT_t *err_report,
                                       bool invalidate);

/**
 * \brief Clear IOPMP entries
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] idx_start         The start index of entries to be cleared
 * \param[in] num_entry         The number of entries to be cleared
 */
void generic_clear_entries(IOPMP_t *iopmp, uint32_t idx_start,
                           uint32_t num_entry);

/**
 * \brief Set the IOPMP HWCFG0.enable
 *
 * \param[in] iopmp             The IOPMP instance to be set
 */
void generic_enable(IOPMP_t *iopmp);

/**
 * \brief Check if there is an MSI write error and clear the flag
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[out] msi_werr         The pointer to flag
 */
void generic_get_and_clear_msi_werr(IOPMP_t *iopmp, bool *msi_werr);

/**
 * \brief Get the global entries from IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be read
 * \param[out] entry_array      The array of entries
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be read
 */
void generic_get_entries(IOPMP_t *iopmp, struct iopmp_entry *entry_array,
                         uint32_t idx_start, uint32_t num_entry);

/**
 * \brief Get subsequent violation window
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
 * \retval IOPMP_ERR_NOT_EXIST if there is no any subsequent violation
 *
 * \note Expected to be called after iopmp_capture_error() to get ERR_INFO.svc
 */
enum iopmp_error generic_get_sv_window(IOPMP_t *iopmp, uint16_t *svi,
                                       uint16_t *svw);

/**
 * \brief Invalidate the error record by clearing ERR_INFO.v bit
 *
 * \param[in] iopmp             The IOPMP instance to be invalidated
 */
void generic_invalidate_error(IOPMP_t *iopmp);

/**
 * \brief Lock ENTRY(0) ~ ENTRY(entry_num - 1)
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] entry_num     Input the number of entry to be locked. Output
 *                              WARL value
 * \param[in] lock              Lock ENTRYLCK register or not
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p entry_num does not match
 *         the actual value. The actual value is output via \p entry_num
 */
enum iopmp_error generic_lock_entries(IOPMP_t *iopmp, uint32_t *entry_num,
                                      bool lock);

/**
 * \brief Set IOPMP ERR_CFG.l to lock ERR_CFG register
 *
 * \param[in] iopmp             The IOPMP instance to be set
 */
void generic_lock_err_cfg(IOPMP_t *iopmp);

/**
 * \brief Lock number of priority entry
 *
 * \param[in] iopmp             The IOPMP instance
 */
void generic_lock_prio_entry_num(IOPMP_t *iopmp);

/**
 * \brief Lock the RRID tagged to outgoing transactions
 *
 * \param[in] iopmp             The IOPMP instance
 */
void generic_lock_rrid_transl(IOPMP_t *iopmp);

/**
 * \brief Poll until MDSTALL.is_busy == 0
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 * \param[in] polling           Set true to poll the status until takes effect
 * \param[in] stall_or_resume   Set true to poll for stall status or set false
 *                              to poll for resume status
 *
 * \retval 1 if the previous operation has taken effect
 * \retval 0 if the previous operation has not taken effect yet
 */
bool generic_poll_mdstall(IOPMP_t *iopmp, bool polling, bool stall_or_resume);

/**
 * \brief Resume the stalled transactions previously stalled, and poll the
 * resume status until resuming takes effect if necessary
 *
 * \param[in] iopmp             The IOPMP instance to be resumed
 * \param[in] polling           Set true to poll the resume status until
 *                              resuming takes effect
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value
 */
enum iopmp_error generic_resume_transactions(IOPMP_t *iopmp, bool polling);

/**
 * \brief Set the global entries into IOPMP
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] entry_array       The array of entries
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be written
 *
 * \return IOPMP_OK
 */
enum iopmp_error generic_set_entries(IOPMP_t *iopmp,
                                     const struct iopmp_entry *entry_array,
                                     uint32_t idx_start, uint32_t num_entry);

/**
 * \brief Set IOPMP ERR_CFG.rs to suppress/express error response
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] suppress      True to suppress or false to express
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p suppress does not match
 *         the actual value. The actual value is output via \p suppress
 */
enum iopmp_error generic_set_global_err_resp(IOPMP_t *iopmp, bool *suppress);

/**
 * \brief Set IOPMP ERR_CFG.ie to enable/disable IOPMP global interrupt
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in] enable            True to enable or false to disable
 */
void generic_set_global_intr(IOPMP_t *iopmp, bool enable);

/**
 * \brief Set IOPMP message-signaled interrupts (MSI) information
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] msiaddr64     Input 64-bit MSI address. Output WARL value
 * \param[in,out] msidata       Input 11-bit MSI data. Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if \p msiaddr64 or \p msidata can not be
 *         written into \p iopmp
 */
enum iopmp_error generic_set_msi_info(IOPMP_t *iopmp, uint64_t *msiaddr64,
                                      uint16_t *msidata);

/**
 * \brief Set IOPMP message-signaled interrupts (MSI) enable/disable
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] enable        True to enable or false to disable
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if \p enable can not be written into \p iopmp
 */
enum iopmp_error generic_set_msi_sel(IOPMP_t *iopmp, bool *enable);

/**
 * \brief Set IOPMP HWCFG2.prio_entry
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] num_entry     Input the number of entries to be matched with
 *                              priority. Output WARL value.
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p num_entry does not match
 *         the actual value. The actual value is output via \p num_entry
 */
enum iopmp_error generic_set_prio_entry_num(IOPMP_t *iopmp,
                                            uint16_t *num_entry);

/**
 * \brief Write RRIDSCP with given RRID and operation
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] rrid          Input the RRID to be stalled. Output WARL value
 * \param[in] op                The operation of RRIDSCP
 * \param[out] stat             The pointer to store the RRIDSCP.stat
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p rrid does not match the
 *         actual value. The actual value is output via \p rrid
 */
enum iopmp_error generic_set_rridscp(IOPMP_t *iopmp, uint32_t *rrid,
                                     enum iopmp_rridscp_op op,
                                     uint32_t *stat);

/**
 * \brief Set IOPMP HWCFG3.rrid_transl
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] rrid_transl   Input the value of rrid_transl to be set. Output
 *                              WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p rrid_transl does not
 *         match the actual value. The actual value is output via \p rrid_transl
 */
enum iopmp_error generic_set_rrid_transl(IOPMP_t *iopmp,
                                         uint16_t *rrid_transl);

/**
 * \brief Set IOPMP ERR_CFG.stall_violation_en
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] enable        Input 1 to enable, 0 to disable. Output WARL
 *                              value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if \p enable can't be written into \p iopmp
 */
enum iopmp_error generic_set_stall_violation_en(IOPMP_t *iopmp, bool *enable);

/**
 * \brief Set MDSTALL to stall the transactions related to MDs bitmap, and poll
 * the stall status until stall takes effect if necessary
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] mds           Input the MD bitmap to be stalled. Output WARL
 *                              value
 * \param[in] exempt            Stall transactions with exempt selected MDs
 * \param[in] polling           Set true to poll the stall status until stalling
 *                              takes effect
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value. The actual value is output via \p mds
 */
enum iopmp_error generic_stall_by_mds(IOPMP_t *iopmp, uint64_t *mds,
                                      bool exempt, bool polling);

/******************************************************************************/
/* SPS extension operations                                                   */
/******************************************************************************/

/**
 * \brief Get RRID's read permission to MD(0) ~ MD(62)
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be got
 *
 * \return SRCMD_RH(rrid).mdh | SRCMD_R(rrid).md
 *
 * \note This operation is only supported by IOPMP/SPS extension
 */
uint64_t ext_sps_get_srcmd_r_64_md(IOPMP_t *iopmp, uint32_t rrid);

/**
 * \brief Set RRID's read permission to MD(0) ~ MD(62)
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in,out] mds           Input the read permission bitmap associated with
 *                              \p rrid. Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 *
 * \note This operation is only supported by IOPMP/SPS extension
 */
enum iopmp_error ext_sps_set_srcmd_r_64_md(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds);

/**
 * \brief Get RRID's write permission to MD(0) ~ MD(62)
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be got
 *
 * \return SRCMD_WH(rrid).mdh | SRCMD_W(rrid).md
 *
 * \note This operation is only supported by IOPMP/SPS extension
 */
uint64_t ext_sps_get_srcmd_w_64_md(IOPMP_t *iopmp, uint32_t rrid);

/**
 * \brief Set RRID's write permission to MD(0) ~ MD(62)
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in,out] mds           Input the write permission bitmap associated
 *                              with \p rrid. Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 *
 * \note This operation is only supported by IOPMP/SPS extension
 */
enum iopmp_error ext_sps_set_srcmd_w_64_md(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds);

/**
 * \brief Get RRID's instruction fetch permission to MD(0) ~ MD(62)
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be got
 *
 * \return SRCMD_XH(rrid).mdh | SRCMD_X(rrid).md
 *
 * \note This operation is only supported by IOPMP/SPS extension
 */
uint64_t ext_sps_get_srcmd_x_64_md(IOPMP_t *iopmp, uint32_t rrid);

/**
 * \brief Set RRID's instruction fetch permission to MD(0) ~ MD(62)
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in,out] mds           Input the instruction fetch permission bitmap
 *                              associated with \p rrid. Output WARL value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 *
 * \note This operation is only supported by IOPMP/SPS extension
 */
enum iopmp_error ext_sps_set_srcmd_x_64_md(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds);
#endif
