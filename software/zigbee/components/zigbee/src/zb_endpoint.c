/**
 * @file zb_endpoint.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>

/* Setup logging */
#include "esp_log.h"
static const char* TAG = "zb_endpoint";

/* Library header */
#include "zb_endpoint.h"

/* Zigbee library */
#include "esp_zigbee_core.h"

/* Values for basic cluster */
#define BASIC_ZCL_VERSION                   ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE
#define BASIC_APPLICATION_VERSION           ESP_ZB_ZCL_BASIC_APPLICATION_VERSION_DEFAULT_VALUE
#define BASIC_STACK_VERSION                 ESP_ZB_ZCL_BASIC_STACK_VERSION_DEFAULT_VALUE
#define BASIC_HW_VERSION                    ESP_ZB_ZCL_BASIC_HW_VERSION_DEFAULT_VALUE
#define BASIC_MANUFACTURER_NAME             " xxx"
#define BASIC_MODEL_NAME                    " xxx"
#define BASIC_POWER_SOURCE                  ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE

/* Values for identify cluster */
#define IDENTIFY_IDENTIFY_TIME              ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE

/**
 * @brief Create endpoint for dlms
 */
static void zb_dlms_ep(esp_zb_ep_list_t *esp_zb_ep_list)
{
    /* === CREATE BASIC CLUSTER (REQUIRED) (0x00) === */
    esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_BASIC);

    /* Add attribute zcl_version (0x00) */
    static uint8_t zcl_version = BASIC_ZCL_VERSION;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_ZCL_VERSION_ID, &zcl_version));

    /* Add attribute application_version (0x01) */
    static uint8_t application_version = BASIC_APPLICATION_VERSION;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID, &application_version));

    /* Add attribute stack_version (0x02) */
    static uint8_t stack_version = BASIC_STACK_VERSION;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_STACK_VERSION_ID, &stack_version));

    /* Add attribute hw_version (0x03) */
    static uint8_t hw_version = BASIC_HW_VERSION;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_HW_VERSION_ID, &hw_version));

    /* Add attribute manufacturer_name (0x04) */
    static char manufacturer_name[] = BASIC_MANUFACTURER_NAME;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, &manufacturer_name[0]));

    /* Add attribute model_identifier (0x05) */
    static char model_identifier[] = BASIC_MODEL_NAME;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, &model_identifier[0]));

    /* Add attribute power_source (0x07) */
    static uint8_t power_source = BASIC_POWER_SOURCE;
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID, &power_source));

    /* === CREATE IDENTITY CLUSTER (REQUIRED) (0x03) === */
    esp_zb_attribute_list_t *esp_zb_identify_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY);

    /* Add attribute identify_time (0x00) */
    static uint16_t identify_time = IDENTIFY_IDENTIFY_TIME;
    ESP_ERROR_CHECK(esp_zb_identify_cluster_add_attr(esp_zb_identify_cluster, ESP_ZB_ZCL_ATTR_IDENTIFY_IDENTIFY_TIME_ID, &identify_time));

    /* === CREATE CLUSTER CLIENT ROLES === */
    esp_zb_attribute_list_t *esp_zb_identify_client_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY);

    /* === CREATE CLUSTER LIST === */
    esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    //TODO: Add smartmeter specific clusters

    /* Server clusters */
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    /* Client clusters */
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_client_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

    /* === ADD CREATED ENDPOINT TO LIST === */
    //TODO: ESP_ERROR_CHECK(esp_zb_ep_list_add_ep(esp_zb_ep_list, esp_zb_cluster_list, HA_DLMS_ENDPOINT, ESP_ZB_AF_HA_PROFILE_ID, ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID));
}