#ifndef __IOPMP_REF_API_H__
#define __IOPMP_REF_API_H__

#include "iopmp_registers.h"
#include "iopmp_req_rsp.h"

typedef struct iopmp_dev_t iopmp_dev_t;
typedef struct iopmp_cfg_t iopmp_cfg_t;

extern int reset_iopmp(iopmp_dev_t *iopmp, iopmp_cfg_t *cfg);
extern reg_intf_dw read_register(iopmp_dev_t *iopmp, uint64_t offset, uint8_t num_bytes);
extern void write_register(iopmp_dev_t *iopmp, uint64_t offset, reg_intf_dw data, uint8_t num_bytes);
extern void iopmp_validate_access(iopmp_dev_t *iopmp, iopmp_trans_req_t *trans_req, iopmp_trans_rsp_t* iopmp_trans_rsp, uint8_t *intrpt);
extern void iopmp_check_access(iopmp_dev_t *iopmp, iopmp_trans_req_t *trans_req, iopmp_trans_rsp_t *iopmp_trans_rsp, iopmp_violation_t *violation);
extern void iopmp_report_violation(iopmp_dev_t *iopmp, const iopmp_violation_t *violation, uint8_t *intrpt);

#endif
