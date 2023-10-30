# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ARM_SMMU_V3_DRIVER         arm_smmu_v3.c)
