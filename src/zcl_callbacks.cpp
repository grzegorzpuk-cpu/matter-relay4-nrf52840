/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "relay_manager.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app::Clusters;

/**
 * Wywoływane po każdej zmianie atrybutu Matter.
 * Mapowanie: EndpointId → RelayManager::SetRelay(index)
 *   EP1 → relay 0
 *   EP2 → relay 1
 *   EP3 → relay 2
 *   EP4 → relay 3
 */
void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
				       uint16_t size, uint8_t *value)
{
	const EndpointId ep = attributePath.mEndpointId;
	const ClusterId clusterId = attributePath.mClusterId;
	const AttributeId attributeId = attributePath.mAttributeId;

	/* Obsługujemy tylko klaster OnOff, atrybut OnOff, na EP1-EP4 */
	if (clusterId != OnOff::Id || attributeId != OnOff::Attributes::OnOff::Id) {
		return;
	}

	if (ep < 1 || ep > 4) {
		return;
	}

	const uint8_t relayIndex = static_cast<uint8_t>(ep - 1); /* EP1→0, EP4→3 */
	const bool on = (*value != 0);

	ChipLogProgress(Zcl, "Matter → Relay %d: %s (EP%d)", relayIndex, on ? "ON" : "OFF", ep);
	RelayManager::SetRelay(relayIndex, on);
}

/**
 * Callback wywoływany przy inicjalizacji klastra OnOff na EP1.
 * Dla EP2-EP4 (dynamiczne) ten callback nie jest wywoływany automatycznie —
 * stan po resecie to OFF (GPIO_OUTPUT_INACTIVE).
 */
void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
	if (endpoint != 1) {
		return;
	}

	Protocols::InteractionModel::Status status;
	bool storedValue = false;

	status = OnOff::Attributes::OnOff::Get(endpoint, &storedValue);
	if (status == Protocols::InteractionModel::Status::Success) {
		/* Odtwórz zapisany stan przekaźnika 0 po restarcie */
		RelayManager::SetRelay(0, storedValue);
		ChipLogProgress(Zcl, "EP1 OnOff init: restored relay 0 → %s", storedValue ? "ON" : "OFF");
	}
}
