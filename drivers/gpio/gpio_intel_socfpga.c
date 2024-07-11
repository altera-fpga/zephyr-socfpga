/*
 * Copyright (C) 2022-2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_socfpga_gpio

/**
 * @file
 * @brief Intel GPIO SoCFPGA Controller Driver
 *
 * The GPIO controller on Intel SoCFPGA serves
 * as GPIO modules. This driver provides
 * the GPIO functionality.
 *
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_intel_socfpga);

#define DEV_CFG(_dev) ((const struct gpio_intel_socfpga_config *)(_dev)->config)

#define DEV_DATA(_dev) ((struct gpio_intel_socfpga_data *const)(_dev)->data)


#define GPIO_SWPORTA_DDR_OFFSET	0x04
#define GPIO_EXT_PORTA_OFFSET	0x50
#define GPIO_ID_CODE_OFFSET	0x64
#define GPIO_VER_ID_CODE_OFFSET	0x6c

#define PMUX_SEL_OFFSET		4
#define PMUX_40_SEL		40
#define PMUX_40_SEL_GAP		96
#define PMUX_GPIO_VAL		8

struct gpio_intel_socfpga_config {
	/** GPIO common config */
	struct gpio_driver_config gpio_config;
	/** GPIO MMIO mapped physical address */
	DEVICE_MMIO_NAMED_ROM(gpio_mmio);
	/** GPIO port number (0 or 1) */
	const uint32_t gpio_port;
	/** GPIO pin number per port */
	uint32_t ngpios;
	/** GPIO reset device information */
	struct reset_dt_spec reset;
};

struct gpio_intel_socfpga_data {
	/** GPIO common config */
	struct gpio_driver_data gpio_data;
	/** GPIO MMIO mapped virtual address */
	DEVICE_MMIO_NAMED_RAM(gpio_mmio);
};

static int gpio_socfpga_configure(const struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, gpio_mmio);
	uintptr_t addr;

	addr = reg_base + GPIO_SWPORTA_DDR_OFFSET;

	/* As per the flag set doing pin configuration, following state is possible:
	 * - Pin is input
	 * - Pin is output and with low signal
	 * - Pin is output and with high signal
	 */
	if (((flags & GPIO_PULL_DOWN) != 0U) ||
		((flags & GPIO_PULL_UP) != 0U)) {
		LOG_ERR("Invalid flag option for GPIO operation");
		return -ENOTSUP;
	} else if (flags & GPIO_INPUT) {
		sys_clear_bits(addr, BIT(pin));
	} else if (flags & GPIO_OUTPUT) {
		sys_set_bits(addr, BIT(pin));
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			sys_set_bits(reg_base, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			sys_clear_bits(reg_base, BIT(pin));
		}
	} else {
		LOG_ERR("Invalid flag option for GPIO operation");
		return -EINVAL;
	}

	return 0;
}


static int gpio_socfpga_port_get_raw(const struct device *dev, uint32_t *value)
{
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, gpio_mmio);

	*value = sys_read32((reg_base + GPIO_EXT_PORTA_OFFSET));

	return 0;
}

static int gpio_socfpga_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, gpio_mmio);

	sys_set_bits(reg_base, mask);

	return 0;
}

static int gpio_socfpga_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, gpio_mmio);

	sys_clear_bits(reg_base, mask);

	return 0;
}

static int gpio_socfpga_port_toggle_bits(const struct device *dev, gpio_port_pins_t mask)
{
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(dev, gpio_mmio);

	uint32_t value;

	value = sys_read32((reg_base));
	value ^= mask;
	sys_write32(value, reg_base);

	return 0;
}

static int gpio_socfpga_port_set_masked_raw(const struct device *port,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	return -ENOSYS;
}

static int gpio_socfpga_pin_interrupt_configure(const struct device *port,
					gpio_pin_t pin,
					enum gpio_int_mode mode,
					enum gpio_int_trig trig)
{
	return -ENOSYS;
}

static int gpio_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, gpio_mmio, K_MEM_CACHE_NONE);
	const struct gpio_intel_socfpga_config *cfg = DEV_CFG(dev);
	int ret = 0;

	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("Reset device is not ready");
		return -ENODEV;
	}

	ret = reset_line_toggle(cfg->reset.dev, cfg->reset.id);
	if (ret != 0) {
		LOG_ERR("Disable/Reset operation failed");
	}

	return ret;
}

static const struct gpio_driver_api gpio_socfpga_driver_api = {
	.pin_configure = gpio_socfpga_configure,
	.port_get_raw = gpio_socfpga_port_get_raw,
	.port_set_masked_raw = gpio_socfpga_port_set_masked_raw,
	.port_set_bits_raw = gpio_socfpga_port_set_bits_raw,
	.port_clear_bits_raw = gpio_socfpga_port_clear_bits_raw,
	.port_toggle_bits = gpio_socfpga_port_toggle_bits,
	.pin_interrupt_configure = gpio_socfpga_pin_interrupt_configure,
	.manage_callback = NULL
};

#define CREATE_GPIO_DEVICE(_inst)							\
	static struct gpio_intel_socfpga_data gpio_data_##_inst;			\
	static const struct gpio_intel_socfpga_config gpio_config_##_inst = {		\
		DEVICE_MMIO_NAMED_ROM_INIT(gpio_mmio, DT_DRV_INST(_inst)),		\
		.gpio_config.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(_inst),	\
		.gpio_port = _inst,							\
		.ngpios = DT_INST_PROP(_inst, ngpios),					\
		.reset = RESET_DT_SPEC_INST_GET(_inst)					\
	};										\
	DEVICE_DT_INST_DEFINE(_inst,							\
			gpio_init,							\
			NULL,								\
			&gpio_data_##_inst,						\
			&gpio_config_##_inst,						\
			PRE_KERNEL_1,							\
			CONFIG_GPIO_INIT_PRIORITY,					\
			&gpio_socfpga_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_GPIO_DEVICE)
