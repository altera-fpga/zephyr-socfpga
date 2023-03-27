/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FREEZE_CONTROLLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_FREEZE_CONTROLLER_H_

#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*fpga_api_freeze_region)(const struct device *dev);
typedef int (*fpga_api_unfreeze_region)(const struct device *dev);

__subsystem struct freeze_ctrl_driver {
	fpga_api_freeze_region freeze;
	fpga_api_unfreeze_region unfreeze;
};

/**
 * @file
 *
 * @brief API for FPGA freeze controller where it is used to freeze the FPGA partially
 * configurable regions.
 */

/**
 * @brief FPGA freeze controller API will freeze the specific FPGA region
 *
 * @param dev Freeze controller device
 * @return int 0 on success, negative errno code on fail
 */
static inline int fpga_freeze_region(const struct device *dev)
{
	if (dev == NULL) {
		return -ENODEV;
	}

	const struct freeze_ctrl_driver *api = (const struct freeze_ctrl_driver *)dev->api;

	return api->freeze(dev);
}

/**
 * @brief FPGA freeze controller API will unfreeze the specific FPGA region
 *
 * @param dev Freeze controller device
 * @return int 0 on success, negative errno code on fail
 */
static inline int fpga_unfreeze_region(const struct device *dev)
{
	if (dev == NULL) {
		return -ENODEV;
	}

	const struct freeze_ctrl_driver *api = (const struct freeze_ctrl_driver *)dev->api;

	return api->unfreeze(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FREEZE_CONTROLLER_H_ */
