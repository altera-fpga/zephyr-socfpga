/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARM_SMMU_V3_H
#define _ARM_SMMU_V3_H

#include <zephyr/sys/util.h>
#include <zephyr/arch/arm64/arm_mmu.h>

/* SMMU Register Offsets and Masks */

#define ARM_SMMU_IDR0                           0x0
#define ARM_SMMU_IDR0_ST_LVL_MASK               GENMASK(28, 27)
#define ARM_SMMU_IDR0_ST_LVL_2LVL               1
#define ARM_SMMU_IDR0_STALL_MODEL_MASK          GENMASK(25, 24)
#define ARM_SMMU_IDR0_STALL_MODEL_STALL         0
#define ARM_SMMU_IDR0_STALL_MODEL_FORCE         2
#define ARM_SMMU_IDR0_TTENDIAN_MASK             GENMASK(22, 21)
#define ARM_SMMU_IDR0_TTENDIAN_MIXED            0
#define ARM_SMMU_IDR0_TTENDIAN_LE               2
#define ARM_SMMU_IDR0_TTENDIAN_BE               3
#define ARM_SMMU_IDR0_CD2L                      BIT(19)
#define ARM_SMMU_IDR0_VMID16                    BIT(18)
#define ARM_SMMU_IDR0_PRI                       BIT(16)
#define ARM_SMMU_IDR0_SEV                       BIT(14)
#define ARM_SMMU_IDR0_MSI                       BIT(13)
#define ARM_SMMU_IDR0_ASID16                    BIT(12)
#define ARM_SMMU_IDR0_ATS                       BIT(10)
#define ARM_SMMU_IDR0_HYP                       BIT(9)
#define ARM_SMMU_IDR0_COHACC                    BIT(4)
#define ARM_SMMU_IDR0_TTF_MASK                  GENMASK(3, 2)
#define ARM_SMMU_IDR0_TTF_AARCH64               2
#define ARM_SMMU_IDR0_TTF_AARCH32_64            3
#define ARM_SMMU_IDR0_S1P                       BIT(1)
#define ARM_SMMU_IDR0_S2P                       BIT(0)

#define ARM_SMMU_IDR1                           0x4
#define ARM_SMMU_IDR1_TABLES_PRESET             BIT(30)
#define ARM_SMMU_IDR1_QUEUES_PRESET             BIT(29)
#define ARM_SMMU_IDR1_REL                       BIT(28)
#define ARM_SMMU_IDR1_CMDQS_MASK                GENMASK(25, 21)
#define ARM_SMMU_IDR1_EVTQS_MASK                GENMASK(20, 16)
#define ARM_SMMU_IDR1_PRIQS_MASK                GENMASK(15, 11)
#define ARM_SMMU_IDR1_SSIDSIZE_MASK             GENMASK(10, 6)
#define ARM_SMMU_IDR1_SIDSIZE_MASK              GENMASK(5, 0)

#define ARM_SMMU_IDR3                           0xc
#define ARM_SMMU_IDR3_RIL                       BIT(10)

#define ARM_SMMU_IDR5                           0x14
#define ARM_SMMU_IDR5_STALL_MAX_MASK            GENMASK(31, 16)
#define ARM_SMMU_IDR5_GRAN64K                   BIT(6)
#define ARM_SMMU_IDR5_GRAN16K                   BIT(5)
#define ARM_SMMU_IDR5_GRAN4K                    BIT(4)
#define ARM_SMMU_IDR5_OAS_MASK                  GENMASK(2, 0)
#define ARM_SMMU_IDR5_OAS_32_BIT                0
#define ARM_SMMU_IDR5_OAS_36_BIT                1
#define ARM_SMMU_IDR5_OAS_40_BIT                2
#define ARM_SMMU_IDR5_OAS_42_BIT                3
#define ARM_SMMU_IDR5_OAS_44_BIT                4
#define ARM_SMMU_IDR5_OAS_48_BIT                5
#define ARM_SMMU_IDR5_OAS_52_BIT                6
#define ARM_SMMU_IDR5_VAX_MASK                  GENMASK(11, 10)
#define ARM_SMMU_IDR5_VAX_52_BIT                1

#define ARM_SMMU_CR0                            0x20
#define ARM_SMMU_CR0_ATSCHK                     BIT(4)
#define ARM_SMMU_CR0_CMDQEN                     BIT(3)
#define ARM_SMMU_CR0_EVTQEN                     BIT(2)
#define ARM_SMMU_CR0_PRIQEN                     BIT(1)
#define ARM_SMMU_CR0_SMMUEN                     BIT(0)

#define ARM_SMMU_CR0ACK                         0x24

#define ARM_SMMU_CR1                            0x28
#define ARM_SMMU_CR1_TABLE_SH_MASK              GENMASK(11, 10)
#define ARM_SMMU_CR1_TABLE_OC_MASK              GENMASK(9, 8)
#define ARM_SMMU_CR1_TABLE_IC_MASK              GENMASK(7, 6)
#define ARM_SMMU_CR1_QUEUE_SH_MASK              GENMASK(5, 4)
#define ARM_SMMU_CR1_QUEUE_OC_MASK              GENMASK(3, 2)
#define ARM_SMMU_CR1_QUEUE_IC_MASK              GENMASK(1, 0)
#define ARM_SMMU_CR1_CACHE_NC                   0
#define ARM_SMMU_CR1_CACHE_WB                   1
#define ARM_SMMU_CR1_CACHE_WT                   2

#define ARM_SMMU_CR2                            0x2c
#define ARM_SMMU_CR2_PTM                        BIT(2)
#define ARM_SMMU_CR2_RECINVSID                  BIT(1)
#define ARM_SMMU_CR2_E2H                        BIT(0)

#define ARM_SMMU_GBPA                           0x44
#define ARM_SMMU_GBPA_UPDATE                    BIT(31)
#define ARM_SMMU_GBPA_ABORT                     BIT(20)

#define ARM_SMMU_IRQ_CTRL                       0x50
#define ARM_SMMU_IRQ_CTRL_EVTQ_IRQEN            BIT(2)
#define ARM_SMMU_IRQ_CTRL_PRIQ_IRQEN            BIT(1)
#define ARM_SMMU_IRQ_CTRL_GERROR_IRQEN          BIT(0)

#define ARM_SMMU_IRQ_CTRLACK                    0x54

#define ARM_SMMU_GERROR                         0x60
#define ARM_SMMU_GERROR_SFM_ERR                 BIT(8)
#define ARM_SMMU_GERROR_MSI_GERROR_ABT_ERR      BIT(7)
#define ARM_SMMU_GERROR_MSI_PRIQ_ABT_ERR        BIT(6)
#define ARM_SMMU_GERROR_MSI_EVTQ_ABT_ERR        BIT(5)
#define ARM_SMMU_GERROR_MSI_CMDQ_ABT_ERR        BIT(4)
#define ARM_SMMU_GERROR_PRIQ_ABT_ERR            BIT(3)
#define ARM_SMMU_GERROR_EVTQ_ABT_ERR            BIT(2)
#define ARM_SMMU_GERROR_CMDQ_ERR                BIT(0)
#define ARM_SMMU_GERROR_ERR_MASK                0x1fd

#define ARM_SMMU_GERRORN                        0x64

#define ARM_SMMU_GERROR_IRQ_CFG0                0x68
#define ARM_SMMU_GERROR_IRQ_CFG1                0x70
#define ARM_SMMU_GERROR_IRQ_CFG2                0x74

#define ARM_SMMU_STRTAB_BASE                    0x80
#define ARM_SMMU_STRTAB_BASE_RA                 BIT(62)
#define ARM_SMMU_STRTAB_BASE_ADDR_MASK          GENMASK(51, 6)

#define ARM_SMMU_STRTAB_BASE_CFG                0x88
#define ARM_SMMU_STRTAB_BASE_CFG_FMT_MASK       GENMASK(17, 16)
#define ARM_SMMU_STRTAB_BASE_CFG_FMT_LINEAR     0
#define ARM_SMMU_STRTAB_BASE_CFG_FMT_2LVL       1
#define ARM_SMMU_STRTAB_BASE_CFG_SPLIT_MASK     GENMASK(10, 6)
#define ARM_SMMU_STRTAB_BASE_CFG_LOG2SIZE_MASK  GENMASK(5, 0)

#define ARM_SMMU_CMDQ_BASE                      0x90
#define ARM_SMMU_CMDQ_PROD                      0x98
#define ARM_SMMU_CMDQ_CONS                      0x9c

#define ARM_SMMU_EVTQ_BASE                      0xa0
#define ARM_SMMU_EVTQ_PROD                      0xa8
#define ARM_SMMU_EVTQ_CONS                      0xac
#define ARM_SMMU_EVTQ_IRQ_CFG0                  0xb0
#define ARM_SMMU_EVTQ_IRQ_CFG1                  0xb8
#define ARM_SMMU_EVTQ_IRQ_CFG2                  0xbc

#define ARM_SMMU_PRIQ_BASE                      0xc0
#define ARM_SMMU_PRIQ_PROD                      0xc8
#define ARM_SMMU_PRIQ_CONS                      0xcc
#define ARM_SMMU_PRIQ_IRQ_CFG0                  0xd0
#define ARM_SMMU_PRIQ_IRQ_CFG1                  0xd8
#define ARM_SMMU_PRIQ_IRQ_CFG2                  0xdc

#define ARM_SMMU_REG_SZ                         0xe00

/* Common MSI config fields */
#define ARM_SMMU_MSI_CFG0_ADDR_MASK             GENMASK(51, 2)
#define ARM_SMMU_MSI_CFG2_SH_MASK               GENMASK(5, 4)
#define ARM_SMMU_MSI_CFG2_MEMATTR_MASK          GENMASK(3, 0)

/* Common memory attribute values */
#define ARM_SMMU_SH_NSH                         0
#define ARM_SMMU_SH_OSH                         2
#define ARM_SMMU_SH_ISH                         3
#define ARM_SMMU_MEMATTR_DEVICE_nGnRE           0x1
#define ARM_SMMU_MEMATTR_OIWB                   0xf

/*
 * Stream table.
 *
 * Linear: Enough to cover 1 << IDR1.SIDSIZE entries
 * 2lvl: 128k L1 entries,
 *       256 lazy entries per table (each table covers a PCI bus)
 */
#define ARM_SMMU_STRTAB_L1_SZ_SHIFT             20
#define ARM_SMMU_STRTAB_SPLIT                   8

#define ARM_SMMU_STRTAB_L1_DESC_DWORDS          1
#define ARM_SMMU_STRTAB_L1_DESC_SPAN_MASK       GENMASK(4, 0)
#define ARM_SMMU_STRTAB_L1_DESC_L2PTR_MASK      GENMASK(51, 6)

#define ARM_SMMU_STRTAB_STE_SIZE_SHIFT          6
#define ARM_SMMU_STRTAB_STE_SIZE                BIT(ARM_SMMU_STRTAB_STE_SIZE_SHIFT)
#define ARM_SMMU_STRTAB_STE_DWORDS              (ARM_SMMU_STRTAB_STE_SIZE >> 3)

#define ARM_SMMU_STRTAB_STE_0_V                 BIT(0)
#define ARM_SMMU_STRTAB_STE_0_CFG_MASK          GENMASK(3, 1)
#define ARM_SMMU_STRTAB_STE_0_CFG_ABORT         0
#define ARM_SMMU_STRTAB_STE_0_CFG_BYPASS        4
#define ARM_SMMU_STRTAB_STE_0_CFG_S1_TRANS      5
#define ARM_SMMU_STRTAB_STE_0_CFG_S2_TRANS      6
#define ARM_SMMU_STRTAB_STE_0_CFG_S1S2_TRANS    7

#define ARM_SMMU_STRTAB_STE_0_S1FMT_MASK        GENMASK(5, 4)
#define ARM_SMMU_STRTAB_STE_0_S1FMT_LINEAR      0
#define ARM_SMMU_STRTAB_STE_0_S1FMT_64K_L2      2
#define ARM_SMMU_STRTAB_STE_0_S1CTXPTR_MASK     GENMASK(51, 6)
#define ARM_SMMU_STRTAB_STE_0_S1CDMAX_MASK      GENMASK(63, 59)

#define ARM_SMMU_STRTAB_STE_1_S1DSS_MASK        GENMASK(1, 0)
#define ARM_SMMU_STRTAB_STE_1_S1DSS_TERMINATE   0x0
#define ARM_SMMU_STRTAB_STE_1_S1DSS_BYPASS      0x1
#define ARM_SMMU_STRTAB_STE_1_S1DSS_SSID0       0x2

#define ARM_SMMU_STRTAB_STE_1_S1C_CACHE_NC      0UL
#define ARM_SMMU_STRTAB_STE_1_S1C_CACHE_WBRA    1UL
#define ARM_SMMU_STRTAB_STE_1_S1C_CACHE_WT      2UL
#define ARM_SMMU_STRTAB_STE_1_S1C_CACHE_WB      3UL
#define ARM_SMMU_STRTAB_STE_1_S1CIR_MASK        GENMASK(3, 2)
#define ARM_SMMU_STRTAB_STE_1_S1COR_MASK        GENMASK(5, 4)
#define ARM_SMMU_STRTAB_STE_1_S1CSH_MASK        GENMASK(7, 6)

#define ARM_SMMU_STRTAB_STE_1_S1STALLD          BIT(27)

#define ARM_SMMU_STRTAB_STE_1_EATS_MASK         GENMASK(29, 28)
#define ARM_SMMU_STRTAB_STE_1_EATS_ABT          0UL
#define ARM_SMMU_STRTAB_STE_1_EATS_TRANS        1UL
#define ARM_SMMU_STRTAB_STE_1_EATS_S1CHK        2UL

#define ARM_SMMU_STRTAB_STE_1_STRW_MASK         GENMASK(31, 30)
#define ARM_SMMU_STRTAB_STE_1_STRW_NSEL1        0UL
#define ARM_SMMU_STRTAB_STE_1_STRW_EL2          2UL

#define ARM_SMMU_STRTAB_STE_1_SHCFG_MASK        GENMASK(45, 44)
#define ARM_SMMU_STRTAB_STE_1_SHCFG_INCOMING    1UL

#define ARM_SMMU_STRTAB_STE_2_S2VMID_MASK       GENMASK64(15, 0)
#define ARM_SMMU_STRTAB_STE_2_VTCR_MASK         GENMASK64(50, 32)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2T0SZ_MASK  GENMASK64(37, 32)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2SL0_MASK   GENMASK64(39, 38)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2IR0_MASK   GENMASK64(41, 40)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2OR0_MASK   GENMASK64(43, 42)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2SH0_MASK   GENMASK64(45, 44)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2TG_MASK    GENMASK64(47, 46)
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_MASK    GENMASK64(50, 48)
#define ARM_SMMU_STRTAB_STE_2_S2AA64            BIT64(51)
#define ARM_SMMU_STRTAB_STE_2_S2ENDI            BIT64(52)
#define ARM_SMMU_STRTAB_STE_2_S2AFFD            BIT64(53)
#define ARM_SMMU_STRTAB_STE_2_S2PTW             BIT64(54)
#define ARM_SMMU_STRTAB_STE_2_S2HTTU_MASK       GENMASK64(56, 55)
#define ARM_SMMU_STRTAB_STE_2_S2R               BIT64(58)

/**
 * stage 2: stream table entry VMID
 */
#define ARM_SMMU_STRTAB_STE_2_VMID              0U

/**
 * stage 2: stream table entry granule size
 */

#define ARM_SMMU_STRTAB_STE_2_S2TG_4KB          0x0
#define ARM_SMMU_STRTAB_STE_2_S2TG_64KB         0x1
#define ARM_SMMU_STRTAB_STE_2_S2TG_16KB         0x2

/**
 * stage 2: stream table physical address size
 */
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_32BIT   0U
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_36BIT   1U
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_40BIT   2U
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_42BIT   3U
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_44BIT   4U
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_48BIT   5U
#define ARM_SMMU_STRTAB_STE_2_VTCR_S2PS_52BIT   6U

/* stage 2 physical address size */
#define ARM_SMMU_S2PS_SIZE_32BIT                32U
#define ARM_SMMU_S2PS_SIZE_36BIT                36U
#define ARM_SMMU_S2PS_SIZE_40BIT                40U
#define ARM_SMMU_S2PS_SIZE_42BIT                42U
#define ARM_SMMU_S2PS_SIZE_44BIT                44U
#define ARM_SMMU_S2PS_SIZE_48BIT                48U
#define ARM_SMMU_S2PS_SIZE_52BIT                52U

#define ARM_SMMU_S2PS_SIZE                      ARM_SMMU_S2PS_SIZE_40BIT
#define ARM_SMMU_S2T0SZ                         (64U - ARM_SMMU_S2PS_SIZE)

/*
 * stage 2: stream table entry HTTU  access/dirty flags
 */
#define ARM_SMMU_STRTAB_STE_2_S2HTTU_DISABLE    0
#define ARM_SMMU_STRTAB_STE_2_S2HTTU_AF_EN      2
#define ARM_SMMU_STRTAB_STE_2_S2HTTU_AF_DIRTY   3

#define ARM_SMMU_STRTAB_STE_3_S2TTB_MASK        GENMASK64(51, 4)

/**
 * stage 2: inner region cache-ability
 */

/* inner region: non-cacheable */
#define ARM_SMMU_STRTAB_STE_2_S2IR0_NC          0
/* inner region: Write-back Cacheable, Read-Allocate, Write-Allocate */
#define ARM_SMMU_STRTAB_STE_2_S2IR0_WBRAWA      1
/* inner region: write-through cacheable, Read-Allocate */
#define ARM_SMMU_STRTAB_STE_2_S2IR0_WTRA        2
/* inner region: Write-back cacheable, Read-Allocate, no Write-Allocate */
#define ARM_SMMU_STRTAB_STE_2_S2IR0_WBRAnWA      3

/**
 * stage 2: outer region cache-ability
 */

/* inner region: non-cacheable */
#define ARM_SMMU_STRTAB_STE_2_S2OR0_NC          0
/* inner region: Write-back Cacheable, Read-Allocate, Write-Allocate */
#define ARM_SMMU_STRTAB_STE_2_S2OR0_WBRAWA      1
/* inner region: write-through cacheable, Read-Allocate */
#define ARM_SMMU_STRTAB_STE_2_S2OR0_WTRA        2
/* inner region: Write-back cacheable, Read-Allocate, no Write-Allocate */
#define ARM_SMMU_STRTAB_STE_2_S2OR0_WBRAnWA      3


/**
 * stage 2: shareability
 */

/* non-shareable */
#define ARM_SMMU_STRTAB_STE_2_SH0_NS            0
/* reserved: behaves as non-shareable */
#define ARM_SMMU_STRTAB_STE_2_SH0_RSVD          1
/* outer shareable */
#define ARM_SMMU_STRTAB_STE_2_SH0_OS            2
/* inner shareable */
#define ARM_SMMU_STRTAB_STE_2_SH0_IS            3

/*
 * Context descriptors.
 *
 * Linear: when less than 1024 SSIDs are supported
 * 2lvl: at most 1024 L1 entries,
 *       1024 lazy entries per table.
 */
#define ARM_SMMU_CTXDESC_SPLIT                  10
#define ARM_SMMU_CTXDESC_L2_ENTRIES             BIT(ARM_SMMU_CTXDESC_SPLIT)

#define ARM_SMMU_CTXDESC_L1_DESC_DWORDS         1
#define ARM_SMMU_CTXDESC_L1_DESC_V              BIT(0)
#define ARM_SMMU_CTXDESC_L1_DESC_L2PTR_MASK     GENMASK(51, 12)

#define ARM_SMMU_CTXDESC_CD_SIZE_SHIFT          6
#define ARM_SMMU_CTXDESC_CD_SIZE                BIT(ARM_SMMU_CTXDESC_CD_SIZE_SHIFT)
#define ARM_SMMU_CTXDESC_CD_DWORDS              (ARM_SMMU_CTXDESC_CD_SIZE >> 3)

#define ARM_SMMU_CTXDESC_CD_0_TCR_T0SZ_MASK     GENMASK(5, 0)
#define ARM_SMMU_CTXDESC_CD_0_TCR_TG0_MASK      GENMASK(7, 6)
#define ARM_SMMU_CTXDESC_CD_0_TCR_IRGN0_MASK    GENMASK(9, 8)
#define ARM_SMMU_CTXDESC_CD_0_TCR_ORGN0_MASK    GENMASK(11, 10)
#define ARM_SMMU_CTXDESC_CD_0_TCR_SH0_MASK      GENMASK(13, 12)
#define ARM_SMMU_CTXDESC_CD_0_TCR_EPD0          BIT(14)
#define ARM_SMMU_CTXDESC_CD_0_TCR_EPD1          BIT(30)

#define ARM_SMMU_CTXDESC_CD_0_ENDI              BIT(15)
#define ARM_SMMU_CTXDESC_CD_0_V                 BIT(31)

#define ARM_SMMU_CTXDESC_CD_0_TCR_IPS           GENMASK(34, 32)
#define ARM_SMMU_CTXDESC_CD_0_TCR_TBI0          BIT(38)

#define ARM_SMMU_CTXDESC_CD_0_AA64              BIT(41)
#define ARM_SMMU_CTXDESC_CD_0_S                 BIT(44)
#define ARM_SMMU_CTXDESC_CD_0_R                 BIT(45)
#define ARM_SMMU_CTXDESC_CD_0_A                 BIT(46)
#define ARM_SMMU_CTXDESC_CD_0_ASET              BIT(47)
#define ARM_SMMU_CTXDESC_CD_0_ASID_MASK         GENMASK(63, 48)

#define ARM_SMMU_CTXDESC_CD_1_TTB0_MASK         GENMASK(51, 4)

/*
 * When the SMMU only supports linear context descriptor tables, pick a
 * reasonable size limit (64kB).
 */
#define ARM_SMMU_CTXDESC_LINEAR_CDMAX           ilog2(SZ_64K / (ARM_SMMU_CTXDESC_CD_DWORDS << 3))

/* Queue maintenace */
#define ARM_SMMU_Q_IDX(x, qp)                    ((x) & (BIT(qp->qbits) - 1))
#define ARM_SMMU_Q_WRP(x, qp)                    ((x) & BIT(qp->qbits))

#define ARM_SMMU_Q_OVERFLOW_FLAG                BIT(31)
#define ARM_SMMU_Q_OVF(x)                       ((x) & ARM_SMMU_Q_OVERFLOW_FLAG)

#define ARM_SMMU_Q_BASE_RWA                     BIT(62)
#define ARM_SMMU_Q_BASE_ADDR_MASK               GENMASK(51, 5)
#define ARM_SMMU_Q_BASE_LOG2SIZE_MASK           GENMASK(4, 0)

#define ARM_SMMU_Q_MIN_SZ_SHIFT                 (12) /* PAGE_SIZE_SHIFT */

/* Command queue */
#define ARM_SMMU_CMDQ_ENT_SIZE_SHIFT            4
#define ARM_SMMU_CMDQ_ENT_SIZE                  BIT(ARM_SMMU_CMDQ_ENT_SIZE_SHIFT)
#define ARM_SMMU_CMDQ_ENT_DWORDS                (ARM_SMMU_CMDQ_ENT_SIZE >> 3)

#define ARM_SMMU_CMDQ_CONS_ERR_MASK             GENMASK(30, 24)
#define ARM_SMMU_CMDQ_ERR_CERROR_NONE_IDX       0
#define ARM_SMMU_CMDQ_ERR_CERROR_ILL_IDX        1
#define ARM_SMMU_CMDQ_ERR_CERROR_ABT_IDX        2
#define ARM_SMMU_CMDQ_ERR_CERROR_ATC_INV_IDX    3

#define ARM_SMMU_CMDQ_PROD_OWNED_FLAG           ARM_SMMU_Q_OVERFLOW_FLAG

/*
 * This is used to size the command queue and therefore must be at least
 * BITS_PER_LONG so that the valid_map works correctly (it relies on the
 * total number of queue entries being a multiple of BITS_PER_LONG).
 */
#define ARM_SMMU_CMDQ_BATCH_ENTRIES             BITS_PER_LONG

#define ARM_SMMU_CMDQ_0_OP_MASK                 GENMASK(7, 0)
#define ARM_SMMU_CMDQ_0_SSV                     BIT(11)

#define ARM_SMMU_CMDQ_PREFETCH_0_SID_MASK       GENMASK(63, 32)
#define ARM_SMMU_CMDQ_PREFETCH_1_SIZE_MASK      GENMASK(4, 0)
#define ARM_SMMU_CMDQ_PREFETCH_1_ADDR_MASK      GENMASK(63, 12)

#define ARM_SMMU_CMDQ_CFGI_0_SSID_MASK          GENMASK(31, 12)
#define ARM_SMMU_CMDQ_CFGI_0_SID_MASK           GENMASK(63, 32)
#define ARM_SMMU_CMDQ_CFGI_1_LEAF               BIT(0)
#define ARM_SMMU_CMDQ_CFGI_1_RANGE_MASK         GENMASK(4, 0)

#define ARM_SMMU_CMDQ_TLBI_0_NUM_MASK           GENMASK(16, 12)
#define ARM_SMMU_CMDQ_TLBI_RANGE_NUM_MAX        31
#define ARM_SMMU_CMDQ_TLBI_0_SCALE_MASK         GENMASK(24, 20)
#define ARM_SMMU_CMDQ_TLBI_0_VMID_MASK          GENMASK(47, 32)
#define ARM_SMMU_CMDQ_TLBI_0_ASID_MASK          GENMASK(63, 48)
#define ARM_SMMU_CMDQ_TLBI_1_LEAF               BIT(0)
#define ARM_SMMU_CMDQ_TLBI_1_TTL_MASK           GENMASK(9, 8)
#define ARM_SMMU_CMDQ_TLBI_1_TG_MASK            GENMASK(11, 10)
#define ARM_SMMU_CMDQ_TLBI_1_VA_MASK            GENMASK(63, 12)
#define ARM_SMMU_CMDQ_TLBI_1_IPA_MASK           GENMASK(51, 12)

#define ARM_SMMU_CMDQ_ATC_0_SSID_MASK           GENMASK(31, 12)
#define ARM_SMMU_CMDQ_ATC_0_SID_MASK            GENMASK(63, 32)
#define ARM_SMMU_CMDQ_ATC_0_GLOBAL              BIT(9)
#define ARM_SMMU_CMDQ_ATC_1_SIZE_MASK           GENMASK(5, 0)
#define ARM_SMMU_CMDQ_ATC_1_ADDR_MASK           GENMASK(63, 12)

#define ARM_SMMU_CMDQ_PRI_0_SSID_MASK           GENMASK(31, 12)
#define ARM_SMMU_CMDQ_PRI_0_SID_MASK            GENMASK(63, 32)
#define ARM_SMMU_CMDQ_PRI_1_GRPID_MASK          GENMASK(8, 0)
#define ARM_SMMU_CMDQ_PRI_1_RESP_MASK           GENMASK(13, 12)

#define ARM_SMMU_CMDQ_RESUME_0_RESP_TERM        0UL
#define ARM_SMMU_CMDQ_RESUME_0_RESP_RETRY       1UL
#define ARM_SMMU_CMDQ_RESUME_0_RESP_ABORT       2UL
#define ARM_SMMU_CMDQ_RESUME_0_RESP_MASK        GENMASK(13, 12)
#define ARM_SMMU_CMDQ_RESUME_0_SID_MASK         GENMASK(63, 32)
#define ARM_SMMU_CMDQ_RESUME_1_STAG_MASK        GENMASK(15, 0)

#define ARM_SMMU_CMDQ_SYNC_0_CS_MASK            GENMASK(13, 12)
#define ARM_SMMU_CMDQ_SYNC_0_CS_NONE            0
#define ARM_SMMU_CMDQ_SYNC_0_CS_IRQ             1
#define ARM_SMMU_CMDQ_SYNC_0_CS_SEV             2
#define ARM_SMMU_CMDQ_SYNC_0_MSH_MASK           GENMASK(23, 22)
#define ARM_SMMU_CMDQ_SYNC_0_MSIATTR_MASK       GENMASK(27, 24)
#define ARM_SMMU_CMDQ_SYNC_0_MSIDATA_MASK       GENMASK(63, 32)
#define ARM_SMMU_CMDQ_SYNC_1_MSIADDR_MASK       GENMASK(51, 2)

/* Event queue */
#define ARM_SMMU_EVTQ_ENT_SIZE_SHIFT            5
#define ARM_SMMU_EVTQ_ENT_SIZE                  BIT(ARM_SMMU_EVTQ_ENT_SIZE_SHIFT)
#define ARM_SMMU_EVTQ_ENT_DWORDS                (ARM_SMMU_EVTQ_ENT_SIZE >> 3)

#define ARM_SMMU_EVTQ_0_ID_MASK                 GENMASK(7, 0)

#define ARM_SMMU_EVT_ID_BAD_STREAMID            0x2
#define ARM_SMMU_EVT_ID_STE_FETCH               0x3
#define ARM_SMMU_EVT_ID_BAD_STE                 0x4
#define ARM_SMMU_EVT_ID_TRANSLATION_FAULT       0x10
#define ARM_SMMU_EVT_ID_ADDR_SIZE_FAULT         0x11
#define ARM_SMMU_EVT_ID_ACCESS_FAULT            0x12
#define ARM_SMMU_EVT_ID_PERMISSION_FAULT        0x13

#define ARM_SMMU_EVTQ_0_SSV                     BIT(11)
#define ARM_SMMU_EVTQ_0_SSID_MASK               GENMASK(31, 12)
#define ARM_SMMU_EVTQ_0_SID_MASK                GENMASK(63, 32)
#define ARM_SMMU_EVTQ_1_STAG_MASK               GENMASK(15, 0)
#define ARM_SMMU_EVTQ_1_STALL                   BIT(31)
#define ARM_SMMU_EVTQ_1_PnU                     BIT(33)
#define ARM_SMMU_EVTQ_1_InD                     BIT(34)
#define ARM_SMMU_EVTQ_1_RnW                     BIT(35)
#define ARM_SMMU_EVTQ_1_S2                      BIT(39)
#define ARM_SMMU_EVTQ_1_CLASS_MASK              GENMASK(41, 40)
#define ARM_SMMU_EVTQ_1_TT_READ                 BIT(44)
#define ARM_SMMU_EVTQ_2_ADDR_MASK               GENMASK(63, 0)
#define ARM_SMMU_EVTQ_3_IPA_MASK                GENMASK(51, 0)
#define ARM_SMMU_EVTQ_3_STE_ADDR_FETCH_MASK     GENMASK(51, 0)

/* PRI queue */
#define ARM_SMMU_PRIQ_ENT_SIZE_SHIFT            4
#define ARM_SMMU_PRIQ_ENT_SIZE                  BIT(ARM_SMMU_PRIQ_ENT_SIZE_SHIFT)
#define ARM_SMMU_PRIQ_ENT_DWORDS                (ARM_SMMU_PRIQ_ENT_SIZE >> 3)


#define ARM_SMMU_PRIQ_0_SID_MASK                GENMASK(31, 0)
#define ARM_SMMU_PRIQ_0_SSID_MASK               GENMASK(51, 32)
#define ARM_SMMU_PRIQ_0_PERM_PRIV               BIT(58)
#define ARM_SMMU_PRIQ_0_PERM_EXEC               BIT(59)
#define ARM_SMMU_PRIQ_0_PERM_READ               BIT(60)
#define ARM_SMMU_PRIQ_0_PERM_WRITE              BIT(61)
#define ARM_SMMU_PRIQ_0_PRG_LAST                BIT(62)
#define ARM_SMMU_PRIQ_0_SSID_V                  BIT(63)

#define ARM_SMMU_PRIQ_1_PRG_IDX_MASK            GENMASK(8, 0)
#define ARM_SMMU_PRIQ_1_ADDR_MASK               GENMASK(63, 12)

/* High-level queue structures */
#define ARM_SMMU_ARM_SMMU_POLL_TIMEOUT_US       1000000 /* 1s! */
#define ARM_SMMU_ARM_SMMU_POLL_SPIN_COUNT       10

#define ARM_SMMU_MSI_IOVA_BASE                  0x8000000
#define ARM_SMMU_MSI_IOVA_LENGTH                0x100000

#define ARM_SMMU_PAGE_4KB                       (0x1000UL)
#define ARM_SMMU_BLOCK_64KB                     (0x10000UL)
#define ARM_SMMU_BLOCK_2MB                      (0x200000UL)
#define ARM_SMMU_BLOCK_512MB                    (0x20000000UL)
#define ARM_SMMU_BLOCK_1GB                      (0x40000000UL)
#define ARM_SMMU_BLOCK_512GB                    (0x8000000000UL)
#define ARM_SMMU_BLOCK_4TB                      (0x40000000000UL)

/*
 * 48-bit address with 4KB granule size:
 *
 * +------------+------------+------------+------------+-----------+
 * | VA [47:39] | VA [38:30] | VA [29:21] | VA [20:12] | VA [11:0] |
 * +---------------------------------------------------------------+
 * |     L0     |     L1     |     L2     |     L3     | block off |
 * +------------+------------+------------+------------+-----------+
 */

/* Only 4K granule is supported */
#define ARM_SMMU_PAGE_SIZE_SHIFT                12U

/* 48-bit VA address */
#define ARM_SMMU_VA_SIZE_SHIFT_MAX              48U

/* Maximum 3 XLAT table levels (L1 - L3) */
#define ARM_SMMU_XLAT_LAST_LEVEL                2U

/* Number of VA bits to assign to each table (9 bits) */
#define ARM_SMMU_Ln_XLAT_VA_SIZE_SHIFT          (ARM_SMMU_PAGE_SIZE_SHIFT - 3)

/* The VA shift of L3 depends on the granule size */
#define ARM_SMMU_L3_XLAT_VA_SIZE_SHIFT          ARM_SMMU_PAGE_SIZE_SHIFT

/* Starting bit in the VA address for each level */
#define ARM_SMMU_L2_XLAT_VA_SIZE_SHIFT	\
		(ARM_SMMU_L3_XLAT_VA_SIZE_SHIFT + ARM_SMMU_Ln_XLAT_VA_SIZE_SHIFT)
#define ARM_SMMU_L1_XLAT_VA_SIZE_SHIFT	\
		(ARM_SMMU_L2_XLAT_VA_SIZE_SHIFT + ARM_SMMU_Ln_XLAT_VA_SIZE_SHIFT)

#define ARM_SMMU_LEVEL_TO_VA_SIZE_SHIFT(level) \
	(ARM_SMMU_PAGE_SIZE_SHIFT + (ARM_SMMU_Ln_XLAT_VA_SIZE_SHIFT * \
	(ARM_SMMU_XLAT_LAST_LEVEL - (level))))

/* Number of entries for each table (512) */
#define ARM_SMMU_Ln_XLAT_NUM_ENTRIES             ((1U << ARM_SMMU_PAGE_SIZE_SHIFT) / 8U)

/* Virtual Address Index within a given translation table level */
#define ARM_SMMU_XLAT_TABLE_VA_IDX(va_addr, level) \
	((va_addr >> ARM_SMMU_LEVEL_TO_VA_SIZE_SHIFT(level)) & (ARM_SMMU_Ln_XLAT_NUM_ENTRIES - 1))

/*
 * Calculate the initial translation table level from CONFIG_ARM64_VA_BITS
 * For a 4 KB page size:
 *
 * (va_bits <= 21)	 - base level 3
 * (22 <= va_bits <= 30) - base level 2
 * (31 <= va_bits <= 39) - base level 1
 * (40 <= va_bits <= 48) - base level 0
 */
#define ARM_SMMU_GET_BASE_XLAT_LEVEL(va_bits) \
	 ((va_bits > ARM_SMMU_L0_XLAT_VA_SIZE_SHIFT) ? 0U \
	: (va_bits > ARM_SMMU_L1_XLAT_VA_SIZE_SHIFT) ? 1U \
	: (va_bits > ARM_SMMU_L2_XLAT_VA_SIZE_SHIFT) ? 2U : 3U)

/* Level for the base XLAT */
#define ARM_SMMU_BASE_XLAT_LEVEL                ARM_SMMU_GET_BASE_XLAT_LEVEL(CONFIG_ARM64_VA_BITS)

/*
 * #if (CONFIG_ARM64_PA_BITS == 48)
 * #define TCR_PS_BITS TCR_PS_BITS_256TB
 * #elif (CONFIG_ARM64_PA_BITS == 44)
 * #define TCR_PS_BITS TCR_PS_BITS_16TB
 * #elif (CONFIG_ARM64_PA_BITS == 42)
 * #define TCR_PS_BITS TCR_PS_BITS_4TB
 * #elif (CONFIG_ARM64_PA_BITS == 40)
 * #define TCR_PS_BITS TCR_PS_BITS_1TB
 * #elif (CONFIG_ARM64_PA_BITS == 36)
 * #define TCR_PS_BITS TCR_PS_BITS_64GB
 * #else
 * #define TCR_PS_BITS TCR_PS_BITS_4GB
 * #endif
 */

/* Descriptor entry type
 * LSb 11: Table Descriptor
 * LSb 01: Block Entry
 * LSb 10: Table Entry
 * LSb 00: Ignored
 */
#define ARM_SMMU_DESC_TYPE_MASK                 PTE_DESC_TYPE_MASK
#define ARM_SMMU_DESC_TYPE_BLOCK                PTE_BLOCK_DESC
#define ARM_SMMU_DESC_TYPE_TABLE                PTE_TABLE_DESC
#define ARM_SMMU_DESC_TYPE_PAGE                 PTE_PAGE_DESC
#define ARM_SMMU_DESC_TYPE_INVALID              PTE_INVALID_DESC

/*
 * Table descriptor with 4KB granule size for (levels 0, 1 and 2):
 *
 * +-------------+---------+-----------------------+--------+-------+
 * |   [63:52]   | [51-47] |        [47:12]        | [11:2] | [1:0] |
 * +-------------+---------+-----------------------+--------+-------+
 * | Upper Attrs |  RES0   | Next Level Table Addr | Ignore | Type  |
 * +-------------+---------+-----------------------+--------+-------+
 */

#define ARM_SMMU_DESC_Ln_TABLE_ADDR_MASK        GENMASK64(63, ARM_SMMU_PAGE_SIZE_SHIFT)

/* Table attributes and entries */
#define ARM_SMMU_DESC_TABLE_UPPER_DEFAULT       0ULL
#define ARM_SMMU_DESC_TABLE_UPPER_S             0ULL
#define ARM_SMMU_DESC_TABLE_UPPER_NS            BIT64(63)
#define ARM_SMMU_DESC_TABLE_UPPER_AP(ap)        ((uint64_t)((ap) & (0x3)) << 61)
#define ARM_SMMU_DESC_TABLE_UPPER_XN            BIT64(60)
#define ARM_SMMU_DESC_TABLE_UPPER_PXN           BIT64(59)

#define ARM_SMMU_Ln_TABLE_ENTRY(table, upper) \
	((uint64_t)(((upper) & ARM_SMMU_DESC_ATTRS_UPPER_MASK) | \
	((uint64_t)(table) & ARM_SMMU_DESC_Ln_TABLE_ADDR_MASK) | \
	ARM_SMMU_DESC_TYPE_TABLE))

/*
 * Block descriptor with 4KB granule size:
 *
 * Level 1 : n = 30
 * Level 2 : n = 21
 * Level 3 : n = 12
 *
 * +-------------+---------+---------------------------+-------+
 * |   [63:52]   | [51-47] |    [47:n]   |   [11:2]    | [1:0] |
 * +-------------+---------+-------------+-------------+-------+
 * | Upper Attrs |  RES0   | Output Addr | Lower Attrs | Type  |
 * +-------------+---------+-------------+-------------+-------+
 *
 */
#define ARM_SMMU_DESC_L1_BLOCK_ADDR_MASK        GENMASK64(63, ARM_SMMU_L1_XLAT_VA_SIZE_SHIFT)
#define ARM_SMMU_DESC_L2_BLOCK_ADDR_MASK        GENMASK64(63, ARM_SMMU_L2_XLAT_VA_SIZE_SHIFT)
#define ARM_SMMU_DESC_L3_BLOCK_ADDR_MASK        GENMASK64(63, ARM_SMMU_L3_XLAT_VA_SIZE_SHIFT)

/* Upper and lower attributes mask for page/block descriptor */
#define ARM_SMMU_DESC_ATTRS_UPPER_MASK          GENMASK64(63, 52)
#define ARM_SMMU_DESC_ATTRS_LOWER_MASK          GENMASK(11, 2)
#define ARM_SMMU_DESC_ATTRS_MASK	\
		(ARM_SMMU_DESC_ATTRS_UPPER_MASK | ARM_SMMU_DESC_ATTRS_LOWER_MASK)

/* Block attributes and entries */
#define ARM_SMMU_DESC_BLOCK_UPPER_DEFAULT       0ULL
#define ARM_SMMU_DESC_BLOCK_UPPER_UXN_XN        PTE_BLOCK_DESC_UXN
#define ARM_SMMU_DESC_BLOCK_UPPER_PXN           PTE_BLOCK_DESC_PXN
#define ARM_SMMU_DESC_BLOCK_UPPER_CONTIGUOUS    BIT64(52)

#define ARM_SMMU_S2_MEMORY_TYPE_ATTR			(3U << 4)
#define ARM_SMMU_S2_MEMORY_CACHE_ATTR			(3U << 2)
#define ARM_SMMU_DESC_BLOCK_LOWER_ATTR_INDX(x)  PTE_BLOCK_DESC_MEMTYPE(x)

#define ARM_SMMU_DESC_BLOCK_LOWER_MEMORY \
	(ARM_SMMU_DESC_BLOCK_LOWER_ATTR_INDX(MT_NORMAL) | \
	PTE_BLOCK_DESC_AF | (PTE_BLOCK_DESC_AP_EL_HIGHER | \
	PTE_BLOCK_DESC_AP_RW))

/*
 * AF[10] : Software managed
 * AP[7:6] : Read Only
 * MemAttr[5:4] : Normal memory with Outer Write-Back Cacheable
 * MemAttr[3:2] : Normal memory with Inner Write-Back Cacheable
 */
#define ARM_SMMU_S2_DESC_BLOCK_LOWER_MEMORY \
	(ARM_SMMU_S2_MEMORY_CACHE_ATTR | \
	ARM_SMMU_S2_MEMORY_TYPE_ATTR | \
	PTE_BLOCK_DESC_AF | (PTE_BLOCK_DESC_AP_ELx))

#define ARM_SMMU_DESC_BLOCK_LOWER_MEMORY_NONSECURE \
	(ARM_SMMU_DESC_BLOCK_LOWER_ATTR_INDX(MT_NORMAL) | \
	PTE_BLOCK_DESC_NS | PTE_BLOCK_DESC_AF)

#define ARM_SMMU_DESC_BLOCK_LOWER_DEVICE \
	(ARM_SMMU_DESC_BLOCK_LOWER_ATTR_INDX(MT_DEVICE_nGnRE) | PTE_BLOCK_DESC_AF)

#define ARM_SMMU_DESC_BLOCK_LOWER_STRONG \
	(ARM_SMMU_DESC_BLOCK_LOWER_ATTR_INDX(MT_DEVICE_nGnRnE) | PTE_BLOCK_DESC_AF)

/*
 * Shareability for non Memory does not apply. Those locations are automatically marked outer
 * shareable.
 */
#define ARM_SMMU_DESC_BLOCK_LOWER_MEMORY_SHARED_INNER \
	(ARM_SMMU_DESC_BLOCK_LOWER_MEMORY | PTE_BLOCK_DESC_INNER_SHARE)

#define ARM_SMMU_DESC_BLOCK_LOWER_MEMORY_SHARED_OUTER \
	(ARM_SMMU_DESC_BLOCK_LOWER_MEMORY | PTE_BLOCK_DESC_OUTER_SHARE)

#define ARM_SMMU_S2_DESC_BLOCK_LOWER_MEMORY_SHARED_OUTER \
	(ARM_SMMU_S2_DESC_BLOCK_LOWER_MEMORY | PTE_BLOCK_DESC_OUTER_SHARE)

#define ARM_SMMU_DESC_BLOCK_LOWER_MEMORY_SHARED_OUTER_NS \
	(ARM_SMMU_DESC_BLOCK_LOWER_MEMORY_NONSECURE | PTE_BLOCK_DESC_OUTER_SHARE)

/* Fault Entries
 * Emit the possible address so if the fault is transformed into block,
 * it would have a default address
 */
#define SMMU_L1_FAULT_ENTRY(addr) \
	((uint64_t)(((uint64_t)(addr) & ARM_SMMU_DESC_L1_BLOCK_ADDR_MASK) | 0))

/* Block Entries */
#define ARM_SMMU_L1_BLOCK_ENTRY(block, upper, lower) \
	((uint64_t)(((upper) & ARM_SMMU_DESC_ATTRS_UPPER_MASK) | \
	((uint64_t)(block) & ARM_SMMU_DESC_L1_BLOCK_ADDR_MASK) | \
	((lower) & ARM_SMMU_DESC_ATTRS_LOWER_MASK) | \
	ARM_SMMU_DESC_TYPE_BLOCK))

#define ARM_SMMU_L2_BLOCK_ENTRY(block, upper, lower) \
	((uint64_t)(((upper) & ARM_SMMU_DESC_ATTRS_UPPER_MASK) | \
	((uint64_t)(block) & ARM_SMMU_DESC_L2_BLOCK_ADDR_MASK) | \
	((lower) & ARM_SMMU_DESC_ATTRS_LOWER_MASK) | \
	ARM_SMMU_DESC_TYPE_BLOCK))

#define ARM_SMMU_L3_BLOCK_ENTRY(block, upper, lower) \
	((uint64_t)(((upper) & ARM_SMMU_DESC_ATTRS_UPPER_MASK) | \
	((uint64_t)(block) & ARM_SMMU_DESC_L3_BLOCK_ADDR_MASK) | \
	((lower) & ARM_SMMU_DESC_ATTRS_LOWER_MASK) | \
	ARM_SMMU_DESC_TYPE_BLOCK))

/* Command Queue Entries and fields*/
enum arm_smmu_v3_pri_resp {
	PRI_RESP_DENY = 0,
	PRI_RESP_FAIL = 1,
	PRI_RESP_SUCC = 2,
};

struct arm_smmu_v3_cmdq_fields {
	/* Common fields */
	uint8_t     opcode;
	bool        substream_valid;

	/* Command-specific fields */
	union {
		#define ARM_SMMU_CMDQ_OP_PREFETCH_CFG       0x1
		struct {
			uint32_t                sid;
		} prefetch;

		#define ARM_SMMU_CMDQ_OP_CFGI_STE           0x3
		#define ARM_SMMU_CMDQ_OP_CFGI_ALL           0x4
		#define ARM_SMMU_CMDQ_OP_CFGI_CD            0x5
		#define ARM_SMMU_CMDQ_OP_CFGI_CD_ALL        0x6
		struct {
			uint32_t                sid;
			uint32_t                ssid;
			union {
				bool                leaf;
				uint8_t             span;
			};
		} cfgi;

		#define ARM_SMMU_CMDQ_OP_TLBI_NH_ASID       0x11
		#define ARM_SMMU_CMDQ_OP_TLBI_NH_VA         0x12
		#define ARM_SMMU_CMDQ_OP_TLBI_EL2_ALL       0x20
		#define ARM_SMMU_CMDQ_OP_TLBI_EL2_ASID      0x21
		#define ARM_SMMU_CMDQ_OP_TLBI_EL2_VA        0x22
		#define ARM_SMMU_CMDQ_OP_TLBI_S12_VMALL     0x28
		#define ARM_SMMU_CMDQ_OP_TLBI_S2_IPA        0x2a
		#define ARM_SMMU_CMDQ_OP_TLBI_NSNH_ALL      0x30
		struct {
			uint8_t                 num;
			uint8_t                 scale;
			uint16_t                asid;
			uint16_t                vmid;
			bool                    leaf;
			uint8_t                 ttl;
			uint8_t                 tg;
			uint64_t                addr;
		} tlbi;

		#define ARM_SMMU_CMDQ_OP_ATC_INV            0x40
		#define ARM_SMMU_ATC_INV_SIZE_ALL           52
		struct {
			uint32_t                sid;
			uint32_t                ssid;
			uint64_t                addr;
			uint8_t                 size;
			bool                    global;
		} atc;

		#define ARM_SMMU_CMDQ_OP_PRI_RESP            0x41
		struct {
			uint32_t                sid;
			uint32_t                ssid;
			uint16_t                grpid;
			enum arm_smmu_v3_pri_resp  resp;
		} pri;

		#define ARM_SMMU_CMDQ_OP_RESUME              0x44
		struct {
			uint32_t                sid;
			uint16_t                stag;
			uint8_t                 resp;
		} resume;

		#define ARM_SMMU_CMDQ_OP_CMD_SYNC            0x46
		struct {
			uint64_t                msiaddr;
		} sync;
	};
};

struct arm_smmu_v3_cmdq_ent {
	/* DWORD_[0:1] = 16 Bytes */
	uint64_t field[ARM_SMMU_CMDQ_ENT_DWORDS];
};

/* Event Queue Table Entries */
struct arm_smmu_v3_eventq_ent {
	/* DWORD_[0:3] = 32 Bytes */
	uint64_t field[ARM_SMMU_EVTQ_ENT_DWORDS];
};

/* Stream Table Entries */
struct arm_smmu_v3_strtab_ent {
	/* DWORD_[0:7] = 64 Bytes */
	uint64_t field[ARM_SMMU_STRTAB_STE_DWORDS];
};

/* Context Descriptor Entries */
struct arm_smmu_v3_ctx_desc_ent {
	/* DWORD_[0:7] = 64 Bytes */
	uint64_t field[ARM_SMMU_CTXDESC_CD_DWORDS];
};

#endif /* _ARM_SMMU_V3_H */
