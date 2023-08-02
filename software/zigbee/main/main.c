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

#include "zb_main.h"

void app_main(void)
{
    /* Start application */
    ESP_ERROR_CHECK(zb_run());
}