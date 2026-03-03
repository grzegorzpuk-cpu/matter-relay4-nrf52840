/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "relay_manager.h"

#include "app/matter_init.h"
#include "app/task_executor.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

/* Dynamic endpoint headers */
#include <app/util/endpoint-config-defines.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{

/* EP1 jest zdefiniowany w ZAP; EP2-EP4 dodajemy dynamicznie */
constexpr EndpointId kRelay1EndpointId = 1;
constexpr EndpointId kRelay2EndpointId = 2;
constexpr EndpointId kRelay3EndpointId = 3;
constexpr EndpointId kRelay4EndpointId = 4;

/* Identify server na EP1 */
Identify sIdentify = { kRelay1EndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };

/* ─── Dynamic endpoint data dla EP2-EP4 ─────────────────────────────── */

/* Atrybuty klastra OnOff dla endpointu dynamicznego */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(sOnOffAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::OnOff::Id, BOOLEAN, 1, ZAP_ATTRIBUTE_MASK(TOKENIZE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::ClusterRevision::Id, INT16U, 2, 0),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

/* Atrybuty klastra Descriptor */
DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(sDescriptorAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Descriptor::Attributes::DeviceTypeList::Id, ARRAY, 0,
			  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Descriptor::Attributes::ServerList::Id, ARRAY, 0,
				  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Descriptor::Attributes::ClientList::Id, ARRAY, 0,
				  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Descriptor::Attributes::PartsList::Id, ARRAY, 0,
				  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Descriptor::Attributes::ClusterRevision::Id, INT16U, 2,
				  ZAP_ATTRIBUTE_MASK(EXTERNAL_STORAGE)),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

/* Lista klastrów dynamicznego endpointu */
DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(sRelayClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::OnOff::Id, sOnOffAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, sDescriptorAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER_LIST_END;

/* Typ endpointu */
DECLARE_DYNAMIC_ENDPOINT(sRelayEndpointType, sRelayClusters);

/* DataVersion storage dla każdego dynamicznego endpointu (po jednej wersji na klaster) */
DataVersion sDataVersions2[ArraySize(sRelayClusters)];
DataVersion sDataVersions3[ArraySize(sRelayClusters)];
DataVersion sDataVersions4[ArraySize(sRelayClusters)];

/* Device type: On/Off Plug-in Unit (0x010A), rev 2 */
const EmberAfDeviceType sRelayDeviceTypes[] = { { 0x010A, 2 } };

#define APPLICATION_BUTTON_MASK DK_BTN2_MSK

} /* namespace */

void AppTask::IdentifyStartHandler(Identify *)
{
	Nrf::PostTask(
		[] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms); });
}

void AppTask::IdentifyStopHandler(Identify *)
{
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	/* Przycisk 2 toggleuje relay 0 (EP1) ręcznie */
	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		Nrf::PostTask([] {
			bool current = RelayManager::GetRelay(0);
			RelayManager::SetRelay(0, !current);
			/* Synchronizacja stanu z klastrem Matter */
			SystemLayer().ScheduleLambda([] {
				Protocols::InteractionModel::Status status = Clusters::OnOff::Attributes::OnOff::Set(
					kRelay1EndpointId, RelayManager::GetRelay(0));
				if (status != Protocols::InteractionModel::Status::Success) {
					LOG_ERR("Updating OnOff cluster EP1 failed: %x", to_underlying(status));
				}
			});
		});
	}
}

CHIP_ERROR AppTask::RegisterDynamicEndpoints()
{
	/* EP2 */
	EmberAfStatus status = emberAfSetDynamicEndpoint(
		0, kRelay2EndpointId, &sRelayEndpointType, Span<DataVersion>(sDataVersions2, ArraySize(sDataVersions2)),
		Span<const EmberAfDeviceType>(sRelayDeviceTypes, ArraySize(sRelayDeviceTypes)));
	VerifyOrReturnError(status == EMBER_ZCL_STATUS_SUCCESS, CHIP_ERROR_INTERNAL,
			    LOG_ERR("Failed to set dynamic EP2: %d", status));

	/* EP3 */
	status = emberAfSetDynamicEndpoint(
		1, kRelay3EndpointId, &sRelayEndpointType, Span<DataVersion>(sDataVersions3, ArraySize(sDataVersions3)),
		Span<const EmberAfDeviceType>(sRelayDeviceTypes, ArraySize(sRelayDeviceTypes)));
	VerifyOrReturnError(status == EMBER_ZCL_STATUS_SUCCESS, CHIP_ERROR_INTERNAL,
			    LOG_ERR("Failed to set dynamic EP3: %d", status));

	/* EP4 */
	status = emberAfSetDynamicEndpoint(
		2, kRelay4EndpointId, &sRelayEndpointType, Span<DataVersion>(sDataVersions4, ArraySize(sDataVersions4)),
		Span<const EmberAfDeviceType>(sRelayDeviceTypes, ArraySize(sRelayDeviceTypes)));
	VerifyOrReturnError(status == EMBER_ZCL_STATUS_SUCCESS, CHIP_ERROR_INTERNAL,
			    LOG_ERR("Failed to set dynamic EP4: %d", status));

	LOG_INF("Dynamic endpoints EP2, EP3, EP4 registered");
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::Init()
{
	/* Inicjalizacja GPIO przekaźników */
	int ret = RelayManager::Init();
	if (ret != 0) {
		LOG_ERR("RelayManager init failed: %d", ret);
		return chip::System::MapErrorZephyr(ret);
	}

	/* Inicjalizacja stosu Matter */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	/* Rejestracja endpointów dynamicznych po starcie serwera Matter */
	ReturnErrorOnFailure(Nrf::Matter::StartServer());
	ReturnErrorOnFailure(RegisterDynamicEndpoints());

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
