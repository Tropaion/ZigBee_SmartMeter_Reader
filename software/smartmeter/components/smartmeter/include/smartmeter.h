/**
 * @file smartmeter.h
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

/* ===== GENERAL CONFIGURATION ===== */
#define DATA_BUFFER_SIZE                1024            /* < General buffer size for data handling */

/* Parsing/Update interval of data */
/* e.g. smartmeter sends data every 5s * DATA_UPDATE_INTERVAL = data is sent via ZigBee every 10 seconds */
#define DATA_UPDATE_INTERVAL            2

/**
 * @brief Initialize uart and dlms
 * 
 * @return esp_err_t 
 */
esp_err_t smartmeter_init();

#ifdef __cplusplus
} // extern "C"
#endif
