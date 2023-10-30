/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_smmu_v3

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/math/ilog2.h>
#include "arm_smmu_v3.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(arm_smmu_v3, CONFIG_LOG_DEFAULT_LEVEL);

#define DEV_CFG(_dev) ((const struct arm_smmu_v3_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct arm_smmu_v3_data *const)(_dev)->data)

#define REG_SYNC_TIMEOUT_US                     500

struct arm_smmu_v3_device_features {
	/* IDR0 COnfigurations */
	bool sev;
	bool hyp;
	uint32_t ias;
	uint32_t asid_bits;
	uint32_t vmdid_bits;

	/* IDR1 Configurations */
	uint32_t sid_bits;
	uint32_t ssid_bits;
	uint32_t cmdq_bits;
	uint32_t eventq_bits;
	uint32_t priq_bits;

	/* IDR5 Configurations */
	uint32_t pgsize_bitmap;
	uint32_t oas;
};

struct arm_smmu_v3_config {
	DEVICE_MMIO_ROM;
	const uint32_t cmdq_bits;
	const uint32_t eventq_bits;
	const uint32_t sid_bits;
	const uint32_t max_streams;
	const uint32_t max_ctx_desc;
	void (*irq_config)(void);
};

struct arm_smmu_v3_data {
	DEVICE_MMIO_RAM;
	struct arm_smmu_v3_device_features features;
	struct arm_smmu_v3_cmdq_ent *cmdq_ent;
	struct arm_smmu_v3_eventq_ent *eventq_ent;
	struct arm_smmu_v3_strtab_ent *strtab_ent;
	struct arm_smmu_v3_ctx_desc_ent *ctx_desc_ent;
};

struct arm_smmu_v3_queue_params {
	uint32_t prod;
	uint32_t cons;
	uint32_t qbits;
};

static uint64_t arm_smmu_v3_l1_xlat_def_table[ARM_SMMU_Ln_XLAT_NUM_ENTRIES]
	__aligned(ARM_SMMU_PAGE_4KB);
static uint64_t arm_smmu_v3_l2_xlat_def_table[ARM_SMMU_Ln_XLAT_NUM_ENTRIES]
	__aligned(ARM_SMMU_PAGE_4KB);

static bool arm_smmu_v3_queue_empty(const struct arm_smmu_v3_queue_params *qp)
{
	bool empty = false;

	if ((ARM_SMMU_Q_IDX(qp->prod, qp) == ARM_SMMU_Q_IDX(qp->cons, qp)) &&
		(ARM_SMMU_Q_WRP(qp->prod, qp) == ARM_SMMU_Q_WRP(qp->cons, qp))) {
		empty = true;
	}

	return empty;
}

static uint32_t arm_smmu_v3_queue_get_cons_inc(const struct arm_smmu_v3_queue_params *qp)
{
	uint32_t cons;

	/*
	 * hardware consumer value extract.
	 */
	cons = (ARM_SMMU_Q_WRP(qp->cons, qp) | ARM_SMMU_Q_IDX(qp->cons, qp)) + 1;

	/* set the Wrap bit for consumer value and set the consumer index */
	cons = (ARM_SMMU_Q_OVF(qp->cons) | ARM_SMMU_Q_WRP(cons, qp) | ARM_SMMU_Q_IDX(cons, qp));

	return cons;
}

/* event handler */
static void arm_smmu_v3_handle_event(uint64_t *event)
{
	uint32_t event_id, stream_id;
	uint64_t input_addr, ste_fetch_addr, ipa;
	bool rnw, is_inst_data, privilege;

	event_id = FIELD_GET(ARM_SMMU_EVTQ_0_ID_MASK, event[0]);

	/* get stream id from event queue element */
	stream_id = FIELD_GET(ARM_SMMU_EVTQ_0_SID_MASK, event[0]);

	switch (event_id) {
	case ARM_SMMU_EVT_ID_BAD_STREAMID:
		LOG_ERR("StreamID:%u out of range", stream_id);
		break;

	case ARM_SMMU_EVT_ID_STE_FETCH:
		ste_fetch_addr = FIELD_GET(ARM_SMMU_EVTQ_3_STE_ADDR_FETCH_MASK, event[3]);
		LOG_ERR("STE Fetch error\n"
			"Stream ID: %u\n"
			"STE fetch addr: 0x%llx", stream_id, ste_fetch_addr);
		break;

	case ARM_SMMU_EVT_ID_BAD_STE:
		LOG_ERR("STE is invalid");
		break;

	case ARM_SMMU_EVT_ID_TRANSLATION_FAULT:

		rnw = ARM_SMMU_EVTQ_1_RnW & event[1];
		is_inst_data = ARM_SMMU_EVTQ_1_InD & event[1];
		privilege = ARM_SMMU_EVTQ_1_PnU & event[1];
		ipa = FIELD_GET(ARM_SMMU_EVTQ_3_IPA_MASK, event[3]);

		input_addr = FIELD_GET(ARM_SMMU_EVTQ_2_ADDR_MASK, event[2]);
		LOG_ERR("Translation Fault: failed to translate address\n"
			"Stream ID: %u\n"
			"64-bit Input address: 0x%llx\n"
			"Read/Write nature of incoming transaction RnW: %s\n"
			"Instruction/Data: %s\n"
			"PnU: %s\nIPA:0x%llx",
			stream_id, input_addr, rnw ? "read" : "write",
			is_inst_data ? "Instruction" : "Data",
			privilege ? "Privileged" : "Unprivileged", ipa);
		break;

	case ARM_SMMU_EVT_ID_ADDR_SIZE_FAULT:
		rnw = ARM_SMMU_EVTQ_1_RnW & event[1];
		is_inst_data = ARM_SMMU_EVTQ_1_InD & event[1];
		privilege = ARM_SMMU_EVTQ_1_PnU & event[1];
		ipa = FIELD_GET(ARM_SMMU_EVTQ_3_IPA_MASK, event[3]);

		input_addr = FIELD_GET(ARM_SMMU_EVTQ_2_ADDR_MASK, event[2]);
		LOG_ERR("Address Size Fault: output addr of translation caused size fault\n"
			"Stream ID: %u\n"
			"64-bit Input address: 0x%llx\n"
			"Read/Write nature of incoming transaction RnW: %s\n"
			"Instruction/Data: %s\n"
			"PnU: %s\nIPA:0x%llx",
			stream_id, input_addr, rnw ? "read" : "write",
			is_inst_data ? "Instruction" : "Data",
			privilege ? "Privileged" : "Unprivileged", ipa);
		break;

	case ARM_SMMU_EVT_ID_ACCESS_FAULT:
		rnw = ARM_SMMU_EVTQ_1_RnW & event[1];
		is_inst_data = ARM_SMMU_EVTQ_1_InD & event[1];
		privilege = ARM_SMMU_EVTQ_1_PnU & event[1];
		ipa = FIELD_GET(ARM_SMMU_EVTQ_3_IPA_MASK, event[3]);

		input_addr = FIELD_GET(ARM_SMMU_EVTQ_2_ADDR_MASK, event[2]);
		LOG_ERR("Access Fault occurred\n"
			"Stream ID: %u\n"
			"64-bit Input address: 0x%llx\n"
			"Read/Write nature of incoming transaction RnW: %s\n"
			"Instruction/Data: %s\n"
			"PnU: %s\nIPA:0x%llx",
			stream_id, input_addr, rnw ? "read" : "write",
			is_inst_data ? "Instruction" : "Data",
			privilege ? "Privileged" : "Unprivileged", ipa);
		break;

	case ARM_SMMU_EVT_ID_PERMISSION_FAULT:
		rnw = ARM_SMMU_EVTQ_1_RnW & event[1];
		is_inst_data = ARM_SMMU_EVTQ_1_InD & event[1];
		privilege = ARM_SMMU_EVTQ_1_PnU & event[1];
		ipa = FIELD_GET(ARM_SMMU_EVTQ_3_IPA_MASK, event[3]);

		input_addr = FIELD_GET(ARM_SMMU_EVTQ_2_ADDR_MASK, event[2]);
		LOG_ERR("Permission Fault occurred on page access\n"
			"Stream ID: %u\n"
			"64-bit Input address: 0x%llx\n"
			"Read/Write nature of incoming transaction RnW: %s\n"
			"Instruction/Data: %s\n"
			"PnU: %s\nIPA:0x%llx",
			stream_id, input_addr, rnw ? "read" : "write",
			is_inst_data ? "Instruction" : "Data",
			privilege ? "Privileged" : "Unprivileged", ipa);
		break;
	default:
		LOG_ERR("Event ID: %u Stream ID: %u\n", event_id, stream_id);
	}
}

static void arm_smmu_v3_queue_get_params(const struct device *dev,
	struct arm_smmu_v3_queue_params *qp, uint32_t queue_off,
	uint32_t prod_off, uint32_t cons_off)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	qp->prod = sys_read32((reg_base + prod_off));
	qp->cons = sys_read32((reg_base + cons_off));
	qp->qbits = (sys_read64((reg_base + queue_off)) & ARM_SMMU_Q_BASE_LOG2SIZE_MASK);
}

static void arm_smmu_v3_event_q_handler(const struct device *dev)
{
	uint32_t cons;
	uint64_t *evtq_entry = NULL;
	uint64_t evt[ARM_SMMU_EVTQ_ENT_DWORDS];
	struct arm_smmu_v3_queue_params qp;
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);

	uint64_t *queue_base = (uint64_t *)dev_data->eventq_ent;

	arm_smmu_v3_queue_get_params(dev, &qp, ARM_SMMU_CMDQ_BASE,
			ARM_SMMU_EVTQ_PROD, ARM_SMMU_EVTQ_CONS);

	if (arm_smmu_v3_queue_empty(&qp) == true) {
		LOG_ERR("Queue is empty\n");
		return;
	}

	do {
		cons = qp.cons;
		/* get the entry from event queue slot based on the consumer index */
		evtq_entry = (queue_base + (ARM_SMMU_Q_IDX(cons, (&qp)) *
				ARM_SMMU_EVTQ_ENT_DWORDS));
		memcpy((void *)evt, (void *)evtq_entry, ARM_SMMU_EVTQ_ENT_SIZE);

		/* handle an event */
		arm_smmu_v3_handle_event(evt);

		/* update consumer value once event is consumed and update the consumer index */
		cons = arm_smmu_v3_queue_get_cons_inc(&qp);
		sys_write32(cons, (reg_base + ARM_SMMU_EVTQ_CONS));

		arm_smmu_v3_queue_get_params(dev, &qp, ARM_SMMU_CMDQ_BASE,
			ARM_SMMU_EVTQ_PROD, ARM_SMMU_EVTQ_CONS);

	} while (arm_smmu_v3_queue_empty(&qp) == false);
}

static void arm_smmu_v3_gerror_handler(const struct device *dev)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t gerror, gerrorn, active;

	gerror = sys_read32(reg_base + ARM_SMMU_GERROR);
	gerrorn = sys_read32(reg_base + ARM_SMMU_GERRORN);

	active = gerror ^ gerrorn;

	LOG_WRN("Unexpected global error code: 0x%x", gerror);

	if (active & ARM_SMMU_GERROR_CMDQ_ERR) {
		LOG_ERR("Command queue error is reported");
	} else if (active & ARM_SMMU_GERROR_EVTQ_ABT_ERR) {
		LOG_ERR("Event queue abort error - Events may be lost");
	} else if (active & ARM_SMMU_GERROR_SFM_ERR) {
		LOG_ERR("Device has entered Service Failure Mode(SFM)!");
	} else if (active & ARM_SMMU_GERROR_MSI_GERROR_ABT_ERR) {
		LOG_ERR("GERROR MSI write aborted");
	} else if (active & ARM_SMMU_GERROR_MSI_EVTQ_ABT_ERR) {
		LOG_ERR("Event Queue MSI write aborted");
	} else if (active & ARM_SMMU_GERROR_MSI_CMDQ_ABT_ERR) {
		LOG_ERR("Command queue MSI write aborted");
	} else if (active & ARM_SMMU_GERROR_MSI_PRIQ_ABT_ERR) {
		LOG_ERR("PRIQ MSI write aborted");
	} else if (active & ARM_SMMU_GERROR_PRIQ_ABT_ERR) {
		LOG_ERR("PRI Queue write aborted");
	}

	sys_write32(gerror, reg_base + ARM_SMMU_GERRORN);
}

/**
 * L1, L2 and L3 tables are filled as flat block entries
 * having one-to-one mapping for the address ranges
 * as per their respective tables.
 *
 * Each table has 512 entries and is of 4KB each.
 *
 * These tables entries will be used as default entries to
 * populate the translation tables created dynamically during
 * device attach for each stream.
 *
 * The dynamic table entries will be then mapped to required
 * actual physical locations for the stream ID when the mapping
 * function is called.
 */
static void arm_smmu_v3_xlat_def_table_init(void)
{
	uint32_t i = 0;

	/*
	 * L1 Table can hold upto 512GB of address ranges.
	 * Each L1 Entry can hold upto 1GB address range
	 */
	arm_smmu_v3_l1_xlat_def_table[0] = ARM_SMMU_Ln_TABLE_ENTRY(arm_smmu_v3_l2_xlat_def_table,
				ARM_SMMU_DESC_BLOCK_UPPER_DEFAULT);

	/*
	 * currently we are sharing 512MB if the DDR Memory to SDM
	 * So we will only fill Half the entries.
	 */
	for (i = 0; i < (ARM_SMMU_Ln_XLAT_NUM_ENTRIES/2); i++) {
		/* L2 Table can hold upto 1GB of address ranges.
		 * Each L2 Entry can hold upto 2MB of address range.
		 */
		arm_smmu_v3_l2_xlat_def_table[i] =
				ARM_SMMU_L2_BLOCK_ENTRY((0x0080000000ULL +
					(i * ARM_SMMU_BLOCK_2MB)),
					ARM_SMMU_DESC_BLOCK_UPPER_DEFAULT,
					ARM_SMMU_S2_DESC_BLOCK_LOWER_MEMORY_SHARED_OUTER);
	}
}

/* writes the value to the register and waits for the associated acknowledgment bit to be set */
static int arm_smmu_v3_write_reg_sync(const struct device *dev, uint32_t val,
		uint32_t reg_off, uint32_t ack_off)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	int ret = 0;

	sys_write32(val, (reg_base + reg_off));

	/* TODO: Check on the NULL parameter */
	ret = !WAIT_FOR((sys_read32((reg_base + ack_off)) == val), REG_SYNC_TIMEOUT_US, NULL);

	if (ret != 0) {
		ret = -ETIMEDOUT;
	}

	return ret;
}

/* queue maintenance functions */
static void arm_smmu_v3_queue_write(uint64_t *queue_base, uint64_t *cmd,
	const struct arm_smmu_v3_queue_params *qp, uint32_t prod,
	uint32_t n_dwords)
{
	uint64_t *cmdq_entry = NULL;

	/* get the next empty command queue slot based on the producer index */
	cmdq_entry = (queue_base + (ARM_SMMU_Q_IDX(prod, qp) * n_dwords));

	memcpy((void *)cmdq_entry, (void *)cmd, ARM_SMMU_CMDQ_ENT_SIZE);
}

/* check if there is enough space in the queue */
static bool arm_smmu_v3_queue_has_space(const struct arm_smmu_v3_queue_params *qp,
	uint32_t ncmds)
{
	uint32_t space, prod, cons;

	prod = ARM_SMMU_Q_IDX(qp->prod, qp);
	cons = ARM_SMMU_Q_IDX(qp->cons, qp);

	if (ARM_SMMU_Q_WRP(qp->prod, qp) == ARM_SMMU_Q_WRP(qp->cons, qp)) {
		space = BIT(qp->qbits) - (prod - cons);
	} else {
		space = cons - prod;
	}

	return (space >= ncmds);
}

/* check if the queue is consumed until the specified producer value */
static bool arm_smmu_v3_queue_consumed(const struct arm_smmu_v3_queue_params *qp, uint32_t prod)
{
	bool consumed = false;

	if ((ARM_SMMU_Q_IDX(prod, qp) == ARM_SMMU_Q_IDX(qp->cons, qp)) &&
		 (ARM_SMMU_Q_WRP(prod, qp) == ARM_SMMU_Q_WRP(qp->cons, qp))) {
		consumed = true;
	}

	return consumed;
}

static uint32_t arm_smmu_v3_queue_get_prod_inc_n(const struct arm_smmu_v3_queue_params *qp,
							uint32_t n)
{
	uint32_t prod = 0;

	/*
	 * hardware producer value extract.
	 * add the number of commands to be added to current producer index
	 */
	prod = (ARM_SMMU_Q_WRP(qp->prod, qp) | ARM_SMMU_Q_IDX(qp->prod, qp)) + n;

	/* set the wrap bit for new producer value and set the producer index */
	prod = (ARM_SMMU_Q_OVF(qp->prod) | ARM_SMMU_Q_WRP(prod, qp) | ARM_SMMU_Q_IDX(prod, qp));

	return prod;
}

static int arm_smmu_v3_cmdq_build_cmd(uint64_t *cmd, const struct arm_smmu_v3_cmdq_fields *fields)
{
	memset(cmd, 0, ARM_SMMU_CMDQ_ENT_SIZE);

	cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_0_OP_MASK, fields->opcode);

	switch (fields->opcode) {
	case ARM_SMMU_CMDQ_OP_TLBI_EL2_ALL:
	case ARM_SMMU_CMDQ_OP_TLBI_NSNH_ALL:
		break;

	case ARM_SMMU_CMDQ_OP_PREFETCH_CFG:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_PREFETCH_0_SID_MASK, fields->prefetch.sid);
		break;

	case ARM_SMMU_CMDQ_OP_CFGI_CD:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_CFGI_0_SSID_MASK, fields->cfgi.ssid);
		/* fallthrough */
	case ARM_SMMU_CMDQ_OP_CFGI_STE:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_CFGI_0_SID_MASK, fields->cfgi.sid);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_CFGI_1_LEAF, fields->cfgi.leaf);
		break;

	case ARM_SMMU_CMDQ_OP_CFGI_CD_ALL:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_CFGI_0_SID_MASK, fields->cfgi.sid);
		break;

	case ARM_SMMU_CMDQ_OP_CFGI_ALL:
		/* cover the entire SID range */
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_CFGI_1_RANGE_MASK, 31);
		break;

	case ARM_SMMU_CMDQ_OP_TLBI_NH_VA:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_VMID_MASK, fields->tlbi.vmid);
		/* fallthrough */
	case ARM_SMMU_CMDQ_OP_TLBI_EL2_VA:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_NUM_MASK, fields->tlbi.num);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_SCALE_MASK, fields->tlbi.scale);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_ASID_MASK, fields->tlbi.asid);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_1_LEAF, fields->tlbi.leaf);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_1_TTL_MASK, fields->tlbi.ttl);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_1_TG_MASK, fields->tlbi.tg);
		cmd[1] |= fields->tlbi.addr & ARM_SMMU_CMDQ_TLBI_1_VA_MASK;
		break;

	case ARM_SMMU_CMDQ_OP_TLBI_S2_IPA:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_NUM_MASK, fields->tlbi.num);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_SCALE_MASK, fields->tlbi.scale);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_VMID_MASK, fields->tlbi.vmid);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_1_LEAF, fields->tlbi.leaf);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_1_TTL_MASK, fields->tlbi.ttl);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_1_TG_MASK, fields->tlbi.tg);
		cmd[1] |= fields->tlbi.addr & ARM_SMMU_CMDQ_TLBI_1_IPA_MASK;
		break;

	case ARM_SMMU_CMDQ_OP_TLBI_NH_ASID:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_ASID_MASK, fields->tlbi.asid);
		/* fallthrough */
	case ARM_SMMU_CMDQ_OP_TLBI_S12_VMALL:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_VMID_MASK, fields->tlbi.vmid);
		break;

	case ARM_SMMU_CMDQ_OP_TLBI_EL2_ASID:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_TLBI_0_ASID_MASK, fields->tlbi.asid);
		break;

	case ARM_SMMU_CMDQ_OP_ATC_INV:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_0_SSV, fields->substream_valid);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_ATC_0_GLOBAL, fields->atc.global);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_ATC_0_SSID_MASK, fields->atc.ssid);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_ATC_0_SID_MASK, fields->atc.sid);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_ATC_1_SIZE_MASK, fields->atc.size);
		cmd[1] |= fields->atc.addr & ARM_SMMU_CMDQ_ATC_1_ADDR_MASK;
		break;

	case ARM_SMMU_CMDQ_OP_PRI_RESP:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_0_SSV, fields->substream_valid);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_PRI_0_SSID_MASK, fields->pri.ssid);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_PRI_0_SID_MASK, fields->pri.sid);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_PRI_1_GRPID_MASK, fields->pri.grpid);
		switch (fields->pri.resp) {
		case PRI_RESP_DENY:
		case PRI_RESP_FAIL:
		case PRI_RESP_SUCC:
			break;
		default:
			return -EINVAL;
		}
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_PRI_1_RESP_MASK, fields->pri.resp);
		break;

	case ARM_SMMU_CMDQ_OP_RESUME:
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_RESUME_0_SID_MASK, fields->resume.sid);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_RESUME_0_RESP_MASK, fields->resume.resp);
		cmd[1] |= FIELD_PREP(ARM_SMMU_CMDQ_RESUME_1_STAG_MASK, fields->resume.stag);
		break;

	case ARM_SMMU_CMDQ_OP_CMD_SYNC:
		if (fields->sync.msiaddr) {
			cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_SYNC_0_CS_MASK,
						ARM_SMMU_CMDQ_SYNC_0_CS_IRQ);
			cmd[1] |= fields->sync.msiaddr & ARM_SMMU_CMDQ_SYNC_1_MSIADDR_MASK;
		} else {
			cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_SYNC_0_CS_MASK,
						ARM_SMMU_CMDQ_SYNC_0_CS_SEV);
		}
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_SYNC_0_MSH_MASK, ARM_SMMU_SH_ISH);
		cmd[0] |= FIELD_PREP(ARM_SMMU_CMDQ_SYNC_0_MSIATTR_MASK, ARM_SMMU_MEMATTR_OIWB);
		break;

	default:
		LOG_ERR("invalid command queue opcode 0x%x\n", fields->opcode);
		return -EINVAL;
	}

	return 0;
}

static void arm_smmu_v3_cmdq_build_sync_cmd(uint64_t *cmd)
{
	const struct arm_smmu_v3_cmdq_fields cmd_fields = {
		.opcode = ARM_SMMU_CMDQ_OP_CMD_SYNC,
	};

	/* TODO: MSI Handling */

	arm_smmu_v3_cmdq_build_cmd(cmd, &cmd_fields);
}

static int arm_smmu_v3_cmdq_issue_cmdlist(const struct device *dev,
		uint64_t *cmds, uint32_t ncmds, bool sync)
{
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	struct arm_smmu_v3_queue_params qp = {0};
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t i = 0;
	uint32_t prod = 0;
	uint64_t cmd_sync[ARM_SMMU_CMDQ_ENT_DWORDS] = {0};

	/* wait for command queue to have space to hold ncmds */
	while (true) {
		arm_smmu_v3_queue_get_params(dev, &qp, ARM_SMMU_CMDQ_BASE,
			ARM_SMMU_CMDQ_PROD, ARM_SMMU_CMDQ_CONS);

		if (arm_smmu_v3_queue_has_space(&qp, (ncmds + sync)) == true) {
			break;
		}
		/* TODO: Check if you want to add Wait FOR */
	}

	/* write all the commands into the command queue */
	for (i = 0; i < ncmds; ++i) {
		uint64_t *cmd = &cmds[i * ARM_SMMU_CMDQ_ENT_DWORDS];
		/* TODO : Get latest hardware status for runtime processing design */
		/* Ensure to calculate the producer everytime we add a command into queue
		 * so that we get correct command slot if there was a wrap around.
		 */
		prod = arm_smmu_v3_queue_get_prod_inc_n(&qp, i);

		/* write the command to queue */
		arm_smmu_v3_queue_write((uint64_t *)dev_data->cmdq_ent, cmd, &qp,
			prod, ARM_SMMU_CMDQ_ENT_DWORDS);
	}

	/* add sync command to the queue if required */
	if (sync) {
		arm_smmu_v3_cmdq_build_sync_cmd(cmd_sync);

		/* calculate the producer value with number of commands and additional
		 * sync command.
		 * ncmd = 0 to no. of commands + sync comma
		 */
		prod = arm_smmu_v3_queue_get_prod_inc_n(&qp, (ncmds));

		/* Write the sync command to queue */
		arm_smmu_v3_queue_write((uint64_t *)dev_data->cmdq_ent, cmd_sync, &qp,
			prod, ARM_SMMU_CMDQ_ENT_DWORDS);
	}

	/* initiate Data synchronization on bus */
	/* dsb(); */

	/* advance the hardware command queue producer */
	prod = arm_smmu_v3_queue_get_prod_inc_n(&qp, (ncmds + sync));

	sys_write32(prod, (reg_base + ARM_SMMU_CMDQ_PROD));

	/* if sync is required wait until the sync command is processed */
	if (sync) {
		/* TODO: add some timeout OR MSI based logic */
		do {
			/* get the current consumer value */
			arm_smmu_v3_queue_get_params(dev, &qp, ARM_SMMU_CMDQ_BASE,
				ARM_SMMU_CMDQ_PROD, ARM_SMMU_CMDQ_CONS);

			/* check if the queue has been consumed until the sync command entry */
			if (arm_smmu_v3_queue_consumed(&qp, prod) == true) {
				break;
			}

			/* TODO: Add a wait for */

		} while (true);
	}

	return 0;
}

static int arm_smmu_v3_cmdq_issue_cmd(const struct device *dev,
		struct arm_smmu_v3_cmdq_fields *fields, bool sync)
{
	uint64_t cmd[ARM_SMMU_CMDQ_ENT_DWORDS];
	int ret = 0;

	ret = arm_smmu_v3_cmdq_build_cmd(cmd, fields);

	if (ret != 0) {
		return ret;
	}

	ret = arm_smmu_v3_cmdq_issue_cmdlist(dev, cmd, 1, sync);

	return ret;
}

static int arm_smmu_v3_cmdq_init(const struct device *dev)
{
	const struct arm_smmu_v3_config *dev_cfg = DEV_CFG(dev);
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	struct arm_smmu_v3_cmdq_fields cmd_fields;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint64_t reg_val = 0;
	int ret = 0;

	if (dev_cfg->cmdq_bits > dev_data->features.cmdq_bits) {
		LOG_ERR("Configured command queue entries exceed the maximum allowed entries");
		return -EINVAL;
	}

	reg_val =  FIELD_PREP(ARM_SMMU_Q_BASE_LOG2SIZE_MASK, dev_cfg->cmdq_bits);
	reg_val |= ARM_SMMU_Q_BASE_ADDR_MASK & (uint64_t)dev_data->cmdq_ent;
	reg_val |= ARM_SMMU_Q_BASE_RWA;

	/* program the command queue base register */
	sys_write64(reg_val, (reg_base + ARM_SMMU_CMDQ_BASE));

	/* reset the command queue producer and consumer registers */
	sys_write32(0, (reg_base + ARM_SMMU_CMDQ_PROD));

	sys_write32(0, (reg_base + ARM_SMMU_CMDQ_CONS));

	/* enable Command Queue */
	reg_val = sys_read32((reg_base + ARM_SMMU_CR0)) | ARM_SMMU_CR0_CMDQEN;

	ret = arm_smmu_v3_write_reg_sync(dev, reg_val, ARM_SMMU_CR0, ARM_SMMU_CR0ACK);

	if (ret != 0) {
		LOG_ERR("enabling command queue timedout");
		return ret;
	}

	/* invalidate any cached configuration for all StreamID's */
	cmd_fields.opcode = ARM_SMMU_CMDQ_OP_CFGI_ALL;

	ret = arm_smmu_v3_cmdq_issue_cmd(dev, &cmd_fields, true);

	if (ret != 0) {
		LOG_ERR("invalidate cached configuration failed");
		return ret;
	}

	/* invalidate all non-Secure TLB entries */
	cmd_fields.opcode = ARM_SMMU_CMDQ_OP_TLBI_NSNH_ALL;

	ret = arm_smmu_v3_cmdq_issue_cmd(dev, &cmd_fields, true);

	if (ret != 0) {
		LOG_ERR("invalidate Non-Secure TLB entries failed");
	}

	return ret;
}

static int arm_smmu_v3_evtq_init(const struct device *dev)
{
	const struct arm_smmu_v3_config *dev_cfg = DEV_CFG(dev);
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint64_t reg_val = 0;
	int ret = 0;

	if (dev_cfg->eventq_bits > dev_data->features.eventq_bits) {
		LOG_ERR("configured event queue entries exceed the maximum allowed entries");
		return -EINVAL;
	}

	reg_val =  FIELD_PREP(ARM_SMMU_Q_BASE_LOG2SIZE_MASK, dev_cfg->eventq_bits);
	reg_val |= ARM_SMMU_Q_BASE_ADDR_MASK & (uint64_t)dev_data->eventq_ent;
	reg_val |= ARM_SMMU_Q_BASE_RWA;

	/* program the event queue base register */
	sys_write64(reg_val, (reg_base + ARM_SMMU_EVTQ_BASE));

	/* reset the event queue producer and consumer registers */
	sys_write32(0, (reg_base + ARM_SMMU_EVTQ_PROD));

	sys_write32(0, (reg_base + ARM_SMMU_EVTQ_CONS));

	/* enable event queue */
	reg_val = sys_read32((reg_base + ARM_SMMU_CR0)) | ARM_SMMU_CR0_EVTQEN;

	ret = arm_smmu_v3_write_reg_sync(dev, reg_val, ARM_SMMU_CR0, ARM_SMMU_CR0ACK);

	if (ret != 0) {
		LOG_ERR("enabling Event Queue timedout");
	}

	return ret;
}

static int arm_smmu_v3_update_strtab_ent_smmu(const struct device *dev, uint32_t sid)
{
	uint64_t ste_val;
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	const struct arm_smmu_v3_config *dev_cfg = DEV_CFG(dev);

	uint64_t *ste_tab_base = (uint64_t *)dev_data->strtab_ent;
	uint64_t *entry;

	if (sid > dev_cfg->max_streams || sid == 0) {
		LOG_ERR("invalid stream ID:%u\n", sid);
		return -EINVAL;
	}

	/* linear stream table for the given sid */
	entry = ste_tab_base + (sid * ARM_SMMU_STRTAB_STE_DWORDS);

	/**
	 * configure entry[0] i.e., [64:0]
	 */

	/* set V[0]: valid stream */
	ste_val = ARM_SMMU_STRTAB_STE_0_V;

	/* set config[3:1]: stream configuration for stage 2 translation */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_0_CFG_MASK, ARM_SMMU_STRTAB_STE_0_CFG_S2_TRANS);

	entry[0] = ste_val;

	/**
	 * set SHCFG[109:108] shareability override
	 * set the shareability to incoming
	 */
	entry[1] = FIELD_PREP(ARM_SMMU_STRTAB_STE_1_SHCFG_MASK,
		ARM_SMMU_STRTAB_STE_1_SHCFG_INCOMING);

	/* set stage 2 translate and stage 1 bypass */

	/**
	 * configure entry[2] i.e., [191:128]
	 */

	/* set VMID to 0 [143:128]*/
	ste_val = FIELD_PREP(ARM_SMMU_STRTAB_STE_2_S2VMID_MASK, ARM_SMMU_STRTAB_STE_2_VMID);

	/* set S2T0SZ[165:160] size of IPA */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2T0SZ_MASK, ARM_SMMU_S2T0SZ);

	/* set S2SL0[167:166] starting level of stage 2 translatop table walk */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2SL0_MASK, 1);

	/*
	 * set S2IR0[169:168] inner region cacheability for stage 2 translation table
	 * setting Write-back Cacheable, Read-Allocate, Write-Allocate
	 */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2IR0_MASK,
		ARM_SMMU_STRTAB_STE_2_S2IR0_WBRAWA);

	/*
	 * set S2IO0[171:170] inner region cacheability for stage 2 translation table
	 * setting Write-back Cacheable, Read-Allocate, Write-Allocate
	 */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2OR0_MASK,
		ARM_SMMU_STRTAB_STE_2_S2OR0_WBRAWA);

	/*
	 * set S2SH0[173:172 shareability for stage 2 translation table access
	 * set outer shareable
	 */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2SH0_MASK, ARM_SMMU_STRTAB_STE_2_SH0_OS);

	/* set S2TG[175:174]: stage 2 translation granule size: 4KB */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2TG_MASK, ARM_SMMU_STRTAB_STE_2_S2TG_4KB);

	/*
	 * set S2PS[178:176]: set physical address size
	 * setting physical address size as 40-bit to represent 1TiB space
	 */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_MASK,
		ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_40BIT);

	/*
	 * set S2AA64[179] to 1: set Arm architecture 64(AArch64)
	 * set S2AFFD[181] to 1: access flag fault never happens
	 */
	ste_val |= ARM_SMMU_STRTAB_STE_2_S2AA64 | ARM_SMMU_STRTAB_STE_2_S2AFFD;

	/* set S2ENDI[180] to 0: little endian table walk */
	ste_val |= (ste_val & ~ARM_SMMU_STRTAB_STE_2_S2ENDI);

	/* set S2AFFD[181] to 1: access flag fault never happens */
	ste_val |= ARM_SMMU_STRTAB_STE_2_S2AFFD;

	/* set [S2HA:S2HD][184:183]: HTTU disabled */
	ste_val |= FIELD_PREP(ARM_SMMU_STRTAB_STE_2_S2HTTU_MASK,
		ARM_SMMU_STRTAB_STE_2_S2HTTU_DISABLE);

	entry[2] = ste_val;

	/* setting stage2 TTBR register */
	entry[3] = (ARM_SMMU_STRTAB_STE_3_S2TTB_MASK & (uint64_t)arm_smmu_v3_l1_xlat_def_table);

	return 0;
}

static int arm_smmu_v3_strtab_init(const struct device *dev)
{
	const struct arm_smmu_v3_config *dev_cfg = DEV_CFG(dev);
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint64_t reg_val = 0;
	/* uint64_t *strtab_entry = (uint64_t *)dev_data->strtab_ent; */

	if (dev_cfg->sid_bits > dev_data->features.sid_bits) {
		LOG_ERR("configured streamID entries exceed the maximum allowed entries");
		return -EINVAL;
	}

	/* set all Stream table entries to bypass mode */
	/*
	 * for (i = 0; i < dev_cfg->max_streams; ++i) {
	 *	arm_smmu_v3_write_strtab_ent(dev, strtab_entry, UINT32_MAX, true, true);
	 *	strtab_entry += ARM_SMMU_STRTAB_STE_DWORDS;
	 * }
	 */

	/* setting the STE */
	arm_smmu_v3_update_strtab_ent_smmu(dev,  10);

	/* setup stream table base address and read allocate hint */
	reg_val = ARM_SMMU_STRTAB_BASE_ADDR_MASK & (uint64_t)dev_data->strtab_ent;
	reg_val |= ARM_SMMU_STRTAB_BASE_RA;

	/* program the stream table base register */
	sys_write64(reg_val, (reg_base + ARM_SMMU_STRTAB_BASE));

	/* setup stream table base configurations.
	 * set as a Liner stream table with no spit.
	 */
	reg_val =  FIELD_PREP(ARM_SMMU_STRTAB_BASE_CFG_LOG2SIZE_MASK, dev_cfg->sid_bits);
	reg_val |= FIELD_PREP(ARM_SMMU_STRTAB_BASE_CFG_FMT_MASK,
		ARM_SMMU_STRTAB_BASE_CFG_FMT_LINEAR);

	/* program the stream table base config register */
	sys_write32(reg_val, (reg_base + ARM_SMMU_STRTAB_BASE_CFG));

	return 0;
}

static int arm_smmu_v3_device_disable(const struct device *dev)
{
	int ret;

	ret = arm_smmu_v3_write_reg_sync(dev, 0, ARM_SMMU_CR0, ARM_SMMU_CR0ACK);

	if (ret != 0) {
		LOG_ERR("timedout while disabling smmu");
	}

	return ret;
}

static int arm_smmu_v3_device_reset(const struct device *dev)
{
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	const struct arm_smmu_v3_config *dev_cfg = DEV_CFG(dev);
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint64_t reg_val = 0;
	int ret = 0;

	reg_val = sys_read32((reg_base + ARM_SMMU_CR0));

	/* clear CR0 and sync (disables SMMU and queue processing) */
	if ((reg_val & ARM_SMMU_CR0_SMMUEN) == ARM_SMMU_CR0_SMMUEN) {
		LOG_WRN("SMMU currently enabled! Resetting!!...");

		/* TODO: arm_smmu_v3_update_gbpa(smmu, GBPA_ABORT, 0); */
	}

	ret = arm_smmu_v3_device_disable(dev);

	if (ret != 0) {
		return ret;
	}

	/* program the Table and Queue attributes */
	reg_val = FIELD_PREP(ARM_SMMU_CR1_TABLE_SH_MASK, ARM_SMMU_SH_ISH) |
			  FIELD_PREP(ARM_SMMU_CR1_TABLE_OC_MASK, ARM_SMMU_CR1_CACHE_WB) |
			  FIELD_PREP(ARM_SMMU_CR1_TABLE_IC_MASK, ARM_SMMU_CR1_CACHE_WB) |
			  FIELD_PREP(ARM_SMMU_CR1_QUEUE_SH_MASK, ARM_SMMU_SH_ISH) |
			  FIELD_PREP(ARM_SMMU_CR1_QUEUE_OC_MASK, ARM_SMMU_CR1_CACHE_WB) |
			  FIELD_PREP(ARM_SMMU_CR1_QUEUE_IC_MASK, ARM_SMMU_CR1_CACHE_WB);

	sys_write32(reg_val, (reg_base + ARM_SMMU_CR1));

	/* TODO: recheck on CR2 settings */
	reg_val = ARM_SMMU_CR2_PTM | ARM_SMMU_CR2_RECINVSID;

	if (dev_data->features.hyp == true) {
		reg_val |= ARM_SMMU_CR2_E2H;
	}

	sys_write32(reg_val, (reg_base + ARM_SMMU_CR2));

	/* initialize stream table */
	ret = arm_smmu_v3_strtab_init(dev);

	/* initialize and enable command queue */
	ret = arm_smmu_v3_cmdq_init(dev);

	if (ret != 0) {
		return ret;
	}

	/* initialize and enable event queue */
	ret = arm_smmu_v3_evtq_init(dev);

	if (ret != 0) {
		return ret;
	}

	/* TODO: iRQ Setup */
	dev_cfg->irq_config();

	reg_val = sys_read32(reg_base + ARM_SMMU_IRQ_CTRL) | ARM_SMMU_IRQ_CTRL_EVTQ_IRQEN |
						ARM_SMMU_IRQ_CTRL_GERROR_IRQEN;

	ret = arm_smmu_v3_write_reg_sync(dev, reg_val, ARM_SMMU_IRQ_CTRL, ARM_SMMU_IRQ_CTRLACK);

	if (ret != 0) {
		LOG_ERR("timedout while enabling interrupts");
	}

	return 0;
}

static int arm_smmu_v3_device_probe(const struct device *dev)
{
	struct arm_smmu_v3_data *const dev_data = DEV_DATA(dev);
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t capability;

	dev_data->features.ias = 40;
	dev_data->features.oas = 40;
	dev_data->features.pgsize_bitmap = 100;

	capability = sys_read32(reg_base + ARM_SMMU_IDR0);
	dev_data->features.asid_bits = capability & ARM_SMMU_IDR0_ASID16;
	dev_data->features.vmdid_bits = capability & ARM_SMMU_IDR0_VMID16;
	dev_data->features.sev = !!(capability & ARM_SMMU_IDR0_SEV);
	dev_data->features.hyp = !!(capability & ARM_SMMU_IDR0_HYP);
	capability = sys_read32(reg_base + ARM_SMMU_IDR1);
	dev_data->features.cmdq_bits = FIELD_GET(ARM_SMMU_IDR1_CMDQS_MASK, capability);
	dev_data->features.eventq_bits = FIELD_GET(ARM_SMMU_IDR1_EVTQS_MASK, capability);
	dev_data->features.sid_bits = FIELD_GET(ARM_SMMU_IDR1_SIDSIZE_MASK, capability);
	dev_data->features.ssid_bits = FIELD_GET(ARM_SMMU_IDR1_SSIDSIZE_MASK, capability);
	dev_data->features.priq_bits = FIELD_GET(ARM_SMMU_IDR1_PRIQS_MASK, capability);

	capability = sys_read32(reg_base + ARM_SMMU_IDR3);
	capability = sys_read32(reg_base + ARM_SMMU_IDR5);

	return 0;
}

static int arm_smmu_v3_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint64_t reg_val = 0;
	int ret = 0;

	ret = arm_smmu_v3_device_probe(dev);

	if (ret != 0) {
		LOG_ERR("SMMU device probe failed");
		return ret;
	}

	arm_smmu_v3_xlat_def_table_init();

	ret = arm_smmu_v3_device_reset(dev);

	if (ret != 0) {
		LOG_ERR("SMMU initialization failed");
		return ret;
	}

	/* Enable SMMU HW */
	reg_val = sys_read32((reg_base + ARM_SMMU_CR0)) | ARM_SMMU_CR0_SMMUEN;

	ret = arm_smmu_v3_write_reg_sync(dev, reg_val, ARM_SMMU_CR0, ARM_SMMU_CR0ACK);

	if (ret != 0) {
		LOG_ERR("timedout while enabling SMMU");
	}

	return 0;
}

#define CREATE_SMMU_V3_DEVICE(inst)								\
												\
static struct arm_smmu_v3_cmdq_ent cmdq_ent_##inst[DT_INST_PROP(inst, cmdq_entries)]		\
	__aligned((DT_INST_PROP(inst, cmdq_entries) * ARM_SMMU_CMDQ_ENT_SIZE));			\
												\
static struct arm_smmu_v3_eventq_ent eventq_ent_##inst[DT_INST_PROP(inst, eventq_entries)]	\
	__aligned((DT_INST_PROP(inst, eventq_entries) * ARM_SMMU_EVTQ_ENT_SIZE));		\
												\
static struct arm_smmu_v3_strtab_ent strtab_ent_##inst[DT_INST_PROP(inst, max_streams)]		\
	__aligned((DT_INST_PROP(inst, max_streams) * ARM_SMMU_STRTAB_STE_SIZE));		\
												\
static struct arm_smmu_v3_ctx_desc_ent ctx_desc_ent_##inst[DT_INST_PROP(inst, max_streams)]	\
	__aligned((DT_INST_PROP(inst, max_streams) * ARM_SMMU_CTXDESC_CD_SIZE));		\
												\
static struct arm_smmu_v3_data arm_smmu_v3_data_##inst = {					\
	.cmdq_ent = cmdq_ent_##inst,								\
	.eventq_ent = eventq_ent_##inst,							\
	.strtab_ent = strtab_ent_##inst,							\
	.ctx_desc_ent = ctx_desc_ent_##inst,							\
	};											\
												\
static void arm_smmu_v3_irq_config_##inst(void)							\
{												\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),						\
			DT_INST_IRQ_BY_IDX(inst, 0, priority),					\
			arm_smmu_v3_gerror_handler,						\
			DEVICE_DT_INST_GET(inst),						\
			DT_INST_IRQ_BY_IDX(inst, 0, flags));					\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));				\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 1, irq),						\
			DT_INST_IRQ_BY_IDX(inst, 1, priority),					\
			arm_smmu_v3_event_q_handler,						\
			DEVICE_DT_INST_GET(inst),						\
			DT_INST_IRQ_BY_IDX(inst, 1, flags));					\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, 1, irq));				\
}												\
static struct arm_smmu_v3_config arm_smmu_v3_config_##inst = {					\
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),						\
	.cmdq_bits = ilog2(DT_INST_PROP(inst, cmdq_entries)),					\
	.eventq_bits = ilog2(DT_INST_PROP(inst, eventq_entries)),				\
	.sid_bits = ilog2(DT_INST_PROP(inst, max_streams)),					\
	.max_streams = DT_INST_PROP(inst, max_streams),						\
	.max_ctx_desc = DT_INST_PROP(inst, max_streams),					\
	.irq_config = arm_smmu_v3_irq_config_##inst,						\
};												\
DEVICE_DT_INST_DEFINE(inst,									\
		arm_smmu_v3_init,								\
		NULL,										\
		&arm_smmu_v3_data_##inst,							\
		&arm_smmu_v3_config_##inst,							\
		POST_KERNEL,									\
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,						\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(CREATE_SMMU_V3_DEVICE)
