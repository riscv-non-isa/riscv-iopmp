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

#include "libiopmp_def.h"

#include "iopmp_drv_common.h"

/* K=1 makes every entry its own MD, so SRCMD_PERM(H) is written with it */
static struct iopmp_operations_override srcmd_fmt_2_mdcfg_fmt_1_k1_ops = {
    .set_entries = srcmd_fmt_2_mdcfg_fmt_1_md_entry_num_0_set_entries,
};

const struct iopmp_driver iopmp_drv_srcmd_fmt_2_mdcfg_fmt_1;

static enum iopmp_error
iopmp_drv_srcmd_fmt_2_mdcfg_fmt_1_init(IOPMP_t *iopmp, uintptr_t addr)
{
    enum iopmp_error ret;

    ret = iopmp_drv_init_common(iopmp, addr,
                                iopmp_drv_srcmd_fmt_2_mdcfg_fmt_1.srcmd_fmt,
                                iopmp_drv_srcmd_fmt_2_mdcfg_fmt_1.mdcfg_fmt);
    if (ret == IOPMP_OK && iopmp->md_entry_num == 0) {
        iopmp->ops_override = &srcmd_fmt_2_mdcfg_fmt_1_k1_ops;
    }

    return ret;
}

const struct iopmp_driver iopmp_drv_srcmd_fmt_2_mdcfg_fmt_1 = {
    .srcmd_fmt = IOPMP_SRCMD_FMT_2,
    .mdcfg_fmt = IOPMP_MDCFG_FMT_1,
    .impid = IOPMP_IMPID_NOT_SPECIFIED,
    .init = iopmp_drv_srcmd_fmt_2_mdcfg_fmt_1_init,
};
