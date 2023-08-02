/**
 * @file zb_main.h
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_zigbee_core.h"

/* ===== ZIGBEE GENERAL CONFIGURATION ===== */
#define RESET_BUTTON_ANIMATION_TIME_MS      2500

/* ===== ZIGBEE RF CONFIGURATION ===== */
// https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/developing.html
#define MAX_CHILDREN                        10                                    /* the max amount of connected devices */
#define INSTALLCODE_POLICY_ENABLE           false                                 /* enable the install code policy for security */
#define ESP_ZB_PRIMARY_CHANNEL_MASK         ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK  /* Zigbee primary channel mask use in the example */

/**
 * @brief Configure as zigbee as router
 */
#define ESP_ZB_ZR_CONFIG()                                                              \
{                                                                                   \
    .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,                                       \
    .install_code_policy = INSTALLCODE_POLICY_ENABLE,                               \
    .nwk_cfg.zczr_cfg = {                                                           \
        .max_children = MAX_CHILDREN,                                               \
    },                                                                              \
}

/**
 * @brief No co-processing, zb and application run on same processor
 *  https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/protocols/zigbee/architectures.html
 */
#define ZB_DEFAULT_RADIO_CONFIG()                           \
{                                                           \
    .radio_mode = RADIO_MODE_NATIVE,                        \
}

#define ZB_DEFAULT_HOST_CONFIG()                            \
{                                                           \
    .host_connection_mode = HOST_CONNECTION_MODE_NONE,      \
}

/**
 * @brief Initialize and start ZB operation
 * 
 * @return esp_err_t 
 */
esp_err_t zb_run();

#ifdef __cplusplus
}
#endif