
/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Yazan Hussnain (yazan.hussain@10xengineers.ai)
//         Gull Ahmed (gull.ahmed@10xengineers.ai)
// Date: October 24, 2024
// Description: IOPMP Interrupt Generation
// When interrupt generation is valid, this function is used to generate
// WSI or MSI based on the configuration in ERR_CFG register. This also
// check for the interrrupt suppression.
***************************************************************************/

#include "iopmp.h"

/**
 * @brief Generate an interrupt based on configuration and suppress flag.
 *
 * This function checks if interrupts are enabled and not suppressed. If MSI (Message Signaled Interrupt)
 * is disabled, it generates a wired signal interrupt. When MSI is enabled, it constructs the MSI address
 * and data, writes them to memory, and handles any potential bus errors during the write operation.
 *
 * @param iopmp The IOPMP instance.
 * @param gen_intrpt Generate interrupt or not.
 * @param intrpt Pointer to the variable to store wired interrupt flag.
 *               This flag is set to 1 if the following conditions are true:
 *                 - the interrupts are not suppressed
 *                 - IOPMP doesn't implement MSI extension, or MSI is not enabled
 *               This flag is set to 0 if the following conditions are true:
 *                 - the interrupts are suppressed, or IOPMP implements MSI extension
 *                   and triggers MSI instead of wired interrupt
 */
void generate_interrupt(iopmp_dev_t *iopmp, bool gen_intrpt, uint8_t *intrpt) {

    if (!iopmp->reg_file.hwcfg2.msi_en) {
        // If IOPMP doesn't implement Message-Signaled Interrupts (MSI)
        // extension, IOPMP only triggers wired interrupts.
        // IOPMP triggers wired interrupt if the following conditions are true:
        //   - IOPMP interrupts are enabled
        //   - The interrupts are not suppressed
        *intrpt = gen_intrpt;
        return;
    }

    // IOPMP implements Message-Signaled Interrupts (MSI) extension.
    // IOPMP triggers wired interrupt if the following conditions are true:
    //   - IOPMP interrupts are enabled
    //   - The interrupts are not suppressed
    //   - IOPMP doesn't enable MSI
    *intrpt = gen_intrpt && !iopmp->reg_file.err_cfg.msi_sel;

    // IOPMP implements Message-Signaled Interrupts (MSI) extension.
    // IOPMP triggers MSI if the following conditions are true:
    //   - IOPMP interrupts are enabled
    //   - The interrupts are not suppressed
    //   - MSI is enabled
    //   - (implementation-specific) There are no pending MSI write error
    bool msi = gen_intrpt && iopmp->reg_file.err_cfg.msi_sel &&
               !iopmp->reg_file.err_info.msi_werr;
    if (msi) {
        // Construct MSI address and data for enabled MSI.
        uint64_t msi_addr;
        if (iopmp->reg_file.hwcfg0.addrh_en) {
            // {MSI_ADDRH[63:32], MSI_ADDR[31:0]}
            msi_addr = CONCAT32(iopmp->reg_file.err_msiaddrh.raw, iopmp->reg_file.err_msiaddr.raw);
        } else {
            // {MSI_ADDR[33:2], 2'b00}
            msi_addr = (uint64_t)iopmp->reg_file.err_msiaddr.raw << 2;
        }
        uint64_t msi_data = iopmp->reg_file.err_cfg.msidata;
        // Write MSI data to memory
        uint8_t status = write_memory(&msi_data, msi_addr, MSI_DATA_BYTE);

        // Handle bus errors during MSI write
        if (status == BUS_ERROR) {
            iopmp->reg_file.err_info.msi_werr = 1;
        }
    }
}
