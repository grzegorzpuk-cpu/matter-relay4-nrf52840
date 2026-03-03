/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <zephyr/drivers/gpio.h>

/* Liczba przekaźników */
#define RELAY_COUNT 4

class RelayManager {
public:
	/* Inicjalizacja GPIO przekaźników */
	static int Init();

	/* Ustaw stan przekaźnika (index 0-3 → EP1-EP4) */
	static void SetRelay(uint8_t index, bool on);

	/* Pobierz aktualny stan przekaźnika */
	static bool GetRelay(uint8_t index);

private:
	static const struct gpio_dt_spec sRelays[RELAY_COUNT];
};
