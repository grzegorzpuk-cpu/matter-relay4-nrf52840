/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "relay_manager.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

/* GPIO aliasy z DTS overlay - ACTIVE_LOW (logiczne 1 = przekaźnik włączony,
 * fizycznie pin LOW = cewka zasilona) */
const struct gpio_dt_spec RelayManager::sRelays[RELAY_COUNT] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(relay_0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay_1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay_2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(relay_3), gpios),
};

int RelayManager::Init()
{
	for (uint8_t i = 0; i < RELAY_COUNT; i++) {
		if (!gpio_is_ready_dt(&sRelays[i])) {
			LOG_ERR("Relay GPIO %d not ready", i);
			return -ENODEV;
		}

		int ret = gpio_pin_configure_dt(&sRelays[i], GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure relay GPIO %d: %d", i, ret);
			return ret;
		}
		LOG_INF("Relay %d initialized (OFF)", i);
	}
	return 0;
}

void RelayManager::SetRelay(uint8_t index, bool on)
{
	if (index >= RELAY_COUNT) {
		LOG_ERR("Invalid relay index: %d", index);
		return;
	}

	/* GPIO_ACTIVE_LOW: gpio_pin_set_dt(true) → pin LOW → przekaźnik ON */
	gpio_pin_set_dt(&sRelays[index], on ? 1 : 0);
	LOG_INF("Relay %d: %s", index, on ? "ON" : "OFF");
}

bool RelayManager::GetRelay(uint8_t index)
{
	if (index >= RELAY_COUNT) {
		return false;
	}
	return gpio_pin_get_dt(&sRelays[index]) > 0;
}
