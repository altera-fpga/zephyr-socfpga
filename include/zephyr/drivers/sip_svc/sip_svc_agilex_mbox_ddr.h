/*
 * Copyright (c) 2026, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SIP_SVC_AGILEX_MBOX_DDR_H_
#define ZEPHYR_INCLUDE_SIP_SVC_AGILEX_MBOX_DDR_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/devicetree.h>

/**
 * @file
 * @brief DDR mailbox buffer region for Agilex5 SiP SMC.
 *
 * External TF-A validates mailbox command buffers with is_address_in_ddr_range(),
 * which requires addresses >= BL31_LIMIT (0x82000000) on Agilex5. Zephyr SRAM
 * (mem0 @ 0x80100000) is below that limit, so mailbox buffers must be placed
 * in the MBOX_DDR linker region defined in devicetree.
 */

#if DT_NODE_EXISTS(DT_NODELABEL(mbox_ddr))
#define SOCFPGA_MBOX_DDR_BASE	DT_REG_ADDR(DT_NODELABEL(mbox_ddr))
#define SOCFPGA_MBOX_DDR_SIZE	DT_REG_SIZE(DT_NODELABEL(mbox_ddr))
#else
#define SOCFPGA_MBOX_DDR_BASE	0x82000000UL
#define SOCFPGA_MBOX_DDR_SIZE	0x00010000UL
#endif

#define SOCFPGA_MBOX_DDR __attribute__((section("MBOX_DDR")))

static inline bool sip_svc_is_mbox_ddr_buffer(const void *ptr)
{
	uintptr_t addr = (uintptr_t)ptr;

	return addr >= SOCFPGA_MBOX_DDR_BASE &&
	       addr < (SOCFPGA_MBOX_DDR_BASE + SOCFPGA_MBOX_DDR_SIZE);
}

#endif /* ZEPHYR_INCLUDE_SIP_SVC_AGILEX_MBOX_DDR_H_ */
