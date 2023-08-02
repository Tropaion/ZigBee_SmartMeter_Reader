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

#include "dlms.h"

void app_main(void)
{
    ESP_ERROR_CHECK(dlms_init());
}