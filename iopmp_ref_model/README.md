# IOPMP Reference Model Documentation

The **Input/Output Physical Memory Protection (IOPMP)** is a hardware component designed to control and validate accesses issued from bus initiators. It checks the validity of these accesses in real-time. The **IOPMP Reference Model** is developed in compliance with the **RISC-V IOPMP Specification Version 0.8.2, 2026**. This model will be updated with future specification revisions.

## IOPMP Model Overview

The IOPMP Reference Model includes several distinct configurations, each offering different levels of functionality and flexibility in controlling physical memory protection. The following table provides an overview of the available models:

| **Model Name** | **SRCMD_FMT** | **MDCFG_FMT** | **Description** |
|----------------|---------------|---------------|-----------------|
| **Full Model** | 0             | 0             | **Max Supported RRIDs:** 65535<br> **Max Supported MDs:** 63<br> Uses the RRID to obtain `SRCMD_EN(H)`, which indicates the associated MDs.<br> The `MDCFG` table is traversed to retrieve the associated MDs.<br> Associated IOPMP entries are extracted from the respective MD and traversed for address matching and permission checks. |
| **Rapid-k Model** | 0          | 1             | **Max Supported RRIDs:** 65535<br> **Max Supported MDs:** 63<br> Uses the RRID to obtain `SRCMD_EN(H)`, indicating the associated MDs.<br> There is no physical `MDCFG` table in this model. Each MD has *k* associated IOPMP entries.<br> IOPMP entries linked to the MD associated with the RRID are traversed for address matching and permission checks.<br> **IOPMP Entry Ranges for Each MD:**<br>  `MD0 → 0 to (k - 1)`<br>  `MD1 → k to (2k - 1)`<br>  `MD2 → 2k to (3k - 1)`, and so on. |
| **Dynamic-k Model** | 0        | 2             | Same as the **Rapid-k Model**, but the value of *k* is programmable. |
| **Isolation Model** | 1        | 0             | **Max Supported RRIDs:** 63<br> **Max Supported MDs:** 63<br> There is no physical `SRCMD` table. Instead, `RRID i` directly maps to `MD i`.<br> Associated IOPMP entries are extracted from `MD i` and traversed for address matching and permission checks. |
| **Compact-k Model** | 1        | 1             | **Max Supported RRIDs:** 63<br> **Max Supported MDs:** 63<br> There is no physical `SRCMD` table. `RRID i` directly maps to `MD i`.<br> There is no physical `MDCFG` table. Each MD has *k* associated IOPMP entries.<br> **Associated IOPMP Entry Ranges:**<br>  `(i × k)` to `((i + 1) × k - 1)` for address matching and permission checks. |
| **Unnamed Model 1** | 1 | 2           | Same as the **Compact-k Model**, but the value of *k* is programmable. |
| **Unnamed Model 2** | 2                   | 0             | **Max Supported RRIDs:** 32<br> **Max Supported MDs:** 63<br> `SRCMD_EN(H)` and `SPS` extension registers are replaced with `SRCMD_PERM(H)`.<br> All MDs are associated with the given RRID.<br> The `MDCFG` table is traversed for all MDs.<br> Associated IOPMP entries are extracted and traversed for address matching and permission checks. |
| **Unnamed Model 3** | 2                   | 1             | **Max Supported RRIDs:** 32<br> **Max Supported MDs:** 63<br> `SRCMD_EN(H)` and `SPS` extension registers are replaced with `SRCMD_PERM(H)`.<br> There is no physical `MDCFG` table. Each MD has *k* associated IOPMP entries.<br> `SRCMD_PERM(H)` only defines permissions, not associated MDs. All IOPMP entries are traversed for address matching and permission checks. |
| **Unnamed Model 4** | 2                   | 2             | Same as **Unnamed Model 3**, but the value of *k* is programmable. |

## Supported Features
The **IOPMP Reference Model** incorporates all IOPMP Models and each model can be configured based upon SRCMD_FMT and MDCFG_FMT flags at compilation time. All features outlined in the **RISC-V IOPMP Specification Version 0.8.1, 2025**. You could find the key feature configurations under `iopmp_cfg_t` structure and `config.h`.

### Key feature configuration parameters in `iopmp_cfg_t` include:
| **Feature**  | **Possible Values** | **Description**                                                                                                                                                                                                                    |
| ------------------ | ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| vendor             | IMP                       | The JEDEC manufacturer ID. |
| specver            | IMP                       | The specification version. |
| impid              | IMP                       | The user-defined implementation ID. |
| enable             | 0/1                       | Indicate if the IOPMP checks transactions by default.<br />**0**: Initial to 0 and sticky to 1. IOPMP doesn't check transactions by default. <br />**1**: IOPMP checks transactions by default. |
| no_err_rec         | 0/1                       | Indicates if the Error Capture Record feature is not implemented.<br />**0**: Error record is implemented. <br />**1**: Error record is not implemented.                                                                       |
| md_num             | 0 - 63                    | Indicates the maximum number of supported Memory Domain (MD). |
| addrh_en           | 0/1                       | Indicates if higher-address fields are available.<br />**0**: Higher-address fields not available. <br />**1**: Higher-address fields are implemented.                                                                       |
| tor_en             | 0/1                       | Indicates if Top-Of-Range (TOR) addressing mode is supported.<br />**0**: TOR not supported. <br />**1**: TOR supported                                                                                                      |
| rrid_num           | 0 - 65535                 | Indicates the maximum number of Requestor IDs (RRIDs) supported.                                                                                                                                                                         |
| entry_num          | 0 - IMP                   | Indicates the total number of entries supported.                                                                                                                                                                                         |
| prio_entry         | 1 - ENTRY_NUM             | Indicates the number of entries matched based on priority.                                                                                                                                                                               |
| prio_ent_prog      | 0/1                       | Indicates if the `prio_entry` field in HWCFG2 is programmable.<br /> **0**: Field is fixed. <br />**1**: Field is programmable.    |
| non_prio_en        | 0/1                       | Indicates whether the IOPMP supports non-priority entries.    |
| msi_en             | 0/1                       | Indicates whether the IOPMP supports MSI extension.<br />**0:** MSI is not supported.<br />**1:** MSI is supported.                                                                                                           |
| peis               | 0/1                       | Indicates if Per-Entry Interrupt Suppression is supported.<br />**0**: Feature not supported. <br />**1**: Interrupts can be suppressed per entry.                                                                           |
| pees               | 0/1                       | Indicates if Per-Entry Error Suppression is supported.<br />**0**: Feature not supported. <br />**1**: Errors can be suppressed per entry.                                                                                   |
| sps_en             | 0/1                       | Indicates if Secondary Permission Settings (SPS) are supported.<br />**0**: SPS not supported. <br />**1**: SPS is supported                                                                                                    |
| stall_en           | 0/1                       | Indicates if stalling is supported.<br />**0**: No stalling supported. <br />**1**: Stalling features are implemented.                                                                                                       |
| mfr_en             | 0/1                       | Indicates if Multi-Faults Record Extension is supported.<br />**0**: Feature not supported. <br />**1**: Subsequent violations will be recorded.                                                                              |
| mdcfg_fmt          | 0 - 2                     | Indicate the MDCFG format:<br />**0**: Format 0. MDCFG Table is implemented. <br />**1**: Format 1. No MDCFG Table. HWCFG3.md_entry_num is fixed. <br />**2**: Format 2. No MDCFG Table. HWCFG3.md_entry_num is programmable.                                                                          |
| srcmd_fmt          | 0 - 2                     | Indicate the SRCMD Table format:<br />**0**: Format 0 (baseline). SRCMD_EN(s) and SRCMD_ENH(s) are available. <br />**1**: Exclusive Format. No SRCMD Table. RRID i associates exclusively with memory domain i, i = 0 … HWCFG0.md_num-1. <br />**2**: MD-indexed Format. SRCMD_PERM(m) and SRCMD_PERMH(m) are available.                                                                             |
| md_entry_num       | 0 - IMP                   | Indicates the number of entries associated with each Memory Domain (MD) when `mdcfg_fmt` is not 0.<br />Implementation defined value                                                                                                   |
| xinr               | 0/1                       | Indicates whether the IOPMP treats the instruction fetch accesses as data read accesses.<br />**0**: Fields of instruction fetches are not ignored<br />**1**: Fields of instruction fetches are ignored |
| no_x               | 0/1                       | Valid only if `xinr` = 0. <br />**0**: Instruction fetch checks are permissive. <br />**1**: Instruction fetches are always denied.                                                                                       |
| no_w               | 0/1                       | Indicates if write accesses are always denied.<br />**0**: Write accesses are allowed based on permissions. <br />**1**: Write accesses are always denied.                                                                   |
| rrid_transl_en     | 0/1                       | Indicates if tagging a new Requestor ID (RRID) on the initiator port is supported.<br />**0**: Tagging not supported. <br />**1**: New RRID tagging is supported.                                                            |
| rrid_transl_prog   | 0/1                       | Indicates if the `rrid_transl` field is programmable. <br />**0**: Field is fixed. <br />**1**: Field is programmable                                                                                                      |
| rrid_transl        | IMP                       | The RRID tagged to outgoing transactions. Supported only for `rrid_transl_en`=1. It is writable only when `rrid_transl_prog`=1.                                                                                                      |
| entryoffset        | IMP                       | The offset address of the IOPMP array from the base of an IOPMP instance.                                                                                                      |
| granularity        | IMP                       | The granularity (bytes) of protected regions by entry.                                                                                                      |
| imp_mdlck          | 0/1                       | Indicates if the Memory Domain Lock (MDLCK) feature is implemented.<br />**0**: Feature is not implemented<br />**1**: Memory domains can be locked.                                                                         |
| imp_err_reqid_eid  | 0/1                       | Indicates if `ERR_REQID.eid` is implemented. <br />**0**: Feature not supported. <br />**1**: Errored RRID and Entry num is recorded.                                                                                          |
| imp_rridscp        | 0/1                       | Indicates if RRIDSCP register is implemented.<br />**0:** RRIDSCP register is not implemented.<br />**1:** RRIDSCP register is implemented.                                                                                                 |
| imp_stall_buffer   | 0/1                       | Indicates if IOPMP implements buffer to record and store stalled transactions.<br />**0:** Stall buffer is not implemented.<br />**1:** Stall buffer is implemented.                                                                                                 |
| rridscp_unselectable | IMP                     | Unselectable RRID in RRID Cherry Pick Stall Control feature.<br />If `rridscp_unselectable[rrid]`=0, the RRID is selectable when writing RRIDSCP.<br />If `rridscp_unselectable[rrid]`=1, the RRID is unselectable when writing RRIDSCP.<br /><br />This configuration is optional. Implementation can make all RRIDs be selectable by default.                                                                                                 |
| stall_buffer_size  | IMP                       | Size of the buffer used to store stalled transactions.                                                                                                          |

### Key feature configuration parameters in `config.h` include:
| **Feature**  | **Possible Values** | **Description**                                                                                                                                                                                                                    |
| ------------------ | ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| USER               | IMP                       | User-defined value for error suppression success responses.                                                                                                           |
| SRC_ENFORCEMENT_EN             | 0/1                       | Indicates if Source Enforcement is enabled.<br />**0:** Source Enforcement is not enabled.<br />**1:** Source Enforcement is enabled.                                                                                                           |
| REG_INTF_BUS_WIDTH | 4/8                       | Specifies the width (in bytes) of the register interface bus.<br />**4**: 4-byte width. <br />**8**: 8-byte width.                                                                                                           |

## Reference Model Functions

These functions are designed for use within a testbench to input stimuli and obtain responses from the respective models:

1. **`int reset_iopmp(iopmp_dev_t *iopmp, iopmp_cfg_t *cfg)`**
   Resets the IOPMP states and registers to their default values. The function returns 0 if the reference model is successfully initialized. The instance is specified by `iopmp`. The desired configurations when reset are specified by `cfg`.

2. **`void write_register(iopmp_dev_t *iopmp, uint64_t offset, reg_intf_dw data, uint8_t num_bytes)`**
   Writes data to a memory-mapped register identified by the specified offset. The IOPMP instance to be written is specified by `iopmp`. The number of bytes written is specified by `num_bytes`. If the access is invalid, the write is ignored. The data type `reg_intf_dw` depends on the configuration in the `config.h` file (e.g., `uint32_t` for 4-byte width, `uint64_t` for 8-byte width).

3. **`reg_intf_dw read_register(iopmp_dev_t *iopmp, uint64_t offset, uint8_t num_bytes)`**
   Reads data from a memory-mapped register identified by the specified offset. The IOPMP instance to be read is specified by `iopmp`. The number of bytes to read is specified by `num_bytes`. If the access is invalid, the function returns zeros. The data type `reg_intf_dw` is dependent on the bus width defined in `config.h`.

4. **`void iopmp_validate_access(iopmp_dev_t *iopmp, iopmp_trans_req_t *trans_req, iopmp_trans_rsp_t *iopmp_trans_rsp, uint8_t *intrpt)`**
   `iopmp_check_access()` followed by `iopmp_report_violation()`.

5. **`void iopmp_check_access(iopmp_dev_t *iopmp, iopmp_trans_req_t *trans_req, iopmp_trans_rsp_t *iopmp_trans_rsp, iopmp_violation_t *violation)`**
   Checks the transaction request `trans_req` and returns the response in `iopmp_trans_rsp` without updating the error capture registers. `violation` holds the violation for `iopmp_report_violation()`. `ERR_CFG.ie`, `ERR_CFG.rs`, and the per-entry suppression bits are sampled by this function.

6. **`void iopmp_report_violation(iopmp_dev_t *iopmp, const iopmp_violation_t *violation, uint8_t *intrpt)`**
   Reports a violation found by `iopmp_check_access()`: IOPMP sets `ERR_INFO.v`, logs the error capture record if it is implemented, and triggers the interrupt. `intrpt` is set to 1 if a wired interrupt is triggered. The error capture record holds the first violation reported while `ERR_INFO.v` is 0. Reporting a legal transaction does nothing.

7. **`int create_memory(uint8_t mem_gb)`**
   Allocates memory of the specified size in gigabytes. mem_gb is used for the size of the memory to allocate in gigabytes. It returns 0 if the memory allocation was successful, -1 otherwise.

8. **`uint8_t read_memory(uint64_t addr, uint8_t size, uint64_t *data)`**
   Read data from a specific memory address. The param addr is the memory address from where the data should be read. The param size is the size of the data in bytes. The pram data is the pointer to the data for read. It returns 0 if the read was successful, BUS_ERROR if the address corresponds to a bus error.

9. **`uint8_t write_memory(uint64_t *data, uint64_t addr, uint32_t size)`**
   This function writes data to a specific memory address, the param data indicates Pointer to the data to write, the param addr is the memory address where the data should be written and param size will be the size of the data in bytes. it returns 0 if the write was successful, BUS_ERROR if the address corresponds to a bus error.

10. **`void configure_srcmd_n(iopmp_dev_t *iopmp, uint8_t srcmd_reg, uint16_t srcmd_idx, reg_intf_dw data, uint8_t num_bytes)`**
   This function is used for SRCMD Table Configurations. The param srcmd_reg could be SRCMD_EN, SRCMD_ENH, SRCMD_R, SRCMD_RH, SRCMD_W, SRCMD_WH. srcmd_idx could be any legal SRCMD table index, data contains the value that you want to write in this register. The IOPMP instance to be configured is specified by `iopmp`. The number of bytes to write is specified by `num_bytes`. It could be 4-Byte write or 8-Byte Write.

11. **`void configure_mdcfg_n(iopmp_dev_t *iopmp, uint8_t md_idx, reg_intf_dw data, uint8_t num_bytes)`**
   This function is used for MDCFG Table Configurations. md_idx could be any legal MDCFG table index, data contains the value that you want to write in this register. The IOPMP instance to be configured is specified by `iopmp`. The number of bytes to write is specified by `num_bytes`. It could be 4-Byte write or 8-Byte Write.

12. **`void configure_entry_n(iopmp_dev_t *iopmp, uint8_t entry_reg, uint64_t entry_idx, reg_intf_dw data, uint8_t num_bytes)`**
   This function is used for Entry Table Configurations. entry_idx could be any legal Entry table index, data contains the value that you want to write in this register. The IOPMP instance to be configured is specified by `iopmp`. The number of bytes to write is specified by `num_bytes`. It could be 4-Byte write or 8-Byte Write.

13. **`void receiver_port(uint16_t rrid, uint64_t addr, uint32_t length, uint32_t size, perm_type_e perm, bool is_amo, iopmp_trans_req_t *iopmp_trans_req)`**
   This function contains receiver port signals, that will be passed to the iopmp, where rrid is RRID Of the Bus Initiator, addr is Address to be checked, length is length Number of transfers, size is It should be 0 for 1-byte, 1 for 2-byte, 2 for 4-byte access, perm is the permissions required for this transcation, is_amo indicates whether this transaction is AMO or not, iopmp_trans_req is a pointer, pass it as it is.

14. **`int error_record_chk(iopmp_dev_t *iopmp, uint8_t err_type, uint8_t req_perm, uint64_t req_addr, bool err_rcd)`**
   This could be used to check the error record register data. The IOPMP instance to be checked is specified by `iopmp`. err_type is the type of Error, req_perm contains the Requested permissions, req_addr contains the requested address, err_rcd indicated if Set if error should be recorded

## **Compilation and Simulation of IOPMP Models**

The **IOPMP Reference Model** is written in C and requires a GCC compiler for compilation. Follow these steps to compile and simulate:

1. Adjust the `config.h` and `iopmp_cfg_t` as needed to customize IOPMP configurations.
2. Use the following commands to compile:

   - **Compile All Models**:
     ```bash
     make all
     ```
   - **Compile Specific Models**:
     ```bash
     make build model=full_model        # Full Model
     make build model=rapid_k_model     # Rapid-k Model
     make build model=dynamic_k_model   # Dynamic-k Model
     make build model=isolation_model   # Isolation Model
     make build model=compact_k_model   # Compact-k Model
     make build model=unnamed_model_1   # Unnamed Model 1
     make build model=unnamed_model_2   # Unnamed Model 2
     make build model=unnamed_model_3   # Unnamed Model 3
     make build model=unnamed_model_4   # Unnamed Model 4
     ```
   - **Compile Specific Models with libiopmp**:
     ```bash
     make all libiopmp=1
     make build libiopmp=1
     ```

3. After compilation, binaries are generated in the `bin` folder and the library files are generated in `lib` folder.

## IOPMP Reference Model Test Files

The `verif` directory contains a `test` folder that includes test files for each of the 9 models. You can add custom tests to the relevant test file for your preferred model.

---

This document serves as a comprehensive guide to the **IOPMP Reference Model** for Input/Output Physical Memory Protection, offering both detailed specifications and practical instructions for use in simulation and verification tasks.
