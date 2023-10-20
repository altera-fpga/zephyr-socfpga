/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_FPGA_BRIDGE_INTEL_H_
#define ZEPHYR_SUBSYS_FPGA_BRIDGE_INTEL_H_

#include <zephyr/kernel.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include "../fpga_manager/fpga_manager_intel.h"

/* Mask for FPGA-HPS bridges */
#define BRIDGE_MASK					0x0F
/* Mailbox command header index */
#define MBOX_CMD_HEADER_INDEX       0x00
/* Mailbox command memory size */
#define FPGA_MB_CMD_ADDR_MEM_SIZE   20
/* Mailbox command response memory size */
#define FPGA_MB_RESPONSE_MEM_SIZE   20
/* Config status response length */
#define FPGA_CONFIG_STATUS_RESPONSE_LEN	   0x07

enum smc_request {
	/* SMC request parameter a2 index*/
	SMC_REQUEST_A2_INDEX = 0x00,
	/* SMC request parameter a3 index */
	SMC_REQUEST_A3_INDEX = 0x01
};

/* SIP SVC response private data */
struct private_data_t {
	struct sip_svc_response response;
	uint32_t *mbox_response_data;
	uint32_t mbox_response_len;
	struct k_sem smc_sem;
	struct fpga_config_status config_status;
};

#endif
