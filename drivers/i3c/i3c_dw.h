/*
 * Copyright (c) 2026, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_DW_H_
#define ZEPHYR_DRIVERS_I3C_DW_H_

/* Device Control Register */
#define DEVICE_CTRL				(0x0)
#define DEV_CTRL_ENABLE				BIT(31)
#define DEV_CTRL_RESUME				BIT(30)
#define DEV_CTRL_HOT_JOIN_NACK			BIT(8)
#define DEV_CTRL_I2C_SLAVE_PRESENT		BIT(7)

/* Device Address Register */
#define DEVICE_ADDR				(0x4)
#define DEV_ADDR_DYNAMIC_ADDR_VALID		BIT(31)
#define DEV_ADDR_DYNAMIC(x)			(((x) << 16) & GENMASK(22, 16))

/* Hardware Capability register */
#define HW_CAPABILITY				(0x8)
#define HW_CAPABILITY_MASK(x)		((x) & GENMASK(2, 0))

/* COMMAND_QUEUE_PORT register */
#define COMMAND_QUEUE_PORT			(0xc)
#define COMMAND_PORT_TOC			BIT(30)
#define COMMAND_PORT_READ_TRANSFER		BIT(28)
#define COMMAND_PORT_ROC			BIT(26)
#define COMMAND_PORT_SPEED(x)			(((x) << 21) & GENMASK(23, 21))
#define COMMAND_PORT_DEV_INDEX(x)		(((x) << 16) & GENMASK(20, 16))
#define COMMAND_PORT_CP				BIT(15)
#define COMMAND_PORT_CMD(x)			(((x) << 7) & GENMASK(14, 7))
#define COMMAND_PORT_TID(x)			(((x) << 3) & GENMASK(6, 3))
#define I3C_DDR_SPEED				(0x6)

#define COMMAND_PORT_ARG_DATA_LEN(x)		(((x) << 16) & GENMASK(31, 16))
#define COMMAND_PORT_TRANSFER_ARG		(0x01)

#define COMMAND_PORT_DEV_COUNT(x)		(((x) << 21) & GENMASK(25, 21))
#define COMMAND_PORT_ADDR_ASSGN_CMD		(0x03)

/* RESPONSE_QUEUE_PORT register */
#define RESPONSE_QUEUE_PORT			(0x10)
#define RESPONSE_PORT_ERR_STATUS(x)		(((x) & GENMASK(31, 28)) >> 28)
#define RESPONSE_NO_ERROR			(0)
#define RESPONSE_ERROR_CRC			(1)
#define RESPONSE_ERROR_PARITY			(2)
#define RESPONSE_ERROR_FRAME			(3)
#define RESPONSE_ERROR_IBA_NACK			(4)
#define RESPONSE_ERROR_ADDRESS_NACK		(5)
#define RESPONSE_ERROR_OVER_UNDER_FLOW		(6)
#define RESPONSE_ERROR_TRANSF_ABORT		(8)
#define RESPONSE_ERROR_I2C_W_NACK_ERR		(9)
#define RESPONSE_PORT_TID(x)			(((x) & GENMASK(27, 24)) >> 24)
#define RESPONSE_PORT_DATA_LEN(x)		((x) & GENMASK(15, 0))

#define RX_TX_BUFFER_DATA_PACKET_SIZE		(0x4)

/* Receive/Transmit Data Port Register */
#define RX_TX_DATA_PORT				(0x14)

/* In-Band Interrupt Queue Status Register */
#define IBI_QUEUE_STATUS			(0x18)
#define IBI_QUEUE_IBI_ID(x)			(((x) & GENMASK(15, 8)) >> 8)
#define IBI_QUEUE_IBI_ID_ADDR(x)		((x) >> 1)

/* Queue Threshold Control Register */
#define QUEUE_THLD_CTRL				(0x1c)
#define QUEUE_THLD_CTRL_IBI_STS_MASK		GENMASK(31, 24)
#define QUEUE_THLD_CTRL_RESP_BUF_MASK		GENMASK(15, 8)
#define QUEUE_THLD_CTRL_RESP_BUF(x)		(((x) - 1) << 8)

/* Data Buffer Threshold Control Register */
#define DATA_BUFFER_THLD_CTRL			(0x20)
#define DATA_BUFFER_THLD_CTRL_RX_BUF		GENMASK(11, 8)

/*  IBI MR Request Rejection Control Register */
#define IBI_MR_REQ_REJECT			0x2C

/*  IBI SIR Request Rejection Control Register */
#define IBI_SIR_REQ_REJECT			0x30
#define IBI_SIR_REQ_ID(x)			((((x) & GENMASK(6, 5)) >> 5) +\
						((x) & GENMASK(4, 0)))
#define IBI_REQ_REJECT_ALL			GENMASK(31, 0)

/* Reset Control Register */
#define RESET_CTRL				(0x34)
#define RESET_CTRL_IBI_QUEUE			BIT(5)
#define RESET_CTRL_RX_FIFO			BIT(4)
#define RESET_CTRL_TX_FIFO			BIT(3)
#define RESET_CTRL_RESP_QUEUE			BIT(2)
#define RESET_CTRL_CMD_QUEUE			BIT(1)
#define RESET_CTRL_SOFT				BIT(0)
#define RESET_CTRL_ALL				\
	(RESET_CTRL_IBI_QUEUE | RESET_CTRL_RX_FIFO |	\
	 RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE |	\
	 RESET_CTRL_CMD_QUEUE | RESET_CTRL_SOFT)

/* Interrupt Status Register */
#define INTR_STATUS				(0x3c)

/* Interrupt Status Enable Register */
#define INTR_STATUS_EN				(0x40)

/* Interrupt Signal Enable Register */
#define INTR_SIGNAL_EN				(0x44)
#define INTR_BUSOWNER_UPDATE_STAT		BIT(13)
#define INTR_IBI_UPDATED_STAT			BIT(12)
#define INTR_READ_REQ_RECV_STAT			BIT(11)
#define INTR_TRANSFER_ERR_STAT			BIT(9)
#define INTR_DYN_ADDR_ASSGN_STAT		BIT(8)
#define INTR_CCC_UPDATED_STAT			BIT(6)
#define INTR_TRANSFER_ABORT_STAT		BIT(5)
#define INTR_RESP_READY_STAT			BIT(4)
#define INTR_CMD_QUEUE_READY_STAT		BIT(3)
#define INTR_IBI_THLD_STAT			BIT(2)
#define INTR_RX_THLD_STAT			BIT(1)
#define INTR_TX_THLD_STAT			BIT(0)
#define INTR_ALL				\
	(INTR_BUSOWNER_UPDATE_STAT | INTR_IBI_UPDATED_STAT |	\
	 INTR_READ_REQ_RECV_STAT | INTR_TRANSFER_ERR_STAT |	\
	 INTR_DYN_ADDR_ASSGN_STAT | INTR_CCC_UPDATED_STAT |	\
	 INTR_TRANSFER_ABORT_STAT | INTR_RESP_READY_STAT |	\
	 INTR_CMD_QUEUE_READY_STAT | INTR_IBI_THLD_STAT |	\
	 INTR_TX_THLD_STAT | INTR_RX_THLD_STAT)

#define INTR_CONTROLLER_MASK			\
	(INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT | INTR_IBI_THLD_STAT)

/* Queue Status Level Register */
#define QUEUE_STATUS_LEVEL			(0x4c)
#define QUEUE_STATUS_IBI_BUF_BLR(x)		(((x) & GENMASK(23, 16)) >> 16)
#define QUEUE_STATUS_LEVEL_RESP(x)		(((x) & GENMASK(15, 8)) >> 8)
#define QUEUE_STATUS_LEVEL_CMD(x)	((x) & GENMASK(7, 0))

/* Data Buffer Status Level Register */
#define DATA_BUFFER_STATUS_LEVEL	(0x50)
#define DATA_BUFFER_STATUS_LEVEL_TX(x)	((x) & GENMASK(7, 0))

/* Pointer for Device Address Table Registers */
#define DEVICE_ADDR_TABLE_POINTER		(0x5c)
#define DEVICE_ADDR_TABLE_ADDR(x)		((x) & GENMASK(15, 0))

/* Pointer for Device Characteristics Table */
#define DEV_CHAR_TABLE_POINTER			(0x60)
#define DEVICE_CHAR_TABLE_ADDR(x)		((x) & GENMASK(11, 0))

/* SCL I3C Open Drain Timing Register */
#define SCL_I3C_OD_TIMING			(0xb4)

/* SCL I3C Push Pull Timing Register */
#define SCL_I3C_PP_TIMING			(0xb8)
#define SCL_I3C_TIMING_HCNT(x)			((((uint32_t)x) << 16) & GENMASK(23, 16))
#define SCL_I3C_TIMING_LCNT(x)			((x) & GENMASK(7, 0))
#define SCL_I3C_TIMING_CNT_MIN			(5)

/* SCL I2C Fast Mode Timing Register */
#define SCL_I2C_FM_TIMING			(0xbc)
#define SCL_I2C_FM_TIMING_HCNT(x)		((((uint32_t)x) << 16) & GENMASK(31, 16))
#define SCL_I2C_FM_TIMING_LCNT(x)		((x) & GENMASK(15, 0))

/* SCL I2C Fast Mode Plus Timing Register */
#define SCL_I2C_FMP_TIMING			(0xc0)
#define SCL_I2C_FMP_TIMING_HCNT(x)		((((uint32_t)x) << 16) & GENMASK(23, 16))
#define SCL_I2C_FMP_TIMING_LCNT(x)		((x) & GENMASK(15, 0))

/*SCL Extended Low Count Timing Register*/
#define SCL_EXT_LCNT_TIMING			(0xc8)
#define SCL_EXT_LCNT_4(x)			((((uint32_t)x) << 24) & GENMASK(31, 24))
#define SCL_EXT_LCNT_3(x)			((((uint32_t)x) << 16) & GENMASK(23, 16))
#define SCL_EXT_LCNT_2(x)			((((uint32_t)x) << 8) & GENMASK(15, 8))
#define SCL_EXT_LCNT_1(x)			((x) & GENMASK(7, 0))

/* Bus Free Timing Register */
#define BUS_FREE_TIMING				(0xd4)
#define BUS_I3C_MST_FREE(x)			((x) & GENMASK(15, 0))

#define DEV_ADDR_TABLE_LEGACY_I2C_DEV	BIT(31)
#define DEV_ADDR_TABLE_STATIC_ADDR(x)	((x) & GENMASK(6, 0))

#define DEV_ADDR_TABLE_DYNAMIC_ADDR(x)		(((x) << 16) & GENMASK(23, 16))
#define DEV_ADDR_TABLE_LOC(start, idx)		((start) + ((idx) << 2))

#define DEV_CHAR_TABLE_LOC1(start, idx)		((start) + ((idx) << 4))
#define DEV_CHAR_TABLE_MSB_PID(x)		((x) & GENMASK(31, 16))
#define DEV_CHAR_TABLE_LSB_PID(x)		((x) & GENMASK(15, 0))
#define DEV_CHAR_TABLE_LOC2(start, idx)		((DEV_CHAR_TABLE_LOC1(start,\
						 idx)) + 4)
#define DEV_CHAR_TABLE_LOC3(start, idx)		((DEV_CHAR_TABLE_LOC1(start,\
						 idx)) + 8)
#define DEV_CHAR_TABLE_DCR(x)			((x) & GENMASK(7, 0))
#define DEV_CHAR_TABLE_BCR(x)			(((x) & GENMASK(15, 8)) >> 8)

#define I3C_BUS_SDR1_SCL_RATE			(8000000) /* 8 MHz */
#define I3C_BUS_SDR2_SCL_RATE			(6000000) /* 6 MHz */
#define I3C_BUS_SDR3_SCL_RATE			(4000000) /* 4 MHz */
#define I3C_BUS_SDR4_SCL_RATE			(2000000) /* 2 MHz */
#define I3C_BUS_I2C_FM_TLOW_MIN_NS		(1300)
#define I3C_BUS_I2C_FMP_TLOW_MIN_NS		(500)
#define I3C_BUS_THIGH_MAX_NS			(41)
#define I3C_PERIOD_NS				(1000000000)

#define I3C_SDR_MODE				(0x0)
#define I3C_HDR_MODE				(0x1)
#define I2C_SLAVE				(2)
#define I3C_SLAVE				(3)
#define I3C_GETMXDS_FORMAT_1_LEN		(2)
#define I3C_GETMXDS_FORMAT_2_LEN		(5)

/*
 * CCC event bits.
 */
#define I3C_CCC_EVENT_SIR			BIT(0)
#define I3C_CCC_EVENT_MR			BIT(1)
#define I3C_CCC_EVENT_HJ			BIT(3)

#define I3C_BUS_TYP_I3C_SCL_RATE		(12500000) /* 12.5 MHz */
#define I3C_BUS_I2C_FM_PLUS_SCL_RATE		(1000000) /*1 MHz*/
#define I3C_BUS_I2C_FM_SCL_RATE			(400000) /* 400 KHz */
#define I3C_BUS_TLOW_OD_MIN_NS			(200)

#define I3C_LVR_I2C_INDEX_MASK			GENMASK(7, 5)
#define I3C_LVR_I2C_INDEX(x)			((x) << 5)
#define I3C_LVR_I2C_FM_MODE_AG			BIT(4)

#define I3C_HOT_JOIN_ADDR			(0x2)

/*
 * ---------------------------------------------------------------------------
 * Macros migrated from i3c_dw.c (not previously present in this header).
 *
 * NOTE: Some of these overlap semantically with existing defines above but
 * use a different name (the driver's DEVICE_ADDR_* vs this header's
 * DEV_ADDR_*, IBI_QUEUE_STATUS_IBI_ID vs IBI_QUEUE_IBI_ID,
 * INTR_MASTER_MASK vs INTR_CONTROLLER_MASK, HW_CAPABILITY_* fields vs
 * HW_CAPABILITY_MASK). They are kept verbatim so i3c_dw.c compiles against
 * this header unchanged; unify the names in a follow-up cleanup before
 * upstream submission.
 * ---------------------------------------------------------------------------
 */

/* Device Control Register - extra bit */
#define DEV_CTRL_IBA_INCLUDE			BIT(0)

/* Device Address Register - driver-style aliases */
#define DEVICE_ADDR_DYNAMIC_ADDR_VALID		BIT(31)
#define DEVICE_ADDR_DYNAMIC(x)			(((x) << 16) & GENMASK(22, 16))
#define DEVICE_ADDR_STATIC_ADDR_VALID		BIT(15)
#define DEVICE_ADDR_STATIC_MASK			GENMASK(6, 0)
#define DEVICE_ADDR_STATIC(x)			((x) & DEVICE_ADDR_STATIC_MASK)

/* Hardware Capability Register - fields */
#define HW_CAPABILITY_SLV_IBI_CAP		BIT(19)
#define HW_CAPABILITY_SLV_HJ_CAP		BIT(18)
#define HW_CAPABILITY_HDR_TS_EN			BIT(4)
#define HW_CAPABILITY_HDR_DDR_EN		BIT(3)
#define HW_CAPABILITY_DEVICE_ROLE_CONFIG_MASK	GENMASK(2, 0)

/* Command Queue Port - extra fields */
/* COMMAND_PORT_DEV_COUNT occupies GENMASK(25, 21): a 5-bit field, max 31 */
#define DAA_DEV_COUNT_MAX			31U
#define COMMAND_PORT_SDAP			BIT(27)
#define COMMAND_PORT_DBP			BIT(25)
#define COMMAND_PORT_SPEED_I2C_FM		0
#define COMMAND_PORT_SPEED_I2C_FMP		1
#define COMMAND_PORT_SPEED_I3C_DDR		6
#define COMMAND_PORT_SPEED_I3C_TS		7
#define COMMAND_PORT_ARG_DB(x)			(((x) << 8) & GENMASK(15, 8))
#define COMMAND_PORT_ARG_DATA_LEN_MAX		65536
#define COMMAND_PORT_SDA_DATA_BYTE_3(x)		(((x) << 24) & GENMASK(31, 24))
#define COMMAND_PORT_SDA_DATA_BYTE_2(x)		(((x) << 16) & GENMASK(23, 16))
#define COMMAND_PORT_SDA_DATA_BYTE_1(x)		(((x) << 8) & GENMASK(15, 8))
#define COMMAND_PORT_SDA_BYTE_STRB_3		BIT(5)
#define COMMAND_PORT_SDA_BYTE_STRB_2		BIT(4)
#define COMMAND_PORT_SDA_BYTE_STRB_1		BIT(3)
#define COMMAND_PORT_SHORT_DATA_ARG		0x02

/* IBI Queue Status Register - driver-style accessors */
#define IBI_QUEUE_STATUS_IBI_STS(x)		(((x) & GENMASK(31, 28)) >> 28)
#define IBI_QUEUE_STATUS_IBI_ID(x)		(((x) & GENMASK(15, 8)) >> 8)
#define IBI_QUEUE_STATUS_DATA_LEN(x)		((x) & GENMASK(7, 0))
#define IBI_QUEUE_IBI_ADDR(x)			(IBI_QUEUE_STATUS_IBI_ID(x) >> 1)
#define IBI_QUEUE_IBI_RNW(x)			(IBI_QUEUE_STATUS_IBI_ID(x) & BIT(0))
#define IBI_TYPE_MR(x)								\
	((IBI_QUEUE_IBI_ADDR(x) != I3C_HOT_JOIN_ADDR) && !IBI_QUEUE_IBI_RNW(x))
#define IBI_TYPE_HJ(x)								\
	((IBI_QUEUE_IBI_ADDR(x) == I3C_HOT_JOIN_ADDR) && !IBI_QUEUE_IBI_RNW(x))
#define IBI_TYPE_SIRQ(x)							\
	((IBI_QUEUE_IBI_ADDR(x) != I3C_HOT_JOIN_ADDR) && IBI_QUEUE_IBI_RNW(x))

/* IBI Queue Control Register */
#define IBI_QUEUE_CTRL				0x24

/* Slave Event Status Register */
#define SLV_EVENT_STATUS			0x38
#define SLV_EVENT_STATUS_HJ_EN			BIT(3)
#define SLV_EVENT_STATUS_MR_EN			BIT(1)
#define SLV_EVENT_STATUS_SIR_EN			BIT(0)

/* Interrupt registers - extras */
#define INTR_FORCE				0x48
#define INTR_READ_REQ_RECV_STAT_ALT		INTR_READ_REQ_RECV_STAT
#define INTR_DEFSLV_STAT			BIT(10)

#ifdef CONFIG_I3C_USE_IBI
#define INTR_MASTER_MASK							\
	(INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT | INTR_IBI_THLD_STAT)
#else
#define INTR_MASTER_MASK							\
	(INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT)
#endif
#define INTR_SLAVE_MASK								\
	(INTR_TRANSFER_ERR_STAT | INTR_IBI_UPDATED_STAT | INTR_READ_REQ_RECV_STAT |	\
	 INTR_DYN_ADDR_ASSGN_STAT | INTR_RESP_READY_STAT)

/* Queue Status Level Register - extra field */
#define QUEUE_STATUS_IBI_STATUS_CNT(x)		(((x) & GENMASK(28, 24)) >> 24)

/* Data Buffer Status Level Register - extra field */
#define DATA_BUFFER_STATUS_LEVEL_RX(x)		(((x) & GENMASK(23, 16)) >> 16)

/* Present State Register */
#define PRESENT_STATE				0x54
#define PRESENT_STATE_CURRENT_MASTER		BIT(2)

/* CCC Device Status Register */
#define CCC_DEVICE_STATUS			0x58

/* Device Address Table Pointer - extra accessor */
#define DEVICE_ADDR_TABLE_DEPTH(x)		(((x) & GENMASK(31, 16)) >> 16)

/* Vendor Specific Register Pointer */
#define VENDOR_SPECIFIC_REG_POINTER		0x6c

/* Slave MIPI ID Value Register */
#define SLV_MIPI_ID_VALUE			0x70
#define SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK	GENMASK(15, 1)
#define SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID(x)	((x) & SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK)
#define SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL	BIT(0)

/* Slave PID Value Register */
#define SLV_PID_VALUE				0x74

/* Slave Characteristic Control Register */
#define SLV_CHAR_CTRL				0x78
#define SLV_CHAR_CTRL_MAX_DATA_SPEED_LIMIT	BIT(0)
#define SLV_CHAR_CTRL_IBI_REQUEST_CAPABLE	BIT(1)
#define SLV_CHAR_CTRL_IBI_PAYLOAD		BIT(2)
#define SLV_CHAR_CTRL_BCR_MASK			GENMASK(7, 0)
#define SLV_CHAR_CTRL_BCR(x)			((x) & SLV_CHAR_CTRL_BCR_MASK)
#define SLV_CHAR_CTRL_DCR_MASK			GENMASK(15, 8)
#define SLV_CHAR_CTRL_DCR(x)			(((x) & SLV_CHAR_CTRL_DCR_MASK) >> 8)
#define SLV_CHAR_CTRL_HDR_CAP_MASK		GENMASK(23, 16)
#define SLV_CHAR_CTRL_HDR_CAP(x)		(((x) & SLV_CHAR_CTRL_HDR_CAP_MASK) >> 16)

/* Slave Max Length Register */
#define SLV_MAX_LEN				0x7c
#define SLV_MAX_LEN_MRL(x)			(((x) & GENMASK(31, 16)) >> 16)
#define SLV_MAX_LEN_MWL(x)			((x) & GENMASK(15, 0))

/* Max Read Turnaround Register */
#define MAX_READ_TURNAROUND			0x80
#define MAX_READ_TURNAROUND_MXDX_MAX_RD_TURN(x)	((x) & GENMASK(23, 0))

/* Max Data Speed / Slave Debug Status Registers */
#define MAX_DATA_SPEED				0x84
#define SLV_DEBUG_STATUS			0x88

/* Slave Interrupt Request Register */
#define SLV_INTR_REQ				0x8c
#define SLV_INTR_REQ_SIR_DATA_LENGTH(x)		(((x) << 16) & GENMASK(23, 16))
#define SLV_INTR_REQ_MDB(x)			(((x) << 8) & GENMASK(15, 8))
#define SLV_INTR_REQ_IBI_STS(x)			(((x) & GENMASK(9, 8)) >> 8)
#define SLV_INTR_REQ_IBI_STS_IBI_ACCEPT		0x01
#define SLV_INTR_REQ_IBI_STS_IBI_NO_ATTEMPT	0x03
#define SLV_INTR_REQ_TS				BIT(4)
#define SLV_INTR_REQ_MR				BIT(3)
#define SLV_INTR_REQ_SIR_CTRL(x)		(((x) & GENMASK(2, 1)) >> 1)
#define SLV_INTR_REQ_SIR			BIT(0)

/* Slave SIR Data Register */
#define SLV_SIR_DATA				0x94
#define SLV_SIR_DATA_BYTE3(x)			(((x) << 24) & GENMASK(31, 24))
#define SLV_SIR_DATA_BYTE2(x)			(((x) << 16) & GENMASK(23, 16))
#define SLV_SIR_DATA_BYTE1(x)			(((x) << 8) & GENMASK(15, 8))
#define SLV_SIR_DATA_BYTE0(x)			((x) & GENMASK(7, 0))

/* Slave IBI Response Register */
#define SLV_IBI_RESP				0x98
#define SLV_IBI_RESP_DATA_LENGTH(x)		(((x) & GENMASK(23, 8)) >> 8)
#define SLV_IBI_RESP_IBI_STS(x)			((x) & GENMASK(1, 0))
#define SLV_IBI_RESP_IBI_STS_ACK		0x01
#define SLV_IBI_RESP_IBI_STS_EARLY_TERMINATE	0x02
#define SLV_IBI_RESP_IBI_STS_NACK		0x03

/* Slave NACK Request Register */
#define SLV_NACK_REQ				0x9c
#define SLV_NACK_REQ_NACK_REQ(x)		((x) & GENMASK(1, 0))
#define SLV_NACK_REQ_NACK_REQ_ACK		0x00
#define SLV_NACK_REQ_NACK_REQ_NACK		0x01

/* Device Control Extended Register */
#define DEVICE_CTRL_EXTENDED				0xb0
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE(x)	((x) & GENMASK(1, 0))
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_MASTER	0
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_SLAVE	1

/* SCL I3C timing - extra bound */
#define SCL_I3C_TIMING_CNT_MAX			255

/* SCL Extended Termination Low Count Timing Register */
#define SCL_EXT_TERMN_LCNT_TIMING		0xcc

/* SDA Hold and Switch Delay Timing Register */
#define SDA_HOLD_SWITCH_DLY_TIMING			0xd0
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_TX_HOLD(x)	(((x) & GENMASK(18, 16)) >> 16)
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_PP_OD_SWITCH_DLY(x)	(((x) & GENMASK(10, 8)) >> 8)
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_OD_PP_SWITCH_DLY(x)	((x) & GENMASK(2, 0))

/* Bus Free Timing - extras */
/* Bus available time of 1us in ns */
#define I3C_BUS_AVAILABLE_TIME_NS		1000U
#define BUS_I3C_AVAIL_TIME(x)			(((x) << 16) & GENMASK(31, 16))

/* Bus Idle Timing Register */
#define BUS_IDLE_TIMING				0xd8
/* Bus Idle time of 1ms in ns */
#define I3C_BUS_IDLE_TIME_NS			1000000U
#define BUS_I3C_IDLE_TIME(x)			((x) & GENMASK(19, 0))

/* Version / Release timing Registers */
#define I3C_VER_ID				0xe0
#define I3C_VER_TYPE				0xe4
#define RELEASE_SDA_TIMING			0xec

/* Queue Size Capability Register */
#define QUEUE_SIZE_CAPABILITY				0xe8
#define QUEUE_SIZE_CAPABILITY_IBI_BUF_DWORD_SIZE(x)	(2 << (((x) & GENMASK(19, 16)) >> 16))
#define QUEUE_SIZE_CAPABILITY_RESP_BUF_DWORD_SIZE(x)	(2 << (((x) & GENMASK(15, 12)) >> 12))
#define QUEUE_SIZE_CAPABILITY_CMD_BUF_DWORD_SIZE(x)	(2 << (((x) & GENMASK(11, 8)) >> 8))
#define QUEUE_SIZE_CAPABILITY_RX_BUF_DWORD_SIZE(x)	(2 << (((x) & GENMASK(7, 4)) >> 4))
#define QUEUE_SIZE_CAPABILITY_TX_BUF_DWORD_SIZE(x)	(2 << ((x) & GENMASK(3, 0)))

/* Device Address Table - extra bits */
#define DEV_ADDR_TABLE_DYNAMIC_ADDR_MASK	GENMASK(23, 16)
#define DEV_ADDR_TABLE_SIR_REJECT		BIT(13)
#define DEV_ADDR_TABLE_IBI_WITH_DATA		BIT(12)

/* Bus rate / timing constants - extras */
#define I3C_BUS_TCAS_PS				38400
/* NOTE: i3c_dw.c defines I3C_PERIOD_NS as 1000000000ULL; this header uses
 * (1000000000). I3C_PERIOD_PS relies on the ULL literal below to avoid
 * overflow, so it is safe, but consider aligning the I3C_PERIOD_NS type.
 */
#define I3C_PERIOD_PS				(I3C_PERIOD_NS * 1000ULL)
#define I3C_BUS_MAX_I3C_SCL_RATE		12900000

/* Driver-local constants */
#define NANO_SEC				1000000000ULL
#define BYTES_PER_DWORD				4
#define DW_I3C_MAX_DEVS				32
#define DW_I3C_MAX_CMD_BUF_SIZE			16

#endif /* ZEPHYR_DRIVERS_I3C_DW_H_ */
