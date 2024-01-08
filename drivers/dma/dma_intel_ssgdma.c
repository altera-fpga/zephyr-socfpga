/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_ssgdma

#include <zephyr/kernel.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dma/dma_intel_ssgdma.h>
#include <string.h>

/**
 * The channel ID is design dependent and has fixed order: D2H_ST->H2D_ST->H2D_MM
 * Example a: 1 D2H_ST, 1 H2D_ST, 0 H2D_MM Ports
 * channel 0 = D2H_ST0, channel 1 = H2D_ST0
 * Example b: 2 D2H_ST, 0 H2D_ST, 2 H2D_MM Ports
 * channel 0 = D2H_ST0, channel 1 = D2H_ST1, channel 2 = H2D_MM0, channel 3 = H2D_MM1
 * Example c: 0 D2H_ST, 1 H2D_ST, 1 H2D_MM Ports
 * channel 0 = H2D_ST0, channel 1 = H2D_MM0
 * Example d: 1 D2H_ST, 1 H2D_ST, 1 H2D_MM Ports
 * channel 0 = H2D_ST0, channel 1 = H2D_ST0, channel 2 = H2D_MM0
 */

LOG_MODULE_REGISTER(intel_ssgdma, CONFIG_DMA_LOG_LEVEL);

#define DEV_CFG(_dev)	((const struct intel_ssgdma_dev_cfg *)(_dev)->config)
#define DEV_DATA(_dev)  ((struct intel_ssgdma_dev_data *const)(_dev)->data)

/**
 * @brief dma driver channel specific information
 */
struct intel_ssgdma_channel_data {
	/* Base address of device port control and status register */
	uint32_t port_csr_base; /* csr address space of the channel */
	enum intel_ssgdma_port_type_e port_type;
	uint16_t port_num;
	/* user should set same q_size and q_resp_size. */
	uint8_t q_size; /* number of descriptor blocks. */
	uint8_t q_resp_size; /* number of responder blocks. */
	uint16_t max_desc_hw_idx; /* maximum hw descriptor index. */
	uint16_t max_resp_buff_idx; /* maximum responder index to wrap around. */
	uint16_t max_desc_data_desc; /* maximum number of data descriptors. */
	/* desc_ptr and resp_ptr should only be set by the driver! */
	union intel_ssgdma_hw_queue_desc *desc_ptr;  /* ptr to data descriptors. */
	union intel_ssgdma_hw_queue_desc *resp_ptr;  /* ptr to responders. */
	uint16_t sw_insert_hw_idx; /* tracker for sw filled descriptors. */
	uint16_t sw_extract_hw_idx; /* tracker for sw processed responder. */
	uint16_t resp_ptr_act_idx; /* tracker for current slot in resp_ptr. */
	/* Each bits represents a sw status for the device port. Refer #define. */
	uint16_t channel_flags;
	enum dma_channel_direction channel_direction; /* direction of last transfer */
	bool error_callback_en;
};

/**
 * @brief dma controller driver data structure
 */
struct intel_ssgdma_dev_data {
	/* mmio address mapping info for dma controller */
	DEVICE_MMIO_NAMED_RAM(dma_mmio);
	/* max number of channel supported by dma controller */
	uint16_t num_channels_per_type[INTEL_SSGDMA_TOTAL_PORT_TYPES];
	uint16_t max_hw_channel;
	/* if true, for 32-bit system host address is restricted to 32 byte aligned and
	 * descriptor length must be multiple of 4. Doubles for 64-bit system.
	 */
	bool restrict_unaligned_access;
	struct intel_ssgdma_channel_data *channel_data;
	/* Each bits represents a sw status for the device port. Refer #define. */
	uint16_t device_errors_flags;
	/* user call back for dma transfer completion */
	dma_callback_t complete_callback;
	/* user data for dma callback for dma transfer completion */
	void *user_data;
	struct k_spinlock lock;
	uint8_t *desc_buff; /* descriptor buffer that is statically allocated during init.*/
};

/**
 * @brief Device constant configuration structure
 */
struct intel_ssgdma_dev_cfg {
	/* dma address space to map */
	DEVICE_MMIO_NAMED_ROM(dma_mmio);
	/* DEVICE_MMIO_ROM; */
	/* dma controller interrupt configuration function pointer */
	void (*irq_config)(void);
};

/*************************************************************************************************
 * Driver APIs that are only called internally.
 *************************************************************************************************/

/**
 * @brief get insert and extract pointer.
 */
static int intel_ssgdma_get_ie_ptr(uint32_t port_csr_base, uint16_t *hw_insert_ptr,
	uint16_t *hw_extract_ptr)
{
	if ((port_csr_base == 0) || (hw_insert_ptr == NULL) || (hw_extract_ptr == NULL)) {
		LOG_ERR("Unable to get insert and extract pointers. Invalid input");
		return -EINVAL;
	}

	*hw_insert_ptr = sys_read32(port_csr_base + INTEL_SSGDMA_QCSR_Q_INSERT_POINTER)
					& INTEL_SSGDMA_QCSR_Q_INSERT_POINTER_Q_INS_PTR_MSK;
	*hw_extract_ptr = sys_read32(port_csr_base + INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER)
					& INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER_Q_EXTR_PTR_MSK;

	return 0;
}

/**
 * @brief check responder status for error.
 */
bool intel_ssgdma_resp_error(union intel_ssgdma_hw_queue_desc *resp_loc, uint32_t channel)
{
	bool ret = false;

	if (resp_loc->resp_desc.format_field & INTEL_SSGDMA_MEMORY_TRANSFER_MSK) {
		if (resp_loc->resp_desc.status & INTEL_SSGDMA_MEM_RESP_DEVICE_ERROR_MSK) {
			LOG_ERR("Device error detected for channel %d, block index %d",
				channel, resp_loc->resp_desc.descr_idx);
			ret = true;
		}

		if (resp_loc->resp_desc.status & INTEL_SSGDMA_MEM_RESP_EARLY_TERMINATION_MSK) {
			LOG_ERR("Early termintation detected for channel %d, block index %d",
				channel, resp_loc->resp_desc.descr_idx);
			ret = true;
		}
	} else if (resp_loc->resp_desc.format_field & INTEL_SSGDMA_STREAMING_TRANSFER_MSK) {
		if (resp_loc->resp_desc.status & INTEL_SSGDMA_ST_RESP_EARLY_TERMINATION_MSK) {
			LOG_ERR("Early termintation detected for channel %d, block index %d",
				channel, resp_loc->resp_desc.descr_idx);
			ret = true;
		}
	} else {
		/* return error for unknown responder format. */
		ret = true;
	}

	return ret;
}


/**
 * @brief service the completed responder
 *
 * @details if responder writeback is enabled, hw will construct and send responder upon descriptor
 * completion. This function will clear the complete status and check if there are any error in
 * status. If error is detected, function will return INTEL_SSGDMA_ERROR.
 */
int intel_ssgdma_service_all_completed_resp(struct intel_ssgdma_channel_data *channel_ptr,
	uint32_t channel)
{
	int ret = 0;
	union intel_ssgdma_hw_queue_desc *resp_loc;

	if (channel_ptr->q_resp_size == 0U) {
		return ret; /* no error if responder is disabled. */
	}

	while (channel_ptr->sw_insert_hw_idx != channel_ptr->sw_extract_hw_idx) {
		resp_loc = channel_ptr->resp_ptr + channel_ptr->resp_ptr_act_idx;
		if (resp_loc->resp_desc.status & INTEL_SSGDMA_RESP_COMPLETE_MSK) {
			/* clear complete status */
			resp_loc->resp_desc.status &= ~INTEL_SSGDMA_RESP_COMPLETE_MSK;
			if (intel_ssgdma_resp_error(resp_loc, channel)) {
				ret = INTEL_SSGDMA_ERROR;
			}

			/* update sw tracker */
			channel_ptr->sw_extract_hw_idx = resp_loc->resp_desc.descr_idx;
			channel_ptr->resp_ptr_act_idx++;

			/* wrap around! */
			if (channel_ptr->resp_ptr_act_idx > channel_ptr->max_resp_buff_idx) {
				channel_ptr->resp_ptr_act_idx = 0;
			}
		} else {
			/* first none complete found break. */
			break;
		}
	}

	return ret;
}

/***
 * @brief interrupt service routine
 */
static void intel_ssgdma_isr(const struct device *dev)
{
	int cb_status;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	struct intel_ssgdma_channel_data *channel_ptr = NULL;
	uint32_t i, irq_source, q_status, global_status;
	bool complete;
	bool error = false;
	uint16_t hw_insert_ptr = 0;
	uint16_t hw_extract_ptr = 0;

	/* determie the interrupt source. */
	irq_source = sys_read32(reg_base + INTEL_SSGDMA_GCSR_DEVICE_PORTS_IRQ_STATUS);
	for (i = 0; i < ssgdma_dev_data->max_hw_channel; i++) {
		if (irq_source & (1U << i)) {
			channel_ptr = &(ssgdma_dev_data->channel_data[i]);
			if (channel_ptr->port_csr_base == 0U) {
				LOG_ERR("Unable to service irq. port_csr_base is 0. Check init.");
				return;
			}
			q_status = sys_read32(channel_ptr->port_csr_base +
				INTEL_SSGDMA_QCSR_STATUS);
			if (q_status & (INTEL_SSGDMA_QCSR_STATUS_Q_PREFETCH_ERROR_MSK |
				INTEL_SSGDMA_QCSR_STATUS_Q_ERROR_MSK)) {
				error = true;
			}

			/* Descriptor complete. */
			complete = (q_status & (INTEL_SSGDMA_QCSR_STATUS_Q_DESC_COMPLETION_MSK |
				INTEL_SSGDMA_QCSR_STATUS_Q_VIDEO_FLUSHING_EVENT_MSK));
			if (complete & (ssgdma_dev_data->complete_callback != NULL)) {
				/* service completed responder. */
				if (intel_ssgdma_service_all_completed_resp(channel_ptr, i) != 0) {
					error = true;
				}
				if (error && (channel_ptr->error_callback_en)) {
					/* errors are handled in user callback. */
					cb_status = INTEL_SSGDMA_ERROR;
				} else {
					/* return value ignored, as no error in this context. */
					(void)intel_ssgdma_get_ie_ptr(channel_ptr->port_csr_base,
						&hw_insert_ptr, &hw_extract_ptr);
					if (hw_insert_ptr != hw_extract_ptr) {
						cb_status = DMA_STATUS_BLOCK;
					} else {
						cb_status = DMA_STATUS_COMPLETE;
					}
				}
				ssgdma_dev_data->complete_callback(dev,
					ssgdma_dev_data->user_data, i, cb_status);
			}
			/* clear status after service. Related bits are W1C*/
			sys_write32(q_status, channel_ptr->port_csr_base +
				INTEL_SSGDMA_QCSR_STATUS);
		}
	}
	/* clear global status. All status bits are W1C. */
	global_status = sys_read32(reg_base + INTEL_SSGDMA_GCSR_STATUS);
	sys_write32(global_status, reg_base + INTEL_SSGDMA_GCSR_STATUS);
}

/***
 * @brief check if given direction is supported by intel ssgdma
 */
static int intel_ssgdma_check_direction(enum intel_ssgdma_port_type_e port_type,
	uint32_t channel_direction)
{
	int ret = -EINVAL;

	if (channel_direction == PERIPHERAL_TO_PERIPHERAL) {
		return -ENOTSUP;
	}

	switch (port_type) {
	case INTEL_SSGDMA_D2H_ST_PORT:
		if (channel_direction == PERIPHERAL_TO_MEMORY) {
			ret = 0;
		}
		break;
	case INTEL_SSGDMA_H2D_ST_PORT:
		if (channel_direction == MEMORY_TO_PERIPHERAL) {
			ret = 0;
		}
		break;
	case INTEL_SSGDMA_H2D_MM_PORT:
		if ((channel_direction == HOST_TO_MEMORY) ||
			(channel_direction == MEMORY_TO_HOST) ||
			(channel_direction == MEMORY_TO_MEMORY)) {
			ret = 0;
		}
		break;
	default:
		break;
	}

	return ret;
}

/***
 * @brief set register to start a channel.
 *
 * @details this function does not check if channel_ptr and channel_ptr->port_csr_base
 * are valid. These should be checked in the caller function.
 * No spinlock for this function, as the lock is acquired when calling intel_ssgdma_setup_channel.
 */
static void intel_ssgdma_queue_ctrl_enable(struct intel_ssgdma_channel_data *channel_ptr)
{
	uint32_t mask;

	mask = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
	/* guard to prevent enable and reset set at the same time. */
	mask |= INTEL_SSGDMA_QCSR_CTRL_Q_EN_MSK;
	mask &= ~INTEL_SSGDMA_QCSR_CTRL_Q_SW_RESET_REQ_MSK;
	sys_write32(mask, channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
}

/***
 * @brief set memory for descriptor.
 *
 * @details All inputs arguments are checked in intel_ssgdma_setup_channel.
 * The descriptor buffer for the whole dma controller is allocated statically with the init macro.
 * This function assigned the descriptor block to the individual channel.
 */
static void intel_ssgdma_channel_data_prepare_desc(struct intel_ssgdma_dev_data *const
	ssgdma_dev_data, uint32_t channel)
{
	uint32_t num_desc_bytes = 0;
	ssgdma_addr_t resp_offset;
	struct intel_ssgdma_channel_data *channel_ptr;

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);

	if (channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_REUSE_SETUP_MSK) {
		channel_ptr->channel_flags &= ~INTEL_SSGDMA_DEVICE_PORT_REUSE_SETUP_MSK;
	} else {
		/* cast to uint32_t to avoid multiplication overflow. */
		num_desc_bytes = (uint32_t) channel_ptr->q_size * INTEL_DESC_BLOCK_BYTES;

		/* assign memory */
		channel_ptr->desc_ptr = (union intel_ssgdma_hw_queue_desc *)
			(ssgdma_dev_data->desc_buff + (num_desc_bytes * channel));

		if (channel_ptr->q_resp_size) {
			resp_offset = (ssgdma_addr_t) num_desc_bytes *
				ssgdma_dev_data->max_hw_channel;

			/* cast to uint32_t to avoid multiplication overflow. */
			num_desc_bytes = (uint32_t) channel_ptr->q_resp_size *
				INTEL_DESC_BLOCK_BYTES;
			/* assign memory */
			channel_ptr->resp_ptr = (union intel_ssgdma_hw_queue_desc *)
			(ssgdma_dev_data->desc_buff + resp_offset + (num_desc_bytes * channel));
		} else {
			channel_ptr->resp_ptr = NULL;
		}
	}
}

/***
 * @brief checks if given index belongs to a link descriptor.
 */
static bool intel_ssgdma_is_link_desc_idx(uint16_t idx)
{
	return (bool)((idx % INTEL_SSGDMA_NUM_DESC_PER_BLOCK) ==
			INTEL_SSGDMA_LINK_DESC_IDX_IN_BLOCK);
}

/***
 * @brief configure link descriptor.
 */
static int intel_ssgdma_configure_link_desc(
	union intel_ssgdma_hw_queue_desc *desc, uint16_t desc_idx,
	ssgdma_addr_t next_block_addr,
	enum intel_ssgdma_port_type_e port_type)
{
	int ret = 0;

	/*checks if this is a link descriptor*/
	if (desc && intel_ssgdma_is_link_desc_idx(desc_idx)) {
		switch (port_type) {
		case INTEL_SSGDMA_H2D_MM_PORT:
			desc->queue_link_desc.format_field = INTEL_SSGDMA_H2D_MM_LINK_DESC_FORMAT;
			break;
		case INTEL_SSGDMA_H2D_ST_PORT:
			desc->queue_link_desc.format_field = INTEL_SSGDMA_H2D_ST_LINK_DESC_FORMAT;
			break;
		case INTEL_SSGDMA_D2H_ST_PORT:
			desc->queue_link_desc.format_field = INTEL_SSGDMA_D2H_ST_LINK_DESC_FORMAT;
			break;
		default:
			ret = -EINVAL;
			break;
		}

		/*only configure if port type is valid. */
		if (ret == 0) {
			desc->queue_link_desc.control = INTEL_SSGDMA_VALID_DESC_MSK;
			desc->queue_link_desc.descr_idx = desc_idx;
			desc->queue_link_desc.next_block_address_lower =
				INTEL_SSGDMA_LOWER_32_BITS(next_block_addr);
			desc->queue_link_desc.next_block_address_upper =
				INTEL_SSGDMA_UPPER_32_BITS(next_block_addr);
			desc->queue_link_desc.reserved1 = 0U;
		}
	} else {
		ret = -EINVAL;
	}

	return ret;
}

/***
 * @brief set addresses of responder list.
 */
static int intel_ssgdma_queue_insert_responder_block(
		struct intel_ssgdma_channel_data *channel_ptr,
		ssgdma_addr_t q_addr, uint8_t responder_block_idx)
{
	int ret = -EINVAL;
	uint32_t q_resp_addr_h, q_resp_addr_l, responder_ofst;

	if (responder_block_idx >= INTEL_SSGDMA_MAX_RESP_BLOCKS) {
		/* This should not happen. */
		LOG_ERR("API : queue insert responder block invalid index!");
	} else {
		if (channel_ptr->port_csr_base) {
			q_resp_addr_l = INTEL_SSGDMA_LOWER_32_BITS(q_addr);
			q_resp_addr_h = INTEL_SSGDMA_UPPER_32_BITS(q_addr);

			/* offset for ADDR_L */
			responder_ofst =
				(uint32_t)INTEL_SSGDMA_QCSR_Q_RESPONDER_ADDR_L_START +
				((uint32_t)responder_block_idx *
				INTEL_SSGDMA_QCSR_Q_RESPONDER_ADDR_REG_SIZE);

			/* write to register */
			sys_write32(q_resp_addr_l, channel_ptr->port_csr_base + responder_ofst);

			/* offset for ADDR_H */
			responder_ofst += INTEL_SSGDMA_QCSR_Q_RESPONDER_ADDR_H_OFST;

			/* write to register */
			sys_write32(q_resp_addr_h, channel_ptr->port_csr_base + responder_ofst);
			ret = 0;
		} else {
			LOG_ERR("intel ssgdma cannot insert responder as port_csr_base is null.");
		}
	}

	return ret;
}

/***
 * @brief set start address of descriptor list. HW will fetch from this address.
 */
static int intel_ssgdma_queue_set_start_address(struct intel_ssgdma_channel_data *channel_ptr,
	ssgdma_addr_t q_addr)
{
	uint32_t q_addr_h, q_addr_l;
	int ret = -EINVAL;

	if (channel_ptr->port_csr_base) {
		q_addr_l = INTEL_SSGDMA_LOWER_32_BITS(q_addr);
		q_addr_h = INTEL_SSGDMA_UPPER_32_BITS(q_addr);

		sys_write32(q_addr_l, channel_ptr->port_csr_base +
			INTEL_SSGDMA_QCSR_Q_START_ADDR_L);
		sys_write32(q_addr_h, channel_ptr->port_csr_base +
			INTEL_SSGDMA_QCSR_Q_START_ADDR_H);

		ret = 0;
	}

	return ret;
}

/***
 * @brief initialize all descriptors of a channel.
 */
static int intel_ssgdma_channel_data_init_desc(struct intel_ssgdma_channel_data *channel_ptr)
{
	ssgdma_addr_t desc_block_start, resp_block_start;
	ssgdma_addr_t desc_addr_tmp, resp_addr_tmp;
	uint8_t block_idx;
	uint16_t desc_idx, data_desc_idx;
	union intel_ssgdma_hw_queue_desc *desc;
	int ret = 0;

	if (channel_ptr->desc_ptr) {
		/* set start address */
		desc_block_start = (uintptr_t)channel_ptr->desc_ptr;
		resp_block_start = (uintptr_t)channel_ptr->resp_ptr;

		ret = intel_ssgdma_queue_set_start_address(channel_ptr, desc_block_start);

		if (ret != 0) {
			/* return early if set start address fails. */
			LOG_ERR("init : queue start address failed! ret : %d, channel_ptr : %p,"
				" desc_block_start : %p", ret, (void *)channel_ptr,
				(void *)((uintptr_t)desc_block_start));

			return ret;
		}

		data_desc_idx = 0U;

		for (block_idx = 0U; block_idx < channel_ptr->q_size; block_idx++) {
			/* Get next block physical address */
			if ((block_idx + 1) < channel_ptr->q_size) {
				desc_addr_tmp = desc_block_start + ((block_idx + 1) *
						INTEL_DESC_BLOCK_BYTES);
			} else {
				/* loop back last block to the first. */
				desc_addr_tmp = desc_block_start;
			}

			/* Each block will start with link descriptor. */
			data_desc_idx++;
			desc =  &(channel_ptr->desc_ptr[(data_desc_idx-1)]);

			ret = intel_ssgdma_configure_link_desc(
						(union intel_ssgdma_hw_queue_desc *)desc,
						data_desc_idx, desc_addr_tmp,
						channel_ptr->port_type);

			if (ret != 0) {
				/* return early if configuration of link descriptor fails. */
				LOG_ERR("init : link descriptor configuration failed! ret "
					": %d, desc : %p, link idx : %d", ret, desc, data_desc_idx);
				return ret;
			}

			/* set index for data descriptor */
			for (desc_idx = 1; desc_idx < INTEL_SSGDMA_NUM_DESC_PER_BLOCK;
				desc_idx++) {
				/* First data descriptor index is 1, not 0*/
				data_desc_idx++;
				desc =  &(channel_ptr->desc_ptr[(data_desc_idx-1)]);

				if (desc) {
					/* all descriptors have same position for DescrIDX. */
					desc->mem_queue_data_desc.descr_idx = data_desc_idx;
				}
			}
		}

		/* add responder block if enabled. */
		for (block_idx = 0U; block_idx < channel_ptr->q_resp_size; block_idx++) {
			resp_addr_tmp = resp_block_start + (uint32_t)block_idx *
					INTEL_DESC_BLOCK_BYTES;

			/* setup responder link */
			ret = intel_ssgdma_queue_insert_responder_block(channel_ptr, resp_addr_tmp,
				block_idx);
			if (ret != 0) {
				/* return early if set responder block address fails. */
				LOG_ERR("init : queue insert responder block failed! ret : %d, "
				"channel_ptr : %p, block_idx : %d", ret, channel_ptr, block_idx);
				return ret;
			}
		}
	} else {
		/* This should not happen. */
		ret = -ENOMEM;
	}

	return ret;
}

/***
 * @brief helper function to configure a channel.
 */
static int intel_ssgdma_setup_channel(struct intel_ssgdma_dev_data *const ssgdma_dev_data,
	uint32_t channel, uint8_t q_size, uint8_t q_resp_size)
{
	int ret = 0;
	struct intel_ssgdma_channel_data *channel_ptr;
	uint32_t control_msk;

	if (ssgdma_dev_data->device_errors_flags & INTEL_SSGDMA_CONFIG_MISMATCH_MSK) {
		LOG_ERR("hw and sw config mismatch, please check dts and platform designer.");
		return -EIO;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (channel_ptr->port_csr_base == 0U) {
		LOG_ERR("unable to setup. port csr base is 0. Please check init.");
		return -EIO;
	}

	if (!(q_size > 0) || !(q_size <= INTEL_SSGDMA_MAX_DESC_BLOCKS) ||
		!(q_resp_size <= INTEL_SSGDMA_MAX_RESP_BLOCKS)) {
		LOG_ERR("unable to setup. Invalid.");
		return -EINVAL;
	}

	channel_ptr->q_size = q_size;
	channel_ptr->q_resp_size = q_resp_size;

	/* No multiplication overflow, if q_size <= INTEL_SSGDMA_MAX_DESC_BLOCKS */
	channel_ptr->max_desc_hw_idx = (uint16_t)((uint32_t)q_size *
		INTEL_SSGDMA_NUM_DESC_PER_BLOCK);
	channel_ptr->max_resp_buff_idx = (uint16_t)((uint32_t)q_resp_size *
		INTEL_SSGDMA_NUM_DESC_PER_BLOCK) - 1U;
	channel_ptr->max_desc_data_desc = (uint16_t)((uint32_t)q_size *
		INTEL_SSGDMA_DATA_DESC_PER_BLOCK);

	/* set the number of descriptor block to hw */
	sys_write32(q_size, channel_ptr->port_csr_base +
		INTEL_SSGDMA_QCSR_Q_SIZE);

	/* initialize insert pointer to 0. */
	channel_ptr->sw_insert_hw_idx = 0;

	/* set the insert pointer of hw to 0 */
	sys_write32(channel_ptr->sw_insert_hw_idx, channel_ptr->port_csr_base +
		INTEL_SSGDMA_QCSR_Q_INSERT_POINTER);

	/* stores the value of control register. */
	control_msk = sys_read32(channel_ptr->port_csr_base +
		INTEL_SSGDMA_QCSR_CTRL);

	/* setup responder */
	if (q_resp_size) {
		/* set the number of responder block to hw */
		sys_write32(q_resp_size, channel_ptr->port_csr_base +
			INTEL_SSGDMA_QCSR_Q_RESP_SIZE);
		channel_ptr->sw_extract_hw_idx = 0;
		if (!(control_msk & INTEL_SSGDMA_QCSR_CTRL_WB_EN_MSK)) {
			/* only write if not enabled. */
			control_msk |= INTEL_SSGDMA_QCSR_CTRL_WB_EN_MSK;
			sys_write32(control_msk, channel_ptr->port_csr_base +
				INTEL_SSGDMA_QCSR_CTRL);
		}
	} else {
		/* disable responder */
		if (control_msk & INTEL_SSGDMA_QCSR_CTRL_WB_EN_MSK) {
			/* only write if different. */
			control_msk &= ~INTEL_SSGDMA_QCSR_CTRL_WB_EN_MSK;
			sys_write32(control_msk, channel_ptr->port_csr_base +
				INTEL_SSGDMA_QCSR_CTRL);
		}
	}

	/* enable queue */
	intel_ssgdma_queue_ctrl_enable(channel_ptr);

	/* prepare memory for descriptor. */
	intel_ssgdma_channel_data_prepare_desc(ssgdma_dev_data, channel);

	/* init descriptors. */
	ret = intel_ssgdma_channel_data_init_desc(channel_ptr);

	/* this should be the very last to indicate setup success! */
	channel_ptr->channel_flags |= INTEL_SSGDMA_DEVICE_PORT_SETUP_MSK;


	return ret;
}

/***
 * @brief check address alignment and length limit.
 */
static int intel_ssgdma_check_address_and_length(ssgdma_addr_t src_addr, ssgdma_addr_t dest_addr,
	uint32_t length, bool restrict_unaligned_access)
{
	int ret = 0;

	if (length == 0) {
		LOG_ERR("length = 0 is invalid.");
		return -EINVAL;
	}

	if (restrict_unaligned_access) {
		/* length must be multiple of 4 */
		if (length % INTEL_SSGDMA_LENGTH_ALIGN) {
			LOG_ERR("length must be multiple of 4.");
			ret = -EINVAL;
		} else {
			/* check for address alignment. */
			if ((src_addr | dest_addr) & INTEL_SSGDMA_ADDRESS_MASK) {
				LOG_ERR("Address must be aligned to 32 bytes.");
				ret = -EINVAL;
			}
		}
	}
	return ret;
}

/***
 * @brief get control field of H2D_ST or D2H_ST descriptors.
 */
static uint8_t intel_ssgdma_get_st_control(int block_nr, uint32_t total_block,
	bool complete_callback_en)
{
	uint8_t control;

	control = INTEL_SSGDMA_VALID_DESC_MSK;

	if (total_block == 1) {
		control |= INTEL_SSGDMA_D2H_ST_START_ON_SOP_MSK | INTEL_SSGDMA_D2H_ST_EOP_MSK;
	} else {
		if (block_nr == 0) {
			control |= INTEL_SSGDMA_D2H_ST_START_ON_SOP_MSK;
		} else if (block_nr == total_block-1) {
			/* last block. */
			control |= INTEL_SSGDMA_D2H_ST_EOP_MSK;
		}
	}

	if (complete_callback_en) {
		/* enable callback for completion of each block. */
		control |= INTEL_SSGDMA_DESC_IRQ_EN_MSK;
	} else {
		/* only invoke at last block. */
		if (block_nr == total_block-1) {
			control |= INTEL_SSGDMA_DESC_IRQ_EN_MSK;
		}
	}

	return control;
}

/***
 * @brief get control field of H2D_MM descriptors.
 */
static uint8_t intel_ssgdma_get_mem_control(int i, uint32_t block_count,
	enum dma_channel_direction direction, bool complete_callback_en)
{
	int control = 0;

	switch (direction) {
	case MEMORY_TO_MEMORY:
	case HOST_TO_MEMORY:
		control = INTEL_SSGDMA_VALID_DESC_MSK | INTEL_SSGDMA_MEM_WRITE_MSK;
		break;
	case MEMORY_TO_HOST:
		control = INTEL_SSGDMA_VALID_DESC_MSK;
		break;
	default:
		/* should not happen, as this is already checked before calling this function! */
		break;
	}

	if (control) {
		if (complete_callback_en) {
			/* enable callback for completion of each block. */
			control |= INTEL_SSGDMA_DESC_IRQ_EN_MSK;
		} else {
			/* only invoke at last block. */
			if (i == (block_count - 1)) {
				control |= INTEL_SSGDMA_DESC_IRQ_EN_MSK;
			}
		}
	}

	return control;
}

/***
 * @brief configure D2H_ST descriptors.
 */
static int intel_ssgdma_configure_d2h_st(union intel_ssgdma_hw_queue_desc *desc, uint8_t control,
	uint32_t length, ssgdma_addr_t addr)
{
	if (desc) {
		desc->d2h_st_queue_data_desc.host_dest_address_lower =
			INTEL_SSGDMA_LOWER_32_BITS(addr);
		desc->d2h_st_queue_data_desc.host_dest_address_upper =
			INTEL_SSGDMA_UPPER_32_BITS(addr);
		desc->d2h_st_queue_data_desc.length = length;
		desc->d2h_st_queue_data_desc.format_field = INTEL_SSGDMA_D2H_ST_FORMAT;
		desc->d2h_st_queue_data_desc.control = control;
		desc->d2h_st_queue_data_desc.host_interface_control =
			CONFIG_DMA_INTEL_SSGDMA_HOST_INTERFACE_CONTROL;
	} else {
		LOG_ERR("d2h_st descriptor is NULL. Please check if allocation is successful.");
		return -EINVAL;
	}
	return 0;
}

/***
 * @brief configure H2D_ST descriptors.
 */
static int intel_ssgdma_configure_h2d_st(union intel_ssgdma_hw_queue_desc *desc, uint8_t control,
	uint32_t length, ssgdma_addr_t addr)
{
	if (desc) {
		desc->h2d_st_queue_data_desc.host_source_address_lower =
			INTEL_SSGDMA_LOWER_32_BITS(addr);
		desc->h2d_st_queue_data_desc.host_source_address_upper =
			INTEL_SSGDMA_UPPER_32_BITS(addr);
		desc->h2d_st_queue_data_desc.length = length;
		desc->h2d_st_queue_data_desc.format_field = INTEL_SSGDMA_H2D_ST_FORMAT;
		desc->h2d_st_queue_data_desc.control = control;
		desc->h2d_st_queue_data_desc.sideband_signal =
			CONFIG_DMA_INTEL_SSGDMA_SIDEBAND_SIGNAL;
		desc->h2d_st_queue_data_desc.host_interface_control =
			CONFIG_DMA_INTEL_SSGDMA_HOST_INTERFACE_CONTROL;
	} else {
		LOG_ERR("h2d_st descriptor is NULL. Please check if allocation is successful.");
		return -EINVAL;
	}
	return 0;
}

/***
 * @brief configure H2D_MM descriptors.
 */
static int intel_ssgdma_configure_mem(union intel_ssgdma_hw_queue_desc *desc, uint8_t control,
	uint32_t length, ssgdma_addr_t host_addr, ssgdma_addr_t dev_addr)
{
	if (control == 0U) {
		LOG_ERR("mem control must not be 0. Please check dma_channel_direction");
		return -EINVAL;
	}

	if (desc) {
		if (control & INTEL_SSGDMA_MEM_WRITE_MSK) {
			desc->mem_queue_data_desc.host_address_lower =
				INTEL_SSGDMA_LOWER_32_BITS(host_addr);
			desc->mem_queue_data_desc.host_address_upper =
				INTEL_SSGDMA_UPPER_32_BITS(host_addr);

			desc->mem_queue_data_desc.device_address_lower =
				INTEL_SSGDMA_LOWER_32_BITS(dev_addr);
			desc->mem_queue_data_desc.device_address_upper =
				INTEL_SSGDMA_UPPER_32_BITS(dev_addr);
		} else {
			/* for MEMORY_TO_HOST, the IP will read from
			 * device_address_lower(dest_address).
			 * and write to host_address_lower (source_address),
			 * To avoid confusion, driver will invert this internally.
			 * So user will always expect source_address -> dest_address
			 */
			desc->mem_queue_data_desc.host_address_lower =
				INTEL_SSGDMA_LOWER_32_BITS(dev_addr);
			desc->mem_queue_data_desc.host_address_upper =
				INTEL_SSGDMA_UPPER_32_BITS(dev_addr);

			desc->mem_queue_data_desc.device_address_lower =
				INTEL_SSGDMA_LOWER_32_BITS(host_addr);
			desc->mem_queue_data_desc.device_address_upper =
				INTEL_SSGDMA_UPPER_32_BITS(host_addr);
		}

		desc->mem_queue_data_desc.length = length;
		desc->mem_queue_data_desc.format_field =
			INTEL_SSGDMA_H2D_MEM_FORMAT;
		desc->mem_queue_data_desc.control = control;
		desc->mem_queue_data_desc.host_interface_control =
			CONFIG_DMA_INTEL_SSGDMA_HOST_INTERFACE_CONTROL;
		desc->mem_queue_data_desc.device_interface_control =
			CONFIG_DMA_INTEL_SSGDMA_DEVICE_INTERFACE_CONTROL;
	} else {
		LOG_ERR("mem descriptor is NULL. Please check if allocation is successful.");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Helper function to get next descriptor index.
 */
static uint16_t intel_ssgdma_get_next_desc_hw_idx(struct intel_ssgdma_channel_data *channel_ptr)
{
	uint16_t next_desc_hw_idx = 0;
	uint32_t tmp_idx;

	if (channel_ptr->sw_insert_hw_idx == 0U) {
		/* Buffer is empty as this is the first call after init. */
		next_desc_hw_idx = INTEL_SSGDMA_FIRST_DATA_DESC_IDX;
	} else {
		/* get index of next descriptor, overflows are handled here. */
		tmp_idx = (uint32_t)channel_ptr->sw_insert_hw_idx + 1U;

		/* After the last desc, start again from first data desc. */
		if (tmp_idx > channel_ptr->max_desc_hw_idx) {
			next_desc_hw_idx = INTEL_SSGDMA_FIRST_DATA_DESC_IDX;
		} else {
			next_desc_hw_idx = (uint16_t)tmp_idx;
		}

		/* plus one again, if next_free_desc_hw_idx is a link desc. */
		if (intel_ssgdma_is_link_desc_idx(next_desc_hw_idx)) {
			next_desc_hw_idx++;
		}
	}

	return next_desc_hw_idx;
}

/**
 * @brief Helper function to get next descriptor.
 */
static union intel_ssgdma_hw_queue_desc *intel_ssgdma_get_next_desc(
	struct intel_ssgdma_channel_data *channel_ptr)
{
	uint16_t next_desc_hw_idx;
	union intel_ssgdma_hw_queue_desc *next_desc = NULL;

	next_desc_hw_idx = intel_ssgdma_get_next_desc_hw_idx(channel_ptr);
	next_desc = &(channel_ptr->desc_ptr[next_desc_hw_idx -
		INTEL_SSGDMA_HW_DESC_OFST]);
	channel_ptr->sw_insert_hw_idx = next_desc_hw_idx;

	return next_desc;
}

/**
 * @brief Descriptor are group in a set of INTEL_SSGDMA_NUM_DESC_PER_BLOCK_OFST.
 * This function gets group number of a given descriptor index.
 */
uint8_t intel_ssgdma_get_desc_block_num(uint16_t desc_idx)
{
	uint8_t block_num = 0;

	if (desc_idx) {
		/* minus offset as descriptor index starts with 1. */
		block_num = (desc_idx - INTEL_SSGDMA_HW_DESC_OFST) >>
					INTEL_SSGDMA_NUM_DESC_PER_BLOCK_OFST;
	}

	return block_num;
}

/**
 * @brief Get number of free space in descriptor ring buffer.
 */
static uint16_t intel_ssgdma_get_free_space_desc(struct intel_ssgdma_channel_data *channel_ptr)
{
	uint16_t num_desc_available = 0;
	uint8_t insert_block_num, extract_block_num, tmp;
	uint16_t hw_extract_pointer;

	if (channel_ptr->port_csr_base) {
		/* if channel is not setup, free space is 0. */
		if (!(channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_SETUP_MSK)) {
			return num_desc_available;
		}

		/* Read from register directly to avoid redudant checkings. */
		hw_extract_pointer = sys_read32(channel_ptr->port_csr_base +
			INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER) &
			INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER_Q_EXTR_PTR_MSK;

		if (hw_extract_pointer == channel_ptr->sw_insert_hw_idx) {
			/* descriptor queue is empty.*/
			num_desc_available = channel_ptr->max_desc_data_desc;
		} else {
			extract_block_num = intel_ssgdma_get_desc_block_num(hw_extract_pointer);
			insert_block_num = intel_ssgdma_get_desc_block_num(
				channel_ptr->sw_insert_hw_idx);
			/* calculate available space */
			if (hw_extract_pointer > channel_ptr->sw_insert_hw_idx) {
				num_desc_available = hw_extract_pointer -
					channel_ptr->sw_insert_hw_idx;

				/* -1 again, if sw_insert_hw_idx is still at init value. */
				if (channel_ptr->sw_insert_hw_idx == 0U) {
					num_desc_available -= 1U;
				}

				/* tmp to store number of link descriptors. */
				tmp = extract_block_num - insert_block_num;
			} else {
				num_desc_available = channel_ptr->max_desc_hw_idx -
					channel_ptr->sw_insert_hw_idx + hw_extract_pointer;
				if (hw_extract_pointer) {
					/* tmp to store number of link descriptors. */
					tmp = channel_ptr->q_size -
						insert_block_num + extract_block_num;
				} else {
					/* No wrap around for first run. */
					tmp = channel_ptr->q_size - insert_block_num -
						INTEL_SSGDMA_Q_BLOCK_OFST;
				}
			}
			/* remove link descriptor from available slots */
			num_desc_available -= tmp;
		}
		/* last slot of circular buffer is used to indicate full. */
		num_desc_available -= INTEL_SSGDMA_DESC_BUFF_FULL_OFST;
	}

	return num_desc_available;
}

/**
 * @brief check dma_block_config
 */
static int intel_ssgdma_check_dma_block_config(struct dma_block_config *desc_cfg,
	bool restrict_unaligned_access)
{
	int ret;

	ret = intel_ssgdma_check_address_and_length(desc_cfg->source_address,
		desc_cfg->dest_address, desc_cfg->block_size, restrict_unaligned_access);
	if (ret) {
		LOG_ERR("dma_block_config invalid address or length");
		return ret;
	}

	/* all of these are not supported. */
	if (desc_cfg->source_gather_interval || desc_cfg->dest_scatter_interval ||
		desc_cfg->dest_scatter_count || desc_cfg->source_gather_count ||
		desc_cfg->source_reload_en || desc_cfg->dest_reload_en ||
		desc_cfg->source_addr_adj || desc_cfg->dest_addr_adj ||
		desc_cfg->fifo_mode_control || desc_cfg->flow_control_mode) {
		LOG_ERR("dma_block_config feature not supported. This IP only supports "
			"incremental address. Reload can be done manually in isr callback.");
		ret = -ENOTSUP;
	}

	return ret;
}

/**
 * @brief Helper function to fill descriptors.
 */
static int intel_ssgdma_fill_descriptors(struct intel_ssgdma_channel_data *channel_ptr,
	struct dma_config *dma_cfg, uint32_t channel, bool restrict_unaligned_access)
{
	int ret = 0;
	int i;
	uint8_t control;
	union intel_ssgdma_hw_queue_desc *desc;
	uint16_t num_desc_available;
	struct dma_block_config *desc_cfg = dma_cfg->head_block;

	num_desc_available = intel_ssgdma_get_free_space_desc(channel_ptr);
	if (dma_cfg->block_count > num_desc_available) {
		LOG_ERR("Unable to configure channel %d. Queue is full. Request %d, Free %d",
			channel, dma_cfg->block_count, num_desc_available);
		return INTEL_SSGDMA_ERROR;
	}

	for (i = 0; i < dma_cfg->block_count; i++) {
		ret = intel_ssgdma_check_dma_block_config(desc_cfg, restrict_unaligned_access);
		if (ret != 0) {
			LOG_ERR("invalid dma_block_config for channel %d, block %d", channel, i);
			return ret;
		}

		/* update the sw internal tracker. */
		desc = intel_ssgdma_get_next_desc(channel_ptr);

		switch (channel_ptr->port_type) {
		case INTEL_SSGDMA_D2H_ST_PORT:
			control = intel_ssgdma_get_st_control(i, dma_cfg->block_count,
				dma_cfg->complete_callback_en);
			ret = intel_ssgdma_configure_d2h_st(desc, control,
				desc_cfg->block_size, desc_cfg->dest_address);
			break;
		case INTEL_SSGDMA_H2D_ST_PORT:
			control = intel_ssgdma_get_st_control(i, dma_cfg->block_count,
				dma_cfg->complete_callback_en);
			ret = intel_ssgdma_configure_h2d_st(desc, control, desc_cfg->block_size,
				desc_cfg->source_address);
			break;
		case INTEL_SSGDMA_H2D_MM_PORT:
			control = intel_ssgdma_get_mem_control(i, dma_cfg->block_count,
				dma_cfg->channel_direction, dma_cfg->complete_callback_en);
			ret = intel_ssgdma_configure_mem(desc, control, desc_cfg->block_size,
				desc_cfg->source_address, desc_cfg->dest_address);
			break;
		default:
			LOG_ERR("Unable to fill descriptor. Port type is not supported.");
			ret = INTEL_SSGDMA_ERROR;
			break;
		}

		if (ret != 0) {
			break; /* return early upon error. */
		}
		/* User should ensure that the head_block array size matches with the block_count */
		desc_cfg++;
	}
	return ret;
}

/**
 * @brief check if a channel is paused.
 *
 * @details this function does not check if channel_ptr and channel_ptr->port_csr_base
 * are valid. These should be checked in the caller function.
 */
bool intel_ssgdma_channel_is_paused(struct intel_ssgdma_channel_data *channel_ptr)
{
	bool is_paused;
	uint32_t status;

	status = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_STATUS);
	/* 1 = paused, 0 = not paused. */
	is_paused = ((status & INTEL_SSGDMA_QCSR_STATUS_Q_AGENT_CONTROL_PAUSED_MSK) ==
			INTEL_SSGDMA_QCSR_STATUS_Q_AGENT_CONTROL_PAUSED_MSK);
	return is_paused;
}

/**
 * @brief check if a channel reset is done.
 *
 * @details this function does not check if channel_ptr and channel_ptr->port_csr_base
 * are valid. These should be checked in the caller function.
 */
bool intel_ssgdma_channel_reset_done(struct intel_ssgdma_channel_data *channel_ptr)
{
	bool reset_done = false;
	uint32_t control, status;

	control = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
	status = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_STATUS);
	if (!(control & INTEL_SSGDMA_QCSR_CTRL_Q_SW_RESET_REQ_MSK) &&
		!(status & INTEL_SSGDMA_QCSR_STATUS_Q_RESETTING_MSK)) {
		reset_done = true;
	}

	return reset_done;
}

/**
 * @brief Helper function to reset a channel.
 */
static int intel_ssgdma_reset_channel(struct intel_ssgdma_dev_data *const ssgdma_dev_data,
	 uint32_t channel)
{
	uint32_t mask;
	struct intel_ssgdma_channel_data *channel_ptr;
	bool reset_done;
	k_spinlock_key_t key;

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (channel_ptr->port_csr_base == 0U) {
		LOG_ERR("unable to reset. port csr base is 0. Please check init.");
		return -EIO;
	}

	/* reset channel */
	mask = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
	/* guard to prevent enable and reset set at the same time. */
	mask |= INTEL_SSGDMA_QCSR_CTRL_Q_SW_RESET_REQ_MSK;
	mask &= ~INTEL_SSGDMA_QCSR_CTRL_Q_EN_MSK;
	key = k_spin_lock(&ssgdma_dev_data->lock);
	sys_write32(mask, channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
	k_spin_unlock(&ssgdma_dev_data->lock, key);

	/* block until hw has performed the reset. */
	reset_done = WAIT_FOR(intel_ssgdma_channel_reset_done(channel_ptr),
		CONFIG_DMA_INTEL_SSGDMA_RESET_TIMEOUT,
		k_usleep(CONFIG_DMA_INTEL_SSGDMA_TIMEOUT_INTERVAL));
	if (!reset_done) {
		LOG_ERR("Unable to reset dma channel %d. Timeout!", channel);
		return -EIO;
	}

	/* prepare flags for next dma_config. */
	channel_ptr->channel_flags |= INTEL_SSGDMA_DEVICE_PORT_REUSE_SETUP_MSK;
	channel_ptr->channel_flags &= ~INTEL_SSGDMA_DEVICE_PORT_SETUP_MSK;

	return 0;
}

/**
 * @brief Intel SSGDMA cannot be configured during runtime.
 * All channel capabilities are fixed according to the design in platform desginer.
 */
static int intel_ssgdma_sync_hw_sw_config(const struct device *dev)
{
	int ret = 0;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	uint32_t mask;
	uint16_t num_channels_per_type_tmp[INTEL_SSGDMA_TOTAL_PORT_TYPES];
	bool restrict_unaligned_access_tmp;
	int memcmp_result;

	/* read configuration from HW */
	mask = sys_read32(reg_base + INTEL_SSGDMA_GCSR_IP_PARAM1);
	num_channels_per_type_tmp[INTEL_SSGDMA_D2H_ST_PORT] = (mask &
					INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_D2H_ST_PORTS_MSK) >>
					INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_D2H_ST_PORTS_OFST;
	num_channels_per_type_tmp[INTEL_SSGDMA_H2D_ST_PORT] = (mask &
					INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_ST_PORTS_MSK)  >>
					INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_ST_PORTS_OFST;
	num_channels_per_type_tmp[INTEL_SSGDMA_H2D_MM_PORT] = (mask &
					INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_MM_PORTS_MSK) >>
					INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_MM_PORTS_OFST;
	restrict_unaligned_access_tmp = !((mask &
					INTEL_SSGDMA_GCSR_IP_PARAM1_UNALIGNED_ACCESS_EN_MSK) >>
					INTEL_SSGDMA_GCSR_IP_PARAM1_UNALIGNED_ACCESS_EN_OFST);

	memcmp_result = memcmp(num_channels_per_type_tmp, ssgdma_dev_data->num_channels_per_type,
			sizeof(num_channels_per_type_tmp));
	if ((memcmp_result != 0U) ||
		(restrict_unaligned_access_tmp != ssgdma_dev_data->restrict_unaligned_access)) {
		LOG_ERR("sw and hw mismatch! Please check dts and platform designer design.");
		ret = INTEL_SSGDMA_ERROR;
		ssgdma_dev_data->device_errors_flags |= INTEL_SSGDMA_CONFIG_MISMATCH_MSK;
	}

	return ret;
}

/**
 * @brief Sets port type, port number and csr base address for all of device port
 */
static void intel_ssgdma_init_all_channel_base_adr(const struct device *dev)
{
	uint16_t port_type_i, port_num_i;
	struct intel_ssgdma_channel_data *channel_ptr = NULL;
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	uint8_t channel = 0;
	uint32_t port_ofst = 0;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);

	for (port_type_i = 0; port_type_i < INTEL_SSGDMA_TOTAL_PORT_TYPES;
		port_type_i++) {
		for (port_num_i = 0;
			port_num_i < ssgdma_dev_data->num_channels_per_type[port_type_i];
			port_num_i++) {
			channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
			channel_ptr->port_type = (enum intel_ssgdma_port_type_e)
				port_type_i;
			channel_ptr->port_num = port_num_i;
			/* type cast to avoid overflow */
			port_ofst = (uint32_t) INTEL_SSGDMA_DEVICE_PORT_CSR_BASE_OFST + ((uint32_t)
			channel_ptr->port_type << INTEL_SSGDMA_DEVICE_PORT_TYPE_SHIFT) +
			((uint32_t) INTEL_SSGDMA_DEVICE_PORT_NUM_OFST * channel_ptr->port_num);
			channel_ptr->port_csr_base = reg_base + port_ofst;
			channel++;
			/* prefetcher error irq is enabled by default. */
			channel_ptr->error_callback_en = 1;
		}
	}
}

/**
 * @brief Set register to enable prefetcher.
 */
static void intel_ssgdma_enable_prefetch_engine(const struct device *dev)
{
	uint32_t mask;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, dma_mmio);
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	k_spinlock_key_t key;

	mask = sys_read32(reg_base + INTEL_SSGDMA_GCSR_CTRL);
	/* guard to prevent run and reset set at the same time. */
	mask |= INTEL_SSGDMA_GCSR_CTRL_RUN_PREFETCH_ENGINE_MSK;
	mask &= ~INTEL_SSGDMA_GCSR_CTRL_RESET_PREFETCH_ENGINE_MSK;

	key = k_spin_lock(&ssgdma_dev_data->lock);
	sys_write32(mask, reg_base + INTEL_SSGDMA_GCSR_CTRL);
	k_spin_unlock(&ssgdma_dev_data->lock, key);
}

/**
 * @brief set csr to pause a channel.
 *
 * @details this function does not check if channel_ptr and channel_ptr->port_csr_base
 * are valid. These should be checked in the caller function.
 */
void intel_ssgdma_pause_channel(struct intel_ssgdma_dev_data *const ssgdma_dev_data,
	struct intel_ssgdma_channel_data *channel_ptr)
{
	uint32_t mask;

	mask = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
	mask |= INTEL_SSGDMA_QCSR_CTRL_Q_PAUSE_AGENT_CONTROL_MSK;
	k_spinlock_key_t key = k_spin_lock(&ssgdma_dev_data->lock);

	sys_write32(mask, channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
	k_spin_unlock(&ssgdma_dev_data->lock, key);
}
/*************************************************************************************************/

/*************************************************************************************************
 * Driver APIs that are exposed to users.
 *************************************************************************************************/

/**
 * @brief initialize the driver.
 */
static int intel_ssgdma_init(const struct device *dev)
{
	int ret;
	const struct intel_ssgdma_dev_cfg *config = DEV_CFG(dev);

	DEVICE_MMIO_NAMED_MAP(dev, dma_mmio, K_MEM_CACHE_NONE);

	/* configures driver data */
	ret = intel_ssgdma_sync_hw_sw_config(dev);
	__ASSERT(ret == 0, "init error. sw and hw parameter mismatch.");

	intel_ssgdma_init_all_channel_base_adr(dev);

	/* configure and enable interrupt lines */
	config->irq_config();

	intel_ssgdma_enable_prefetch_engine(dev);
	return 0;
}

/**
 * @brief Configures a given channel.
 *
 * @details Configures a given channel. After configuration, if dma_start is not called,
 * hw insert pointer will not be updated. The dma transfer will only start after hw insert pointer
 * is updated. After dma_start is called, calling this function will update the hw insert pointer.
 * dma_config->dma_slot: not used.
 * dma_config->channel_priority: no priority. Round robin is used.
 * dma_config->source_chaining_en, dma_cfg->dest_chaining_en: driver will ignore these input,
 * as this is defined by the descriptor block.
 * dma_block_config will be checked in intel_ssgdma_fill_descriptors.
 */
static int intel_ssgdma_config(const struct device *dev, uint32_t channel,
	struct dma_config *dma_cfg)
{
	int ret = 0;
	struct intel_ssgdma_channel_data *channel_ptr;
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	uint32_t control_msk;
	k_spinlock_key_t key;

	if (channel >= ssgdma_dev_data->max_hw_channel) {
		LOG_ERR("Unable to configure. Invalid dma channel %d", channel);
		return -EINVAL;
	}

	if (dma_cfg == NULL) {
		LOG_ERR("invalid dma config for channel %d", channel);
		return -EINVAL;
	}

	if (dma_cfg->head_block == NULL) {
		LOG_ERR("dma head block is null for channel %d", channel);
		return -EINVAL;
	}

	if (dma_cfg->linked_channel || dma_cfg->cyclic) {
		LOG_ERR("Intel SSGDMA does not support linked channel or cyclic transfer.");
		return -ENOTSUP;
	}

	if (dma_cfg->block_count == 0) {
		LOG_ERR("invalid block_count for channel %d", channel);
		return -EINVAL;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);

	/* block config, if user have not called dma_resume after dma_suspend */
	if (intel_ssgdma_channel_is_paused(channel_ptr)) {
		LOG_ERR("Unable to configure. channel %d is not resumed. Please call dma_resume.",
			channel);
		return -EIO;
	}

	/* check direction. */
	ret = intel_ssgdma_check_direction(channel_ptr->port_type, dma_cfg->channel_direction);
	if (ret != 0) {
		LOG_ERR("invalid direction: %d for dma channel %d", channel,
			dma_cfg->channel_direction);
		return ret;
	}

	/* set user callback */
	ssgdma_dev_data->complete_callback = dma_cfg->dma_callback;
	ssgdma_dev_data->user_data = dma_cfg->user_data;



	/* enable or disable prefetch_irq */
	if (channel_ptr->error_callback_en != dma_cfg->error_callback_en) {
		control_msk = sys_read32(channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
		channel_ptr->error_callback_en = dma_cfg->error_callback_en;
		if (channel_ptr->error_callback_en) {
			control_msk |= INTEL_SSGDMA_QCSR_CTRL_PREFETCH_IRQ_EN_MSK;
		} else {
			control_msk &= ~INTEL_SSGDMA_QCSR_CTRL_PREFETCH_IRQ_EN_MSK;
		}
		key = k_spin_lock(&ssgdma_dev_data->lock);
		sys_write32(control_msk, channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_CTRL);
		k_spin_unlock(&ssgdma_dev_data->lock, key);
	}

	/* setup channel */
	if (!(channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_SETUP_MSK)) {
		key = k_spin_lock(&ssgdma_dev_data->lock);
		ret = intel_ssgdma_setup_channel(ssgdma_dev_data, channel,
			CONFIG_DMA_INTEL_SSGDMA_Q_SIZE, CONFIG_DMA_INTEL_SSGDMA_Q_RESP_SIZE);
		k_spin_unlock(&ssgdma_dev_data->lock, key);
		if (ret != 0) {
			LOG_ERR("intel ssgdma channel %d setup fail ", channel);
			return ret;
		}
	}

	/* fill descriptors */
	ret = intel_ssgdma_fill_descriptors(channel_ptr, dma_cfg, channel,
		ssgdma_dev_data->restrict_unaligned_access);
	if (ret == 0) {
		/* update the channel direction to report in status. */
		channel_ptr->channel_direction = dma_cfg->channel_direction;

		if (channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_DMA_START_MSK) {
			/* update insert pointer, so that hw start to fetch the descriptors. */
			key = k_spin_lock(&ssgdma_dev_data->lock);
			sys_write32(channel_ptr->sw_insert_hw_idx, channel_ptr->port_csr_base +
				INTEL_SSGDMA_QCSR_Q_INSERT_POINTER);
			k_spin_unlock(&ssgdma_dev_data->lock, key);
		}
	} else {
		LOG_ERR("intel ssgdma channel %d setup, fill descriptors failed %d.", channel, ret);
	}
	return ret;
}

/**
 * @brief start a dma channel
 */
static int intel_ssgdma_start(const struct device *dev, uint32_t channel)
{
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	int ret = 0;
	struct intel_ssgdma_channel_data *channel_ptr;

	/* channel should be valid */
	if (channel >= ssgdma_dev_data->max_hw_channel) {
		LOG_ERR("Unable to start. Invalid dma channel %d", channel);
		return -EINVAL;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (!(channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_SETUP_MSK)) {
		LOG_ERR("Unable to start. channel %d not is configured. Please call dma_config.",
			channel);
		return -EIO;
	}

	/* block start, if user have not called dma_resume after dma_suspend */
	if (intel_ssgdma_channel_is_paused(channel_ptr)) {
		LOG_ERR("Unable to start. channel %d is not resumed. Please call dma_resume.",
			channel);
		return -EIO;
	}

	if (!(channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_DMA_START_MSK)) {
		/* set flag so that insert pointer is updated at next config or reload. */
		channel_ptr->channel_flags |= INTEL_SSGDMA_DEVICE_PORT_DMA_START_MSK;
		/* clear reset flag */
		channel_ptr->channel_flags &= ~INTEL_SSGDMA_DEVICE_PORT_RESET_MSK;

		/* update insert pointer */
		k_spinlock_key_t key = k_spin_lock(&ssgdma_dev_data->lock);

		/* update insert pointer, so that hw start to fetch the descriptors. */
		sys_write32(channel_ptr->sw_insert_hw_idx, channel_ptr->port_csr_base +
			INTEL_SSGDMA_QCSR_Q_INSERT_POINTER);
		k_spin_unlock(&ssgdma_dev_data->lock, key);
	}

	return ret;
}

/**
 * @brief suspend a dma channel.
 */
static int intel_ssgdma_suspend(const struct device *dev, uint32_t channel)
{
	int ret = 0;
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	struct intel_ssgdma_channel_data *channel_ptr = NULL;
	bool is_paused;

	/* channel should be valid */
	if (channel >= ssgdma_dev_data->max_hw_channel) {
		LOG_ERR("Unable to suspend. Invalid dma channel %d", channel);
		return -EINVAL;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (channel_ptr->port_csr_base == 0U) {
		LOG_ERR("unable to suspend. port csr base is 0. Please check init.");
		return -EIO;
	}

	/* clear start flag. */
	channel_ptr->channel_flags &= ~INTEL_SSGDMA_DEVICE_PORT_DMA_START_MSK;

	/* check if channel is suspended. */
	is_paused = intel_ssgdma_channel_is_paused(channel_ptr);
	if (!is_paused) {
		intel_ssgdma_pause_channel(ssgdma_dev_data, channel_ptr);
		is_paused = WAIT_FOR(intel_ssgdma_channel_is_paused(channel_ptr),
			CONFIG_DMA_INTEL_SSGDMA_PAUSE_TIMEOUT,
			k_usleep(CONFIG_DMA_INTEL_SSGDMA_TIMEOUT_INTERVAL));
		if (!is_paused) {
			LOG_ERR("Unable to suspend dma channel %d. Timeout!", channel);
			ret = -EIO;
		}
	}

	return ret;
}

/**
 * @brief Stop a dma channel.
 *
 * @details The driver will suspend and then perform a reset to restore the dma channel to its
 * default settings. The remaining descriptors will be dropped and the IP will start again from
 * the first descriptor block. After this, user needs to call dma_config followed by dma_start
 * to restart the dma transfer.
 */
static int intel_ssgdma_stop(const struct device *dev, uint32_t channel)
{
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	int ret = 0;
	struct intel_ssgdma_channel_data *channel_ptr = NULL;

	/* channel should be valid */
	if (channel >= ssgdma_dev_data->max_hw_channel) {
		LOG_ERR("Unable to stop. Invalid dma channel %d", channel);
		return -EINVAL;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (!(channel_ptr->channel_flags & INTEL_SSGDMA_DEVICE_PORT_RESET_MSK)) {
		ret = intel_ssgdma_suspend(dev, channel);
		if (ret != 0) {
			LOG_ERR("Unable to stop channel %d. Suspend failed", channel);
			return ret;
		}
		/* reset channel to default state */
		ret = intel_ssgdma_reset_channel(ssgdma_dev_data, channel);
		if (ret != 0) {
			LOG_ERR("Unable to stop channel %d. Reset failed", channel);
			return ret;
		}

		channel_ptr->channel_flags |= INTEL_SSGDMA_DEVICE_PORT_RESET_MSK;
	}

	return ret;
}

/**
 * @brief resume a dma channel after suspend.
 */
static int intel_ssgdma_resume(const struct device *dev, uint32_t channel)
{
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	bool is_paused;
	struct intel_ssgdma_channel_data *channel_ptr;

	/* channel should be valid */
	if (channel >= ssgdma_dev_data->max_hw_channel) {
		LOG_ERR("Unable to resume. Invalid dma channel %d", channel);
		return -EINVAL;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (channel_ptr->port_csr_base == 0U) {
		LOG_ERR("Unable to resume dma channel %d. Please check init.", channel);
		return -EIO;
	}

	/* check port paused and set q_en.*/
	is_paused = intel_ssgdma_channel_is_paused(channel_ptr);
	if (is_paused) {
		k_spinlock_key_t key = k_spin_lock(&ssgdma_dev_data->lock);

		/* clear paused enable bit. All status csrs are write 1 to clear. */
		sys_write32(INTEL_SSGDMA_QCSR_STATUS_Q_AGENT_CONTROL_PAUSED_MSK,
			channel_ptr->port_csr_base + INTEL_SSGDMA_QCSR_STATUS);
		/* enable queue. */
		intel_ssgdma_queue_ctrl_enable(channel_ptr);
		k_spin_unlock(&ssgdma_dev_data->lock, key);
	}

	return 0;
}

/**
 * @brief Get dma status.
 *
 * @details pending_length and total_copied are not implemented, as the transfer will be based
 * on descriptors. DMA buffer here refers to the descriptors blocks.
 */
static int intel_ssgdma_get_status(const struct device *dev, uint32_t channel,
	struct dma_status *status)
{
	struct intel_ssgdma_channel_data *channel_ptr;
	struct intel_ssgdma_dev_data *const ssgdma_dev_data = DEV_DATA(dev);
	uint16_t hw_insert_ptr = 0;
	uint16_t hw_extract_ptr = 0;

	if ((channel >= ssgdma_dev_data->max_hw_channel) || (status == NULL)) {
		LOG_ERR("Unable to get status. Invalid dma channel %d, or status %p",
			channel, status);
		return -EINVAL;
	}

	channel_ptr = &(ssgdma_dev_data->channel_data[channel]);
	if (channel_ptr->port_csr_base == 0U) {
		LOG_ERR("unable to get status. Please check init for channel %d.", channel);
		return -EIO;
	}

	status->dir = channel_ptr->channel_direction;
	intel_ssgdma_get_ie_ptr(channel_ptr->port_csr_base, &hw_insert_ptr, &hw_extract_ptr);
	status->busy = (hw_insert_ptr != hw_extract_ptr);
	status->pending_length = 0;
	status->total_copied = 0;
	status->write_position = hw_insert_ptr;
	status->read_position = hw_extract_ptr;
	status->free = intel_ssgdma_get_free_space_desc(channel_ptr);

	return 0;
}

/*************************************************************************************************/

static const struct dma_driver_api intel_ssgdma_driver_api = {
	.config = intel_ssgdma_config,
	.start = intel_ssgdma_start,
	.stop = intel_ssgdma_stop,
	.suspend = intel_ssgdma_suspend,
	.resume = intel_ssgdma_resume,
	.get_status = intel_ssgdma_get_status,
};

#define CONFIGURE_DMA_IRQ(idx, inst)							\
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, idx), (					\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq),				\
			COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, priority),			\
			(DT_INST_IRQ(n, priority)), (0)),				\
			intel_ssgdma_isr,						\
			DEVICE_DT_INST_GET(inst), 0);					\
			irq_enable(DT_INST_IRQ_BY_IDX(inst, idx, irq));			\
	))

#define DMA_INTEL_SSGDMA_TOTAL_CHANNELS(inst)						\
	(DT_INST_PROP(inst, d2h_st_port_numbers) +					\
	 DT_INST_PROP(inst, h2d_st_port_numbers) +					\
	 DT_INST_PROP(inst, h2d_mm_port_numbers))

#define DMA_INTEL_SSGDMA_DESC_BYTES(inst)						\
	(DMA_INTEL_SSGDMA_TOTAL_CHANNELS(inst) *					\
	 (CONFIG_DMA_INTEL_SSGDMA_Q_SIZE + CONFIG_DMA_INTEL_SSGDMA_Q_RESP_SIZE) *	\
	 INTEL_DESC_BLOCK_BYTES)

#define DMA_INTEL_SSGDMA_DEVICE_INIT(inst)						\
	static struct intel_ssgdma_channel_data channel_data_##inst			\
		[DMA_INTEL_SSGDMA_TOTAL_CHANNELS(inst)] = {0};				\
	static COND_CODE_1(DT_INST_PROP(inst, unaligned_access), (), (__aligned(32)))	\
		uint8_t desc_buff_##inst[DMA_INTEL_SSGDMA_DESC_BYTES(inst)] = {0};	\
	static struct intel_ssgdma_dev_data intel_ssgdma_data_##inst = {		\
		.num_channels_per_type[INTEL_SSGDMA_D2H_ST_PORT] =			\
			DT_INST_PROP(inst, d2h_st_port_numbers),			\
		.num_channels_per_type[INTEL_SSGDMA_H2D_ST_PORT] =			\
			DT_INST_PROP(inst, h2d_st_port_numbers),			\
		.num_channels_per_type[INTEL_SSGDMA_H2D_MM_PORT] =			\
			DT_INST_PROP(inst, h2d_mm_port_numbers),			\
		.restrict_unaligned_access =						\
			!DT_INST_PROP(inst, unaligned_access),				\
		.max_hw_channel = DMA_INTEL_SSGDMA_TOTAL_CHANNELS(inst),		\
		.channel_data = channel_data_##inst,					\
		.desc_buff = desc_buff_##inst,						\
	};										\
	static void ssgdma_dma_irq_config_##inst(void);					\
	static const struct intel_ssgdma_dev_cfg intel_ssgdma_config_##inst = {		\
		DEVICE_MMIO_NAMED_ROM_INIT(dma_mmio, DT_DRV_INST(inst)),		\
		.irq_config = ssgdma_dma_irq_config_##inst,				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst,							\
				&intel_ssgdma_init,					\
				NULL,							\
				&intel_ssgdma_data_##inst,				\
				&intel_ssgdma_config_##inst,				\
				POST_KERNEL,						\
				CONFIG_DMA_INIT_PRIORITY,				\
				&intel_ssgdma_driver_api);				\
											\
	static void ssgdma_dma_irq_config_##inst(void)					\
	{										\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(inst)), CONFIGURE_DMA_IRQ, (), inst)	\
	}

DT_INST_FOREACH_STATUS_OKAY(DMA_INTEL_SSGDMA_DEVICE_INIT)
