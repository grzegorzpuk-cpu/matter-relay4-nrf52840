#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-relay4)

# Enable GNU STD support
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START

# Common Matter sources
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

# Data Model utils
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/data_model.cmake)

include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/zap_helpers.cmake)
ncs_get_zap_parent_dir(ZAP_PARENT_DIR)

target_include_directories(app PRIVATE
    src
    ${ZAP_PARENT_DIR}
)

target_sources(app PRIVATE
    src/app_task.cpp
    src/main.cpp
    src/zcl_callbacks.cpp
    src/relay_manager.cpp
    ${CHIP_ROOT}/src/app/util/persistence/DeferredAttributePersistenceProvider.cpp
)

ncs_configure_data_model()
# NORDIC SDK APP END
