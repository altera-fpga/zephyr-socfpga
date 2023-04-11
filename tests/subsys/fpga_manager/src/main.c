/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/device.h>
#include <zephyr/fpga_manager/fpga_manager.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Checking the FPGA configuration status */
ZTEST(fpga_manager_stack, test_fpga_status)
{
	int ret = 0;
	void *config_status_buf = k_malloc(FPGA_RECONFIG_STATUS_BUF_SIZE);

	if (!config_status_buf) {
		zassert_equal(ret, -EFAULT, "Failed to get the status");
	}

	/* Check fpga configuration status */
	ret = fpga_get_status(config_status_buf);
	zassert_equal(ret, 0, "Failed to get the status");
	k_free(config_status_buf);

	/* Check fpga configuration with Null buffer */
	ret = fpga_get_status(NULL);
	zassert_equal(ret, -ENOMEM, "Invalid Memory Address");
}

/* Verifying the FPGA configuration*/
ZTEST(fpga_manager_stack, test_fpga_load_file)
{
	int ret;
	char *freeze_ctrl = NULL;

	/* Check invalid configuration type */
	ret = fpga_load_file("file", -1, freeze_ctrl);
	zassert_equal(ret, -ENOTSUP, "FPGA configuration not supported");

	/* Check null freeze controller driver node */
	ret = fpga_load_file("file", 1, freeze_ctrl);
	zassert_equal(ret, -ENODEV, "No such device found");

	/* Check invalid file with partial configuration type */
	ret = fpga_load_file("file", 1, "freeze_controller0@f9000450");
	zassert_equal(ret, -ENOENT, "No such file or directory");

	/* Check invalid file and null freeze device node with full configuration type */
	ret = fpga_load_file("file", 0, NULL);
	zassert_equal(ret, -ENOENT, "No such file or directory");

	/* Check invalid file and valid freeze device node with full configuration type */
	ret = fpga_load_file("file", 0, "freeze_controller0@f9000450");
	zassert_equal(ret, -ENOENT, "No such file or directory");

	/* Check ascii configuration type */
	ret = fpga_load_file("file", 'A', NULL);
	zassert_equal(ret, -ENOTSUP, "Please provide correct configuration type");
}

/* Verifying the FPGA configuration*/
ZTEST(fpga_manager_stack, test_fpga_load)
{
	char *fpga_memory_addr;
	uint32_t fpga_memory_size = 0;
	int ret = 0;

	/* Check bitstream address and size */
	ret = fpga_get_memory(&fpga_memory_addr, &fpga_memory_size);
	zassert_equal(ret, 0, "Failed to get the memory");

	/* Check invalid configuration type */
	ret = fpga_load(fpga_memory_addr, fpga_memory_size, -1, NULL);
	zassert_equal(ret, -ENOTSUP, "FPGA configuration type not supported");

	/* Check invalid ascii configuration type */
	ret = fpga_load(fpga_memory_addr, fpga_memory_size, 'A', NULL);
	zassert_equal(ret, -ENOTSUP, "FPGA configuration type not supported");

	/* Check null freeze controller device node with partial configuration type */
	ret = fpga_load(fpga_memory_addr, fpga_memory_size, 1, NULL);
	zassert_equal(ret, -ENODEV, "No such device found");

	/* Check invalid freeze controller device node with partial configuration type */
	ret = fpga_load(fpga_memory_addr, fpga_memory_size, 1, "test");
	zassert_equal(ret, -ENODEV, "No such device found");

	/* Check bitstream size zero with partial configuration type */
	ret = fpga_load(fpga_memory_addr, 0, 1, "freeze_controller0@f9000450");
	zassert_equal(ret, -ENOSR, "FPGA configuration failed");

	/* Check out of range bitstream pointer with full configuration type */
	ret = fpga_load(fpga_memory_addr - 0x100, fpga_memory_size, 0, NULL);
	zassert_equal(ret, -EFAULT, "FPGA configuration failed");

	/* Check out of range bitstream size with the full configuration type */
	ret = fpga_load(fpga_memory_addr, fpga_memory_size + 0x100, 0, NULL);
	zassert_equal(ret, -ENOSR, "FPGA configuration failed");
}

ZTEST_SUITE(fpga_manager_stack, NULL, NULL, NULL, NULL, NULL);
