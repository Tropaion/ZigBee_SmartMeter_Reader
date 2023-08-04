/**
 * @file dlms.h
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

/* ===== DLMS CHIP CONFIGURATION ===== */
#define POWER_FAIL_SIGNAL_GPIO          21

/* ===== UART CONFIGURATION ===== */
#define UART_PORT_NUMBER                UART_NUM_1
#define UART_RX_GPIO                    22
#define UART_TX_GPIO                    23
#define UART_BUFFER_SIZE                1024
#define UART_BAUD_RATE                  2400

#define UART_EVENT_BUFFER_SIZE          (UART_BUFFER_SIZE)

/**
 * @brief Initialize uart and dlms
 * 
 * @return esp_err_t 
 */
esp_err_t dlms_init();

#ifdef __cplusplus
} // extern "C"
#endif
