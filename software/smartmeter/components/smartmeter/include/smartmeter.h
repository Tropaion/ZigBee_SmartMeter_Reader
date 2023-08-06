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

/**
 * @brief Initialize uart and dlms
 * 
 * @return esp_err_t 
 */
esp_err_t smartmeter_init();

#ifdef __cplusplus
} // extern "C"
#endif
