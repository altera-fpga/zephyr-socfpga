/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header file for intel ssgdma driver.
 *
 * @details User may include this in their application to gain access to
 * driver specific defines and structure.
 */

#ifndef ZEPHYR_DRIVERS_DMA_INTEL_SSGDMA_H_
#define ZEPHYR_DRIVERS_DMA_INTEL_SSGDMA_H_

#define INTEL_SSGDMA_ERROR     (-1)

#ifndef CONFIG_DMA_64BIT
/* 32 bit address */
typedef uint32_t ssgdma_addr_t;

#define INTEL_SSGDMA_UPPER_32_BITS(n) ((uint32_t)0U)
#define INTEL_SSGDMA_LOWER_32_BITS(n) ((uint32_t)n)
#else
/* 64 bit address */
typedef uint64_t ssgdma_addr_t;

#define INTEL_SSGDMA_UPPER_32_BITS(n) ((uint32_t)(((uint64_t)n) >> 32U))
#define INTEL_SSGDMA_LOWER_32_BITS(n) ((uint32_t)(((uint64_t)n) & 0XFFFFFFFFU))
#endif

#define INTEL_SSGDMA_TOTAL_PORT_TYPES          (3U)

#define INTEL_SSGDMA_DEVICE_PORT_CSR_BASE_OFST (0x200000UL)
#define INTEL_SSGDMA_DEVICE_PORT_NUM_OFST      (0x800U)
#define INTEL_SSGDMA_DEVICE_PORT_TYPE_SHIFT    (18U)
#define INTEL_SSGDMA_TOTAL_PORT_TYPES          (3U)

enum intel_ssgdma_port_type_e {
	INTEL_SSGDMA_D2H_ST_PORT = 0,
	INTEL_SSGDMA_H2D_ST_PORT = 1,
	INTEL_SSGDMA_H2D_MM_PORT = 2,
};

enum intel_ssgdma_valid_e {
	INTEL_SSGDMA_INVALID  = 0,
	INTEL_SSGDMA_VALID    = 1,
};

/* bits for device_errors_flags
 * These bits indicates abnormal operation of the driver.
 * User is advised to check these bits and clear them in case of false alarm.
 * User may also enable INTEL_SSGDMA_ENABLE_DEBUG_MESSAGE to see debug printouts.
 */
/*indicates that software does not match the HW. */
#define INTEL_SSGDMA_CONFIG_MISMATCH_MSK                       (1U << 0U)

/* Bits for channel_flags */
/* set if port is allocated. */
#define INTEL_SSGDMA_DEVICE_PORT_SETUP_MSK                     (1U << 0U)
/* indicates that user has enabled reponder for this device port */
#define INTEL_SSGDMA_DEVICE_PORT_RESPONDER_EN_MSK              (1U << 1U)
/* flag to reuse previous setup */
#define INTEL_SSGDMA_DEVICE_PORT_REUSE_SETUP_MSK               (1U << 2U)
/* indicates a soft reset occurs */
#define INTEL_SSGDMA_DEVICE_PORT_RESET_MSK                     (1U << 3U)
/* indicates dma start is called. */
#define INTEL_SSGDMA_DEVICE_PORT_DMA_START_MSK                 (1U << 4U)

/**************/
/* global csr */
#define INTEL_SSGDMA_GCSR_CTRL                                              0x00000100
#define INTEL_SSGDMA_GCSR_CTRL_RESET_PREFETCH_ENGINE_MSK                    0x00000001
#define INTEL_SSGDMA_GCSR_CTRL_RUN_PREFETCH_ENGINE_MSK                      0x00000002
#define INTEL_SSGDMA_GCSR_CTRL_WATCHDOG_TIMER_EN_MSK                        0x00000004
#define INTEL_SSGDMA_GCSR_CTRL_RESERVED_MSK                                 0xFFFFFFF8
#define INTEL_SSGDMA_GCSR_CTRL_RESET_PREFETCH_ENGINE_OFST                   0x00000000
#define INTEL_SSGDMA_GCSR_CTRL_RUN_PREFETCH_ENGINE_OFST                     0x00000001
#define INTEL_SSGDMA_GCSR_CTRL_WATCHDOG_TIMER_EN_OFST                       0x00000002
#define INTEL_SSGDMA_GCSR_CTRL_RESERVED_OFST                                0x00000003

#define INTEL_SSGDMA_GCSR_STATUS                                            0x00000104
#define INTEL_SSGDMA_GCSR_STATUS_RESERVED_MSK                               0x3FFFFFFF
#define INTEL_SSGDMA_GCSR_STATUS_WATCHDOG_TIMEOUT_ERROR_MSK                 0x40000000
#define INTEL_SSGDMA_GCSR_STATUS_IRQ_MSK                                    0x80000000
#define INTEL_SSGDMA_GCSR_STATUS_RESERVED_OFST                              0x00000000
#define INTEL_SSGDMA_GCSR_STATUS_WATCHDOG_TIMEOUT_ERROR_OFST                0x0000001E
#define INTEL_SSGDMA_GCSR_STATUS_IRQ_OFST                                   0x0000001F

#define INTEL_SSGDMA_GCSR_WB_INTR_TIMEOUT                                   0x00000108
#define INTEL_SSGDMA_GCSR_WB_INTR_TIMEOUT_MSK                               0x000FFFFF
#define INTEL_SSGDMA_GCSR_WB_INTR_TIMEOUT_RESERVED_MSK                      0xFFF00000
#define INTEL_SSGDMA_GCSR_WB_INTR_TIMEOUT_OFST                              0x00000000
#define INTEL_SSGDMA_GCSR_WB_INTR_TIMEOUT_RESERVED_OFST                     0x00000014

#define INTEL_SSGDMA_GCSR_VER_NUM                                           0x0000010C
#define INTEL_SSGDMA_GCSR_VER_NUM_PATCH_VER_MSK                             0x000000FF
#define INTEL_SSGDMA_GCSR_VER_NUM_UPDATE_VER_MSK                            0x0000FF00
#define INTEL_SSGDMA_GCSR_VER_NUM_MAJOR_VER_MSK                             0x00FF0000
#define INTEL_SSGDMA_GCSR_VER_NUM_RESERVED_MSK                              0xFF000000
#define INTEL_SSGDMA_GCSR_VER_NUM_PATCH_VER_OFST                            0x00000000
#define INTEL_SSGDMA_GCSR_VER_NUM_UPDATE_VER_OFST                           0x00000008
#define INTEL_SSGDMA_GCSR_VER_NUM_MAJOR_VER_OFST                            0x00000010
#define INTEL_SSGDMA_GCSR_VER_NUM_RESERVED_OFST                             0x00000018

#define INTEL_SSGDMA_GCSR_WATCHDOG_TIMEOUT                                  0x00000114
#define INTEL_SSGDMA_GCSR_WATCHDOG_TIMEOUT_MSK                              0x000FFFFF
#define INTEL_SSGDMA_GCSR_WATCHDOG_TIMEOUT_RESERVED_MSK                     0xFFF00000
#define INTEL_SSGDMA_GCSR_WATCHDOG_TIMEOUT_OFST                             0x00000000
#define INTEL_SSGDMA_GCSR_WATCHDOG_TIMEOUT_RESERVED_OFST                    0x00000014

#define INTEL_SSGDMA_GCSR_SCRATCH                                           0x00000118
#define INTEL_SSGDMA_GCSR_SCRATCH_MSK                                       0xFFFFFFFF
#define INTEL_SSGDMA_GCSR_SCRATCH_OFST                                      0x00000000

#define INTEL_SSGDMA_GCSR_IP_PARAM1                                         0x0000011C
#define INTEL_SSGDMA_GCSR_IP_PARAM1_DMA_MODE_MSK                            0x00000003
#define INTEL_SSGDMA_GCSR_IP_PARAM1_TILE_MSK                                0x0000000C
#define INTEL_SSGDMA_GCSR_IP_PARAM1_PCIE_FUNC_MODE_MSK                      0x00000030
#define INTEL_SSGDMA_GCSR_IP_PARAM1_ADDR_64BITS_EN_MSK                      0x00000040
#define INTEL_SSGDMA_GCSR_IP_PARAM1_DEBUG_PORT_EN_MSK                       0x00000080
#define INTEL_SSGDMA_GCSR_IP_PARAM1_EXT_INTERRUPT_EN_MSK                    0x00000100
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_EXT_VECTORS_MSK                     0x00003E00
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_BAR_PORTS_MSK                       0x0000C000
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_MM_PORTS_MSK                    0x001F0000
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_ST_PORTS_MSK                    0x03E00000
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_D2H_ST_PORTS_MSK                    0x7C000000
#define INTEL_SSGDMA_GCSR_IP_PARAM1_UNALIGNED_ACCESS_EN_MSK                 0x80000000
#define INTEL_SSGDMA_GCSR_IP_PARAM1_DMA_MODE_OFST                           0x00000000
#define INTEL_SSGDMA_GCSR_IP_PARAM1_TILE_OFST                               0x00000002
#define INTEL_SSGDMA_GCSR_IP_PARAM1_PCIE_FUNC_MODE_OFST                     0x00000004
#define INTEL_SSGDMA_GCSR_IP_PARAM1_ADDR_64BITS_EN_OFST                     0x00000006
#define INTEL_SSGDMA_GCSR_IP_PARAM1_DEBUG_PORT_EN_OFST                      0x00000007
#define INTEL_SSGDMA_GCSR_IP_PARAM1_EXT_INTERRUPT_EN_OFST                   0x00000008
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_EXT_VECTORS_OFST                    0x00000009
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_BAR_PORTS_OFST                      0x0000000E
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_MM_PORTS_OFST                   0x00000010
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_H2D_ST_PORTS_OFST                   0x00000015
#define INTEL_SSGDMA_GCSR_IP_PARAM1_NUM_D2H_ST_PORTS_OFST                   0x0000001A
#define INTEL_SSGDMA_GCSR_IP_PARAM1_UNALIGNED_ACCESS_EN_OFST                0x0000001F

#define INTEL_SSGDMA_GCSR_DEVICE_PORTS_IRQ_STATUS                           0x00000120
#define INTEL_SSGDMA_GCSR_DEVICE_PORTS_IRQ_STATUS_MSK                       0x0000FFFF
#define INTEL_SSGDMA_GCSR_DEVICE_PORTS_IRQ_STATUS_RESERVED_MSK              0xFFFF0000
#define INTEL_SSGDMA_GCSR_DEVICE_PORTS_IRQ_STATUS_OFST                      0x00000000
#define INTEL_SSGDMA_GCSR_DEVICE_PORTS_IRQ_STATUS_RESERVED_OFST             0x00000010

/*******************/
/* device port csr */
#define INTEL_SSGDMA_QCSR_CTRL                                              0x00000000
#define INTEL_SSGDMA_QCSR_CTRL_Q_SW_RESET_REQ_MSK                           0x00000001
#define INTEL_SSGDMA_QCSR_CTRL_Q_PAUSE_AGENT_CONTROL_MSK                    0x00000002
#define INTEL_SSGDMA_QCSR_CTRL_IRQ_EN_MSK                                   0x00000004
#define INTEL_SSGDMA_QCSR_CTRL_PREFETCH_IRQ_EN_MSK                          0x00000008
#define INTEL_SSGDMA_QCSR_CTRL_WB_EN_MSK                                    0x00000010
#define INTEL_SSGDMA_QCSR_CTRL_Q_EN_MSK                                     0x00000020
#define INTEL_SSGDMA_QCSR_CTRL_WB_OPTIMIZED_MODE_MSK                        0x00000040
#define INTEL_SSGDMA_QCSR_CTRL_RESERVED_MSK                                 0xFFFFFF80
#define INTEL_SSGDMA_QCSR_CTRL_Q_SW_RESET_REQ_OFST                          0x00000000
#define INTEL_SSGDMA_QCSR_CTRL_Q_PAUSE_AGENT_CONTROL_OFST                   0x00000001
#define INTEL_SSGDMA_QCSR_CTRL_IRQ_EN_OFST                                  0x00000002
#define INTEL_SSGDMA_QCSR_CTRL_PREFETCH_IRQ_EN_OFST                         0x00000003
#define INTEL_SSGDMA_QCSR_CTRL_WB_EN_OFST                                   0x00000004
#define INTEL_SSGDMA_QCSR_CTRL_Q_EN_OFST                                    0x00000005
#define INTEL_SSGDMA_QCSR_CTRL_WB_OPTIMIZED_MODE_OFST                       0x00000006
#define INTEL_SSGDMA_QCSR_CTRL_RESERVED_OFST                                0x00000007

#define INTEL_SSGDMA_QCSR_STATUS                                            0x00000004
#define INTEL_SSGDMA_QCSR_STATUS_Q_BUSY_MSK                                 0x00000001
#define INTEL_SSGDMA_QCSR_STATUS_Q_RESETTING_MSK                            0x00000002
#define INTEL_SSGDMA_QCSR_STATUS_Q_DESCR_BUFFER_EMPTY_MSK                   0x00000004
#define INTEL_SSGDMA_QCSR_STATUS_Q_DESCR_BUFFER_FULL_MSK                    0x00000008
#define INTEL_SSGDMA_QCSR_STATUS_Q_RESP_BUFFER_EMPTY_MSK                    0x00000010
#define INTEL_SSGDMA_QCSR_STATUS_Q_RESP_BUFFER_FULL_MSK                     0x00000020
#define INTEL_SSGDMA_QCSR_STATUS_Q_AGENT_CONTROL_PAUSED_MSK                 0x00000040
#define INTEL_SSGDMA_QCSR_STATUS_RESERVED_MSK                               0x07FFFF80
#define INTEL_SSGDMA_QCSR_STATUS_Q_VIDEO_FLUSHING_EVENT_MSK                 0x08000000
#define INTEL_SSGDMA_QCSR_STATUS_Q_DESC_COMPLETION_MSK                      0x10000000
#define INTEL_SSGDMA_QCSR_STATUS_Q_PREFETCH_ERROR_MSK                       0x20000000
#define INTEL_SSGDMA_QCSR_STATUS_Q_ERROR_MSK                                0x40000000
#define INTEL_SSGDMA_QCSR_STATUS_Q_IRQ_MSK                                  0x80000000
#define INTEL_SSGDMA_QCSR_STATUS_Q_BUSY_OFST                                0x00000000
#define INTEL_SSGDMA_QCSR_STATUS_Q_RESETTING_OFST                           0x00000001
#define INTEL_SSGDMA_QCSR_STATUS_Q_DESCR_BUFFER_EMPTY_OFST                  0x00000002
#define INTEL_SSGDMA_QCSR_STATUS_Q_DESCR_BUFFER_FULL_OFST                   0x00000003
#define INTEL_SSGDMA_QCSR_STATUS_Q_RESP_BUFFER_EMPTY_OFST                   0x00000004
#define INTEL_SSGDMA_QCSR_STATUS_Q_RESP_BUFFER_FULL_OFST                    0x00000005
#define INTEL_SSGDMA_QCSR_STATUS_Q_AGENT_CONTROL_PAUSED_OFST                0x00000006
#define INTEL_SSGDMA_QCSR_STATUS_RESERVED_OFST                              0x00000007
#define INTEL_SSGDMA_QCSR_STATUS_Q_VIDEO_FLUSHING_EVENT_OFST                0x0000001B
#define INTEL_SSGDMA_QCSR_STATUS_Q_DESC_COMPLETION_OFST                     0x0000001C
#define INTEL_SSGDMA_QCSR_STATUS_Q_PREFETCH_ERROR_OFST                      0x0000001D
#define INTEL_SSGDMA_QCSR_STATUS_Q_ERROR_OFST                               0x0000001E
#define INTEL_SSGDMA_QCSR_STATUS_Q_IRQ_OFST                                 0x0000001F

#define INTEL_SSGDMA_QCSR_Q_START_ADDR_L                                    0x00000008
#define INTEL_SSGDMA_QCSR_Q_START_ADDR_L_MSK                                0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_START_ADDR_L_OFST                               0x00000000

#define INTEL_SSGDMA_QCSR_Q_START_ADDR_H                                    0x0000000C
#define INTEL_SSGDMA_QCSR_Q_START_ADDR_H_MSK                                0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_START_ADDR_H_OFST                               0x00000000

#define INTEL_SSGDMA_QCSR_Q_SIZE                                            0x00000010
#define INTEL_SSGDMA_QCSR_Q_SIZE_MSK                                        0x000000FF
#define INTEL_SSGDMA_QCSR_Q_SIZE_RESERVED_MSK                               0xFFFFFF00
#define INTEL_SSGDMA_QCSR_Q_SIZE_OFST                                       0x00000000
#define INTEL_SSGDMA_QCSR_Q_SIZE_RESERVED_OFST                              0x00000008

#define INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER                                 0x00000014
#define INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER_Q_EXTR_PTR_MSK                  0x0000FFFF
#define INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER_RESERVED_MSK                    0xFFFF0000
#define INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER_Q_EXTR_PTR_OFST                 0x00000000
#define INTEL_SSGDMA_QCSR_Q_EXTRACT_POINTER_RESERVED_OFST                   0x00000010

#define INTEL_SSGDMA_QCSR_Q_INSERT_POINTER                                  0x00000018
#define INTEL_SSGDMA_QCSR_Q_INSERT_POINTER_Q_INS_PTR_MSK                    0x0000FFFF
#define INTEL_SSGDMA_QCSR_Q_INSERT_POINTER_RESERVED_MSK                     0xFFFF0000
#define INTEL_SSGDMA_QCSR_Q_INSERT_POINTER_Q_INS_PTR_OFST                   0x00000000
#define INTEL_SSGDMA_QCSR_Q_INSERT_POINTER_RESERVED_OFST                    0x00000010

#define INTEL_SSGDMA_QCSR_Q_PREFETCH_POINTER                                0x0000001C
#define INTEL_SSGDMA_QCSR_Q_PREFETCH_POINTER_Q_PREFETCH_PTR_MSK             0x0000FFFF
#define INTEL_SSGDMA_QCSR_Q_PREFETCH_POINTER_RESERVED_MSK                   0xFFFF0000
#define INTEL_SSGDMA_QCSR_Q_PREFETCH_POINTER_Q_PREFETCH_PTR_OFST            0x00000000
#define INTEL_SSGDMA_QCSR_Q_PREFETCH_POINTER_RESERVED_OFST                  0x00000010

#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_1                                  0x00000020
#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_1_RESERVED_MSK                     0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_1_RESERVED_OFST                    0x00000000

#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_2                                  0x00000024
#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_2_RESERVED_MSK                     0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_2_RESERVED_OFST                    0x00000000

#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_3                                  0x00000028
#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_3_RESERVED_MSK                     0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_DEBUG_STATUS_3_RESERVED_OFST                    0x00000000

#define INTEL_SSGDMA_QCSR_Q_SCRATCH                                         0x0000002C
#define INTEL_SSGDMA_QCSR_Q_SCRATCH_MSK                                     0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_SCRATCH_OFST                                    0x00000000

#define INTEL_SSGDMA_QCSR_Q_BYTEACK                                         0x00000030
#define INTEL_SSGDMA_QCSR_Q_BYTEACK_MSK                                     0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_BYTEACK_OFST                                    0x00000000

#define INTEL_SSGDMA_QCSR_Q_BYTESSENT                                       0x00000034
#define INTEL_SSGDMA_QCSR_Q_BYTESSENT_MSK                                   0xFFFFFFFF
#define INTEL_SSGDMA_QCSR_Q_BYTESSENT_OFST                                  0x00000000

#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1                                     0x00000038
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_NUM_CHAN_PER_DEVICE_PORT_MSK        0x0000001F
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_D2H_ST_PORT_INIT_FLUSH_EN_MSK       0x00000020
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PORT_DATA_TYPE_MSK               0x00000040
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PORT_INT_TYPE_MSK                0x00000080
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PORT_PKT_MODE_MSK                0x00000100
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PTP_PORT_EN_MSK                  0x00000200
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_D2H_ST_PORT_RUNTIME_FLUSH_EN_MSK    0x00000400
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_RESERVED_MSK                        0xFFFFF800
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_NUM_CHAN_PER_DEVICE_PORT_OFST       0x00000000
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_D2H_ST_PORT_INIT_FLUSH_EN_OFST      0x00000005
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PORT_DATA_TYPE_OFST              0x00000006
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PORT_INT_TYPE_OFST               0x00000007
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PORT_PKT_MODE_OFST               0x00000008
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_ST_PTP_PORT_EN_OFST                 0x00000009
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_D2H_ST_PORT_RUNTIME_FLUSH_EN_OFST   0x0000000A
#define INTEL_SSGDMA_QCSR_Q_PORT_PARAM1_RESERVED_OFST                       0x0000000B

#define INTEL_SSGDMA_QCSR_Q_RESP_SIZE                                       0x0000003C
#define INTEL_SSGDMA_QCSR_Q_RESP_SIZE_MSK                                   0x000000FF
#define INTEL_SSGDMA_QCSR_Q_RESP_SIZE_RESERVED_MSK                          0xFFFFFF00
#define INTEL_SSGDMA_QCSR_Q_RESP_SIZE_OFST                                  0x00000000
#define INTEL_SSGDMA_QCSR_Q_RESP_SIZE_RESERVED_OFST                         0x00000008

#define INTEL_SSGDMA_QCSR_Q_RESPONDER_ADDR_L_START                          0x00000400
#define INTEL_SSGDMA_QCSR_Q_RESPONDER_ADDR_H_OFST                           0x00000004
#define INTEL_SSGDMA_QCSR_Q_RESPONDER_ADDR_REG_SIZE                         0x00000008

/*************************************
 * descriptor defines and structures.*
 *************************************/
#define INTEL_SSGDMA_DESC_BYTES (32U)
#define INTEL_SSGDMA_NUM_DESC_PER_BLOCK (128U)
#define INTEL_SSGDMA_NUM_DESC_PER_BLOCK_OFST (7U) /* 2^7 = 128 */

#define INTEL_SSGDMA_HW_DESC_OFST (1U)
#define INTEL_SSGDMA_Q_BLOCK_OFST (1U)

#define INTEL_SSGDMA_DESC_BUFF_FULL_OFST (1U)

/* first descriptor is link descriptor */
#define INTEL_SSGDMA_DATA_DESC_PER_BLOCK \
		(INTEL_SSGDMA_NUM_DESC_PER_BLOCK - 1U)

/* size of one descriptor block */
#define INTEL_DESC_BLOCK_BYTES (INTEL_SSGDMA_NUM_DESC_PER_BLOCK * \
							   INTEL_SSGDMA_DESC_BYTES)

/* Q_SIZE and Q_RESP_SIZE should not exceed these value. */
#define INTEL_SSGDMA_MAX_DESC_BLOCKS (255U)
#define INTEL_SSGDMA_MAX_RESP_BLOCKS (128U)

#define INTEL_SSGDMA_MAX_DATA_DESC \
	(INTEL_SSGDMA_DATA_DESC_PER_BLOCK * INTEL_SSGDMA_MAX_DESC_BLOCKS)

#define INTEL_SSGDMA_LINK_DESC_IDX_IN_BLOCK (1U)
#define INTEL_SSGDMA_LINK_DESC_PER_BLOCK (1U)
#define INTEL_SSGDMA_FIRST_DATA_DESC_IDX (2U)

/* Descriptor Format field mask*/
#define INTEL_SSGDMA_LINK_DESC_MSK (0x1U)
#define INTEL_SSGDMA_DATA_DESC_MSK (0x2U)
#define INTEL_SSGDMA_RESP_DESC_MSK (0x3U)
#define INTEL_SSGDMA_H2D_MSK (0x0U)
#define INTEL_SSGDMA_D2H_MSK (0x4U)
#define INTEL_SSGDMA_MEMORY_TRANSFER_MSK (0x8U)
#define INTEL_SSGDMA_STREAMING_TRANSFER_MSK (0x10U)
#define INTEL_SSGDMA_RESP_STREAMING_PTP_TRANSFER_MSK (0x18U)

/* format field combinations */
#define INTEL_SSGDMA_H2D_MEM_FORMAT (INTEL_SSGDMA_DATA_DESC_MSK |\
		INTEL_SSGDMA_MEMORY_TRANSFER_MSK)

#define INTEL_SSGDMA_H2D_ST_FORMAT (INTEL_SSGDMA_DATA_DESC_MSK |\
		INTEL_SSGDMA_H2D_MSK | INTEL_SSGDMA_STREAMING_TRANSFER_MSK)
#define INTEL_SSGDMA_D2H_ST_FORMAT (INTEL_SSGDMA_DATA_DESC_MSK |\
		INTEL_SSGDMA_D2H_MSK | INTEL_SSGDMA_STREAMING_TRANSFER_MSK)

#define INTEL_SSGDMA_H2D_MM_LINK_DESC_FORMAT (INTEL_SSGDMA_LINK_DESC_MSK |\
		INTEL_SSGDMA_MEMORY_TRANSFER_MSK)

#define INTEL_SSGDMA_H2D_ST_LINK_DESC_FORMAT (INTEL_SSGDMA_LINK_DESC_MSK |\
		INTEL_SSGDMA_H2D_MSK | INTEL_SSGDMA_STREAMING_TRANSFER_MSK)
#define INTEL_SSGDMA_D2H_ST_LINK_DESC_FORMAT (INTEL_SSGDMA_LINK_DESC_MSK |\
		INTEL_SSGDMA_D2H_MSK | INTEL_SSGDMA_STREAMING_TRANSFER_MSK)

/*********************************/
/* Descriptor control field mask */
/*Position of IRQ_EN and DescValid are same for all data descriptors. */
#define INTEL_SSGDMA_DESC_IRQ_EN_MSK (1U << 1U)
#define INTEL_SSGDMA_VALID_DESC_MSK  (1U << 7U)

/******************************************************
 * H2D/D2H Memory transfer control field
 * 0   Read/Write		1'b0 = Device's Memory Read.
 *						1'b1 = Device's Memory Write.
 * 1   IRQ_EN			Enable Interrupt
 * 2   AbortOnError		Abort if AXI Error (RESP!=2'b00) received on device port
 * 6:3 Reserved			Set to 0.
 * 7   DescValid		If set, indicate the current descriptor content is valid.
 * Bit 0 and 7 will be set by intel_ssgdma_fill_h2d_mem / intel_ssgdma_fill_d2h_mem
 */
#define INTEL_SSGDMA_MEM_WRITE_MSK          (1U << 0U)
#define INTEL_SSGDMA_MEM_ABORT_ON_ERROR_MSK (1U << 2U)

/******************************************************
 * H2D Streaming transfer control field
 * 0   Reserved    Set to 0.
 * 1   IRQ_EN      Enable Interrupt
 * 2   SOP         Start of Packet
 * 3   EOP         End of Packet
 * 6:4 Reserved    Set to 0.
 * 7   DescValid   If set, indicate the current descriptor content is valid.
 * Bit 7 will be set by intel_ssgdma_fill_h2d_st
 */
#define INTEL_SSGDMA_H2D_ST_SOP_MSK (1U << 2U)
#define INTEL_SSGDMA_H2D_ST_EOP_MSK (1U << 3U)

/******************************************************
 * D2H Streaming transfer control field
 * 0   Reserved       Set to 0.
 * 1   IRQ_EN         Enable Interrupt
 * 2   Start on SOP   Start capturing on SOP. Flush all data before SOP.
 * 3   EOP            This mark the last descriptor of the particular data buffer.
 *                    Not valid if  H2D_ST_PORT<PORT>_PKT_MODE parameter is
 *                    disabled when Avalon-ST interface is selected.
 * 6:4 Reserved       Set to 0.
 * 7   DescValid      If set, indicate the current descriptor content is valid.
 * Bit 7 will be set by intel_ssgdma_fill_d2h_st
 */
#define INTEL_SSGDMA_D2H_ST_START_ON_SOP_MSK (1U << 2U)
#define INTEL_SSGDMA_D2H_ST_EOP_MSK          (1U << 3U)

/*************************
 * Responder status mask
 */
#define INTEL_SSGDMA_RESP_COMPLETE_MSK (0x80U)

/******************************************************
 * H2D/D2H Memory transfer responder status mask
 * 1:0 DeviceError
 * 2   EarlyTermination
 * 3   InterruptEnabled
 * 6:4 Reserved
 * 7   Complete
 */
#define INTEL_SSGDMA_MEM_RESP_DEVICE_ERROR_MSK      (0x03U)
#define INTEL_SSGDMA_MEM_RESP_EARLY_TERMINATION_MSK (0x04U)
#define INTEL_SSGDMA_MEM_RESP_INTERRUPT_ENABLED_MSK (0x08U)

/******************************************************
 * H2D/D2H Streaming transfer responder status mask
 * 0   IsEOP
 * 1   EarlyTermination
 * 2   InterruptEnabled
 * 6:3 Reserved
 * 7   Complete
 */

#define INTEL_SSGDMA_ST_RESP_ISEOP_MSK             (0x01U)
#define INTEL_SSGDMA_ST_RESP_EARLY_TERMINATION_MSK (0x02U)
#define INTEL_SSGDMA_ST_RESP_INTERRUPT_ENABLED_MSK (0x04U)

/* descriptor size */
#define INTEL_SSGDMA_DESC_BYTES (32U)
#define INTEL_SSGDMA_NUM_DESC_PER_BLOCK (128U)

#define INTEL_SSGDMA_LENGTH_ALIGN   (4U)
#define INTEL_SSGDMA_ADDRESS_ALIGN  (32U)
#define INTEL_SSGDMA_ADDRESS_MASK   (0x1FU)

struct intel_ssgdma_hw_queue_link_desc {
	uint8_t  format_field;
	uint8_t  control;
	uint16_t descr_idx;
	uint32_t reserved1;
	uint32_t next_block_address_lower;
	uint32_t next_block_address_upper;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t reserved4;
	uint32_t reserved5;
};

struct intel_ssgdma_hw_mem_queue_data_desc {
	uint8_t format_field;
	uint8_t control;
	uint16_t descr_idx;
	uint32_t length;
	uint32_t host_address_lower;
	uint32_t host_address_upper;
	uint32_t device_address_lower;
	uint32_t device_address_upper;
	uint32_t host_interface_control;
	uint32_t device_interface_control;
};

struct intel_ssgdma_hw_h2d_st_queue_data_desc {
	uint8_t format_field;
	uint8_t control;
	uint16_t descr_idx;
	uint32_t length;
	uint32_t host_source_address_lower;
	uint32_t host_source_address_upper;
	uint16_t sideband_signal;
	uint16_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t host_interface_control;
};

struct intel_ssgdma_hw_d2h_st_queue_data_desc {
	uint8_t format_field;
	uint8_t control;
	uint16_t descr_idx;
	uint32_t length;
	uint32_t host_dest_address_lower;
	uint32_t host_dest_address_upper;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t host_interface_control;
};

struct intel_ssgdma_hw_responder_desc {
	uint8_t format_field;
	uint8_t reserved1;
	uint16_t descr_idx;
	uint32_t length;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t reserved4;
	uint32_t reserved5;
	uint32_t reserved6;
	uint16_t status;
	uint16_t reserved7;
};

struct intel_ssgdma_hw_h2d_st_ptp_responder_desc {
	uint8_t format_field;
	uint8_t reserved1;
	uint16_t descr_idx;
	uint32_t length;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t timestamp1;
	uint32_t timestamp2;
	uint32_t timestamp3;
	uint16_t status;
	uint16_t reserved4;
};

struct intel_ssgdma_hw_d2h_st_ptp_responder_desc {
	uint8_t format_field;
	uint8_t reserved1;
	uint16_t descr_idx;
	uint32_t length;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t timestamp1;
	uint32_t timestamp2;
	uint32_t timestamp3;
	uint16_t status;
	uint16_t reserved4;
};

union intel_ssgdma_hw_queue_desc {
	struct intel_ssgdma_hw_queue_link_desc queue_link_desc;
	struct intel_ssgdma_hw_mem_queue_data_desc mem_queue_data_desc;
	struct intel_ssgdma_hw_h2d_st_queue_data_desc h2d_st_queue_data_desc;
	struct intel_ssgdma_hw_d2h_st_queue_data_desc d2h_st_queue_data_desc;
	struct intel_ssgdma_hw_responder_desc resp_desc;
	struct intel_ssgdma_hw_h2d_st_ptp_responder_desc h2d_st_ptp_resp_desc;
	struct intel_ssgdma_hw_d2h_st_ptp_responder_desc d2h_st_ptp_resp_desc;
};

#endif /* ZEPHYR_DRIVERS_DMA_INTEL_SSGDMA_H_ */
