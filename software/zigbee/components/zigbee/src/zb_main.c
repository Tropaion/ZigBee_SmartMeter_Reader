/**
 * @file zb_main.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>

/* Setup logging */
#include "esp_log.h"
static const char* TAG = "zb_main";

/* Library header */
#include "zb_main.h"
#include "zb_endpoint.h"

/* Zigbee libraries */
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"

/* FreeRTOS libraries*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @note Make sure set idf.py menuconfig in zigbee component as zigbee router !
*/
#if !defined CONFIG_ZB_ZCZR
#error Define ZB_ZCZR in idf.py menuconfig to compile light (Router) source code.
#endif

/* ===== CALLBACK FUNCTIONS ===== */
/**
 * @brief Wrapper for commissioning callback
 * 
 * @param mode_mask 
 */
static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}

/**
 * @brief Callback function, for example is called by esp_zb_start()
 * Handles state signal, for example if stack is initialized or network steering is done
 * 
 * @param signal_struct 
 */
//TODO: Copy from latest version
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    /* Get error status of signal */
    esp_err_t err_status = signal_struct->esp_err_status;

    /* Get signal type */
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;

    /* Handle signal according to signal type */
    /* Signal type description: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/zboss/3.5.2.0/group__zb__comm__signals.html#gab55565980ef0580712f8dc160b01ea06*/
    switch (sig_type) {
        /* Called by "esp_zb_start" when autostart is disabled */
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            /* ZigBee stack is now initialized but not started */
            ESP_LOGI(TAG, "Zigbee stack initialized");

            /* Initialize base device behaviour */
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
            break;

        /* Device started for the first time after the NVRAM erase */
        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            /* do nothing */

        /* Device started using the NVRAM contents */
        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            if (err_status == ESP_OK) {
                /* Start forming/joining zigbee network */
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                /* Stack not initialized */
                ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %d)", err_status);
            }
            break;

        /* BDB network steering completed */
        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (err_status == ESP_OK) {
                /* Get and print PAN-ID of joined network */
                esp_zb_ieee_addr_t extended_pan_id;
                esp_zb_get_extended_pan_id(extended_pan_id);
                ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d)",
                        extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                        extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                        esp_zb_get_pan_id(), esp_zb_get_current_channel());

                /* Run initialization functions from callback list */
                ESP_ERROR_CHECK(zb_main_run_init_cb());
            } 
            else {
                /* Failed to join network */
                ESP_LOGI(TAG, "Network steering was not successful (status: %d)", err_status);

                /* Try joining again after 1s */
                esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            }
            break;

        /* Device left network */
        case ESP_ZB_ZDO_SIGNAL_LEAVE:
            esp_zb_zdo_signal_leave_params_t *leave_params = (esp_zb_zdo_signal_leave_params_t *)esp_zb_app_signal_get_params(p_sg_p);
            //TODO: Handle leave request from coordinator

            /* Leaving was initiated from this device (button,..) */
            if (leave_params->leave_type == ESP_ZB_NWK_LEAVE_TYPE_RESET) {
                ESP_LOGI(TAG, "Reset device");
            }
            break;

        /* Handle all other signals */
        default:
            /* Print signal strength and status */
            ESP_LOGI(TAG, "ZDO signal: %d, status: %d", sig_type, err_status);
            break;
    }
}

/* ===== MAIN FUNCTIONS ===== */
/**
 * @brief In this function, the device functionality (clusters, endpoints) is defined and application will be started
 * 
 * @param pvParameters 
 */
static void zb_main_task(void *pvParameters)
{
    /* Initialize zigbee stack with end-device config */ 
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    /* Create endpoint list */
    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();

    /* Create endpoint */
    zb_dlms_ep(esp_zb_ep_list);

    /* Register endpoint list to stack */
    esp_zb_device_register(esp_zb_ep_list);

    /* Set which zigbee channels to use */
    esp_zb_set_primary_network_channel_set(ZB_PRIMARY_CHANNEL_MASK);

    /* Start zigbee and check for error, afterwards, function "esp_zb_app_signal_handler" is called */
    ESP_ERROR_CHECK(esp_zb_start(false));

    /* Run zigbee functionality in loop */
    esp_zb_main_loop_iteration();
}

/**
 * @brief Starts zigbee stack/application
 * 
 * @return esp_err_t 
 */
esp_err_t zb_run()
{
    /* Initialize non-volatile flash */
    ESP_ERROR_CHECK(nvs_flash_init());

    /* Operating mode (single processor or co-processor) */
    esp_zb_platform_config_t config = {
        .radio_config = ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ZB_DEFAULT_HOST_CONFIG(),
    };

    /* Set operating mode */
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    /* Create zigbee background task */
    xTaskCreate(zb_main_task, "ZB_MAIN", 4096, NULL, 5, NULL);

    return ESP_OK;
}