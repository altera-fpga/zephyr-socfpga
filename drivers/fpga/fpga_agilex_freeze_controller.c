/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT intel_freeze_controller

#include <zephyr/device.h>
#include <zephyr/drivers/freeze_controller.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fpga_agilex_freeze_controller);

/* FPGA freeze controller CSR status register offset */
#define FPGA_FREEZE_CSR_STATUS_OFFSET      0
/* FPGA freeze controller CSR control register offset */
#define FPGA_FREEZE_CSR_CTRL_OFFSET	      4
/* FPGA freeze controller CSR illegal request register offset */
#define FPGA_FREEZE_CSR_ILLEGAL_REQ_OFFSET 8
/* FPGA freeze controller version register offset */
#define FPGA_FREEZE_CSR_REG_VERSION	      12

/* FPGA freeze controller CSR supported version */
#define FPGA_FREEZE_CSR_SUPPORTED_VERSION 2
/* FPGA freeze controller CSR official version */
#define FPGA_FREEZE_CSR_OFFICIAL_VERSION  0xad000003
/* FPGA freeze controller freeze request CSR complete bit */
#define FPGA_FREEZE_CSR_STATUS_FREEZE_REQ_DONE   BIT(0)
/* FPGA freeze controller unfreeze request CSR complete bit */
#define FPGA_FREEZE_CSR_STATUS_UNFREEZE_REQ_DONE BIT(1)
/* FPGA freeze controller CSR freeze request bit */
#define FPGA_FREEZE_CSR_CTRL_FREEZE_REQ   BIT(0)
/* FPGA freeze controller CSR reset request bit */
#define FPGA_FREEZE_CSR_CTRL_RESET_REQ    BIT(1)
/* FPGA freeze controller CSR unfreeze request bit */
#define FPGA_FREEZE_CSR_CTRL_UNFREEZE_REQ BIT(2)
/* FPGA freeze controller status read delay */
#define FPGA_FREEZE_STATUS_INTERVAL_DELAY_US 1000
/* FPGA freeze controller status read retry */
#define FPGA_FREEZE_RETRY                  10

struct freeze_ctrl_config {
	DEVICE_MMIO_ROM;
};

struct freeze_ctrl_data {
	DEVICE_MMIO_RAM;
};

static inline mem_addr_t get_reg_base_addr(const struct device *dev)
{
	return (mem_addr_t)DEVICE_MMIO_GET(dev);
}

/**
 * @brief This will check for the request acknowledgment
 *
 * @param[in] addr freeze controller register base address
 * @param[in] req_ack request acknowledgment status mask
 * @return int 0 on success or negative value on fail
 */
static int intel_freeze_req_ack(mem_addr_t addr, uint32_t req_ack)
{
	uint32_t status, illegal, ctrl;
	uint32_t retry = FPGA_FREEZE_RETRY;
	int ret = -ETIMEDOUT;

	do {
		illegal = sys_read32(addr + FPGA_FREEZE_CSR_ILLEGAL_REQ_OFFSET);
		if (illegal) {
			LOG_ERR("Illegal request 0x%08x detected in freeze bridge", illegal);

			sys_write32(illegal, addr + FPGA_FREEZE_CSR_ILLEGAL_REQ_OFFSET);

			illegal = sys_read32(addr + FPGA_FREEZE_CSR_ILLEGAL_REQ_OFFSET);
			if (illegal)
				LOG_ERR("Illegal request 0x%08x detected in freeze bridge are not "
					"cleared",
					illegal);

			ret = -EINVAL;
			break;
		}

		status = sys_read32(addr + FPGA_FREEZE_CSR_STATUS_OFFSET);
		status &= req_ack;
		if (status) {
			ctrl = sys_read32(addr + FPGA_FREEZE_CSR_CTRL_OFFSET);
			LOG_DBG("%s request %x acknowledged %x %x", __func__, req_ack, status,
				ctrl);

			ret = 0;
			break;
		}

		k_sleep(K_MSEC(FPGA_FREEZE_STATUS_INTERVAL_DELAY_US));

	} while (retry--);

	return ret;
}

static int intel_freeze_do_freeze(const struct device *dev)
{
	uint32_t status, revision;
	int ret;
	mem_addr_t base_addr;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device is not ready");
		return -ENODEV;
	}

	base_addr = get_reg_base_addr(dev);

	revision = sys_read32(base_addr + FPGA_FREEZE_CSR_REG_VERSION);
	if ((revision != FPGA_FREEZE_CSR_SUPPORTED_VERSION) &&
	    (revision != FPGA_FREEZE_CSR_OFFICIAL_VERSION)) {
		LOG_ERR("%s : Unexpected revision 0x%x != 0x%x != 0x%x", dev->name, revision,
			FPGA_FREEZE_CSR_SUPPORTED_VERSION, FPGA_FREEZE_CSR_OFFICIAL_VERSION);
		return -EINVAL;
	}

	status = sys_read32(base_addr + FPGA_FREEZE_CSR_STATUS_OFFSET);
	if (status & FPGA_FREEZE_CSR_STATUS_FREEZE_REQ_DONE)
		return 0;
	else if (!(status & FPGA_FREEZE_CSR_STATUS_UNFREEZE_REQ_DONE)) {
		LOG_ERR("%s : Failed to complete the freeze request", dev->name);
		return -EINVAL;
	}

	sys_write32(FPGA_FREEZE_CSR_CTRL_FREEZE_REQ, base_addr + FPGA_FREEZE_CSR_CTRL_OFFSET);

	ret = intel_freeze_req_ack(base_addr, FPGA_FREEZE_CSR_STATUS_FREEZE_REQ_DONE);
	if (ret)
		sys_write32(0, base_addr + FPGA_FREEZE_CSR_CTRL_OFFSET);
	else
		sys_write32(FPGA_FREEZE_CSR_CTRL_RESET_REQ,
			    base_addr + FPGA_FREEZE_CSR_CTRL_OFFSET);

	return ret;
}

static int intel_freeze_do_unfreeze(const struct device *dev)
{
	uint32_t status;
	int ret;
	mem_addr_t base_addr;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device is not ready");
		return -ENODEV;
	}

	base_addr = get_reg_base_addr(dev);

	sys_write32(0, base_addr + FPGA_FREEZE_CSR_CTRL_OFFSET);

	status = sys_read32(base_addr + FPGA_FREEZE_CSR_STATUS_OFFSET);
	if (status & FPGA_FREEZE_CSR_STATUS_UNFREEZE_REQ_DONE)
		return 0;
	else if (!(status & FPGA_FREEZE_CSR_STATUS_FREEZE_REQ_DONE)) {
		LOG_ERR("%s : Failed to complete the unfreeze request", dev->name);
		return -EINVAL;
	}

	sys_write32(FPGA_FREEZE_CSR_CTRL_UNFREEZE_REQ, base_addr + FPGA_FREEZE_CSR_CTRL_OFFSET);

	ret = intel_freeze_req_ack(base_addr, FPGA_FREEZE_CSR_STATUS_UNFREEZE_REQ_DONE);

	sys_write32(0, base_addr + FPGA_FREEZE_CSR_CTRL_OFFSET);

	return ret;
}

static int fpga_freeze_ctrl_init(const struct device *dev)
{
	if (!dev) {
		LOG_ERR("No such device found");
		return -ENODEV;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

static const struct freeze_ctrl_driver freeze_ctrl_driver_api = {
	.freeze = intel_freeze_do_freeze,
	.unfreeze = intel_freeze_do_unfreeze,
};

#define CREATE_FREEZE_CTRL_DEVICE(inst)                                       \
	static struct freeze_ctrl_data freeze_ctrl_data_##inst;                   \
	static const struct freeze_ctrl_config freeze_ctrl_cfg_##inst = {         \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                               \
	};                                                                                  \
	DEVICE_DT_INST_DEFINE(inst, fpga_freeze_ctrl_init, NULL, &freeze_ctrl_data_##inst,   \
			      &freeze_ctrl_cfg_##inst, POST_KERNEL,                    \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &freeze_ctrl_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_FREEZE_CTRL_DEVICE);
