/**
 * @file human_interface.h
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_check.h"

/* ===== BUTTON CONFIGURATION ===== */
#define BUTTON_GPIO_NUM                 0
#define BUTTON_LONG_PRESS_TIME_MS       3000

typedef void (* bttn_cb_t)(void *button_handle, void *usr_data);

/* ===== LED CONFIGURATION ===== */
#define LED_GPIO_NUM                    1
#define LED_BLINK_NUM                   3

/* Define blink types according to priority */
typedef enum {
    BLINK_RESET,
    BLINK_COUPLING,
    BLINK_SENDING_DATA,
} led_blink_type_t;

/* ===== BUTTON FUNCTIONS ===== */
/**
 * @brief Initialize button and register callback for long press
 * 
 * @return esp_err_t 
 */
esp_err_t button_init(bttn_cb_t button_cb);

/* ===== LED FUNCTIONS ===== */
/**
 * @brief Initialize led
 * 
 * @return esp_err_t 
 */
esp_err_t led_init();

/**
 * @brief Start led animation
 * 
 * @param type animation type
 * @return esp_err_t 
 */
esp_err_t led_animation_start(led_blink_type_t type);

/**
 * @brief Stop led animation
 * 
 * @param type animation type
 * @return esp_err_t 
 */
esp_err_t led_animation_stop(led_blink_type_t type);

#ifdef __cplusplus
} // extern "C"
#endif
