/***************************************************************************
// Copyright (c) 2025 by 10xEngineers.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Author: Gull Ahmed (gull.ahmed@10xengineers.ai)
//         Yazan Hussnain (yazan.hussain@10xengineers.ai)
// Date: October 21, 2024
// Description: This file contains all the functions that could be used
// while testing any IOPMP Model.
***************************************************************************/
#include "test_utils.h"
#include "config.h"
#include "iopmp.h"

int test_num;
int8_t *memory;
uint64_t bus_error = 0;

/**
  * @brief Allocates memory of the specified size in gigabytes.
  *
  * @param mem_gb - The size of the memory to allocate in gigabytes.
  * @return 0 if the memory allocation was successful, -1 otherwise.
 **/
int create_memory(uint8_t mem_gb) {
    // Calculate the total memory size in bytes
    size_t total_size = (size_t)mem_gb * 1024UL * 1024UL * 1024UL;

    // Allocate memory and check for allocation failure
    memory = malloc(total_size);
    if (memory == NULL) {
        return -1; // Memory allocation failed
    }

    return 0; // Memory allocation successful
}

/**
  * @brief Read data from a specific memory address.
  *
  * @param addr - The memory address from where the data should be read.
  * @param size - The size of the data in bytes.
  * @param data - Pointer to the data for read.
  * @return 0 if the read was successful, BUS_ERROR if the address corresponds to a bus error.
 **/
uint8_t read_memory(uint64_t addr, uint8_t size, uint64_t *data) {
    uint64_t readData;
    // Validate address against bus_error
    if (addr == bus_error) { return BUS_ERROR; }

    // Perform memory read
    memcpy(&readData, &memory[addr], size);

    if (size == 4) { *data = (readData & (uint64_t)0xFFFFFFFF); }
    else { *data = readData; }

    return 0; // Read successful
}

/**
  * @brief Writes data to a specific memory address.
  *
  * @param data - Pointer to the data to write.
  * @param addr - The memory address where the data should be written.
  * @param size - The size of the data in bytes.
  * @return 0 if the write was successful, BUS_ERROR if the address corresponds to a bus error.
 **/
uint8_t write_memory(uint64_t *data, uint64_t addr, uint32_t size) {
    uint64_t modifiedData;
    // Validate address against bus_error
    if (addr == bus_error) { return BUS_ERROR; }

    if (size == 4) { modifiedData = (*data & (uint64_t)0xFFFFFFFF); }
    else { modifiedData = *data; }

    // Perform memory write
    memcpy(&memory[addr], &modifiedData, size);

    return 0; // Write successful
}

/**
  * @brief SRCMD Table Configurations
  *
  * @param iopmp The IOPMP instance.
  * @param srcmd_reg It could be SRCMD_EN, SRCMD_ENH, SRCMD_R, SRCMD_RH, SRCMD_W, SRCMD_WH.
  * @param srcmd_idx It could be any legal SRCMD Table Index.
  * @param data Value that you want to write in this register.
  * @param num_bytes It could be 4-Byte write or 8-Byte Write.
 **/
void configure_srcmd_n(iopmp_dev_t *iopmp, uint8_t srcmd_reg, uint16_t srcmd_idx, reg_intf_dw data, uint8_t num_bytes){
    write_register(iopmp, SRCMD_TABLE_BASE_OFFSET + srcmd_reg + (srcmd_idx * 32), data, num_bytes);
}
/**
  * @brief MDCFG Table Configurations
  *
  * @param iopmp The IOPMP instance.
  * @param md_idx It could be any legal MDCFG Table Index.
  * @param data Value that you want to write in this register.
  * @param num_bytes It could be 4-Byte write or 8-Byte Write.
 **/
void configure_mdcfg_n(iopmp_dev_t *iopmp, uint8_t md_idx, reg_intf_dw data, uint8_t num_bytes){
    write_register(iopmp, MDCFG_TABLE_BASE_OFFSET + (md_idx * 4), data, num_bytes);
}

/**
  * @brief Entry Table Configurations
  *
  * @param iopmp The IOPMP instance.
  * @param entry_reg It could be ENTRY_ADDR, ENTRY_ADDRH, ENTRY_CFG, ENTRY_USER_CFG.
  * @param entry_idx It could be any legal Entry Table Index.
  * @param data Value that you want to write in this register.
  * @param num_bytes It could be 4-Byte write or 8-Byte Write.
 **/
void configure_entry_n(iopmp_dev_t *iopmp, uint8_t entry_reg, uint64_t entry_idx, reg_intf_dw data, uint8_t num_bytes){
    write_register(iopmp, iopmp->reg_file.entryoffset.offset + entry_reg + (entry_idx * 16), data, num_bytes);    // (364 >> 2) and keeping lsb 0
}

/**
 * @brief Read Entry Table
 *
 * @param iopmp The IOPMP instance.
 * @param entry_reg It could be ENTRY_ADDR, ENTRY_ADDRH, ENTRY_CFG, ENTRY_USER_CFG.
 * @param entry_idx It could be any legal Entry Table Index.
 * @param num_bytes It could be 4-Byte read or 8-Byte read.
  */
reg_intf_dw read_entry_n(iopmp_dev_t *iopmp, uint8_t entry_reg, uint64_t entry_idx, uint8_t num_bytes) {
    return read_register(iopmp, iopmp->reg_file.entryoffset.offset + entry_reg + (entry_idx * ENTRY_REG_STRIDE), num_bytes);
}

/**
  * @brief Receiver Port Signals
  *
  * @param rrid RRID Of the Bus Initiator
  * @param addr Address to be checked
  * @param length Number of transfers
  * @param size It should be 0 for 1-byte, 1 for 2-byte, 2 for 4-byte access.
  * @param perm The permissions required for this transcation.
  * @param is_amo Set this Flag, If it's a AMO transaction.
  * @param iopmp_trans_req This is pointer, pass it as it is.
 **/
void receiver_port(uint16_t rrid, uint64_t addr, uint32_t length, uint32_t size, perm_type_e perm, bool is_amo, iopmp_trans_req_t *iopmp_trans_req){
    iopmp_trans_req->rrid   = rrid;
    iopmp_trans_req->addr   = addr;
    iopmp_trans_req->length = length;
    iopmp_trans_req->size   = size;
    iopmp_trans_req->perm   = perm;
    iopmp_trans_req->is_amo = is_amo;
}

/**
  * @brief Set Enable High
  *
  * @param iopmp The IOPMP instance.
 **/
void set_hwcfg0_enable(iopmp_dev_t *iopmp) {
    hwcfg0_t hwcfg0;

    hwcfg0.raw = read_register(iopmp, HWCFG0_OFFSET, 4);
    hwcfg0.enable = true;
    write_register(iopmp, HWCFG0_OFFSET, hwcfg0.raw, 4);
}

/**
  * @brief error_record_check
  *
  * @param iopmp The IOPMP instance.
  * @param err_type RRID Of the Bus Initiator
  * @param req_perm Address to be checked
  * @param req_addr Errored Address
  * @param err_rcd Set if error should be recorded
 **/
int error_record_chk(iopmp_dev_t *iopmp, uint8_t err_type, uint8_t req_perm, uint64_t req_addr, bool err_rcd) {
    err_info_t err_info_temp;
    err_info_temp.raw = read_register(iopmp, ERR_INFO_OFFSET, 4);
    if (err_rcd){
        FAIL_IF((err_info_temp.v != 1));
        FAIL_IF((err_info_temp.ttype != req_perm));
        FAIL_IF((err_info_temp.etype != (err_type)));
        FAIL_IF((read_register(iopmp, ERR_REQADDR_OFFSET, 4) != (uint32_t)((req_addr >> 2) & 0xFFFFFFFF)));
        FAIL_IF((read_register(iopmp, ERR_REQADDRH_OFFSET, 4) != (iopmp->reg_file.hwcfg0.addrh_en ? (uint32_t)((req_addr >> 34) & 0xFFFFFFFF) : 0)));
    } else {
        FAIL_IF((err_info_temp.v == 1));
    }

    return 0;
}
