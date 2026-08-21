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

/*
 * A worked example of a vendor driver, built with
 * CFG_IOPMP_DRV_VENDOR_EXAMPLE=y. It is not part of the default build and
 * nothing in libiopmp refers to it: copy it, rename it, and register it in
 * src/objects.mk the way this one is.
 *
 * libiopmp implements every operation itself and calls it by name, so a
 * conforming IOPMP needs no driver at all. Write one when the hardware
 * disagrees with the specification libiopmp targets, which in practice means
 * silicon built against an older draft of it.
 */

#include "libiopmp_def.h"

#include "iopmp_drv_common.h"

/*
 * The implementation ID this driver claims, read from the IMPLEMENTATION
 * register. iopmp_init() matches on it, so the value has to be one the
 * application passes and no other driver uses.
 */
#define VENDOR_EXAMPLE_IMPID            0xA5A5A5A5U

/*
 * The difference this example stands in for is a register that moved between
 * drafts: this silicon puts ENTRYLCK somewhere else. The fields and the rules
 * are the ones chapter 4 gives, so the override is libiopmp's own sequence
 * against the address the hardware uses.
 */
#define VENDOR_ENTRYLCK_BASE            0x00c0U
    #define VENDOR_ENTRYLCK_L_SHIFT     0
    #define VENDOR_ENTRYLCK_L_MASK      GENMASK_32(0U, 0U)
    #define VENDOR_ENTRYLCK_F_SHIFT     1
    #define VENDOR_ENTRYLCK_F_MASK      GENMASK_32(16U, 1U)

static enum iopmp_error vendor_example_lock_entries(IOPMP_t *iopmp,
                                                    uint32_t *entry_num,
                                                    bool lock)
{
    uint32_t entrylck;
    uint32_t requested = *entry_num;

    entrylck = MAKE_FIELD_32(lock, VENDOR_ENTRYLCK_L) |
               MAKE_FIELD_32(requested, VENDOR_ENTRYLCK_F);
    io_write32(iopmp->addr + VENDOR_ENTRYLCK_BASE, entrylck);

    /* ENTRYLCK.f is WARL here too, so read it back and report what stuck */
    entrylck = io_read32(iopmp->addr + VENDOR_ENTRYLCK_BASE);
    *entry_num = EXTRACT_FIELD(entrylck, VENDOR_ENTRYLCK_F);

    return requested == *entry_num ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

/*
 * Only the operations that differ. Everything left NULL keeps libiopmp's
 * implementation.
 */
static const struct iopmp_operations_override vendor_example_ops = {
    .lock_entries = vendor_example_lock_entries,
};

const struct iopmp_driver iopmp_drv_vendor_example;

static enum iopmp_error vendor_example_init(IOPMP_t *iopmp, uintptr_t addr)
{
    /*
     * Let libiopmp bring the instance up the standard way, with the table in
     * place from the start. It is installed per instance, so a conforming
     * IOPMP elsewhere in the same system still reaches the built-in
     * operations. A driver whose hardware cannot be initialized this way
     * writes its own initialization instead of calling this.
     */
    return iopmp_drv_init_common(iopmp, addr,
                                 iopmp_drv_vendor_example.srcmd_fmt,
                                 iopmp_drv_vendor_example.mdcfg_fmt,
                                 &vendor_example_ops);
}

const struct iopmp_driver iopmp_drv_vendor_example = {
    .srcmd_fmt = IOPMP_SRCMD_FMT_0,
    .mdcfg_fmt = IOPMP_MDCFG_FMT_0,
    .impid = VENDOR_EXAMPLE_IMPID,
    .init = vendor_example_init,
};
