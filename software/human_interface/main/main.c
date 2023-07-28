/**
 * @file main.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>

/* Setup logging */
#include "esp_log.h"
static const char *TAG = "app_main";

#include "human_interface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Called after 3s button press */
void button_cb(void *button_handle, void *usr_data)
{
    /* Test led animation "reset" */
    ESP_LOGI(TAG, "Start reset animation");
    ESP_ERROR_CHECK(led_animation_start(BLINK_RESET));
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "Start stop animation");
    ESP_ERROR_CHECK(led_animation_stop(BLINK_RESET));
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void app_main(void)
{
    /* ===== TESTING HUMAN INTERFACE ===== */
    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(button_init(button_cb));

    while(1)
    {
        /* Test led animation "coupling" */
        ESP_LOGI(TAG, "Start coupling animation");
        ESP_ERROR_CHECK(led_animation_start(BLINK_COUPLING));
        vTaskDelay(6000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Stop coupling animation");
        ESP_ERROR_CHECK(led_animation_stop(BLINK_COUPLING));

        /* Test led animation "sending data" */
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Start sending data animation");
        ESP_ERROR_CHECK(led_animation_start(BLINK_SENDING_DATA));
        vTaskDelay(3500 / portTICK_PERIOD_MS);
    }
}