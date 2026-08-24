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

#include <string.h>

#include "libiopmp.h"
#include "libiopmp_def.h"

#include "iopmp_drv_common.h"
#include "iopmp_util.h"

extern const struct iopmp_driver *const iopmp_drivers[];

#define IS_ALIGNED(x, a)    (((x) & ((a) - 1)) == 0)

#define MD_ENTRY_NUM_BITS   7
#define MAX_MD_ENTRY_NUM    ((1UL << MD_ENTRY_NUM_BITS) - 1)

#define IOPMP_ADDR_SHIFT    2

int libiopmp_major_version(void)
{
    return LIBIOPMP_VERSION_MAJOR;
}

int libiopmp_minor_version(void)
{
    return LIBIOPMP_VERSION_MINOR;
}

int libiopmp_extra_version(void)
{
    return LIBIOPMP_VERSION_EXTRA;
}

bool libiopmp_check_version(int major, int minor, int extra)
{
    if (major > libiopmp_major_version())
        return true;

    if (major == libiopmp_major_version() && minor > libiopmp_minor_version())
        return true;

    if (major == libiopmp_major_version() &&
        minor == libiopmp_minor_version() && extra > libiopmp_extra_version())
        return true;

    return false;
}

/**
 * \brief Check if the model of IOPMP is Rapid-K, Dynamic-K, or Compact-K
 *
 * \param[in] iopmp             The IOPMP instance to be checked
 *
 * \retval 1 if model is xxx-K
 * \retval 0 if model is not xxx-K
 */
static bool __is_k_model(IOPMP_t *iopmp)
{
    return iopmp->mdcfg_fmt == IOPMP_MDCFG_FMT_1 ||
           iopmp->mdcfg_fmt == IOPMP_MDCFG_FMT_2;
}

enum iopmp_error iopmp_init(IOPMP_t *iopmp, uintptr_t addr, uint8_t srcmd_fmt,
                            uint8_t mdcfg_fmt, uint32_t impid)
{
    const struct iopmp_driver *drv;

    if (srcmd_fmt > IOPMP_SRCMD_FMT_2 || mdcfg_fmt > IOPMP_MDCFG_FMT_2) {
        return IOPMP_ERR_NOT_SUPPORTED;
    }

    /* A vendor driver claims an implementation ID of its own */
    for (int i = 0; iopmp_drivers[i]; i++) {
        drv = iopmp_drivers[i];
        if (srcmd_fmt == drv->srcmd_fmt && mdcfg_fmt == drv->mdcfg_fmt &&
            impid == drv->impid) {
            memset(iopmp, 0, sizeof(*iopmp));
            assert(drv->init != NULL);
            return drv->init(iopmp, addr);
        }
    }

    if (impid != IOPMP_IMPID_NOT_SPECIFIED) {
        return IOPMP_ERR_NOT_SUPPORTED;
    }

    /* Every model the specification defines initializes the same way */
    memset(iopmp, 0, sizeof(*iopmp));

    return iopmp_drv_init_common(iopmp, addr, srcmd_fmt, mdcfg_fmt, NULL);
}

enum iopmp_error iopmp_get_vendor_id(IOPMP_t *iopmp, uint32_t *vendor)
{
    assert(iopmp_is_initialized(iopmp));

    if (!vendor)
        return IOPMP_ERR_INVALID_PARAMETER;

    *vendor = iopmp->vendor;
    return IOPMP_OK;
}

enum iopmp_error iopmp_get_specver(IOPMP_t *iopmp, uint32_t *specver)
{
    assert(iopmp_is_initialized(iopmp));

    if (!specver)
        return IOPMP_ERR_INVALID_PARAMETER;

    *specver = iopmp->specver;
    return IOPMP_OK;
}

enum iopmp_error iopmp_get_impid(IOPMP_t *iopmp, uint32_t *impid)
{
    assert(iopmp_is_initialized(iopmp));

    if (!impid)
        return IOPMP_ERR_INVALID_PARAMETER;

    *impid = iopmp->impid;
    return IOPMP_OK;
}

enum iopmp_error iopmp_lock_prio_entry_num(IOPMP_t *iopmp)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->non_prio_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!iopmp->prio_ent_prog)
        return IOPMP_OK;

    /* If HWCFG2.prio_ent_prog is not wired to 0, this operation is mandatory */
    if (iopmp->ops_override && iopmp->ops_override->lock_prio_entry_num) {
        iopmp->ops_override->lock_prio_entry_num(iopmp);
    } else {
        generic_lock_prio_entry_num(iopmp);
    }
    iopmp->prio_ent_prog = false; /* update local cache */

    return IOPMP_OK;
}

enum iopmp_error iopmp_lock_rrid_transl(IOPMP_t *iopmp)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->rrid_transl_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!iopmp->rrid_transl_prog)
        return IOPMP_OK;

    /*
     * If HWCFG3.rrid_transl_prog is not wired to 0, this operation is mandatory
     */
    if (iopmp->ops_override && iopmp->ops_override->lock_rrid_transl) {
        iopmp->ops_override->lock_rrid_transl(iopmp);
    } else {
        generic_lock_rrid_transl(iopmp);
    }
    iopmp->rrid_transl_prog = false;    /* update local cache */

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_enable(IOPMP_t *iopmp)
{
    assert(iopmp_is_initialized(iopmp));

    /* Already enabled. */
    if (iopmp->enable)
        return IOPMP_OK;

    /* HWCFG0.enable is mandatory W1SS bit */
    if (iopmp->ops_override && iopmp->ops_override->enable) {
        iopmp->ops_override->enable(iopmp);
    } else {
        generic_enable(iopmp);
    }
    iopmp->enable = true;   /* update local cache */

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_prio_entry_num(IOPMP_t *iopmp, uint16_t *num_entry)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->non_prio_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!iopmp->prio_ent_prog)
        return IOPMP_ERR_REG_IS_LOCKED;

    if (!num_entry)
        return IOPMP_ERR_INVALID_PARAMETER;

    /*
     * If HWCFG2.prio_ent_prog is not wired to 0, this operation is mandatory
     */
    if (iopmp->ops_override && iopmp->ops_override->set_prio_entry_num) {
        ret = iopmp->ops_override->set_prio_entry_num(iopmp, num_entry);
    } else {
        ret = generic_set_prio_entry_num(iopmp, num_entry);
    }
    /* HWCFG2.prio_entry is WARL field. We always update local cache for it */
    iopmp->prio_entry_num = *num_entry;

    return ret;
}

enum iopmp_error iopmp_get_rrid_transl_prog(IOPMP_t *iopmp,
                                            bool *rrid_transl_prog)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->rrid_transl_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!rrid_transl_prog)
        return IOPMP_ERR_INVALID_PARAMETER;

    *rrid_transl_prog = iopmp->rrid_transl_prog;
    return IOPMP_OK;
}

enum iopmp_error iopmp_get_rrid_transl(IOPMP_t *iopmp, uint16_t *rrid_transl)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->rrid_transl_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!rrid_transl)
        return IOPMP_ERR_INVALID_PARAMETER;

    *rrid_transl = iopmp->rrid_transl;
    return IOPMP_OK;
}

enum iopmp_error iopmp_set_rrid_transl(IOPMP_t *iopmp, uint16_t *rrid_transl)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->rrid_transl_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!iopmp->rrid_transl_prog)
        return IOPMP_ERR_REG_IS_LOCKED;

    if (!rrid_transl)
        return IOPMP_ERR_INVALID_PARAMETER;

    /*
     * If HWCFG3.rrid_transl_prog is not wired to 0, this operation is mandatory
     */
    if (iopmp->ops_override && iopmp->ops_override->set_rrid_transl) {
        ret = iopmp->ops_override->set_rrid_transl(iopmp, rrid_transl);
    } else {
        ret = generic_set_rrid_transl(iopmp, rrid_transl);
    }
    /* HWCFG3.rrid_transl is WARL field. We always update local cache for it */
    iopmp->rrid_transl = *rrid_transl;

    return ret;
}

enum iopmp_error iopmp_stall_transactions_by_mds(IOPMP_t *iopmp, uint64_t *mds,
                                                 bool exempt, bool polling)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->stall_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    /*
     * According to the specification:
     * MDSTALL can be written at most once and before a resume. Writing a
     * non-zero value to MDSTALL multiple times after a resume leads to RRIDs'
     * stall states being undefined.
     * Thus, we forbid the operation if IOPMP is stalling some transactions.
     */
    if (iopmp->is_stalling)
        return IOPMP_ERR_NOT_ALLOWED;

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    /*
     * Catch an unselectable MD here rather than through the read-back below.
     * Writing MDSTALL stalls transactions, so letting the write go ahead would
     * disturb traffic only to undo it again.
     */
    if (*mds & ~iopmp->stall_md_mask) {
        *mds &= iopmp->stall_md_mask;
        return IOPMP_ERR_ILLEGAL_VALUE;
    }

    if (iopmp->ops_override && iopmp->ops_override->stall_by_mds) {
        ret = iopmp->ops_override->stall_by_mds(iopmp, mds, exempt, polling);
    } else {
        ret = generic_stall_by_mds(iopmp, mds, exempt, polling);
    }
    if (ret == IOPMP_OK)
        iopmp->is_stalling = true;

    return ret;
}

enum iopmp_error iopmp_resume_transactions(IOPMP_t *iopmp, bool polling)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->stall_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    /* We forbid the operation if IOPMP was not stalling any transactions */
    if (!iopmp->is_stalling)
        return IOPMP_ERR_NOT_ALLOWED;

    if (iopmp->ops_override && iopmp->ops_override->resume_transactions) {
        ret = iopmp->ops_override->resume_transactions(iopmp, polling);
    } else {
        ret = generic_resume_transactions(iopmp, polling);
    }
    if (ret == IOPMP_OK)
        iopmp->is_stalling = false;

    return ret;
}

static enum iopmp_error __iopmp_poll_mdstall(IOPMP_t *iopmp,
                                             bool polling,
                                             bool stall_or_resume,
                                             bool *done)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->stall_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!done)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (iopmp->ops_override && iopmp->ops_override->poll_mdstall) {
        *done = iopmp->ops_override->poll_mdstall(iopmp, polling,
                                                  stall_or_resume);
    } else {
        *done = generic_poll_mdstall(iopmp, polling, stall_or_resume);
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_transactions_are_stalled(IOPMP_t *iopmp, bool polling,
                                                bool *stalled)
{
    if (!iopmp->is_stalling)
        return IOPMP_ERR_NOT_EXIST;

    return __iopmp_poll_mdstall(iopmp, polling, true, stalled);
}

enum iopmp_error iopmp_transactions_are_resumed(IOPMP_t *iopmp, bool polling,
                                                bool *resumed)
{
    if (iopmp->is_stalling)
        return IOPMP_ERR_NOT_EXIST;

    return __iopmp_poll_mdstall(iopmp, polling, false, resumed);
}

enum iopmp_error iopmp_probe_stall_by_md(IOPMP_t *iopmp, uint64_t *mds)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->stall_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    /* The probe stalls and then resumes, which MDSTALL forbids in the middle
     * of a stall that is already in progress */
    if (iopmp->is_stalling)
        return IOPMP_ERR_NOT_ALLOWED;

    *mds = GENMASK_64((iopmp->md_num - 1), 0);
    if (iopmp->ops_override && iopmp->ops_override->stall_by_mds) {
        ret = iopmp->ops_override->stall_by_mds(iopmp, mds, false, false);
    } else {
        ret = generic_stall_by_mds(iopmp, mds, false, false);
    }
    if (ret == IOPMP_OK) {
        /* Every MD was accepted, so undo the stall the probe just caused */
        if (iopmp->ops_override && iopmp->ops_override->resume_transactions) {
            ret = iopmp->ops_override->resume_transactions(iopmp, true);
        } else {
            ret = generic_resume_transactions(iopmp, true);
        }
    } else if (ret == IOPMP_ERR_ILLEGAL_VALUE) {
        /* A narrower read-back is the answer, not a failure, and
         * stall_by_mds() has already resumed on our behalf */
        ret = IOPMP_OK;
    }

    if (ret == IOPMP_OK) {
        iopmp->stall_md_mask = *mds;
    }

    return ret;
}

/**
 * \brief Write RRIDSCP with given op and return the stat
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] rrid          Input the RRID to be stalled. Output WARL value
 * \param[in] op                The desired operation of RRIDSCP
 * \param[out] stat             The pointer to store enum iopmp_rridscp_stat
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p rrid is NULL or invalid
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_NOT_SUPPORTED if \p iopmp does not support stall by RRID
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p rrid does not match the
 *         actual value. The actual value is output via \p rrid
 */
static enum iopmp_error __iopmp_set_rridscp(IOPMP_t *iopmp, uint32_t *rrid,
                                            enum iopmp_rridscp_op op,
                                            enum iopmp_rridscp_stat *stat)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->support_stall_by_rrid)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!rrid || !stat)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (*rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (iopmp->ops_override && iopmp->ops_override->set_rridscp) {
        return iopmp->ops_override->set_rridscp(iopmp, rrid, op, stat);
    } else {
        return generic_set_rridscp(iopmp, rrid, op, stat);
    }
}

enum iopmp_error iopmp_stall_cherry_pick_rrid(IOPMP_t *iopmp, uint32_t *rrid,
                                              bool select,
                                              enum iopmp_rridscp_stat *stat)
{
    enum iopmp_rridscp_op op;

    op = select ? IOPMP_RRIDSCP_OP_STALL : IOPMP_RRIDSCP_OP_DONT_STALL;
    return __iopmp_set_rridscp(iopmp, rrid, op, stat);
}

enum iopmp_error iopmp_query_stall_stat_by_rrid(IOPMP_t *iopmp, uint32_t *rrid,
                                                enum iopmp_rridscp_stat *stat)
{
    return __iopmp_set_rridscp(iopmp, rrid, IOPMP_RRIDSCP_OP_QUERY, stat);
}

enum iopmp_error iopmp_get_locked_md(IOPMP_t *iopmp, uint64_t *mds,
                                     bool *mdlck_lock)
{
    assert(iopmp_is_initialized(iopmp));

    if (!mds || !mdlck_lock)
        return IOPMP_ERR_INVALID_PARAMETER;

    *mds = iopmp->mdlck_md;
    *mdlck_lock = iopmp->mdlck_lock;

    return IOPMP_OK;
}

enum iopmp_error iopmp_lock_md(IOPMP_t *iopmp, uint64_t *mds, bool mdlck_lock)
{
    enum iopmp_error ret;
    uint64_t valid_mdlck_md_mask;
    uint64_t __mds;

    assert(iopmp_is_initialized(iopmp));

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    __mds = *mds;

    /* Early return for both zero input */
    if (!__mds && !mdlck_lock)
        return IOPMP_OK;

    /* Check if given 'mds' contains unsupported MD bits */
    valid_mdlck_md_mask = ((uint64_t)1 << iopmp->md_num) - 1;
    if (__mds > valid_mdlck_md_mask)
        return IOPMP_ERR_NOT_SUPPORTED;

    /* Already locked */
    if (iopmp->mdlck_lock) {
        if ((__mds & iopmp->mdlck_md) == __mds &&
            mdlck_lock == iopmp->mdlck_lock)
            return IOPMP_OK;
        else
            return IOPMP_ERR_REG_IS_LOCKED;
    }

    if (iopmp->ops_override && iopmp->ops_override->set_md_lock) {
        ret = iopmp->ops_override->set_md_lock(iopmp, mds, mdlck_lock);
    } else {
        ret = srcmd_fmt_0_2_set_md_lock(iopmp, mds, mdlck_lock);
    }
    /*
     * MDLCK.md and MDLCKH.mdh are WARL fields. We always update local data
     * cache for them.
     */
    iopmp->mdlck_lock = mdlck_lock;
    iopmp->mdlck_md = *mds;

    return ret;
}

enum iopmp_error iopmp_lock_mdcfg(IOPMP_t *iopmp, uint32_t *md_num, bool lock)
{
    enum iopmp_error ret;
    uint32_t mdcfglck_f;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->mdcfg_fmt != IOPMP_MDCFG_FMT_0)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!md_num)
        return IOPMP_ERR_INVALID_PARAMETER;

    mdcfglck_f = *md_num;

    if (iopmp->mdcfglck_f == mdcfglck_f && iopmp->mdcfglck_lock == lock)
        return IOPMP_OK;                /* MDCFGLCK is already what we want */

    if (mdcfglck_f > iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (iopmp->mdcfglck_lock)
        return IOPMP_ERR_REG_IS_LOCKED; /* Already locked */

    if (iopmp->mdcfglck_f > mdcfglck_f)
        return IOPMP_ERR_NOT_ALLOWED;   /* Should be monotonically increased */

    /* MDCFGLCK is mandatory register in MDCFG_FMT=0 */
    if (iopmp->ops_override && iopmp->ops_override->lock_mdcfg) {
        ret = iopmp->ops_override->lock_mdcfg(iopmp, md_num, lock);
    } else {
        ret = mdcfg_fmt_0_lock_mdcfg(iopmp, md_num, lock);
    }
    /* Update local data cache */
    iopmp->mdcfglck_lock = lock;
    iopmp->mdcfglck_f = *md_num;

    return ret;
}

enum iopmp_error iopmp_is_mdcfglck_locked(IOPMP_t *iopmp, bool *locked)
{
    assert(iopmp_is_initialized(iopmp));

    if (iopmp->mdcfg_fmt != IOPMP_MDCFG_FMT_0)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!locked)
        return IOPMP_ERR_INVALID_PARAMETER;

    *locked = iopmp->mdcfglck_lock;
    return IOPMP_OK;
}

enum iopmp_error iopmp_get_locked_mdcfg_num(IOPMP_t *iopmp, uint32_t *md_num)
{
    assert(iopmp_is_initialized(iopmp));

    if (iopmp->mdcfg_fmt != IOPMP_MDCFG_FMT_0)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!md_num)
        return IOPMP_ERR_INVALID_PARAMETER;

    *md_num = iopmp->mdcfglck_f;
    return IOPMP_OK;
}

enum iopmp_error iopmp_lock_entries(IOPMP_t *iopmp, uint32_t *entry_num,
                                    bool lock)
{
    enum iopmp_error ret;
    uint32_t __entry_num;

    assert(iopmp_is_initialized(iopmp));

    if (!entry_num)
        return IOPMP_ERR_INVALID_PARAMETER;

    __entry_num = *entry_num;

    if (iopmp->entrylck_f == __entry_num && iopmp->entrylck_lock == lock)
        return IOPMP_OK;                /* ENTRYLCK is already what we want */

    if (__entry_num > iopmp->entry_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (iopmp->entrylck_lock)
        return IOPMP_ERR_REG_IS_LOCKED; /* Already locked */

    if (iopmp->entrylck_f > __entry_num)
        return IOPMP_ERR_NOT_ALLOWED;   /* Should be monotonically increased */

    /* ENTRYLCK is mandatory register */

    if (iopmp->ops_override && iopmp->ops_override->lock_entries) {
        ret = iopmp->ops_override->lock_entries(iopmp, entry_num, lock);
    } else {
        ret = generic_lock_entries(iopmp, entry_num, lock);
    }
    iopmp->entrylck_lock = lock;
    iopmp->entrylck_f = *entry_num;

    return ret;
}

enum iopmp_error iopmp_lock_err_cfg(IOPMP_t *iopmp)
{
    assert(iopmp_is_initialized(iopmp));

    /* Already locked? */
    if (iopmp->err_cfg_lock)
        return IOPMP_OK;

    /* ERR_CFG.l is mandatory W1SS bit */

    if (iopmp->ops_override && iopmp->ops_override->lock_err_cfg) {
        iopmp->ops_override->lock_err_cfg(iopmp);
    } else {
        generic_lock_err_cfg(iopmp);
    }
    iopmp->err_cfg_lock = true; /* update local cache */

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_global_intr(IOPMP_t *iopmp, bool enable)
{
    assert(iopmp_is_initialized(iopmp));

    /* Already enabled or disabled? */
    if (iopmp->intr_enable == enable)
        return IOPMP_OK;

    /* Already locked? */
    if (iopmp->err_cfg_lock)
        return IOPMP_ERR_REG_IS_LOCKED;

    /* ERR_CFG.ie is mandatory RW bit */

    if (iopmp->ops_override && iopmp->ops_override->set_global_intr) {
        iopmp->ops_override->set_global_intr(iopmp, enable);
    } else {
        generic_set_global_intr(iopmp, enable);
    }
    iopmp->intr_enable = enable;    /* update local cache */

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_global_err_resp(IOPMP_t *iopmp, bool *suppress)
{
    enum iopmp_error ret;
    bool __suppress;

    assert(iopmp_is_initialized(iopmp));

    if (!suppress)
        return IOPMP_ERR_INVALID_PARAMETER;

    __suppress = *suppress;

    /* Already suppressed or expressed? */
    if (iopmp->err_resp_suppress == __suppress)
        return IOPMP_OK;

    /* Already locked? */
    if (iopmp->err_cfg_lock)
        return IOPMP_ERR_REG_IS_LOCKED;

    /* ERR_CFG.rs is optional */
    if (iopmp->ops_override && iopmp->ops_override->set_global_err_resp) {
        ret = iopmp->ops_override->set_global_err_resp(iopmp, suppress);
    } else {
        ret = generic_set_global_err_resp(iopmp, suppress);
    }
    iopmp->err_resp_suppress = *suppress;   /* update local cache */

    return ret;
}

enum iopmp_error iopmp_set_msi_sel(IOPMP_t *iopmp, bool *enable)
{
    enum iopmp_error ret;
    bool __enable;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->msi_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!enable)
        return IOPMP_ERR_INVALID_PARAMETER;

    __enable = *enable;

    /* Already enabled or disabled? */
    if (iopmp->msi_sel == __enable)
        return IOPMP_OK;

    /* Already locked? */
    if (iopmp->err_cfg_lock)
        return IOPMP_ERR_REG_IS_LOCKED;

    /* ERR_CFG.msi_sel can be programmable or hardwired */
    if (iopmp->ops_override && iopmp->ops_override->set_msi_sel) {
        ret = iopmp->ops_override->set_msi_sel(iopmp, enable);
    } else {
        ret = generic_set_msi_sel(iopmp, enable);
    }
    iopmp->msi_sel = *enable;   /* update local cache */

    return ret;
}

enum iopmp_error iopmp_get_msi_addr(IOPMP_t *iopmp, uint64_t *msiaddr64)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->msi_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!msiaddr64)
        return IOPMP_ERR_INVALID_PARAMETER;

    *msiaddr64 = iopmp->msiaddr64;

    return IOPMP_OK;
}

enum iopmp_error iopmp_get_msi_data(IOPMP_t *iopmp, uint16_t *msidata)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->msi_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!msidata)
        return IOPMP_ERR_INVALID_PARAMETER;

    *msidata = iopmp->msidata;

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_msi_info(IOPMP_t *iopmp, uint64_t *msiaddr64,
                                    uint16_t *msidata)
{
    enum iopmp_error ret;
    uint64_t __msiaddr64;
    uint16_t __msidata;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->msi_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!msiaddr64 || !msidata)
        return IOPMP_ERR_INVALID_PARAMETER;

    __msiaddr64 = *msiaddr64;
    __msidata = *msidata;

    /* Already set? */
    if (iopmp->msiaddr64 == __msiaddr64 && iopmp->msidata == __msidata)
        return IOPMP_OK;

    /*
     * If HWCFG0.addrh_en=0, IOPMP only implements ERR_MSIADDR which contains
     * bits 33 to 2 of the address.
     */
    if (!iopmp->addrh_en && __msiaddr64 > 0x3FFFFFFFF)
        return IOPMP_ERR_NOT_SUPPORTED;

    /* ERR_CFG.msidata only supports maximum to 11 bits */
    if (__msidata > 0x7FF)
        return IOPMP_ERR_NOT_SUPPORTED;

    /* Already locked? */
    if (iopmp->err_cfg_lock)
        return IOPMP_ERR_REG_IS_LOCKED;

    if (iopmp->ops_override && iopmp->ops_override->set_msi_info) {
        ret = iopmp->ops_override->set_msi_info(iopmp, msiaddr64, msidata);
    } else {
        ret = generic_set_msi_info(iopmp, msiaddr64, msidata);
    }
    /*
     * ERR_CFG.msidata, ERR_MSIADDR and ERR_MSIADDRH are WARL registers.
     * We need to always update local data cache for them.
     */
    iopmp->msiaddr64 = *msiaddr64;
    iopmp->msidata = *msidata;

    return ret;
}

enum iopmp_error iopmp_get_and_clear_msi_werr(IOPMP_t *iopmp, bool *msi_werr)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->msi_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!msi_werr)
        return IOPMP_ERR_INVALID_PARAMETER;

    /* If HWCFG2.msi_en=1, this operation is mandatory */
    if (iopmp->ops_override && iopmp->ops_override->get_and_clear_msi_werr) {
        iopmp->ops_override->get_and_clear_msi_werr(iopmp, msi_werr);
    } else {
        generic_get_and_clear_msi_werr(iopmp, msi_werr);
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_stall_violation_en(IOPMP_t *iopmp, bool *enable)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->stall_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!enable)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (iopmp->stall_violation_en == *enable)   /* Already set */
        return IOPMP_OK;

    /* Already locked? */
    if (iopmp->err_cfg_lock)
        return IOPMP_ERR_REG_IS_LOCKED;

    /* If HWCFG2.stall_en=1, this operation is mandatory */
    if (iopmp->ops_override && iopmp->ops_override->set_stall_violation_en) {
        ret = iopmp->ops_override->set_stall_violation_en(iopmp, enable);
    } else {
        ret = generic_set_stall_violation_en(iopmp, enable);
    }
    iopmp->stall_violation_en = *enable;

    return ret;
}

enum iopmp_error iopmp_invalidate_error(IOPMP_t *iopmp)
{
    assert(iopmp_is_initialized(iopmp));

    /*
     * The error capture record is optional.
     * If it is not implemented, ERR_INFO.v should be wired to zero.
     */
    if (iopmp->no_err_rec)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (iopmp->ops_override && iopmp->ops_override->invalidate_error) {
        iopmp->ops_override->invalidate_error(iopmp);
    } else {
        generic_invalidate_error(iopmp);
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_capture_error(IOPMP_t *iopmp,
                                     IOPMP_ERR_REPORT_t *err_report,
                                     bool invalidate)
{
    assert(iopmp_is_initialized(iopmp));

    if (iopmp->no_err_rec)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!err_report)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (iopmp->ops_override && iopmp->ops_override->capture_error) {
        return iopmp->ops_override->capture_error(iopmp, err_report,
                                                  invalidate);
    } else {
        return generic_capture_error(iopmp, err_report, invalidate);
    }
}

enum iopmp_error iopmp_mfr_get_sv_window(IOPMP_t *iopmp, uint16_t *svi,
                                         uint16_t *svw)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp->mfr_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!svi || !svw)
        return IOPMP_ERR_INVALID_PARAMETER;

    /* If HWCFG2.mfr_en=1, this operation is mandatory */
    if (iopmp->ops_override && iopmp->ops_override->get_sv_window) {
        return iopmp->ops_override->get_sv_window(iopmp, svi, svw);
    } else {
        return generic_get_sv_window(iopmp, svi, svw);
    }
}

enum iopmp_error iopmp_lock_srcmd_table_fmt_0(IOPMP_t *iopmp, uint32_t rrid)
{
    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_0)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (iopmp->ops_override && iopmp->ops_override->lock_srcmd_table) {
        return iopmp->ops_override->lock_srcmd_table(iopmp, rrid, 0);
    } else {
        return srcmd_fmt_0_lock_srcmd_table(iopmp, rrid, 0);
    }
}

enum iopmp_error iopmp_is_srcmd_table_fmt_0_locked(IOPMP_t *iopmp,
                                                   uint32_t rrid,
                                                   bool *locked)
{
    uint64_t mds;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_0)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!locked)
        return IOPMP_ERR_INVALID_PARAMETER;

    /* Get current SRCMD_EN(rrid) */
    if (iopmp->ops_override && iopmp->ops_override->get_association_rrid_md) {
        iopmp->ops_override->get_association_rrid_md(iopmp, rrid, &mds, locked);
    } else {
        srcmd_fmt_0_get_association_rrid_md(iopmp, rrid, &mds, locked);
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_lock_srcmd_table_fmt_2(IOPMP_t *iopmp, uint32_t mdidx)
{
    enum iopmp_error ret;
    uint64_t mds;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_2)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    mds = (uint64_t)1 << mdidx;
    if ((iopmp->mdlck_md & mds) == mds)
        return IOPMP_OK;

    if (iopmp->mdlck_lock)
        return IOPMP_ERR_REG_IS_LOCKED;

    if (iopmp->ops_override && iopmp->ops_override->lock_srcmd_table) {
        ret = iopmp->ops_override->lock_srcmd_table(iopmp, 0, mdidx);
    } else {
        ret = srcmd_fmt_2_lock_srcmd_table(iopmp, 0, mdidx);
    }
    if (ret == IOPMP_OK)
        iopmp->mdlck_md |= mds;

    return ret;
}

enum iopmp_error iopmp_is_srcmd_table_fmt_2_locked(IOPMP_t *iopmp,
                                                   uint32_t mdidx,
                                                   bool *locked)
{
    uint64_t mds, checked_md;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_2)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!locked)
        return IOPMP_ERR_INVALID_PARAMETER;

    mds = iopmp->mdlck_md;
    checked_md = (uint64_t)1 << mdidx;
    *locked = (checked_md & mds) == checked_md;

    return IOPMP_OK;
}

enum iopmp_error iopmp_get_rrid_md_association(IOPMP_t *iopmp, uint32_t rrid,
                                               uint64_t *mds, bool *lock)
{
    assert(iopmp_is_initialized(iopmp));

    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!lock || !mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (iopmp->srcmd_fmt == IOPMP_SRCMD_FMT_0) {
        /* Check SRCMD table */
        if (iopmp->ops_override &&
            iopmp->ops_override->get_association_rrid_md) {
            iopmp->ops_override->get_association_rrid_md(iopmp, rrid, mds,
                                                         lock);
        } else {
            srcmd_fmt_0_get_association_rrid_md(iopmp, rrid, mds, lock);
        }
    } else if (iopmp->srcmd_fmt == IOPMP_SRCMD_FMT_1) {
        /* Each RRID is exactly associated with a single MD */
        *mds = (uint64_t)1 << rrid;
        *lock = true;
    } else if (iopmp->srcmd_fmt == IOPMP_SRCMD_FMT_2) {
        /* Every RRID implicitly associates all implemented memory domains */
        *mds = GENMASK_64((iopmp->md_num - 1), 0);
        *lock = true;
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_rrid_md_association(IOPMP_t *iopmp, uint32_t rrid,
                                               uint64_t mds_set,
                                               uint64_t mds_clr,
                                               uint64_t *mds,
                                               bool lock)
{
    uint64_t valid_mds;
    bool is_srcmd_en_locked;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_0)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    valid_mds = GENMASK_64((iopmp->md_num - 1), 0);
    if (mds_set > valid_mds || mds_clr > valid_mds)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* One of selected MDs has been locked by MDLCK */
    if (iopmp->mdlck_md & (mds_set | mds_clr))
        return IOPMP_ERR_REG_IS_LOCKED;

    /* Get current SRCMD_EN(rrid) */
    if (iopmp->ops_override && iopmp->ops_override->get_association_rrid_md) {
        iopmp->ops_override->get_association_rrid_md(iopmp, rrid, mds,
                                                     &is_srcmd_en_locked);
    } else {
        srcmd_fmt_0_get_association_rrid_md(iopmp, rrid, mds,
                                            &is_srcmd_en_locked);
    }

    if (is_srcmd_en_locked)
        return IOPMP_ERR_REG_IS_LOCKED;

    /* Set new MD bitmap */
    *mds |= mds_set;
    /* Clear new MD bitmap */
    *mds &= ~mds_clr;

    if (iopmp->ops_override && iopmp->ops_override->set_association_rrid_md) {
        return iopmp->ops_override->set_association_rrid_md(iopmp, rrid, mds,
                                                            lock);
    } else {
        return srcmd_fmt_0_set_association_rrid_md(iopmp, rrid, mds, lock);
    }
}

enum iopmp_error iopmp_set_md_permission(IOPMP_t *iopmp, uint32_t rrid,
                                         uint32_t mdidx, bool *r, bool *w)
{
    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_2)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (rrid >= iopmp->rrid_num || mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (iopmp->mdlck_md & ((uint64_t)1 << mdidx))
        return IOPMP_ERR_REG_IS_LOCKED;

    /* This operation is mandatory for SRCMD_FMT_2 */
    if (iopmp->ops_override && iopmp->ops_override->set_md_permission) {
        return iopmp->ops_override->set_md_permission(iopmp, rrid, mdidx, r, w);
    } else {
        return srcmd_fmt_2_set_md_permission(iopmp, rrid, mdidx, r, w);
    }
}

/**
 * \brief Write a MD's SRCMD_PERM(H) without checking the MD first
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] mdidx             The MD whose permissions are written
 * \param[in,out] cfg           The desired permissions. On
 *                              IOPMP_ERR_ILLEGAL_VALUE its srcmd_perm_val
 *                              holds the actual value
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written value does not match the
 *         actual value
 *
 * \note The caller has to have checked SRCMD_FMT, \p mdidx and MDLCK
 *       already. A driver replacing set_md_permission_multi reaches the write
 *       from here
 */
static enum iopmp_error set_md_permission_multi_nocheck(
    IOPMP_t *iopmp, uint32_t mdidx, IOPMP_SRCMD_PERM_CFG_t *cfg)
{
    if (iopmp->ops_override && iopmp->ops_override->set_md_permission_multi) {
        return iopmp->ops_override->set_md_permission_multi(iopmp, mdidx, cfg);
    } else {
        return srcmd_fmt_2_set_md_permission_multi(iopmp, mdidx, cfg);
    }
}

enum iopmp_error iopmp_set_md_permission_multi(IOPMP_t *iopmp, uint32_t mdidx,
                                               IOPMP_SRCMD_PERM_CFG_t *cfg)
{
    assert(iopmp_is_initialized(iopmp));

    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_2)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!cfg)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (iopmp->mdlck_md & ((uint64_t)1 << mdidx))
        return IOPMP_ERR_REG_IS_LOCKED;

    return set_md_permission_multi_nocheck(iopmp, mdidx, cfg);
}

void iopmp_set_srcmd_perm_cfg_nocheck(IOPMP_SRCMD_PERM_CFG_t *cfg,
                                      uint32_t rrid, bool r, bool w)
{
    uint64_t shift, mask, val;

    shift = (rrid << 1);
    mask  = (uint64_t)IOPMP_SRCMD_PERM_MASK << shift;
    val   = (((uint64_t)w << 1) | ((uint64_t)r << 0)) << shift;
    cfg->srcmd_perm_mask |= mask;
    cfg->srcmd_perm_val   = (cfg->srcmd_perm_val & ~mask) | (val & mask);
}

enum iopmp_error iopmp_set_srcmd_perm_cfg(IOPMP_SRCMD_PERM_CFG_t *cfg,
                                          uint32_t rrid, bool r, bool w)
{
    if (!cfg)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (rrid >= IOPMP_MAX_RRID_SRCMD_FMT_2)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    iopmp_set_srcmd_perm_cfg_nocheck(cfg, rrid, r, w);
    return IOPMP_OK;
}

/**
 * \brief (SPS only) Validate what every SPS setter shares, reading
 * SRCMD_EN(rrid) once
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to set permission to \p rrid
 * \param[in] mds_clr           The desired MDs to clear permission to \p rrid
 *
 * \retval IOPMP_OK if the request may be applied
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or the MD bitmaps are out
 *         of bounds
 * \retval IOPMP_ERR_REG_IS_LOCKED if a selected MD has been locked by MDLCK,
 *         or SRCMD_EN(rrid) has been locked by SRCMD_EN.l
 */
static enum iopmp_error __sps_check(IOPMP_t *iopmp, uint32_t rrid,
                                    uint64_t mds_set, uint64_t mds_clr)
{
    uint64_t valid_mds, srcmd_mds;
    bool is_srcmd_en_locked;

    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    valid_mds = GENMASK_64((iopmp->md_num - 1), 0);
    if (mds_set > valid_mds || mds_clr > valid_mds)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* One of selected MDs has been locked by MDLCK */
    if (iopmp->mdlck_md & (mds_set | mds_clr))
        return IOPMP_ERR_REG_IS_LOCKED;

    /* Get current SRCMD_EN(rrid) */
    if (iopmp->ops_override && iopmp->ops_override->get_association_rrid_md) {
        iopmp->ops_override->get_association_rrid_md(iopmp, rrid, &srcmd_mds,
                                                     &is_srcmd_en_locked);
    } else {
        srcmd_fmt_0_get_association_rrid_md(iopmp, rrid, &srcmd_mds,
                                            &is_srcmd_en_locked);
    }

    return is_srcmd_en_locked ? IOPMP_ERR_REG_IS_LOCKED : IOPMP_OK;
}

/** The SRCMD_{R|W|X} register an SPS helper is to work on */
enum iopmp_sps_perm {
    IOPMP_SPS_PERM_R,
    IOPMP_SPS_PERM_W,
    IOPMP_SPS_PERM_X,
};

/**
 * \brief (SPS only) Read SRCMD_{R|W|X}(rrid).md
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be got
 * \param[in] perm              The permission to be got
 *
 * \return SRCMD_{R|W|X}(rrid).md
 */
static uint64_t __sps_read(IOPMP_t *iopmp, uint32_t rrid,
                           enum iopmp_sps_perm perm)
{
    switch (perm) {
    case IOPMP_SPS_PERM_R:
        return ext_sps_get_srcmd_r_64_md(iopmp, rrid);
    case IOPMP_SPS_PERM_W:
        return ext_sps_get_srcmd_w_64_md(iopmp, rrid);
    default:
        return ext_sps_get_srcmd_x_64_md(iopmp, rrid);
    }
}

/**
 * \brief (SPS only) Write one of SRCMD_{R|W|X}(rrid), assuming __sps_check()
 * has already accepted \p rrid, \p mds_set and \p mds_clr
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to set permission to \p rrid
 * \param[in] mds_clr           The desired MDs to clear permission to \p rrid
 * \param[out] mds              The pointer to an integer to store WARL value of
 *                              SRCMD_{R|W|X}.md after setting
 * \param[in] perm              The permission to be set
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 */
static enum iopmp_error __sps_apply(IOPMP_t *iopmp, uint32_t rrid,
                                    uint64_t mds_set, uint64_t mds_clr,
                                    uint64_t *mds, enum iopmp_sps_perm perm)
{
    *mds = __sps_read(iopmp, rrid, perm);

    /* Set new MD bitmap */
    *mds |= mds_set;
    /* Clear new MD bitmap */
    *mds &= ~mds_clr;

    switch (perm) {
    case IOPMP_SPS_PERM_R:
        return ext_sps_set_srcmd_r_64_md(iopmp, rrid, mds);
    case IOPMP_SPS_PERM_W:
        return ext_sps_set_srcmd_w_64_md(iopmp, rrid, mds);
    default:
        return ext_sps_set_srcmd_x_64_md(iopmp, rrid, mds);
    }
}

/**
 * \brief (SPS only) Set RRID's read/write/instruction fetch permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be set
 * \param[in] mds_set           The desired MDs to set permission to \p rrid
 * \param[in] mds_clr           The desired MDs to clear permission to \p rrid
 * \param[out] mds              The pointer to an integer to store WARL value of
 *                              SRCMD_{R|W|X}.md after setting
 * \param[in] perm              The permission to be set
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid or \p mds is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 * \retval IOPMP_ERR_REG_IS_LOCKED if register has been locked by SRCMD_EN.l
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual values
 */
static enum iopmp_error __sps_set(IOPMP_t *iopmp, uint32_t rrid,
                                  uint64_t mds_set, uint64_t mds_clr,
                                  uint64_t *mds, enum iopmp_sps_perm perm)
{
    enum iopmp_error ret;

    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    ret = __sps_check(iopmp, rrid, mds_set, mds_clr);
    if (ret != IOPMP_OK)
        return ret;

    return __sps_apply(iopmp, rrid, mds_set, mds_clr, mds, perm);
}

/**
 * \brief (SPS only) Get RRID's read/write/instruction fetch permission to MDs
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] rrid              The RRID to be checked
 * \param[out] mds              Pointer to variable to output permission
 * \param[in] perm              The permission to be got
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if given \p rrid is out of bounds
 * \retval IOPMP_ERR_INVALID_PARAMETER if given \p mds is NULL
 */
static enum iopmp_error __sps_get(IOPMP_t *iopmp, uint32_t rrid, uint64_t *mds,
                                  enum iopmp_sps_perm perm)
{
    if (rrid >= iopmp->rrid_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    *mds = __sps_read(iopmp, rrid, perm);

    return IOPMP_OK;
}

enum iopmp_error iopmp_sps_set_rrid_md_read(IOPMP_t *iopmp, uint32_t rrid,
                                            uint64_t mds_set,
                                            uint64_t mds_clr,
                                            uint64_t *mds)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    return __sps_set(iopmp, rrid, mds_set, mds_clr, mds, IOPMP_SPS_PERM_R);
}

enum iopmp_error iopmp_sps_get_rrid_md_read(IOPMP_t *iopmp, uint32_t rrid,
                                            uint64_t *mds)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    return __sps_get(iopmp, rrid, mds, IOPMP_SPS_PERM_R);
}

enum iopmp_error iopmp_sps_set_rrid_md_write(IOPMP_t *iopmp, uint32_t rrid,
                                             uint64_t mds_set,
                                             uint64_t mds_clr,
                                             uint64_t *mds)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    return __sps_set(iopmp, rrid, mds_set, mds_clr, mds, IOPMP_SPS_PERM_W);
}

enum iopmp_error iopmp_sps_get_rrid_md_write(IOPMP_t *iopmp, uint32_t rrid,
                                             uint64_t *mds)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    return __sps_get(iopmp, rrid, mds, IOPMP_SPS_PERM_W);
}

enum iopmp_error iopmp_sps_set_rrid_md_insn_fetch(IOPMP_t *iopmp, uint32_t rrid,
                                                  uint64_t mds_set,
                                                  uint64_t mds_clr,
                                                  uint64_t *mds)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    return __sps_set(iopmp, rrid, mds_set, mds_clr, mds, IOPMP_SPS_PERM_X);
}

enum iopmp_error iopmp_sps_get_rrid_md_insn_fetch(IOPMP_t *iopmp, uint32_t rrid,
                                                  uint64_t *mds)
{
    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    return __sps_get(iopmp, rrid, mds, IOPMP_SPS_PERM_X);
}

enum iopmp_error iopmp_sps_set_rrid_md_rwx(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t mds_set_r,
                                           uint64_t mds_clr_r,
                                           uint64_t mds_set_w,
                                           uint64_t mds_clr_w,
                                           uint64_t mds_set_x,
                                           uint64_t mds_clr_x,
                                           uint64_t *mds_r,
                                           uint64_t *mds_w,
                                           uint64_t *mds_x)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!mds_r || !mds_w || !mds_x)
        return IOPMP_ERR_INVALID_PARAMETER;

    /*
     * The three permissions share one set of constraints, so validate their
     * union once. It reads SRCMD_EN(rrid) a single time instead of once per
     * permission, and leaves the IOPMP untouched if any of them is rejected.
     */
    ret = __sps_check(iopmp, rrid,
                      mds_set_r | mds_set_w | mds_set_x,
                      mds_clr_r | mds_clr_w | mds_clr_x);
    if (ret != IOPMP_OK)
        return ret;

    ret = __sps_apply(iopmp, rrid, mds_set_r, mds_clr_r, mds_r,
                      IOPMP_SPS_PERM_R);
    if (ret != IOPMP_OK)
        return ret;

    ret = __sps_apply(iopmp, rrid, mds_set_w, mds_clr_w, mds_w,
                      IOPMP_SPS_PERM_W);
    if (ret != IOPMP_OK)
        return ret;

    return __sps_apply(iopmp, rrid, mds_set_x, mds_clr_x, mds_x,
                       IOPMP_SPS_PERM_X);
}

enum iopmp_error iopmp_sps_get_rrid_md_rwx(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds_r, uint64_t *mds_w,
                                           uint64_t *mds_x)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (!iopmp_get_support_sps(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    ret = __sps_get(iopmp, rrid, mds_r, IOPMP_SPS_PERM_R);
    if (ret != IOPMP_OK)
        return ret;

    ret = __sps_get(iopmp, rrid, mds_w, IOPMP_SPS_PERM_W);
    if (ret != IOPMP_OK)
        return ret;

    return __sps_get(iopmp, rrid, mds_x, IOPMP_SPS_PERM_X);
}

/* The only operation with more than one implementation, and the only one
 * called for every MD in a loop, so the choice is resolved here once */
static void get_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                             uint32_t *entry_top)
{
    if (iopmp->ops_override && iopmp->ops_override->get_md_entry_top) {
        iopmp->ops_override->get_md_entry_top(iopmp, mdidx, entry_top);
    } else if (iopmp->mdcfg_fmt == IOPMP_MDCFG_FMT_0) {
        mdcfg_fmt_0_get_md_entry_top(iopmp, mdidx, entry_top);
    } else {
        mdcfg_fmt_1_2_get_md_entry_top(iopmp, mdidx, entry_top);
    }
}

static inline void __get_md_entry_association_nocheck(IOPMP_t *iopmp,
                                                      uint32_t mdidx,
                                                      uint32_t *entry_idx_start,
                                                      uint32_t *num_entry)
{
    uint32_t md_entry_top_prev, md_entry_top;

    if (mdidx) {
        get_md_entry_top(iopmp, mdidx - 1, &md_entry_top_prev);
    } else {
        md_entry_top_prev = 0;
    }
    get_md_entry_top(iopmp, mdidx, &md_entry_top);

    *entry_idx_start = md_entry_top_prev;
    /* An improperly set MDCFG table is not monotonic. Report no entry for such
     * a MD rather than underflowing the subtraction */
    *num_entry = (md_entry_top > md_entry_top_prev) ?
                 (md_entry_top - md_entry_top_prev) : 0;
}

enum iopmp_error iopmp_get_md_entry_association(IOPMP_t *iopmp, uint32_t mdidx,
                                                uint32_t *entry_idx_start,
                                                uint32_t *num_entry)
{
    assert(iopmp_is_initialized(iopmp));

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!entry_idx_start || !num_entry)
        return IOPMP_ERR_INVALID_PARAMETER;

    __get_md_entry_association_nocheck(iopmp, mdidx, entry_idx_start,
                                       num_entry);

    return IOPMP_OK;
}

/**
 * \brief Check if given MD(mdidx_start)~MD(mdidx_start + md_num) are valid MDs
 *
 * \retval 1 if MD(mdidx_start) ~ MD(mdidx_start + md_num) are valid MDs
 * \retval 0 if MD(mdidx_start) ~ MD(mdidx_start + md_num) are not valid MDs
 *
 * \note This function avoids unsigned integer overflow
 */
static bool __check_md_idx_range(IOPMP_t *iopmp, uint32_t mdidx_start,
                                 uint32_t md_num)
{
    return mdidx_start < iopmp->md_num &&
           md_num <= (iopmp->md_num - mdidx_start);
}

enum iopmp_error iopmp_set_md_entry_association_multi(IOPMP_t *iopmp,
                                                      uint32_t mdidx_start,
                                                      uint32_t *num_entries,
                                                      uint32_t md_num)
{
    enum iopmp_error ret = IOPMP_OK;    /* md_num=0 skips the loop below */
    uint32_t prev_top, this_top;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->mdcfg_fmt != IOPMP_MDCFG_FMT_0)
        return IOPMP_ERR_NOT_ALLOWED;   /* Must call iopmp_set_md_entry_num() */

    if (!num_entries)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (!__check_md_idx_range(iopmp, mdidx_start, md_num))
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Check if MDCFG(mdidx) has been locked by MDCFGLCK.f */
    if (mdidx_start < iopmp->mdcfglck_f)
        return IOPMP_ERR_REG_IS_LOCKED;

    if (mdidx_start) {
        get_md_entry_top(iopmp, mdidx_start - 1, &prev_top);
    } else {
        prev_top = 0;
    }

    for (uint32_t m = 0; m < md_num; m++) {
        this_top = prev_top + num_entries[m];
        if (this_top > iopmp->entry_num)
            return IOPMP_ERR_OUT_OF_BOUNDS;
        /* This operation is mandatory for MDCFG_FMT_0 */
        if (iopmp->ops_override && iopmp->ops_override->set_md_entry_top) {
            ret = iopmp->ops_override->set_md_entry_top(iopmp,
                                                        mdidx_start + m,
                                                        &this_top);
        } else {
            ret = mdcfg_fmt_0_set_md_entry_top(iopmp, mdidx_start + m,
                                               &this_top);
        }
        /* Return actual number of entries */
        num_entries[m] = this_top - prev_top;
        if (ret != IOPMP_OK)
            return ret;

        /* Update previous top */
        prev_top = this_top;
    }

    return ret;
}

enum iopmp_error iopmp_get_md_entry_num(IOPMP_t *iopmp, uint32_t *md_entry_num)
{
    assert(iopmp_is_initialized(iopmp));

    if (!__is_k_model(iopmp))
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!md_entry_num)
        return IOPMP_ERR_INVALID_PARAMETER;

    *md_entry_num = iopmp->md_entry_num;

    return IOPMP_OK;
}

enum iopmp_error iopmp_set_md_entry_num(IOPMP_t *iopmp, uint32_t *md_entry_num)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    if (iopmp->mdcfg_fmt != IOPMP_MDCFG_FMT_2)
        return IOPMP_ERR_NOT_ALLOWED;

    if (!md_entry_num)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (iopmp->md_entry_num == *md_entry_num)
        return IOPMP_OK;

    /* HWCFG3.md_entry_num is locked if HWCFG0.enable is 1 */
    if (iopmp->enable)
        return IOPMP_ERR_REG_IS_LOCKED;

    /* HWCFG3.md_entry_num is bits 10:4 of the register */
    if (*md_entry_num > MAX_MD_ENTRY_NUM)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Try to write new md_entry_num into IOPMP */
    if (iopmp->ops_override && iopmp->ops_override->set_md_entry_num) {
        ret = iopmp->ops_override->set_md_entry_num(iopmp, md_entry_num);
    } else {
        ret = mdcfg_fmt_2_set_md_entry_num(iopmp, md_entry_num);
    }
    iopmp->md_entry_num = *md_entry_num;    /* Update local cache */

    return ret;
}

/**
 * \brief Encode IOPMP NAPOT entry from given memory region and flags
 *
 * \param[out] entry            The entry to be output
 * \param[in] addr              Address of the memory region
 * \param[in] size              Size of the memory region
 * \param[in] hw_flags          HW flags of the entry for this memory region
 * \param[in] sw_flags          SW flags of the entry for this memory region
 * \param[in] private_data      Private data that can be used in specific model
 *
 * \return 1
 */
static int __encode_entry_pow2(struct iopmp_entry *entry,
                               uint64_t addr, uint64_t size,
                               enum iopmp_entry_flags hw_flags,
                               enum iopmp_entry_flags sw_flags,
                               uint64_t private_data)
{
    /* Encode entry_cfg */
    enum iopmp_entry_flags match;

    if (sw_flags & IOPMP_ENTRY_FORCE_OFF)
        match = IOPMP_ENTRY_A_OFF;
    else
        match = (size == 4) ? IOPMP_ENTRY_A_NA4 : IOPMP_ENTRY_A_NAPOT;

    uint32_t entry_cfg = (hw_flags | match);

    /* Encode entry_addr */
    uint64_t mask = ((uint64_t)1 << (iopmp_ctzll(size) - IOPMP_ADDR_SHIFT)) - 1;
    uint64_t entry_addr = ((addr >> IOPMP_ADDR_SHIFT) & ~mask) | (mask >> 1);

    entry->cfg = entry_cfg;
    entry->addr = entry_addr;
    entry->prient_flag =
        (sw_flags & IOPMP_ENTRY_PRIO) ? IOPMP_PRIENT_PRIORITY :
        (sw_flags & IOPMP_ENTRY_NON_PRIO) ? IOPMP_PRIENT_NON_PRIORITY :
        IOPMP_PRIENT_ANY;
    entry->private_data = private_data;

    return 1;
}

/**
 * \brief Encode IOPMP TOR entries from given memory region and flags
 *
 * \param[out] entries          The array of entry to be output
 * \param[in] addr              Address of the memory region
 * \param[in] size              Size of the memory region
 * \param[in] hw_flags          HW flags of the entry for this memory region
 * \param[in] sw_flags          SW flags of the entry for this memory region
 * \param[in] private_data      Private data that can be used in specific model
 *
 * \retval 1 if successes and the memory region is encoded as first TOR entry
 * \retval 2 if successes and the memory region is encoded as two TOR entries
 */
static int __encode_entry_tor(struct iopmp_entry *entries,
                              uint64_t addr, uint64_t size,
                              enum iopmp_entry_flags hw_flags,
                              enum iopmp_entry_flags sw_flags,
                              uint64_t private_data)
{
    uint32_t entry_cfg0;
    uint32_t entry_cfg1;
    uint64_t entry_addr0;
    uint64_t entry_addr1;

    entry_cfg0 = entry_cfg1 = hw_flags;
    if (sw_flags & IOPMP_ENTRY_FIRST_TOR) {
        entry_cfg0 |= IOPMP_ENTRY_A_TOR;
        entry_addr0 = (addr + size) >> IOPMP_ADDR_SHIFT;

        entries[0].cfg  = entry_cfg0;
        entries[0].addr = entry_addr0;
        entries[0].prient_flag =
            (sw_flags & IOPMP_ENTRY_PRIO) ? IOPMP_PRIENT_PRIORITY :
            (sw_flags & IOPMP_ENTRY_NON_PRIO) ? IOPMP_PRIENT_NON_PRIORITY :
            IOPMP_PRIENT_ANY;
        entries[0].private_data = private_data;

        return 1;
    }

    entry_cfg0 |= IOPMP_ENTRY_A_OFF;
    entry_cfg1 |= (sw_flags & IOPMP_ENTRY_FORCE_OFF) ? IOPMP_ENTRY_A_OFF :
                                                       IOPMP_ENTRY_A_TOR;
    entry_addr0 = addr >> IOPMP_ADDR_SHIFT;
    entry_addr1 = (addr + size) >> IOPMP_ADDR_SHIFT;

    entries[0].cfg  = entry_cfg0;
    entries[0].addr = entry_addr0;
    entries[0].prient_flag =
        (sw_flags & IOPMP_ENTRY_PRIO) ? IOPMP_PRIENT_PRIORITY :
        (sw_flags & IOPMP_ENTRY_NON_PRIO) ? IOPMP_PRIENT_NON_PRIORITY :
        IOPMP_PRIENT_ANY;
    entries[0].private_data = private_data;
    entries[1].cfg  = entry_cfg1;
    entries[1].addr = entry_addr1;
    entries[1].prient_flag =
        (sw_flags & IOPMP_ENTRY_PRIO) ? IOPMP_PRIENT_PRIORITY :
        (sw_flags & IOPMP_ENTRY_NON_PRIO) ? IOPMP_PRIENT_NON_PRIORITY :
        IOPMP_PRIENT_ANY;
    entries[1].private_data = private_data;

    return 2;
}

/**
 * \brief Check if given value is power of 2
 *
 * \param[in] val               The value to be checked
 *
 * \retval 1 if it is power of 2
 * \retval 0 if it is not power of 2
 */
static inline bool ispow2(uint64_t val)
{
    return val && !(val & (val - 1));
}

/**
 * \brief Check if given memory region is NAPOT
 *
 * \param[in] addr              The base address of the memory region
 * \param[in] size              The size(in bytes) of the memory region
 *
 * \retval 1 if it is NAPOT
 * \retval 0 if it is not NAPOT
 */
static inline bool is_napot(uint64_t addr, uint64_t size)
{
    uint64_t max_size;

    assert(size >= 4);

    if (ispow2(size) == false)
        return false;

    if (addr == 0)
        max_size = UINT64_MAX;
    else
        max_size = addr & -addr;

    return size <= max_size;
}

enum iopmp_error iopmp_encode_entry(IOPMP_t *iopmp, struct iopmp_entry *entries,
                                    uint32_t num_entry, uint64_t addr,
                                    uint64_t size, enum iopmp_entry_flags flags,
                                    uint64_t private_data)
{
    enum iopmp_entry_flags hw_flags, sw_flags;

    assert(iopmp_is_initialized(iopmp));

    if (!entries || !num_entry)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (size == 0)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (!IS_ALIGNED(addr, iopmp->granularity) ||
        !IS_ALIGNED(size, iopmp->granularity))
        return IOPMP_ERR_INVALID_PARAMETER;

    if ((addr >> IOPMP_ADDR_SHIFT) > iopmp->entry_addr_bits)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    hw_flags = flags & (IOPMP_ENTRY_RWX | IOPMP_ENTRY_SIE_MASK |
                        IOPMP_ENTRY_SEE_MASK);
    sw_flags = flags & IOPMP_ENTRY_SW_FLAGS_MASK;
    if (!iopmp->peis && ((hw_flags & IOPMP_ENTRY_SIE_MASK) != 0))
        return IOPMP_ERR_NOT_SUPPORTED;
    if (!iopmp->pees && ((hw_flags & IOPMP_ENTRY_SEE_MASK) != 0))
        return IOPMP_ERR_NOT_SUPPORTED;

    /* NA4 or NAPOT region */
    if (is_napot(addr, size) && !(sw_flags & IOPMP_ENTRY_FORCE_TOR))
        return __encode_entry_pow2(entries, addr, size, hw_flags, sw_flags,
                                   private_data);

    /* TOR region */
    if (!iopmp->tor_en)
        return IOPMP_ERR_NOT_SUPPORTED;

    if (!(sw_flags & IOPMP_ENTRY_FIRST_TOR) && (num_entry < 2))
        return IOPMP_ERR_NOT_ALLOWED;

    /* The lower bound of TOR entry 0 must be 0 */
    if ((sw_flags & IOPMP_ENTRY_FIRST_TOR) && (addr != 0))
        return IOPMP_ERR_NOT_ALLOWED;

    return __encode_entry_tor(entries, addr, size, hw_flags, sw_flags,
                              private_data);
}

/**
 * \brief Sanity check on priority of entries
 *
 * \param[in] iopmp             The IOPMP instance
 * \param[in] entry_array       The array of entries
 * \param[in] idx_start         The global start index of target entries
 * \param[in] num_entry         The number of entries to be checked
 *
 * \retval 1 on success
 * \retval 0 on error
 */
static bool __check_entry_priority(IOPMP_t *iopmp,
                                   const struct iopmp_entry *entry_array,
                                   uint32_t idx_start, uint32_t num_entry)
{
    uint32_t num_prient;

    num_prient = iopmp_get_prio_entry_num(iopmp);
    /*
     * Assume IOPMP has N entries.
     * The priority entries are indexed from 0 ~ (num_prient - 1).
     * The non-priority entries are indexed from num_prient ~ (N - 1).
     */
    for (uint32_t i = 0; i < num_entry; i++) {
        switch (entry_array[i].prient_flag) {
        case IOPMP_PRIENT_ANY:
            break;
        case IOPMP_PRIENT_PRIORITY:
            if ((idx_start + i) >= num_prient)
                return false;
            break;
        case IOPMP_PRIENT_NON_PRIORITY:
            if ((idx_start + i) < num_prient)
                return false;
            break;
        default:    /* GCOV_EXCL_LINE */
            break;  /* GCOV_EXCL_LINE */
        }
    }

    return true;
}

/**
 * \brief Check if given ENTRY(idx_start) ~ ENTRY(idx_start + num_entry) are
 * valid entries
 *
 * \retval 1 if ENTRY(idx_start) ~ ENTRY(idx_start + num_entry) are valid
 * \retval 0 if ENTRY(idx_start) ~ ENTRY(idx_start + num_entry) are not valid
 *
 * \note This function avoids unsigned integer overflow
 */
static bool __check_entry_idx_range(IOPMP_t *iopmp, uint32_t idx_start,
                                    uint32_t num_entry)
{
    return idx_start < iopmp->entry_num &&
           num_entry <= (iopmp->entry_num - idx_start);
}

/**
 * \brief Check everything iopmp_set_entries() refuses a range for
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] entry_array       The pointer to the entry array
 * \param[in] idx_start         The global index of the first entry
 * \param[in] num_entry         The number of entries to be written
 *
 * \retval IOPMP_OK if the range can be written
 * \retval IOPMP_ERR_INVALID_PARAMETER if \p entry_array is NULL or
 *         \p num_entry is zero
 * \retval IOPMP_ERR_OUT_OF_BOUNDS if the range exceeds the implemented
 *         entries
 * \retval IOPMP_ERR_INVALID_PRIORITY if an entry is on the wrong side of
 *         HWCFG2.prio_entry
 * \retval IOPMP_ERR_REG_IS_LOCKED if an entry has been locked by ENTRYLCK.f
 *
 * \note Separate from the write so that a caller with more to write, such as
 *       iopmp_set_entries_with_md_permission(), can refuse the whole range
 *       before it writes any part of it
 */
static enum iopmp_error check_set_entries(
    IOPMP_t *iopmp, const struct iopmp_entry *entry_array,
    uint32_t idx_start, uint32_t num_entry)
{
    if (!entry_array || !num_entry) {
        return IOPMP_ERR_INVALID_PARAMETER;
    }

    if (!__check_entry_idx_range(iopmp, idx_start, num_entry)) {
        return IOPMP_ERR_OUT_OF_BOUNDS;
    }

    /* Sanity check priority entries */
    if (!__check_entry_priority(iopmp, entry_array, idx_start, num_entry)) {
        return IOPMP_ERR_INVALID_PRIORITY;
    }

    /* Check if desired entries have been locked by ENTRYLCK.f */
    if (idx_start < iopmp->entrylck_f) {
        return IOPMP_ERR_REG_IS_LOCKED;
    }

    return IOPMP_OK;
}

/**
 * \brief Write IOPMP entries without checking the range first
 *
 * \param[in] iopmp             The IOPMP instance to be written
 * \param[in] entry_array       The pointer to the entry array
 * \param[in] idx_start         The global index of the first entry
 * \param[in] num_entry         The number of entries to be written
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if a written entry does not match the
 *         actual value
 *
 * \note The caller has to have passed the range through check_set_entries()
 *       already.
 */
static enum iopmp_error set_entries_nocheck(
    IOPMP_t *iopmp, const struct iopmp_entry *entry_array,
    uint32_t idx_start, uint32_t num_entry)
{
    if (iopmp->ops_override && iopmp->ops_override->set_entries) {
        return iopmp->ops_override->set_entries(iopmp, entry_array,
                                                idx_start, num_entry);
    } else {
        return generic_set_entries(iopmp, entry_array, idx_start, num_entry);
    }
}

enum iopmp_error iopmp_set_entries(IOPMP_t *iopmp,
                                   const struct iopmp_entry *entry_array,
                                   uint32_t idx_start, uint32_t num_entry)
{
    enum iopmp_error ret;

    assert(iopmp_is_initialized(iopmp));

    ret = check_set_entries(iopmp, entry_array, idx_start, num_entry);
    if (ret != IOPMP_OK) {
        return ret;
    }

    return set_entries_nocheck(iopmp, entry_array, idx_start, num_entry);
}

enum iopmp_error iopmp_set_entries_with_md_permission(
    IOPMP_t *iopmp, const struct iopmp_entry *entry_array,
    uint32_t idx_start, uint32_t num_entry)
{
    enum iopmp_error ret;
    uint64_t md_mask;

    assert(iopmp_is_initialized(iopmp));

    /* Only K=1 gives every entry a MD whose SRCMD_PERM(H) it can carry */
    if (iopmp->srcmd_fmt != IOPMP_SRCMD_FMT_2 ||
        iopmp->mdcfg_fmt != IOPMP_MDCFG_FMT_1 ||
        iopmp->md_entry_num != 0) {
        return IOPMP_ERR_NOT_SUPPORTED;
    }

    /*
     * Nothing may be written until every rule both halves apply has passed.
     * Otherwise a range that iopmp_set_entries() goes on to refuse would have
     * left SRCMD_PERM(H) granting permissions on entries never written.
     */
    ret = check_set_entries(iopmp, entry_array, idx_start, num_entry);
    if (ret != IOPMP_OK) {
        return ret;
    }

    /* Entry i is the only entry of MD i, so it needs a MD to belong to */
    if (idx_start + num_entry > iopmp->md_num) {
        return IOPMP_ERR_OUT_OF_BOUNDS;
    }

    /* MDLCK locks the SRCMD_PERM(H) of a MD */
    md_mask = (((uint64_t)1 << num_entry) - 1) << idx_start;
    if (iopmp->mdlck_md & md_mask) {
        return IOPMP_ERR_REG_IS_LOCKED;
    }

    for (uint32_t i = 0; i < num_entry; i++) {
        IOPMP_SRCMD_PERM_CFG_t cfg;

        IOPMP_SRCMD_PERM_CFG_SET_DIRECT(&cfg, UINT64_MAX,
                                        entry_array[i].private_data);
        ret = set_md_permission_multi_nocheck(iopmp, idx_start + i, &cfg);
        if (ret != IOPMP_OK) {
            return ret;
        }
    }

    return set_entries_nocheck(iopmp, entry_array, idx_start, num_entry);
}

enum iopmp_error iopmp_set_entries_to_md(IOPMP_t *iopmp, uint32_t mdidx,
                                         const struct iopmp_entry *entry_array,
                                         uint32_t idx_start,
                                         uint32_t num_entry)
{
    uint32_t md_entry_idx_start, md_num_entry;

    assert(iopmp_is_initialized(iopmp));

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Get start index and number of entries this MD has */
    __get_md_entry_association_nocheck(iopmp, mdidx, &md_entry_idx_start,
                                       &md_num_entry);

    if (num_entry > md_num_entry)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (idx_start >= md_num_entry)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    return iopmp_set_entries(iopmp, entry_array, md_entry_idx_start + idx_start,
                             num_entry);
}

enum iopmp_error iopmp_get_entries(IOPMP_t *iopmp,
                                   struct iopmp_entry *entry_array,
                                   uint32_t idx_start, uint32_t num_entry)
{
    assert(iopmp_is_initialized(iopmp));

    if (!entry_array || !num_entry)
        return IOPMP_ERR_INVALID_PARAMETER;

    if (!__check_entry_idx_range(iopmp, idx_start, num_entry))
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Read IOPMP entries from IOPMP registers */
    if (iopmp->ops_override && iopmp->ops_override->get_entries) {
        iopmp->ops_override->get_entries(iopmp, entry_array, idx_start,
                                         num_entry);
    } else {
        generic_get_entries(iopmp, entry_array, idx_start, num_entry);
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_get_entries_from_md(IOPMP_t *iopmp, uint32_t mdidx,
                                           struct iopmp_entry *entry_array,
                                           uint32_t idx_start,
                                           uint32_t num_entry)
{
    uint32_t md_entry_idx_start, md_num_entry;

    assert(iopmp_is_initialized(iopmp));

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Get start index and number of entries this MD has */
    __get_md_entry_association_nocheck(iopmp, mdidx, &md_entry_idx_start,
                                       &md_num_entry);

    if (num_entry > md_num_entry)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (idx_start >= md_num_entry)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    return iopmp_get_entries(iopmp, entry_array, md_entry_idx_start + idx_start,
                             num_entry);
}

enum iopmp_error iopmp_clear_entries(IOPMP_t *iopmp, uint32_t idx_start,
                                     uint32_t num_entry)
{
    assert(iopmp_is_initialized(iopmp));

    if (num_entry > iopmp->entry_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!__check_entry_idx_range(iopmp, idx_start, num_entry))
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Check if desired entries have been locked by ENTRYLCK.f */
    if (idx_start < iopmp->entrylck_f)
        return IOPMP_ERR_REG_IS_LOCKED;

    if (iopmp->ops_override && iopmp->ops_override->clear_entries) {
        iopmp->ops_override->clear_entries(iopmp, idx_start, num_entry);
    } else {
        generic_clear_entries(iopmp, idx_start, num_entry);
    }

    return IOPMP_OK;
}

enum iopmp_error iopmp_clear_entries_in_md(IOPMP_t *iopmp, uint32_t mdidx)
{
    uint32_t md_entry_idx_start, md_num_entry;

    assert(iopmp_is_initialized(iopmp));

    if (mdidx >= iopmp->md_num)
        return IOPMP_ERR_OUT_OF_BOUNDS;

    /* Get start index and number of entries this MD has */
    __get_md_entry_association_nocheck(iopmp, mdidx, &md_entry_idx_start,
                                       &md_num_entry);

    return iopmp_clear_entries(iopmp, md_entry_idx_start, md_num_entry);
}

enum iopmp_error iopmp_entries_get_belong_md(IOPMP_t *iopmp, uint32_t idx_start,
                                             uint32_t num_entry, uint64_t *mds)
{
    uint32_t idx_end, md_base, md_top;
    uint64_t __mds;

    assert(iopmp_is_initialized(iopmp));

    if (!__check_entry_idx_range(iopmp, idx_start, num_entry))
        return IOPMP_ERR_OUT_OF_BOUNDS;

    if (!mds)
        return IOPMP_ERR_INVALID_PARAMETER;

    /* An empty index range belongs to no MD */
    if (!num_entry) {
        *mds = 0;
        return IOPMP_OK;
    }

    idx_end = idx_start + num_entry;
    md_base = 0;
    __mds = 0;
    /* The tops are monotonically increasing, so the top of MD(m-1) is the base
     * of MD(m) and a single walk reads each top exactly once */
    for (uint32_t m = 0; m < iopmp->md_num; m++) {
        get_md_entry_top(iopmp, m, &md_top);
        /* [md_base, md_top) overlaps [idx_start, idx_end). An MD owning no
         * entry can not be hit */
        if (md_base < md_top && md_base < idx_end && idx_start < md_top)
            __mds |= (uint64_t)1 << m;
        md_base = md_top;
    }

    *mds = __mds;
    return IOPMP_OK;
}
