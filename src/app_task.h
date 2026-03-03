/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app/matter_init.h"
#include "app/task_executor.h"

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	}

	CHIP_ERROR StartApp();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	CHIP_ERROR Init();

	/* Rejestracja dynamicznych endpointów EP2-EP4 */
	static CHIP_ERROR RegisterDynamicEndpoints();
};
