/*
 * Copyright (c) 2023 Phytec Messtechnik GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "evt_messages.h"
#include <zephyr/zbus/zbus.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;


ZBUS_CHAN_DEFINE(bp_chan,	/* Name */
		 struct bp_msg, /* Message type */

		 NULL,	/* Validator */
		 NULL,	/* User data */
		 ZBUS_OBSERVERS(bp_handler_sub),	/* observers */
		 ZBUS_MSG_INIT(0)	/* Initial value {0} */
);

/* We are checking if the button is pressed after the debounce period */
void debounce_tmr_expiry(struct k_timer *dummy)
{
	struct bp_msg bpm = {0};
	int val;

	val = gpio_pin_get_dt(&button);
	if (val) {
		LOG_INF("Button pressed: Publish message...");

		bpm.pin = (uint32_t)button.pin;

		zbus_chan_pub(&bp_chan, &bpm, K_NO_WAIT);
	}
}

K_TIMER_DEFINE(debounce_tmr, debounce_tmr_expiry, NULL);

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	if (k_timer_remaining_get(&debounce_tmr)) {
		return;
	}

	k_timer_start(&debounce_tmr, K_MSEC(20), K_NO_WAIT);
}

void main(void)
{
	int ret;

	LOG_DBG("Zephyr Example Application %s", CONFIG_BOARD);

	if (!gpio_is_ready_dt(&button)) {
		//LOG_ERR("Error: button device %s is not ready",
		//		button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		//LOG_ERR("Error %d: failed to configure %s pin %d", ret,
		//		button.port->name, button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		//LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
		//	ret, button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
}
