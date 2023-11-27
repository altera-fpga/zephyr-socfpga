/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT altr_pio_1_0

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/shared_irq.h>

#define ILC_SHARED_IRQ_INIT(node_id)                                                               \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_DRV_INST(node_id), shared_irqs, 0))

#define ALTERA_AVALON_PIO_DATA_OFFSET      0x00
#define ALTERA_AVALON_PIO_DIRECTION_OFFSET 0x04
#define ALTERA_AVALON_PIO_IRQ_OFFSET       0x08
#define ALTERA_AVALON_PIO_SET_BITS         0x10
#define ALTERA_AVALON_PIO_CLEAR_BITS       0x14

typedef int (*altera_cfg_func_t)(void);
typedef void (*altera_irq_func_t)(uint32_t irq_num);

struct gpio_altera_config {
	DEVICE_MMIO_ROM;
	struct gpio_driver_config common;
	uint32_t irq_num;
	uint8_t direction;
	uint8_t outset;
	uint8_t outclear;
	altera_cfg_func_t cfg_func;
	altera_irq_func_t pio_irq_enable;
	altera_irq_func_t pio_irq_disable;
};

struct gpio_altera_data {
	DEVICE_MMIO_RAM;
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
	struct k_spinlock lock;
};

static bool gpio_pin_direction(const struct device *dev, uint32_t pin_mask)
{
	const struct gpio_altera_config *cfg = (struct gpio_altera_config *)dev->config;
	const int direction = cfg->direction;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t addr;
	uint32_t pin_direction;

	if (pin_mask == 0) {
		return -EINVAL;
	}

	/* Check if the direction is Bidirectional */
	if (direction != 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_DIRECTION_OFFSET;

	pin_direction = sys_read32(addr);

	if (!(pin_direction & pin_mask)) {
		return false;
	}

	return true;
}

static int gpio_altera_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_altera_config *cfg = (struct gpio_altera_config *)dev->config;
	struct gpio_altera_data *const data = (struct gpio_altera_data *)dev->data;
	const int port_pin_mask = cfg->common.port_pin_mask;
	const int direction = cfg->direction;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	k_spinlock_key_t key;
	uint32_t addr;

	/* Check if pin number is within range */
	if ((port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	/* Check if the direction is Bidirectional */
	if (direction != 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_DIRECTION_OFFSET;

	key = k_spin_lock(&data->lock);

	if (flags == GPIO_INPUT) {
		sys_clear_bits(addr, BIT(pin));
	} else if (flags == GPIO_OUTPUT) {
		sys_set_bits(addr, BIT(pin));
	} else {
		return -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_altera_port_get_raw(const struct device *dev, uint32_t *value)
{
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t addr;

	addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;

	if (value == NULL) {
		return -EINVAL;
	}

	*value = sys_read32((addr));

	return 0;
}

static int gpio_altera_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_altera_config *cfg = (struct gpio_altera_config *)dev->config;
	struct gpio_altera_data *const data = (struct gpio_altera_data *)dev->data;
	const uint8_t outset = cfg->outset;
	const int port_pin_mask = cfg->common.port_pin_mask;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t addr;
	k_spinlock_key_t key;

	if ((port_pin_mask & mask) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, mask)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (outset) {
		addr = reg_base + ALTERA_AVALON_PIO_SET_BITS;
		sys_write32(mask, addr);
	} else {
		addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;
		sys_set_bits(addr, mask);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_altera_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_altera_config *cfg = (struct gpio_altera_config *)dev->config;
	struct gpio_altera_data *const data = (struct gpio_altera_data *)dev->data;
	const uint8_t outclear = cfg->outclear;
	const int port_pin_mask = cfg->common.port_pin_mask;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t addr;
	k_spinlock_key_t key;

	/* Check if mask range within 32 */
	if ((port_pin_mask & mask) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, mask)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (outclear) {
		addr = reg_base + ALTERA_AVALON_PIO_CLEAR_BITS;
		sys_write32(mask, addr);
	} else {
		addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;
		sys_clear_bits(addr, mask);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_init(const struct device *dev)
{
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	const struct gpio_altera_config *cfg = (struct gpio_altera_config *)dev->config;

	/* Configure GPIO device */
	ret = cfg->cfg_func();

	return ret;
}

static int gpio_altera_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(trig);

	const struct gpio_altera_config *cfg = (struct gpio_altera_config *)dev->config;
	struct gpio_altera_data *const data = (struct gpio_altera_data *)dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	const int port_pin_mask = cfg->common.port_pin_mask;
	uint32_t addr;
	k_spinlock_key_t key;

	/* Check if pin number is within range */
	if ((port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, BIT(pin))) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_IRQ_OFFSET;

	key = k_spin_lock(&data->lock);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		/* Disable interrupt of pin */
		sys_clear_bits(addr, BIT(pin));
		cfg->pio_irq_disable(cfg->irq_num);
		break;
	case GPIO_INT_MODE_LEVEL:
	case GPIO_INT_MODE_EDGE:
		/* Enable interrupt of pin */
		sys_set_bits(addr, BIT(pin));
		cfg->pio_irq_enable(cfg->irq_num);
		break;
	default:
		return -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_altera_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{

	struct gpio_altera_data *const data = (struct gpio_altera_data *)dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static void gpio_altera_irq_handler(const struct device *dev)
{
	struct gpio_altera_data *data = (struct gpio_altera_data *)dev->data;
	uintptr_t reg_base = DEVICE_MMIO_GET(dev);
	uint32_t port_value;
	uint32_t addr;
	k_spinlock_key_t key;

	addr = reg_base + ALTERA_AVALON_PIO_IRQ_OFFSET;

	key = k_spin_lock(&data->lock);

	port_value = sys_read32(addr);

	sys_clear_bits(addr, port_value);

	k_spin_unlock(&data->lock, key);

	/* Call the corresponding callback registered for the pin */
	gpio_fire_callbacks(&data->cb, dev, port_value);
}

static const struct gpio_driver_api gpio_altera_driver_api = {
	.pin_configure = gpio_altera_configure,
	.port_get_raw = gpio_altera_port_get_raw,
	.port_set_masked_raw = NULL,
	.port_set_bits_raw = gpio_altera_port_set_bits_raw,
	.port_clear_bits_raw = gpio_altera_port_clear_bits_raw,
	.port_toggle_bits = NULL,
	.pin_interrupt_configure = gpio_altera_pin_interrupt_configure,
	.manage_callback = gpio_altera_manage_callback};

#define GPIO_CFG_IRQ(idx, n)                                                                       \
	IRQ_CONNECT(                                                                               \
		DT_INST_IRQ_BY_IDX(n, idx, irq),                                                   \
		COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, priority), (DT_INST_IRQ(n, priority)), (0)),   \
		gpio_altera_irq_handler, DEVICE_DT_INST_GET(n), 0);

#define CREATE_GPIO_DEVICE(n)                                                                      \
	static int gpio_altera_cfg_func_##n(void);                                                 \
	static void pio_irq_enable_handler_##n(uint32_t irq_num);                                  \
	static void pio_irq_disable_handler_##n(uint32_t irq_num);                                 \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, shared_irqs),                                         \
		    (static int gpio_altera_irq_handler_##n(const struct device *dev,              \
							    unsigned int irq_number);),            \
		    ())                                                                            \
	static struct gpio_altera_data gpio_altera_data_##n;                                       \
	const static struct gpio_altera_config gpio_config_##n = {                                 \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.direction = DT_INST_ENUM_IDX(n, direction),                                       \
		.irq_num = COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (DT_INST_IRQN(n)), (0)),         \
		.cfg_func = gpio_altera_cfg_func_##n,                                              \
		.outset = DT_INST_PROP(n, outset),                                                 \
		.outclear = DT_INST_PROP(n, outclear),                                             \
		.pio_irq_enable = pio_irq_enable_handler_##n,                                      \
		.pio_irq_disable = pio_irq_disable_handler_##n,                                    \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(n, gpio_init, NULL, &gpio_altera_data_##n, &gpio_config_##n,         \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_altera_driver_api);    \
												   \
	static int gpio_altera_cfg_func_##n(void)                                                  \
	{                                                                                          \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, shared_irqs),                                 \
			    (                                                                      \
				int ret;                                                           \
				ret = shared_irq_isr_register(ILC_SHARED_IRQ_INIT(n),              \
								  &gpio_altera_irq_handler_##n,    \
								  DEVICE_DT_INST_GET(n));          \
				if (ret != 0) {							   \
					return ret;						   \
					}),							   \
			    (LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), GPIO_CFG_IRQ, (), n)))           \
		return 0;                                                                          \
	}                                                                                          \
												   \
	static void pio_irq_enable_handler_##n(uint32_t irq_num)                                   \
	{                                                                                          \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, shared_irqs), (                               \
		shared_irq_enable(ILC_SHARED_IRQ_INIT(n), DEVICE_DT_INST_GET(n));),		   \
		(irq_enable(irq_num);))								   \
	}                                                                                          \
												   \
	static void pio_irq_disable_handler_##n(uint32_t irq_num)                                  \
	{                                                                                          \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, shared_irqs), (				   \
		shared_irq_disable(ILC_SHARED_IRQ_INIT(n), DEVICE_DT_INST_GET(n));),               \
		(irq_disable(irq_num);))						           \
	}                                                                                          \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, shared_irqs),                                         \
		    (static int gpio_altera_irq_handler_##n(const struct device *dev,              \
							    unsigned int irq_number) {             \
			gpio_altera_irq_handler(dev);                                              \
			return 0;                                                                  \
		    }),                                                                            \
		    ())

DT_INST_FOREACH_STATUS_OKAY(CREATE_GPIO_DEVICE)
