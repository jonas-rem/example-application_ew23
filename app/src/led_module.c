/*
 * Copyright (c) 2023 Phytec Messtechnik GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "evt_messages.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


ZBUS_CHAN_DECLARE(bp_chan);
ZBUS_SUBSCRIBER_DEFINE(bp_handler_sub, 3);


void led_thread(void)
{
	const struct zbus_channel *chan;
	int ret;

	/* Initialize LED */
	if (!gpio_is_ready_dt(&led)) {
		return;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}
	gpio_pin_set_dt(&led, 0);

	/* Wait for Messages */
	while (!zbus_sub_wait(&bp_handler_sub, &chan, K_FOREVER)) {
		struct bp_msg msg;

		zbus_chan_read(chan, &msg, K_MSEC(200));

		LOG_DBG("Rcv from '%s', Message: GPIO%d\n", chan->name,
				msg.pin);
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
	}
}

K_THREAD_DEFINE(led_thread_id, 1024, led_thread, NULL, NULL, NULL, 5, 0, 0);
