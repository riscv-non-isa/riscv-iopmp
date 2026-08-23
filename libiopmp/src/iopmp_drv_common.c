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

#include "libiopmp.h"
#include "libiopmp_def.h"

#include "iopmp_drv_common.h"

#ifdef ENABLE_IO_WEAK_FUNCTIONS
/* GCOVR_EXCL_START */
__attribute__((weak)) uint32_t io_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

__attribute__((weak)) void io_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}
/* GCOVR_EXCL_STOP */
#endif

/******************************************************************************/
/* Memory-mapped offsets for standard IOPMP                                   */
/******************************************************************************/
/* INFO Registers */
#define IOPMP_VERSION_BASE                      0x0000U
    #define IOPMP_VERSION_VENDOR_SHIFT          0U
    #define IOPMP_VERSION_VENDOR_MASK           GENMASK_32(23U, 0U)
    #define IOPMP_VERSION_SPECVER_SHIFT         24U
    #define IOPMP_VERSION_SPECVER_MASK          GENMASK_32(31U, 24U)

#define IOPMP_IMPLEMENTATION_BASE               0x0004U
    #define IOPMP_IMPLEMENTATION_IMPID_SHIFT    0U
    #define IOPMP_IMPLEMENTATION_IMPID_MASK     GENMASK_32(31U, 0U)

#define IOPMP_HWCFG0_BASE                       0x0008U
    #define IOPMP_HWCFG0_ENABLE_SHIFT           0U
    #define IOPMP_HWCFG0_ENABLE_MASK            GENMASK_32(0U, 0U)
    #define IOPMP_HWCFG0_HWCFG2_EN_SHIFT        1U
    #define IOPMP_HWCFG0_HWCFG2_EN_MASK         GENMASK_32(1U, 1U)
    #define IOPMP_HWCFG0_HWCFG3_EN_SHIFT        2U
    #define IOPMP_HWCFG0_HWCFG3_EN_MASK         GENMASK_32(2U, 2U)
    #define IOPMP_HWCFG0_NO_ERR_REC_SHIFT       23U
    #define IOPMP_HWCFG0_NO_ERR_REC_MASK        GENMASK_32(23U, 23U)
    #define IOPMP_HWCFG0_MD_NUM_SHIFT           24U
    #define IOPMP_HWCFG0_MD_NUM_MASK            GENMASK_32(29U, 24U)
    #define IOPMP_HWCFG0_ADDRH_EN_SHIFT         30U
    #define IOPMP_HWCFG0_ADDRH_EN_MASK          GENMASK_32(30U, 30U)
    #define IOPMP_HWCFG0_TOR_EN_SHIFT           31U
    #define IOPMP_HWCFG0_TOR_EN_MASK            GENMASK_32(31U, 31U)

#define IOPMP_HWCFG1_BASE                       0x000CU
    #define IOPMP_HWCFG1_RRID_NUM_SHIFT         0U
    #define IOPMP_HWCFG1_RRID_NUM_MASK          GENMASK_32(15U, 0U)
    #define IOPMP_HWCFG1_ENTRY_NUM_SHIFT        16U
    #define IOPMP_HWCFG1_ENTRY_NUM_MASK         GENMASK_32(31U, 16U)

#define IOPMP_HWCFG2_BASE                       0x0010U
    #define IOPMP_HWCFG2_PRIO_ENTRY_SHIFT       0U
    #define IOPMP_HWCFG2_PRIO_ENTRY_MASK        GENMASK_32(15U, 0U)
    #define IOPMP_HWCFG2_PRIO_ENT_PROG_SHIFT    16U
    #define IOPMP_HWCFG2_PRIO_ENT_PROG_MASK     GENMASK_32(16U, 16U)
    #define IOPMP_HWCFG2_NON_PRIO_EN_SHIFT      17U
    #define IOPMP_HWCFG2_NON_PRIO_EN_MASK       GENMASK_32(17U, 17U)
    #define IOPMP_HWCFG2_MSI_EN_SHIFT           26U
    #define IOPMP_HWCFG2_MSI_EN_MASK            GENMASK_32(26U, 26U)
    #define IOPMP_HWCFG2_PEIS_SHIFT             27U
    #define IOPMP_HWCFG2_PEIS_MASK              GENMASK_32(27U, 27U)
    #define IOPMP_HWCFG2_PEES_SHIFT             28U
    #define IOPMP_HWCFG2_PEES_MASK              GENMASK_32(28U, 28U)
    #define IOPMP_HWCFG2_SPS_EN_SHIFT           29U
    #define IOPMP_HWCFG2_SPS_EN_MASK            GENMASK_32(29U, 29U)
    #define IOPMP_HWCFG2_STALL_EN_SHIFT         30U
    #define IOPMP_HWCFG2_STALL_EN_MASK          GENMASK_32(30U, 30U)
    #define IOPMP_HWCFG2_MFR_EN_SHIFT           31U
    #define IOPMP_HWCFG2_MFR_EN_MASK            GENMASK_32(31U, 31U)

#define IOPMP_HWCFG3_BASE                       0x0014U
    #define IOPMP_HWCFG3_MDCFG_FMT_SHIFT        0U
    #define IOPMP_HWCFG3_MDCFG_FMT_MASK         GENMASK_32(1U, 0U)
    #define IOPMP_HWCFG3_SRCMD_FMT_SHIFT        2U
    #define IOPMP_HWCFG3_SRCMD_FMT_MASK         GENMASK_32(3U, 2U)
    #define IOPMP_HWCFG3_MD_ENTRY_NUM_SHIFT     4U
    #define IOPMP_HWCFG3_MD_ENTRY_NUM_MASK      GENMASK_32(10U, 4U)
    #define IOPMP_HWCFG3_XINR_SHIFT             11U
    #define IOPMP_HWCFG3_XINR_MASK              GENMASK_32(11U, 11U)
    #define IOPMP_HWCFG3_NO_X_SHIFT             12U
    #define IOPMP_HWCFG3_NO_X_MASK              GENMASK_32(12U, 12U)
    #define IOPMP_HWCFG3_NO_W_SHIFT             13U
    #define IOPMP_HWCFG3_NO_W_MASK              GENMASK_32(13U, 13U)
    #define IOPMP_HWCFG3_RRID_TRANSL_EN_SHIFT   14U
    #define IOPMP_HWCFG3_RRID_TRANSL_EN_MASK    GENMASK_32(14U, 14U)
    #define IOPMP_HWCFG3_RRID_TRANSL_PROG_SHIFT 15U
    #define IOPMP_HWCFG3_RRID_TRANSL_PROG_MASK  GENMASK_32(15U, 15U)
    #define IOPMP_HWCFG3_RRID_TRANSL_SHIFT      16U
    #define IOPMP_HWCFG3_RRID_TRANSL_MASK       GENMASK_32(31U, 16U)

#define IOPMP_HWCFG_USER_BASE                   0x0028U
    #define IOPMP_HWCFG_USER_SHIFT              0U
    #define IOPMP_HWCFG_USER_MASK               GENMASK_32(31U, 0U)

#define IOPMP_ENTRY_OFFSET_BASE                 0x002CU
    #define IOPMP_ENTRYOFFSET_SHIFT             0U
    #define IOPMP_ENTRYOFFSET_MASK              GENMASK_32(31U, 0U)

/* Programming Protection Registers */
#define IOPMP_MDSTALL_BASE                      0x0030U
    #define IOPMP_MDSTALL_EXEMPT_SHIFT          0U
    #define IOPMP_MDSTALL_EXEMPT_MASK           GENMASK_32(0U, 0U)
    #define IOPMP_MDSTALL_IS_BUSY_SHIFT         0U
    #define IOPMP_MDSTALL_IS_BUSY_MASK          IOPMP_MDSTALL_EXEMPT_MASK
    #define IOPMP_MDSTALL_MD_SHIFT              1U
    #define IOPMP_MDSTALL_MD_MASK               GENMASK_32(31U, 1U)

#define IOPMP_MDSTALLH_BASE                     0x0034U
    #define IOPMP_MDSTALLH_MDH_SHIFT            0U
    #define IOPMP_MDSTALLH_MDH_MASK             GENMASK_32(31U, 0U)

#define IOPMP_RRIDSCP_BASE                      0x0038U
    #define IOPMP_RRIDSCP_RRID_SHIFT            0U
    #define IOPMP_RRIDSCP_RRID_MASK             GENMASK_32(15U, 0U)
    #define IOPMP_RRIDSCP_OP_SHIFT              30U
    #define IOPMP_RRIDSCP_OP_MASK               GENMASK_32(31U, 30U)
    #define IOPMP_RRIDSCP_STAT_SHIFT            IOPMP_RRIDSCP_OP_SHIFT
    #define IOPMP_RRIDSCP_STAT_MASK             IOPMP_RRIDSCP_OP_MASK

/* Configuration Protection Registers */
#define IOPMP_MDLCK_BASE                        0x0040U
    #define IOPMP_MDLCK_L_SHIFT                 0U
    #define IOPMP_MDLCK_L_MASK                  GENMASK_32(0U, 0U)
    #define IOPMP_MDLCK_MD_SHIFT                1U
    #define IOPMP_MDLCK_MD_MASK                 GENMASK_32(31U, 1U)

#define IOPMP_MDLCKH_BASE                       0x0044U
    #define IOPMP_MDLCKH_MDH_SHIFT              0U
    #define IOPMP_MDLCKH_MDH_MASK               GENMASK_32(31U, 0U)

#define IOPMP_MDCFGLCK_BASE                     0x0048U
    #define IOPMP_MDCFGLCK_L_SHIFT              0U
    #define IOPMP_MDCFGLCK_L_MASK               GENMASK_32(0U, 0U)
    #define IOPMP_MDCFGLCK_F_SHIFT              1U
    #define IOPMP_MDCFGLCK_F_MASK               GENMASK_32(7U, 1U)

#define IOPMP_ENTRYLCK_BASE                     0x004CU
    #define IOPMP_ENTRYLCK_L_SHIFT              0U
    #define IOPMP_ENTRYLCK_L_MASK               GENMASK_32(0U, 0U)
    #define IOPMP_ENTRYLCK_F_SHIFT              1U
    #define IOPMP_ENTRYLCK_F_MASK               GENMASK_32(16U, 1U)

/* Error Capture Registers */
#define IOPMP_ERR_CFG_BASE                      0x0060U
    #define IOPMP_ERR_CFG_L_SHIFT               0U
    #define IOPMP_ERR_CFG_L_MASK                GENMASK_32(0U, 0U)
    #define IOPMP_ERR_CFG_IE_SHIFT              1U
    #define IOPMP_ERR_CFG_IE_MASK               GENMASK_32(1U, 1U)
    #define IOPMP_ERR_CFG_RS_SHIFT              2U
    #define IOPMP_ERR_CFG_RS_MASK               GENMASK_32(2U, 2U)
    #define IOPMP_ERR_CFG_MSI_SEL_SHIFT         3U
    #define IOPMP_ERR_CFG_MSI_SEL_MASK          GENMASK_32(3U, 3U)
    #define IOPMP_ERR_CFG_STALL_VIO_EN_SHIFT    4U
    #define IOPMP_ERR_CFG_STALL_VIO_EN_MASK     GENMASK_32(4U, 4U)
    #define IOPMP_ERR_CFG_MSIDATA_SHIFT         8U
    #define IOPMP_ERR_CFG_MSIDATA_MASK          GENMASK_32(18U, 8U)

#define IOPMP_ERR_INFO_BASE                         0x0064U
    #define IOPMP_ERR_INFO_V_SHIFT                  0U
    #define IOPMP_ERR_INFO_V_MASK                   GENMASK_32(0U, 0U)
    #define IOPMP_ERR_INFO_TTYPE_SHIFT              1U
    #define IOPMP_ERR_INFO_TTYPE_MASK               GENMASK_32(2U, 1U)
    #define IOPMP_ERR_INFO_MSI_WERR_SHIFT           3U
    #define IOPMP_ERR_INFO_MSI_WERR_MASK            GENMASK_32(3U, 3U)
    #define IOPMP_ERR_INFO_ETYPE_SHIFT              4U
    #define IOPMP_ERR_INFO_ETYPE_MASK               GENMASK_32(7U, 4U)
    #define IOPMP_ERR_INFO_SVC_SHIFT                8U
    #define IOPMP_ERR_INFO_SVC_MASK                 GENMASK_32(8U, 8U)

#define IOPMP_ERR_REQADDR_BASE                      0x0068U
    #define IOPMP_ERR_REQADDR_SHIFT                 0U
    #define IOPMP_ERR_REQADDR_MASK                  GENMASK_32(31U, 0U)

#define IOPMP_ERR_REQADDRH_BASE                     0x006CU
    #define IOPMP_ERR_REQADDRH_SHIFT                0U
    #define IOPMP_ERR_REQADDRH_MASK                 GENMASK_32(31U, 0U)

#define IOPMP_ERR_REQID_BASE                        0x0070U
    #define IOPMP_ERR_REQID_RRID_SHIFT              0U
    #define IOPMP_ERR_REQID_RRID_MASK               GENMASK_32(15U, 0U)
    #define IOPMP_ERR_REQID_EID_SHIFT               16U
    #define IOPMP_ERR_REQID_EID_MASK                GENMASK_32(31U, 16U)

#define IOPMP_ERR_MFR_BASE                          0x0074U
    #define IOPMP_ERR_MFR_SVW_SHIFT                 0U
    #define IOPMP_ERR_MFR_SVW_MASK                  GENMASK_32(15U, 0U)
    #define IOPMP_ERR_MFR_SVI_SHIFT                 16U
    #define IOPMP_ERR_MFR_SVI_MASK                  GENMASK_32(27U, 16U)
    #define IOPMP_ERR_MFR_SVS_SHIFT                 31U
    #define IOPMP_ERR_MFR_SVS_MASK                  GENMASK_32(31U, 31U)

#define IOPMP_ERR_MSIADDR_BASE                      0x0078U

#define IOPMP_ERR_MSIADDRH_BASE                     0x007CU

#define IOPMP_ERR_USER_BASE                         0x0080U
    #define IOPMP_ERR_USER_USER_SHIFT               0U
    #define IOPMP_ERR_USER_USER_MASK                GENMASK_32(31U, 0U)

/* MDCFG Table */
#define IOPMP_MDCFG_BASE                            0x0800U
    #define IOPMP_MDCFG_T_SHIFT                     0U
    #define IOPMP_MDCFG_T_MASK                      GENMASK_32(15U, 0U)

#define IOPMP_MDCFG_STRIDE                          0x4U

/* SRCMD Table Registers */
#define IOPMP_SRCMD_EN_BASE                         0x1000U
    #define IOPMP_SRCMD_EN_L_SHIFT                  0U
    #define IOPMP_SRCMD_EN_L_MASK                   GENMASK_32(0U, 0U)
    #define IOPMP_SRCMD_EN_MD_SHIFT                 1U
    #define IOPMP_SRCMD_EN_MD_MASK                  GENMASK_32(31U, 1U)

#define IOPMP_SRCMD_PERM_BASE                       IOPMP_SRCMD_EN_BASE

#define IOPMP_SRCMD_ENH_BASE                        0x1004U
    #define IOPMP_SRCMD_ENH_MDH_SHIFT               0U
    #define IOPMP_SRCMD_ENH_MDH_MASK                GENMASK_32(31U, 0U)

#define IOPMP_SRCMD_PERMH_BASE                      IOPMP_SRCMD_ENH_BASE

#define IOPMP_SRCMD_R_BASE                          0x1008U
    #define IOPMP_SRCMD_R_MD_SHIFT                  1U
    #define IOPMP_SRCMD_R_MD_MASK                   GENMASK_32(31U, 1U)

#define IOPMP_SRCMD_RH_BASE                         0x100CU
    #define IOPMP_SRCMD_RH_MDH_SHIFT                0U
    #define IOPMP_SRCMD_RH_MDH_MASK                 GENMASK_32(31U, 0U)

#define IOPMP_SRCMD_W_BASE                          0x1010U
    #define IOPMP_SRCMD_W_MD_SHIFT                  1U
    #define IOPMP_SRCMD_W_MD_MASK                   GENMASK_32(31U, 1U)

#define IOPMP_SRCMD_WH_BASE                         0x1014U
    #define IOPMP_SRCMD_WH_MDH_SHIFT                0U
    #define IOPMP_SRCMD_WH_MDH_MASK                 GENMASK_32(31U, 0U)

#define IOPMP_SRCMD_X_BASE                          0x1018U
    #define IOPMP_SRCMD_X_MD_SHIFT                  1U
    #define IOPMP_SRCMD_X_MD_MASK                   GENMASK_32(31U, 1U)

#define IOPMP_SRCMD_XH_BASE                         0x101CU
    #define IOPMP_SRCMD_XH_MDH_SHIFT                0U
    #define IOPMP_SRCMD_XH_MDH_MASK                 GENMASK_32(31U, 0U)

#define IOPMP_SRCMD_STRIDE                          0x0020U

/* Entry Array Registers */
#define IOPMP_ENTRY_ADDR_BASE                       0x0000U
#define IOPMP_ENTRY_ADDRH_BASE                      0x0004U
#define IOPMP_ENTRY_CFG_BASE                        0x0008U
#define IOPMP_ENTRY_USER_CFG_BASE                   0x000CU
#define IOPMP_ENTRY_STRIDE                          0x0010U

/******************************************************************************/
/* Helper macros                                                              */
/******************************************************************************/
/* Helper functions to get base address of MDCFG registers */
static inline uintptr_t get_addr_of_mdcfg(IOPMP_t *iopmp, uint32_t mdidx)
{
    return iopmp->addr + IOPMP_MDCFG_BASE + (mdidx * IOPMP_MDCFG_STRIDE);
}

/* Helper functions to get base address of SRCMD registers */
#define DECLARE_FUNC_GET_ADDR_OF_SRCMD(name, base)                      \
static inline uintptr_t get_addr_of_ ## name(IOPMP_t *iopmp,            \
                                             uint32_t idx)              \
{                                                                       \
    return iopmp->addr + (base) + ((idx) * IOPMP_SRCMD_STRIDE);         \
}
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_en,    IOPMP_SRCMD_EN_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_enh,   IOPMP_SRCMD_ENH_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_perm,  IOPMP_SRCMD_PERM_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_permh, IOPMP_SRCMD_PERMH_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_r,     IOPMP_SRCMD_R_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_rh,    IOPMP_SRCMD_RH_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_w,     IOPMP_SRCMD_W_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_wh,    IOPMP_SRCMD_WH_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_x,     IOPMP_SRCMD_X_BASE);
DECLARE_FUNC_GET_ADDR_OF_SRCMD(srcmd_xh,    IOPMP_SRCMD_XH_BASE);

/* Helper functions to read low 32-bit value from SRCMD registers */
#define DECLARE_FUNC_READ_SRCMD_L(name)                                 \
static uint32_t read_ ## name(IOPMP_t *iopmp, uint32_t idx)             \
{                                                                       \
    return io_read32(get_addr_of_##name(iopmp, idx));                   \
}
DECLARE_FUNC_READ_SRCMD_L(srcmd_en);
DECLARE_FUNC_READ_SRCMD_L(srcmd_perm);
DECLARE_FUNC_READ_SRCMD_L(srcmd_r);
DECLARE_FUNC_READ_SRCMD_L(srcmd_w);
DECLARE_FUNC_READ_SRCMD_L(srcmd_x);

/* Helper functions to read high 32-bit value from SRCMD registers */
#define DECLARE_FUNC_READ_SRCMD_H(name, condition)                      \
static uint32_t read_ ## name(IOPMP_t *iopmp, uint32_t idx)             \
{                                                                       \
    if (condition) {                                                    \
        return io_read32(get_addr_of_##name(iopmp, idx));               \
    }                                                                   \
    return 0;                                                           \
}
DECLARE_FUNC_READ_SRCMD_H(srcmd_enh,   (iopmp->md_num > 31U));
DECLARE_FUNC_READ_SRCMD_H(srcmd_permh, (iopmp->rrid_num > 16U));
DECLARE_FUNC_READ_SRCMD_H(srcmd_rh,    (iopmp->md_num > 31U));
DECLARE_FUNC_READ_SRCMD_H(srcmd_wh,    (iopmp->md_num > 31U));
DECLARE_FUNC_READ_SRCMD_H(srcmd_xh,    (iopmp->md_num > 31U));

/* Helper functions to read 64-bit value from SRCMD registers */
#define DECLARE_FUNC_READ_SRCMD_64(name)                                \
static uint64_t read_ ## name ## _64(IOPMP_t *iopmp, uint32_t idx)      \
{                                                                       \
    uint32_t val_##name    = read_##name(iopmp, idx);                   \
    uint32_t val_##name##h = read_##name##h(iopmp, idx);                \
    return ((uint64_t)val_##name##h << 32) | val_##name;                \
}
DECLARE_FUNC_READ_SRCMD_64(srcmd_en);
DECLARE_FUNC_READ_SRCMD_64(srcmd_perm);
DECLARE_FUNC_READ_SRCMD_64(srcmd_r);
DECLARE_FUNC_READ_SRCMD_64(srcmd_w);
DECLARE_FUNC_READ_SRCMD_64(srcmd_x);

/* Helper functions to write low 32-bit value into SRCMD registers */
#define DECLARE_FUNC_WRITE_SRCMD_L(name)                                \
static void write_ ## name(IOPMP_t *iopmp, uint32_t idx, uint32_t val)  \
{                                                                       \
    io_write32(get_addr_of_##name(iopmp, idx), val);                    \
}
DECLARE_FUNC_WRITE_SRCMD_L(srcmd_en);
DECLARE_FUNC_WRITE_SRCMD_L(srcmd_perm);
DECLARE_FUNC_WRITE_SRCMD_L(srcmd_r);
DECLARE_FUNC_WRITE_SRCMD_L(srcmd_w);
DECLARE_FUNC_WRITE_SRCMD_L(srcmd_x);

/* Helper functions to write high 32-bit value into SRCMD registers */
#define DECLARE_FUNC_WRITE_SRCMD_H(name, condition)                     \
static void write_ ## name(IOPMP_t *iopmp, uint32_t idx, uint32_t val)  \
{                                                                       \
    if (condition) {                                                    \
        io_write32(get_addr_of_##name(iopmp, idx), val);                \
    }                                                                   \
}
DECLARE_FUNC_WRITE_SRCMD_H(srcmd_enh,   (iopmp->md_num > 31U));
DECLARE_FUNC_WRITE_SRCMD_H(srcmd_permh, (iopmp->rrid_num > 16U));
DECLARE_FUNC_WRITE_SRCMD_H(srcmd_rh,    (iopmp->md_num > 31U));
DECLARE_FUNC_WRITE_SRCMD_H(srcmd_wh,    (iopmp->md_num > 31U));
DECLARE_FUNC_WRITE_SRCMD_H(srcmd_xh,    (iopmp->md_num > 31U));

/* Helper functions to write 64-bit value into SRCMD registers */
#define DECLARE_FUNC_WRITE_SRCMD_64(name)                               \
static void write_ ## name ## _64(IOPMP_t *iopmp, uint32_t idx,         \
                                  uint64_t val)                         \
{                                                                       \
    write_##name##h(iopmp, idx, val >> 32);                             \
    write_##name(iopmp, idx, val & 0xFFFFFFFFU);                        \
}
DECLARE_FUNC_WRITE_SRCMD_64(srcmd_en);
DECLARE_FUNC_WRITE_SRCMD_64(srcmd_perm);
DECLARE_FUNC_WRITE_SRCMD_64(srcmd_r);
DECLARE_FUNC_WRITE_SRCMD_64(srcmd_w);
DECLARE_FUNC_WRITE_SRCMD_64(srcmd_x);

/* Helper functions to get base address of ENTRY registers */
#define DECLARE_FUNC_GET_ADDR_OF_ENTRY(name, base)                      \
static inline uintptr_t get_addr_of_ ## name(IOPMP_t *iopmp,            \
                                             uint32_t idx)              \
{                                                                       \
    return iopmp->addr_entry_array + (base) +                           \
           ((idx) * IOPMP_ENTRY_STRIDE);                                \
}
DECLARE_FUNC_GET_ADDR_OF_ENTRY(entry,       IOPMP_ENTRY_ADDR_BASE);
DECLARE_FUNC_GET_ADDR_OF_ENTRY(entry_addr,  IOPMP_ENTRY_ADDR_BASE);
DECLARE_FUNC_GET_ADDR_OF_ENTRY(entry_addrh, IOPMP_ENTRY_ADDRH_BASE);
DECLARE_FUNC_GET_ADDR_OF_ENTRY(entry_cfg,   IOPMP_ENTRY_CFG_BASE);

/******************************************************************************/
/* Internal functions to read/write value from/to IOPMP instance              */
/******************************************************************************/
static void write_hwcfg2(IOPMP_t *iopmp, uint32_t mask, uint32_t val)
{
    uint32_t hwcfg2;

    hwcfg2 = io_read32(iopmp->addr + IOPMP_HWCFG2_BASE);
    /* Clear W1CS field: prio_ent_prog */
    hwcfg2 &= ~IOPMP_HWCFG2_PRIO_ENT_PROG_MASK;
    hwcfg2 = (hwcfg2 & ~mask) | (val & mask);
    io_write32(iopmp->addr + IOPMP_HWCFG2_BASE, hwcfg2);
}

static void write_hwcfg3(IOPMP_t *iopmp, uint32_t mask, uint32_t val)
{
    uint32_t hwcfg3;

    hwcfg3 = io_read32(iopmp->addr + IOPMP_HWCFG3_BASE);
    /* Clear W1CS field: rrid_transl_prog */
    hwcfg3 &= ~IOPMP_HWCFG3_RRID_TRANSL_PROG_MASK;
    hwcfg3 = (hwcfg3 & ~mask) | (val & mask);
    io_write32(iopmp->addr + IOPMP_HWCFG3_BASE, hwcfg3);
}

static void write_err_cfg(IOPMP_t *iopmp, uint32_t mask, uint32_t val)
{
    uint32_t err_cfg;

    err_cfg = io_read32(iopmp->addr + IOPMP_ERR_CFG_BASE);
    err_cfg = (err_cfg & ~mask) | (val & mask);
    io_write32(iopmp->addr + IOPMP_ERR_CFG_BASE, err_cfg);
}

static void detect_stall_function(IOPMP_t *iopmp)
{
    /*
     * HWCFG2.stall_en indicates if the IOPMP implements stall-related features,
     * which are MDSTALL, MDSTALLH, and RRIDSCP registers.
     */
    if (iopmp->stall_en != 0U) {
        /* Assume all MDs selectable; iopmp_probe_stall_by_md() narrows it */
        iopmp->stall_md_mask = GENMASK_64((iopmp->md_num - 1U), 0U);
        /*
         * If RRIDSCP is not implemented, it always returns zero. One can test
         * if it is implemented by writing a zero and then reading it back. Any
         * IOPMP implementing RRIDSCP should not return a zero in RRIDSCP.stat
         * in this case.
         */
        io_write32(iopmp->addr + IOPMP_RRIDSCP_BASE, 0);
        if (io_read32(iopmp->addr + IOPMP_RRIDSCP_BASE) != 0U) {
            iopmp->support_stall_by_rrid = true;
        } else {
            iopmp->support_stall_by_rrid = false;
        }
    } else {
        iopmp->stall_md_mask = 0;
        iopmp->support_stall_by_rrid = false;
    }
}

enum iopmp_error detect_entry_addr_bits(IOPMP_t *iopmp)
{
    uint64_t val = 0;
    uintptr_t entry;
    uint32_t entrylck_f;
    uint32_t i;

    entrylck_f = EXTRACT_FIELD(io_read32(iopmp->addr + IOPMP_ENTRYLCK_BASE),
                               IOPMP_ENTRYLCK_F);
    /*Search for an unlocked and unused entry that we can safely use to detect*/
    for (i = entrylck_f; i < iopmp->entry_num; i++) {
        entry = get_addr_of_entry(iopmp, i);
        /* One read per field, so a used entry costs a single read */
        if (io_read32(entry + IOPMP_ENTRY_CFG_BASE) != 0U) {
            continue;
        }
        if (io_read32(entry + IOPMP_ENTRY_ADDR_BASE) != 0U) {
            continue;
        }
        if ((iopmp->addrh_en != 0U) &&
            (io_read32(entry + IOPMP_ENTRY_ADDRH_BASE) != 0U)) {
            continue;
        }

        io_write32(entry + IOPMP_ENTRY_ADDR_BASE, 0xFFFFFFFFU);
        val = io_read32(entry + IOPMP_ENTRY_ADDR_BASE);
        io_write32(entry + IOPMP_ENTRY_ADDR_BASE, 0);       /* Clear */
        if (iopmp->addrh_en != 0U) {
            io_write32(entry + IOPMP_ENTRY_ADDRH_BASE, 0xFFFFFFFFU);
            val |= (uint64_t)io_read32(entry + IOPMP_ENTRY_ADDRH_BASE) << 32;
            io_write32(entry + IOPMP_ENTRY_ADDRH_BASE, 0);  /* Clear */
        }
        break;
    }

    /* Every entry is locked or in use, so nothing can be probed. iopmp_ctzll()
     * below is undefined for zero, and a zero entry_addr_bits would reject
     * every address handed to iopmp_encode_entry() anyway */
    if (val == 0U) {
        return IOPMP_ERR_NOT_AVAILABLE;
    }

    iopmp->entry_addr_bits = val;

    /*
     * From Privileged Architecture Spec.:
     * Software may determine the PMP granularity by writing zero to pmp0cfg,
     * then writing all ones to pmpaddr0, then reading back pmpaddr0. If G is
     * the index of the least significant bit set, the PMP granularity is
     * 2^(G+2) bytes.
     */
    iopmp->granularity = (uint32_t)1 << (iopmp_ctzll(val) + 2);

    return IOPMP_OK;
}

/******************************************************************************/
/* Implementation of libiopmp APIs                                            */
/******************************************************************************/
void generic_enable(IOPMP_t *iopmp)
{
    io_write32(iopmp->addr + IOPMP_HWCFG0_BASE, IOPMP_HWCFG0_ENABLE_MASK);
}

void generic_lock_prio_entry_num(IOPMP_t *iopmp)
{
    write_hwcfg2(iopmp, IOPMP_HWCFG2_PRIO_ENT_PROG_MASK,
                 IOPMP_HWCFG2_PRIO_ENT_PROG_MASK);
}

void generic_lock_rrid_transl(IOPMP_t *iopmp)
{
    write_hwcfg3(iopmp, IOPMP_HWCFG3_RRID_TRANSL_PROG_MASK,
                 IOPMP_HWCFG3_RRID_TRANSL_PROG_MASK);
}

enum iopmp_error generic_set_prio_entry_num(IOPMP_t *iopmp, uint16_t *num_entry)
{
    uint16_t prio_entry_req = *num_entry;
    uint32_t hwcfg2;

    write_hwcfg2(iopmp, IOPMP_HWCFG2_PRIO_ENTRY_MASK,
                 (prio_entry_req << IOPMP_HWCFG2_PRIO_ENTRY_SHIFT));

    /* HWCFG2.prio_entry is WARL field. Read it back to check the value */
    hwcfg2 = io_read32(iopmp->addr + IOPMP_HWCFG2_BASE);
    *num_entry = EXTRACT_FIELD(hwcfg2, IOPMP_HWCFG2_PRIO_ENTRY);

    return (prio_entry_req == *num_entry) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error generic_set_rrid_transl(IOPMP_t *iopmp, uint16_t *rrid_transl)
{
    uint16_t rrid_transl_req = *rrid_transl;
    uint32_t hwcfg3;

    write_hwcfg3(iopmp, IOPMP_HWCFG3_RRID_TRANSL_MASK,
                 (rrid_transl_req << IOPMP_HWCFG3_RRID_TRANSL_SHIFT));

    /* HWCFG3.rrid_transl is WARL field. Read it back to check the value */
    hwcfg3 = io_read32(iopmp->addr + IOPMP_HWCFG3_BASE);
    *rrid_transl = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_RRID_TRANSL);

    return (rrid_transl_req == *rrid_transl) ? IOPMP_OK :
                                               IOPMP_ERR_ILLEGAL_VALUE;
}

/**
 * \brief Clear MDSTALL to resume the stalled transactions
 *
 * \param[in] iopmp             The IOPMP instance to be resumed
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value
 */
static enum iopmp_error resume_transactions_common(IOPMP_t *iopmp)
{
    uint32_t rb_mdstallh, rb_mdstall_md;

    if (iopmp->md_num > 31U) {
        io_write32(iopmp->addr + IOPMP_MDSTALLH_BASE, 0);
    }
    io_write32(iopmp->addr + IOPMP_MDSTALL_BASE, 0);
    /*
     * MDSTALL.md and MDSTALLH.mdh are WARL fields. Read them back to check the
     * values.
     */
    if (iopmp->md_num > 31U) {
        rb_mdstallh = io_read32(iopmp->addr + IOPMP_MDSTALLH_BASE);
    } else {
        rb_mdstallh = 0;
    }
    rb_mdstall_md = io_read32(iopmp->addr + IOPMP_MDSTALL_BASE);
    rb_mdstall_md = EXTRACT_FIELD(rb_mdstall_md, IOPMP_MDSTALL_MD);
    if (rb_mdstallh != 0U || rb_mdstall_md != 0U) {
        return IOPMP_ERR_ILLEGAL_VALUE;
    }

    return IOPMP_OK;
}

/**
 * \brief Set MDSTALL to stall the transactions related to MDs bitmap
 *
 * \param[in] iopmp             The IOPMP instance to be set
 * \param[in,out] mds           Input the MD bitmap to be stalled. Output WARL
 *                              value
 * \param[in] exempt            Stall transactions with exempt selected MDs
 *
 * \retval IOPMP_OK if successes
 * \retval IOPMP_ERR_ILLEGAL_VALUE if the written \p mds does not match the
 *         actual value. The actual value is output via \p mds
 */
static enum iopmp_error stall_by_mds_common(IOPMP_t *iopmp, uint64_t *mds,
                                            bool exempt)
{
    uint64_t mds_req = *mds;
    uint32_t mdstall, mdstall_md, mdstallh;
    uint32_t rb_mdstall_md, rb_mdstallh;

    mdstallh = mds_req >> 31;           /* mds[62:31] */
    mdstall_md = mds_req & 0x7FFFFFFFU;  /* mds[30: 0] */
    mdstall = MAKE_FIELD_32(mdstall_md, IOPMP_MDSTALL_MD) |
              MAKE_FIELD_32(exempt, IOPMP_MDSTALL_EXEMPT);
    /* Write MD_STALLH first then MD_STALL to take effect. */
    if (mdstallh != 0U) {
        io_write32(iopmp->addr + IOPMP_MDSTALLH_BASE, mdstallh);
    }
    io_write32(iopmp->addr + IOPMP_MDSTALL_BASE, mdstall);

    /*
     * MDSTALL.md and MDSTALLH.mdh are WARL fields. Read them back to check the
     * values.
     */
    if (mdstallh != 0U) {
        rb_mdstallh = io_read32(iopmp->addr + IOPMP_MDSTALLH_BASE);
    } else {
        rb_mdstallh = 0;
    }
    rb_mdstall_md = io_read32(iopmp->addr + IOPMP_MDSTALL_BASE);
    rb_mdstall_md = EXTRACT_FIELD(rb_mdstall_md, IOPMP_MDSTALL_MD);
    *mds = ((uint64_t)rb_mdstallh << 31) | rb_mdstall_md;
    if (rb_mdstallh != mdstallh || rb_mdstall_md != mdstall_md) {
        /* Resume transactions due to error */
        resume_transactions_common(iopmp);
        return IOPMP_ERR_ILLEGAL_VALUE;
    }

    return IOPMP_OK;
}

/**
 * \brief Poll MDSTALL.is_busy until its value is zero
 */
static void polling_mdstall(IOPMP_t *iopmp)
{
    uint32_t mdstall;

    do {
        mdstall = io_read32(iopmp->addr + IOPMP_MDSTALL_BASE);
    } while(EXTRACT_FIELD(mdstall, IOPMP_MDSTALL_IS_BUSY) != 0U);
}

enum iopmp_error generic_stall_by_mds(IOPMP_t *iopmp, uint64_t *mds,
                                      bool exempt, bool polling)
{
    enum iopmp_error ret;

    ret = stall_by_mds_common(iopmp, mds, exempt);
    if (ret != IOPMP_OK) {
        return ret;
    }

    if (polling) {
        polling_mdstall(iopmp);
    }

    return IOPMP_OK;
}

enum iopmp_error generic_resume_transactions(IOPMP_t *iopmp, bool polling)
{
    enum iopmp_error ret;

    ret = resume_transactions_common(iopmp);
    if (ret != IOPMP_OK) {
        return ret;
    }

    if (polling) {
        polling_mdstall(iopmp);
    }

    return IOPMP_OK;
}

bool generic_poll_mdstall(IOPMP_t *iopmp, bool polling, bool stall_or_resume)
{
    uint32_t mdstall;

    /* MDSTALL.is_busy covers both directions. Kept for models that tell them
     * apart */
    (void)stall_or_resume;

    if (polling) {
        polling_mdstall(iopmp);
        return true;
    }

    mdstall = io_read32(iopmp->addr + IOPMP_MDSTALL_BASE);
    return EXTRACT_FIELD(mdstall, IOPMP_MDSTALL_IS_BUSY) == 0U;
}

enum iopmp_error generic_set_rridscp(IOPMP_t *iopmp, uint32_t *rrid,
                                     enum iopmp_rridscp_op op,
                                     uint32_t *stat)
{
    uint32_t rridscp;
    uint32_t rrid_req = *rrid;

    rridscp  = MAKE_FIELD_32(rrid_req, IOPMP_RRIDSCP_RRID);
    rridscp |= MAKE_FIELD_32(op, IOPMP_RRIDSCP_OP);

    /* Write to query. */
    io_write32(iopmp->addr + IOPMP_RRIDSCP_BASE, rridscp);
    /* RRIDSCP.rrid is WARL field. Read it back to check the value */
    rridscp = io_read32(iopmp->addr + IOPMP_RRIDSCP_BASE);
    *rrid = EXTRACT_FIELD(rridscp, IOPMP_RRIDSCP_RRID);
    *stat = EXTRACT_FIELD(rridscp, IOPMP_RRIDSCP_STAT);

    return (rrid_req == *rrid) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error generic_lock_entries(IOPMP_t *iopmp, uint32_t *entry_num,
                                      bool lock)
{
    uint32_t entrylck;
    uint32_t entry_num_req = *entry_num;

    entrylck = MAKE_FIELD_32(lock, IOPMP_ENTRYLCK_L) |
               MAKE_FIELD_32(entry_num_req, IOPMP_ENTRYLCK_F);
    io_write32(iopmp->addr + IOPMP_ENTRYLCK_BASE, entrylck);

    /* ENTRYLCK.f is WARL. Read the value back to check */
    entrylck = io_read32(iopmp->addr + IOPMP_ENTRYLCK_BASE);
    *entry_num = EXTRACT_FIELD(entrylck, IOPMP_ENTRYLCK_F);

    return entry_num_req == *entry_num ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

void generic_lock_err_cfg(IOPMP_t *iopmp)
{
    write_err_cfg(iopmp, IOPMP_ERR_CFG_L_MASK, IOPMP_ERR_CFG_L_MASK);
}

void generic_set_global_intr(IOPMP_t *iopmp, bool enable)
{
    write_err_cfg(iopmp, IOPMP_ERR_CFG_IE_MASK,
                  (enable << IOPMP_ERR_CFG_IE_SHIFT));
}

enum iopmp_error generic_set_global_err_resp(IOPMP_t *iopmp, bool *suppress)
{
    uint32_t err_cfg;
    bool suppress_req = *suppress;

    write_err_cfg(iopmp, IOPMP_ERR_CFG_RS_MASK,
                  (*suppress << IOPMP_ERR_CFG_RS_SHIFT));
    /* ERR_CFG.rs is WARL. Read it back to check the value */
    err_cfg = io_read32(iopmp->addr + IOPMP_ERR_CFG_BASE);
    *suppress = EXTRACT_FIELD(err_cfg, IOPMP_ERR_CFG_RS);

    return suppress_req == *suppress ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error generic_set_msi_sel(IOPMP_t *iopmp, bool *enable)
{
    uint32_t err_cfg;
    bool enable_req = *enable;

    write_err_cfg(iopmp, IOPMP_ERR_CFG_MSI_SEL_MASK,
                  (enable_req << IOPMP_ERR_CFG_MSI_SEL_SHIFT));
    /* ERR_CFG.msi_sel is WARL. Read it back to check the value */
    err_cfg = io_read32(iopmp->addr + IOPMP_ERR_CFG_BASE);
    *enable = EXTRACT_FIELD(err_cfg, IOPMP_ERR_CFG_MSI_SEL);

    return enable_req == *enable ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error generic_set_msi_info(IOPMP_t *iopmp, uint64_t *msiaddr64,
                                      uint16_t *msidata)
{
    uint32_t err_cfg, err_msiaddr, err_msiaddrh;
    uint32_t rb_err_msiaddr, rb_err_msiaddrh;
    uint64_t msiaddr64_req = *msiaddr64;
    uint16_t msidata_req = *msidata;

    write_err_cfg(iopmp, IOPMP_ERR_CFG_MSIDATA_MASK,
                  (msidata_req << IOPMP_ERR_CFG_MSIDATA_SHIFT));
    /* ERR_CFG.msidata is WARL field. Read it back to check the value */
    err_cfg = io_read32(iopmp->addr + IOPMP_ERR_CFG_BASE);
    *msidata = EXTRACT_FIELD(err_cfg, IOPMP_ERR_CFG_MSIDATA);

    if (iopmp->addrh_en == 0U) {
        /* Write bits 33 to 2 of the address into ERR_MSIADDR */
        err_msiaddr = msiaddr64_req >> 2;
        io_write32(iopmp->addr + IOPMP_ERR_MSIADDR_BASE, err_msiaddr);
        /* ERR_MSIADDR.msiaddr is WARL field. Read it back to check the value */
        *msiaddr64 =
            (uint64_t)io_read32(iopmp->addr + IOPMP_ERR_MSIADDR_BASE) << 2;
    } else {
        /* Write bits 31 to 0 of the address into ERR_MSIADDR */
        err_msiaddr = msiaddr64_req & UINT32_MAX;
        io_write32(iopmp->addr + IOPMP_ERR_MSIADDR_BASE, err_msiaddr);
        /* Write bits 63 to 32 of the address into ERR_MSIADDRH */
        err_msiaddrh = msiaddr64_req >> 32;
        io_write32(iopmp->addr + IOPMP_ERR_MSIADDRH_BASE, err_msiaddrh);

        /*
         * ERR_MSIADDR.msiaddr abd ERR_MSIADDRH.msiaddrh are WARL fields.
         * Read them back to check the values.
         */
        rb_err_msiaddrh = io_read32(iopmp->addr + IOPMP_ERR_MSIADDRH_BASE);
        rb_err_msiaddr = io_read32(iopmp->addr + IOPMP_ERR_MSIADDR_BASE);
        *msiaddr64 = reg_pair_to_64(rb_err_msiaddrh, rb_err_msiaddr);
    }

    return (msidata_req == *msidata) && (msiaddr64_req == *msiaddr64) ?
           IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

void generic_get_and_clear_msi_werr(IOPMP_t *iopmp, bool *msi_werr)
{
    uint32_t err_info;

    err_info = io_read32(iopmp->addr + IOPMP_ERR_INFO_BASE);
    *msi_werr = EXTRACT_FIELD(err_info, IOPMP_ERR_INFO_MSI_WERR);

    /* ERR_INFO.msi_werr is W1C */
    io_write32(iopmp->addr + IOPMP_ERR_INFO_BASE, IOPMP_ERR_INFO_MSI_WERR_MASK);
}

enum iopmp_error generic_set_stall_violation_en(IOPMP_t *iopmp, bool *enable)
{
    uint32_t err_cfg;
    bool enable_req = *enable;

    write_err_cfg(iopmp, IOPMP_ERR_CFG_STALL_VIO_EN_MASK,
                  (*enable << IOPMP_ERR_CFG_STALL_VIO_EN_SHIFT));
    /* ERR_CFG.stall_violation_en is WARL. Read it back to check the value */
    err_cfg = io_read32(iopmp->addr + IOPMP_ERR_CFG_BASE);
    *enable = EXTRACT_FIELD(err_cfg, IOPMP_ERR_CFG_STALL_VIO_EN);

    return enable_req == *enable ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

void generic_invalidate_error(IOPMP_t *iopmp)
{
    /* Only ERR_INFO.ip is writable. Write 1 clear */
    io_write32(iopmp->addr + IOPMP_ERR_INFO_BASE, IOPMP_ERR_INFO_V_MASK);
}

enum iopmp_error generic_capture_error(IOPMP_t *iopmp,
                                       IOPMP_ERR_REPORT_t *err_report,
                                       bool invalidate)
{
    uint32_t err_reqaddr, err_reqaddrh, err_reqid, err_info;

    /* Check ERR_INFO.v first */
    err_info = io_read32(iopmp->addr + IOPMP_ERR_INFO_BASE);
    if ((err_info & IOPMP_ERR_INFO_V_MASK) == 0U) {
        return IOPMP_ERR_NOT_EXIST; /* No pending error */
    }

    /* Read ERR_REQADDR, ERR_REQADDRH, and ERR_REQID from IOPMP */
    err_reqaddr  = io_read32(iopmp->addr + IOPMP_ERR_REQADDR_BASE);
    if (iopmp->addrh_en != 0U) {
        err_reqaddrh = io_read32(iopmp->addr + IOPMP_ERR_REQADDRH_BASE);
    } else {
        err_reqaddrh = 0;
    }
    /* Read ERR_REQID from IOPMP */
    err_reqid = io_read32(iopmp->addr + IOPMP_ERR_REQID_BASE);

    /* Record into given structure */
    err_report->addr = reg_pair_to_64(err_reqaddrh, err_reqaddr);
    err_report->rrid = EXTRACT_FIELD(err_reqid, IOPMP_ERR_REQID_RRID);
    err_report->eid = EXTRACT_FIELD(err_reqid, IOPMP_ERR_REQID_EID);
    err_report->ttype = EXTRACT_FIELD(err_info, IOPMP_ERR_INFO_TTYPE);
    err_report->msi_werr = EXTRACT_FIELD(err_info, IOPMP_ERR_INFO_MSI_WERR);
    err_report->etype = EXTRACT_FIELD(err_info, IOPMP_ERR_INFO_ETYPE);
    err_report->svc = EXTRACT_FIELD(err_info, IOPMP_ERR_INFO_SVC);

    /* A driver may have replaced only invalidate_error, so honour it here */
    if (invalidate) {
        if ((iopmp->ops_override != NULL) &&
            (iopmp->ops_override->invalidate_error != NULL)) {
            iopmp->ops_override->invalidate_error(iopmp);
        } else {
            generic_invalidate_error(iopmp);
        }
    }

    return IOPMP_OK;
}

enum iopmp_error generic_get_sv_window(IOPMP_t *iopmp, uint16_t *svi,
                                       uint16_t *svw)
{
    uint32_t err_info;
    uint32_t err_mfr;
    bool svc;

    err_info = io_read32(iopmp->addr + IOPMP_ERR_INFO_BASE);
    svc = EXTRACT_FIELD(err_info, IOPMP_ERR_INFO_SVC);
    if (!svc) {
        return IOPMP_ERR_NOT_EXIST;
    }

    /* Set original position */
    err_mfr = MAKE_FIELD_32(*svi, IOPMP_ERR_MFR_SVI);
    io_write32(iopmp->addr + IOPMP_ERR_MFR_BASE, err_mfr);

    err_mfr = io_read32(iopmp->addr + IOPMP_ERR_MFR_BASE);
    if ((err_mfr & IOPMP_ERR_MFR_SVS_MASK) != 0U) {
        /* Subsequent violation found */
        *svi = EXTRACT_FIELD(err_mfr, IOPMP_ERR_MFR_SVI);
        *svw = EXTRACT_FIELD(err_mfr, IOPMP_ERR_MFR_SVW);
        return IOPMP_OK;
    }

    return IOPMP_ERR_NOT_EXIST;
}

enum iopmp_error generic_set_entries(IOPMP_t *iopmp,
                                     const struct iopmp_entry *entry_array,
                                     uint32_t idx_start, uint32_t num_entry)
{
    uintptr_t e = get_addr_of_entry(iopmp, idx_start);
    uint32_t i;

    for (i = 0; i < num_entry; i++) {
        io_write32(e + IOPMP_ENTRY_CFG_BASE, 0);
        io_write32(e + IOPMP_ENTRY_ADDR_BASE, entry_array[i].addr & UINT32_MAX);
        if (iopmp->addrh_en != 0U) {
            io_write32(e + IOPMP_ENTRY_ADDRH_BASE, entry_array[i].addr >> 32);
        }
        io_write32(e + IOPMP_ENTRY_CFG_BASE, entry_array[i].cfg);

        e += IOPMP_ENTRY_STRIDE;
    }

    return IOPMP_OK;
}

void generic_get_entries(IOPMP_t *iopmp, struct iopmp_entry *entry_array,
                         uint32_t idx_start, uint32_t num_entry)
{
    uintptr_t e = get_addr_of_entry(iopmp, idx_start);
    uint32_t addr, addrh = 0, cfg;
    uint32_t i;

    for (i = 0; i < num_entry; i++) {
        /* Read ENTRY_ADDR(idx), ENTRY_ADDRH(idx), and ENTRY_CFG(idx) from IP */
        if (iopmp->addrh_en != 0U) {
            addrh = io_read32(e + IOPMP_ENTRY_ADDRH_BASE);
        }
        addr = io_read32(e + IOPMP_ENTRY_ADDR_BASE);
        cfg  = io_read32(e + IOPMP_ENTRY_CFG_BASE);
        /* Store into data structure */
        entry_array[i].addr = reg_pair_to_64(addrh, addr);
        entry_array[i].cfg = cfg;

        e += IOPMP_ENTRY_STRIDE;
    }
}

void generic_clear_entries(IOPMP_t *iopmp, uint32_t idx_start,
                           uint32_t num_entry)
{
    uintptr_t e = get_addr_of_entry(iopmp, idx_start);
    uint32_t i;

    for (i = 0; i < num_entry; i++) {
        io_write32(e + IOPMP_ENTRY_CFG_BASE, 0);
        io_write32(e + IOPMP_ENTRY_ADDR_BASE, 0);
        if (iopmp->addrh_en != 0U) {
            io_write32(e + IOPMP_ENTRY_ADDRH_BASE, 0);
        }

        e += IOPMP_ENTRY_STRIDE;
    }
}

/******************************************************************************/
/* Functions specific to some models                                          */
/******************************************************************************/
void srcmd_fmt_0_get_association_rrid_md(IOPMP_t *iopmp, uint32_t rrid,
                                         uint64_t *mds, bool *lock)
{
    uint64_t srcmd_en_64 = read_srcmd_en_64(iopmp, rrid);

    *mds = srcmd_en_64 >> IOPMP_SRCMD_EN_MD_SHIFT;
    *lock = EXTRACT_FIELD(srcmd_en_64, IOPMP_SRCMD_EN_L);
}

enum iopmp_error srcmd_fmt_0_set_association_rrid_md(IOPMP_t *iopmp,
                                                     uint32_t rrid,
                                                     uint64_t *mds,
                                                     bool lock)
{
    uint64_t srcmd_en_64;
    uint64_t mds_req = *mds;

    srcmd_en_64 = MAKE_FIELD_64(lock, IOPMP_SRCMD_EN_L) |
                  (mds_req << IOPMP_SRCMD_EN_MD_SHIFT);
    write_srcmd_en_64(iopmp, rrid, srcmd_en_64);

    /* SRCMD_EN.md and SRCMD_ENH.mdh are WARL. Read them back to check. */
    srcmd_en_64 = read_srcmd_en_64(iopmp, rrid);
    *mds = srcmd_en_64 >> IOPMP_SRCMD_EN_MD_SHIFT;

    return (*mds == mds_req) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error srcmd_fmt_0_2_set_md_lock(IOPMP_t *iopmp, uint64_t *mds,
                                           bool lock_mdlck)
{
    uint32_t mdlck, mdlckh;
    uint64_t mdlck_64;
    uint64_t mds_req = *mds;

    /* Set bits of new MDs and MDLCK.l */
    mdlck_64 = (mds_req << IOPMP_MDLCK_MD_SHIFT);
    if (lock_mdlck) {
        mdlck_64 |= IOPMP_MDLCK_L_MASK;
    }
    mdlckh = mdlck_64 >> 32;
    mdlck  = mdlck_64 & UINT32_MAX;

    /* Write MDLCKH first */
    if (mdlckh != 0U) {
        io_write32(iopmp->addr + IOPMP_MDLCKH_BASE, mdlckh);
        /* MDLCKH.mdh is WARL. Read the value back to check */
        mdlckh = io_read32(iopmp->addr + IOPMP_MDLCKH_BASE);
    }
    /* Write MDLCK */
    io_write32(iopmp->addr + IOPMP_MDLCK_BASE, mdlck);
    /* MDLCK is WARL. Read the value back to check */
    mdlck = io_read32(iopmp->addr + IOPMP_MDLCK_BASE);

    mdlck_64 = reg_pair_to_64(mdlckh, mdlck);
    *mds = mdlck_64 >> IOPMP_MDLCK_MD_SHIFT;

    return ((mds_req & *mds) == mds_req) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error srcmd_fmt_0_lock_srcmd_table(IOPMP_t *iopmp, uint32_t rrid,
                                              uint32_t mdidx)
{
    uint64_t srcmd_en_64;

    (void)mdidx;    /* SRCMD_FMT=0 locks the SRCMD table per RRID */

    srcmd_en_64  = read_srcmd_en_64(iopmp, rrid);
    srcmd_en_64 |= MAKE_FIELD_64(1, IOPMP_SRCMD_EN_L);
    write_srcmd_en_64(iopmp, rrid, srcmd_en_64);

    return IOPMP_OK;
}

enum iopmp_error srcmd_fmt_2_lock_srcmd_table(IOPMP_t *iopmp, uint32_t rrid,
                                              uint32_t mdidx)
{
    uint64_t mds = (uint64_t)1 << mdidx;

    (void)rrid;     /* SRCMD_FMT=2 locks the SRCMD table per MD */

    return srcmd_fmt_0_2_set_md_lock(iopmp, &mds, false);
}

enum iopmp_error mdcfg_fmt_0_lock_mdcfg(IOPMP_t *iopmp, uint32_t *md_num,
                                        bool lock)
{
    uint32_t mdcfglck;
    uint64_t md_num_req = *md_num;

    mdcfglck = MAKE_FIELD_32(lock, IOPMP_MDCFGLCK_L) |
               MAKE_FIELD_32(md_num_req, IOPMP_MDCFGLCK_F);
    io_write32(iopmp->addr + IOPMP_MDCFGLCK_BASE, mdcfglck);
    /* MDCFGLCK.f is WARL field. Read the value back to check */
    mdcfglck = io_read32(iopmp->addr + IOPMP_MDCFGLCK_BASE);
    *md_num = EXTRACT_FIELD(mdcfglck, IOPMP_MDCFGLCK_F);

    return (md_num_req == *md_num) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

void mdcfg_fmt_0_get_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                                  uint32_t *entry_top)
{
    uint32_t mdcfg;

    mdcfg = io_read32(get_addr_of_mdcfg(iopmp, mdidx));
    *entry_top = EXTRACT_FIELD(mdcfg, IOPMP_MDCFG_T);
}

void mdcfg_fmt_1_2_get_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                                    uint32_t *entry_top)
{
    *entry_top = ((uint32_t)iopmp->md_entry_num + 1U) * (mdidx + 1U);
}

enum iopmp_error mdcfg_fmt_0_set_md_entry_top(IOPMP_t *iopmp, uint32_t mdidx,
                                              uint32_t *entry_top)
{
    uint32_t entry_top_req = *entry_top;
    uintptr_t addr_mdcfg;
    uint32_t mdcfg;

    addr_mdcfg = get_addr_of_mdcfg(iopmp, mdidx);
    mdcfg = MAKE_FIELD_32(entry_top_req, IOPMP_MDCFG_T);
    io_write32(addr_mdcfg, mdcfg);
    /* MDCFG.t is WARL field. Read it back to check it */
    mdcfg = io_read32(addr_mdcfg);
    *entry_top = EXTRACT_FIELD(mdcfg, IOPMP_MDCFG_T);

    return (entry_top_req == *entry_top) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error mdcfg_fmt_2_set_md_entry_num(IOPMP_t *iopmp,
                                              uint32_t *md_entry_num)
{
    uint32_t md_entry_num_req = *md_entry_num;
    uint32_t hwcfg3;

    hwcfg3 = MAKE_FIELD_32(md_entry_num_req, IOPMP_HWCFG3_MD_ENTRY_NUM);
    write_hwcfg3(iopmp, IOPMP_HWCFG3_MD_ENTRY_NUM_MASK, hwcfg3);
    /* HWCFG3.md_entry_num is WARL field. Read it back to check it */
    hwcfg3 = io_read32(iopmp->addr + IOPMP_HWCFG3_BASE);
    *md_entry_num = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_MD_ENTRY_NUM);

    return (md_entry_num_req == *md_entry_num) ? IOPMP_OK :
                                                 IOPMP_ERR_ILLEGAL_VALUE;
}

enum iopmp_error srcmd_fmt_2_set_md_permission(IOPMP_t *iopmp, uint32_t rrid,
                                               uint32_t mdidx, bool *r, bool *w)
{
    uint32_t srcmd_perm, shift, mask, val;
    bool r_req = *r, w_req = *w;

    val  = ((uint32_t)r_req << 0);
    val |= ((uint32_t)w_req << 1);
    /* Set SRCMD_PERM if RRID < 16, otherwise set SRCMD_PERMH */
    if (rrid < 16U) {
        srcmd_perm = read_srcmd_perm(iopmp, mdidx);
        shift = rrid << 1;
        mask = IOPMP_SRCMD_PERM_MASK << shift;
        val  = val << shift;
        srcmd_perm = (srcmd_perm & ~mask) | (val & mask);
        write_srcmd_perm(iopmp, mdidx, srcmd_perm);
        /* SRCMD_PERM.perm is WARL field. Read it back to check value */
        srcmd_perm = read_srcmd_perm(iopmp, mdidx);
        *r = ((srcmd_perm & mask) >> shift) & 0b1U;
        *w = ((srcmd_perm & mask) >> (shift + 1U)) & 0b1U;
        if (r_req != *r || w_req != *w) {
            return IOPMP_ERR_ILLEGAL_VALUE;
        }
    } else {
        srcmd_perm = read_srcmd_permh(iopmp, mdidx);
        shift = (rrid - 16U) << 1;
        mask = IOPMP_SRCMD_PERM_MASK << shift;
        val  = val << shift;
        srcmd_perm = (srcmd_perm & ~mask) | (val & mask);
        write_srcmd_permh(iopmp, mdidx, srcmd_perm);
        /* SRCMD_PERMH.permh is WARL field. Read it back to check value */
        srcmd_perm = read_srcmd_permh(iopmp, mdidx);
        *r = ((srcmd_perm & mask) >> shift) & 0b1U;
        *w = ((srcmd_perm & mask) >> (shift + 1U)) & 0b1U;
        if (r_req != *r || w_req != *w) {
            return IOPMP_ERR_ILLEGAL_VALUE;
        }
    }

    return IOPMP_OK;
}

enum iopmp_error
srcmd_fmt_2_set_md_permission_multi(IOPMP_t *iopmp, uint32_t mdidx,
                                    IOPMP_SRCMD_PERM_CFG_t *cfg)
{
    uint64_t srcmd_perm_64, rb_srcmd_perm_64, mask, val;

    srcmd_perm_64 = read_srcmd_perm_64(iopmp, mdidx);
    mask = cfg->srcmd_perm_mask;
    val  = cfg->srcmd_perm_val;
    srcmd_perm_64 = (srcmd_perm_64 & ~mask) | (val & mask);
    write_srcmd_perm_64(iopmp, mdidx, srcmd_perm_64);

    /* SRCMD_PERM(H).perm is WARL field. Read it back to check value */
    rb_srcmd_perm_64 = read_srcmd_perm_64(iopmp, mdidx);
    if (rb_srcmd_perm_64 != srcmd_perm_64) {
        /* Set the value into cfg structure to let user check it */
        cfg->srcmd_perm_val = (rb_srcmd_perm_64 & cfg->srcmd_perm_mask);
        return IOPMP_ERR_ILLEGAL_VALUE;
    }

    return IOPMP_OK;
}

/******************************************************************************/
/* Functions specific to IOPMP/SPS (Secondary Permission Setting) extension   */
/******************************************************************************/
uint64_t ext_sps_get_srcmd_r_64_md(IOPMP_t *iopmp, uint32_t rrid)
{
    return read_srcmd_r_64(iopmp, rrid) >> IOPMP_SRCMD_R_MD_SHIFT;
}

enum iopmp_error ext_sps_set_srcmd_r_64_md(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds)
{
    uint64_t srcmd_r_64;
    uint64_t mds_req = *mds;

    srcmd_r_64 = mds_req << IOPMP_SRCMD_R_MD_SHIFT;
    write_srcmd_r_64(iopmp, rrid, srcmd_r_64);
    /* SRCMD_R.md and SRCMD_RH.mdh are WARL. Read them back to check. */
    *mds = ext_sps_get_srcmd_r_64_md(iopmp, rrid);

    return (*mds == mds_req) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

uint64_t ext_sps_get_srcmd_w_64_md(IOPMP_t *iopmp, uint32_t rrid)
{
    return read_srcmd_w_64(iopmp, rrid) >> IOPMP_SRCMD_W_MD_SHIFT;
}

enum iopmp_error ext_sps_set_srcmd_w_64_md(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds)
{
    uint64_t srcmd_w_64;
    uint64_t mds_req = *mds;

    srcmd_w_64 = mds_req << IOPMP_SRCMD_W_MD_SHIFT;
    write_srcmd_w_64(iopmp, rrid, srcmd_w_64);
    /* SRCMD_W.md and SRCMD_WH.mdh are WARL. Read them back to check. */
    *mds = ext_sps_get_srcmd_w_64_md(iopmp, rrid);

    return (*mds == mds_req) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

uint64_t ext_sps_get_srcmd_x_64_md(IOPMP_t *iopmp, uint32_t rrid)
{
    return read_srcmd_x_64(iopmp, rrid) >> IOPMP_SRCMD_X_MD_SHIFT;
}

enum iopmp_error ext_sps_set_srcmd_x_64_md(IOPMP_t *iopmp, uint32_t rrid,
                                           uint64_t *mds)
{
    uint64_t srcmd_x_64;
    uint64_t mds_req = *mds;

    srcmd_x_64 = mds_req << IOPMP_SRCMD_X_MD_SHIFT;
    write_srcmd_x_64(iopmp, rrid, srcmd_x_64);
    /* SRCMD_X.md and SRCMD_XH.mdh are WARL. Read them back to check. */
    *mds = ext_sps_get_srcmd_x_64_md(iopmp, rrid);

    return (*mds == mds_req) ? IOPMP_OK : IOPMP_ERR_ILLEGAL_VALUE;
}

/******************************************************************************/
/* IOPMP common initialization for standard IOPMP implementation              */
/******************************************************************************/
enum iopmp_error
iopmp_drv_init_common(IOPMP_t *iopmp, uintptr_t addr,
                      uint8_t srcmd_fmt, uint8_t mdcfg_fmt,
                      const struct iopmp_operations_override *ops_override)
{
    uint32_t data, hwcfg0, hwcfg3;
    uint8_t hwcfg3_srcmd_fmt, hwcfg3_mdcfg_fmt;
    bool hwcfg2_en, hwcfg3_en;
    enum iopmp_error ret;

    /* Read HWCFG0 and HWCFG3 first to check srcmd_fmt and mdcfg_fmt */
    hwcfg0 = io_read32(addr + IOPMP_HWCFG0_BASE);
    hwcfg2_en = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_HWCFG2_EN);
    hwcfg3_en = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_HWCFG3_EN);

    if (hwcfg3_en) {
        hwcfg3 = io_read32(addr + IOPMP_HWCFG3_BASE);
        hwcfg3_mdcfg_fmt = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_MDCFG_FMT);
        hwcfg3_srcmd_fmt = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_SRCMD_FMT);
    } else {
        hwcfg3 = 0;
        /* The Full Model is the default IOPMP configuration */
        hwcfg3_mdcfg_fmt = IOPMP_MDCFG_FMT_0;
        hwcfg3_srcmd_fmt = IOPMP_SRCMD_FMT_0;
    }

    if (srcmd_fmt != hwcfg3_srcmd_fmt || mdcfg_fmt != hwcfg3_mdcfg_fmt) {
        return IOPMP_ERR_NOT_SUPPORTED;
    }

    /* Set base address and address width of IOPMP */
    iopmp->addr = addr;
    /* Set overriden operations */
    iopmp->ops_override = ops_override;

    /* Read VERSION */
    data = io_read32(iopmp->addr + IOPMP_VERSION_BASE);
    /* Record into local data structure */
    iopmp->vendor = EXTRACT_FIELD(data, IOPMP_VERSION_VENDOR);
    iopmp->specver = EXTRACT_FIELD(data, IOPMP_VERSION_SPECVER);

    /* Read IMPLEMENTATION */
    data = io_read32(iopmp->addr + IOPMP_IMPLEMENTATION_BASE);
    /* Record into local data structure */
    iopmp->impid = EXTRACT_FIELD(data, IOPMP_IMPLEMENTATION_IMPID);

    /* Record HWCFG0 into local data structure */
    iopmp->enable = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_ENABLE);
    iopmp->no_err_rec = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_NO_ERR_REC);
    iopmp->md_num = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_MD_NUM);
    iopmp->addrh_en = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_ADDRH_EN);
    iopmp->tor_en = EXTRACT_FIELD(hwcfg0, IOPMP_HWCFG0_TOR_EN);

    /* Read HWCFG1 */
    data = io_read32(iopmp->addr + IOPMP_HWCFG1_BASE);
    /* Record into local data structure */
    iopmp->rrid_num = EXTRACT_FIELD(data, IOPMP_HWCFG1_RRID_NUM);
    iopmp->entry_num = EXTRACT_FIELD(data, IOPMP_HWCFG1_ENTRY_NUM);

    /* Read HWCFG2 if it is implemented */
    if (hwcfg2_en) {
        /* Record into local data structure */
        data = io_read32(iopmp->addr + IOPMP_HWCFG2_BASE);
        iopmp->prio_entry_num = EXTRACT_FIELD(data, IOPMP_HWCFG2_PRIO_ENTRY);
        iopmp->prio_ent_prog = EXTRACT_FIELD(data, IOPMP_HWCFG2_PRIO_ENT_PROG);
        iopmp->non_prio_en = EXTRACT_FIELD(data, IOPMP_HWCFG2_NON_PRIO_EN);
        iopmp->msi_en = EXTRACT_FIELD(data, IOPMP_HWCFG2_MSI_EN);
        iopmp->peis = EXTRACT_FIELD(data, IOPMP_HWCFG2_PEIS);
        iopmp->pees = EXTRACT_FIELD(data, IOPMP_HWCFG2_PEES);
        iopmp->sps_en = EXTRACT_FIELD(data, IOPMP_HWCFG2_SPS_EN);
        iopmp->stall_en = EXTRACT_FIELD(data, IOPMP_HWCFG2_STALL_EN);
        iopmp->mfr_en = EXTRACT_FIELD(data, IOPMP_HWCFG2_MFR_EN);
    } else {
        /* All entries are priority entries */
        iopmp->prio_entry_num = iopmp->entry_num;
    }

    /* Read HWCFG3 if it is implemented */
    if (hwcfg3_en) {
        /* Record into local data structure */
        iopmp->mdcfg_fmt = hwcfg3_mdcfg_fmt;
        iopmp->srcmd_fmt = hwcfg3_srcmd_fmt;
        iopmp->md_entry_num = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_MD_ENTRY_NUM);
        iopmp->xinr = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_XINR);
        iopmp->no_x = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_NO_X);
        iopmp->no_w = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_NO_W);
        iopmp->rrid_transl_en = EXTRACT_FIELD(hwcfg3,
                                              IOPMP_HWCFG3_RRID_TRANSL_EN);
        iopmp->rrid_transl_prog = EXTRACT_FIELD(hwcfg3,
                                                IOPMP_HWCFG3_RRID_TRANSL_PROG);
        iopmp->rrid_transl = EXTRACT_FIELD(hwcfg3, IOPMP_HWCFG3_RRID_TRANSL);
    }

    /* Read ENTRY_OFFSET */
    data = io_read32(iopmp->addr + IOPMP_ENTRY_OFFSET_BASE);
    /* Record into local data structure */
    iopmp->addr_entry_array = iopmp->addr + (int32_t)data;

    /* Read ERR_CFG */
    data = io_read32(iopmp->addr + IOPMP_ERR_CFG_BASE);
    /* Record into local data structure */
    iopmp->err_cfg_lock = EXTRACT_FIELD(data, IOPMP_ERR_CFG_L);
    iopmp->intr_enable = EXTRACT_FIELD(data, IOPMP_ERR_CFG_IE);
    iopmp->msi_sel = EXTRACT_FIELD(data, IOPMP_ERR_CFG_MSI_SEL);
    iopmp->stall_violation_en = EXTRACT_FIELD(data, IOPMP_ERR_CFG_STALL_VIO_EN);
    iopmp->msidata = EXTRACT_FIELD(data, IOPMP_ERR_CFG_MSIDATA);

    /* Read ERR_MSIADDR and ERR_MSIADDRH */
    if (iopmp->msi_en != 0U) {
        uint32_t msiaddr, msiaddrh = 0;
        msiaddr = io_read32(iopmp->addr + IOPMP_ERR_MSIADDR_BASE);
        if (iopmp->addrh_en != 0U) {
            msiaddrh = io_read32(iopmp->addr + IOPMP_ERR_MSIADDRH_BASE);
        } else {
            msiaddr = msiaddr << 2; /* ERR_MSIADDR contains address[33:2] */
        }
        iopmp->msiaddr64 = (uint64_t)msiaddrh << 32 | msiaddr;
    }

    /* Read MDLCK(H) */
    if (iopmp->srcmd_fmt == IOPMP_SRCMD_FMT_1) {
        iopmp->mdlck_lock = true;
        iopmp->mdlck_md = ((uint64_t)1 << iopmp->md_num) - 1U;
    } else {
        uint32_t mdlck, mdlckh;
        uint64_t mdlck_64;

        mdlck = io_read32(iopmp->addr + IOPMP_MDLCK_BASE);
        if (iopmp->md_num > 31U) {
            mdlckh = io_read32(iopmp->addr + IOPMP_MDLCKH_BASE);
        } else {
            mdlckh = 0;
        }
        mdlck_64 = ((uint64_t)mdlckh << 32) | mdlck;

        iopmp->mdlck_lock = EXTRACT_FIELD(mdlck, IOPMP_MDLCK_L);
        iopmp->mdlck_md = mdlck_64 >> IOPMP_MDLCK_MD_SHIFT;
    }

    /* Read MDCFGLCK when MDCFG is in Format 0 */
    if (iopmp->mdcfg_fmt == IOPMP_MDCFG_FMT_0) {
        data = io_read32(iopmp->addr + IOPMP_MDCFGLCK_BASE);
        /* Record into local data structure */
        iopmp->mdcfglck_lock = EXTRACT_FIELD(data, IOPMP_MDCFGLCK_L);
        iopmp->mdcfglck_f = EXTRACT_FIELD(data, IOPMP_MDCFGLCK_F);
    }

    /* Read ENTRYLCK */
    data = io_read32(iopmp->addr + IOPMP_ENTRYLCK_BASE);
    /* Record into local data structure */
    iopmp->entrylck_lock = EXTRACT_FIELD(data, IOPMP_ENTRYLCK_L);
    iopmp->entrylck_f = EXTRACT_FIELD(data, IOPMP_ENTRYLCK_F);

    /* Detect if this IOPMP supports stall transactions */
    detect_stall_function(iopmp);

    /* Detect implemented bits of ENTRY_ADDR(H) */
    ret = detect_entry_addr_bits(iopmp);
    if (ret != IOPMP_OK) {
        return ret;
    }

    iopmp->init = true;
    return IOPMP_OK;
}
