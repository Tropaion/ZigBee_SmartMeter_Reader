/**
 * @file human_interface.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "human_interface";

/* Header */
#include "human_interface.h"

/* Button Library */
#include "iot_button.h"

/* Led Library */
#include "led_indicator.h"

/* Led Handle */
static led_indicator_handle_t led_handle = NULL;

/* ===== BUTTON FUNCTIONS ===== */
esp_err_t button_init(bttn_cb_t button_cb)
{
    /* Configuration */
    button_config_t button_config = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = BTTN_LONG_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = BTTN_GPIO_NUM,
            .active_level = 0,
        },
    };

    /* Create button */
    button_handle_t button_handle = iot_button_create(&button_config);

    /* Check for error */
    if(button_handle == NULL)
    {
        ESP_LOGE(TAG, "No button handle!");
        return ESP_FAIL;
    }

    /* Register long press button callback */
    return iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_START, (button_cb_t)button_cb, NULL);
}

/* ===== LED FUNCTIONS ===== */
esp_err_t led_init()
{
    /* reset animation */
    static const blink_step_t blink_reset[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 500},
        {LED_BLINK_HOLD, LED_STATE_OFF, 500},
        {LED_BLINK_LOOP, 0, 0},
    };

    /* resetting device animation */
    static const blink_step_t blink_coupling[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 1000},
        {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
        {LED_BLINK_LOOP, 0, 0},
    };

    /* sending data animation */
    static const blink_step_t blink_sending_data[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 500},
        {LED_BLINK_HOLD, LED_STATE_OFF, 0},
        {LED_BLINK_STOP, 0, 0},
    };

    /* start up animation */
    static const blink_step_t blink_start_up[] = {
        {LED_BLINK_HOLD, LED_STATE_ON, 0},
        {LED_BLINK_STOP, 0, 0},
    };

    /* create animation list */
    static blink_step_t const* blink_list[] = {
        [BLINK_RESET] = blink_reset,
        [BLINK_COUPLING] = blink_coupling,
        [BLINK_SENDING_DATA] = blink_sending_data,
        [BLINK_START_UP] = blink_start_up,
    };

    /* GPIO configuration */
    led_indicator_gpio_config_t gpio_config = {
        .gpio_num = LED_GPIO_NUM,
        .is_active_level_high = 1,
    };

    /* blink configuration */
    led_indicator_config_t blink_config = {
        .led_indicator_gpio_config = &gpio_config,
        .mode = LED_GPIO_MODE,
        .blink_lists = blink_list,
        .blink_list_num = LED_BLINK_NUM,
    };

    /* Create handle */
    led_handle = led_indicator_create(&blink_config);

    /* Check for error */
    if(led_handle == NULL)
    {
        ESP_LOGE(TAG, "No led handle!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t led_animation_start(led_blink_type_t type)
{
    return led_indicator_start(led_handle, type);
}

esp_err_t led_animation_stop(led_blink_type_t type)
{
    return led_indicator_stop(led_handle, type);
}