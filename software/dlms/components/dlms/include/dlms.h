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

/* ===== GENERAL CONFIGURATION ===== */
#define DATA_BUFFER_SIZE                1024            /* < General buffer size for data handling */

/* ===== UART CONFIGURATION ===== */
#define UART_PORT_NUMBER                UART_NUM_1
#define UART_RX_GPIO                    22
#define UART_TX_GPIO                    23
#define UART_BAUD_RATE                  2400

/* === M-BUS PARSER CONFIGURATION === */
#define MBUS_MAX_SIZE                   256

#define MBUS_HEADER_LENGTH              7
#define MBUS_FOOTER_LENGTH              2

#define MBUS_START_VALUE                0x68
#define MBUS_START1_OFFSET              0
#define MBUS_START2_OFFSET              3

#define MBUS_LENGTH1_OFFSET             1
#define MBUS_LENGTH2_OFFSET             2

#define MBUS_USER_DATA_SIZE_OFFSET      3           /* < C-Field, A-Field and CI-Field */
#define MBUS_USER_DATA_OFFSET           7

#define MBUS_STOP_OFFSET                2
#define MBUS_STOP_VALUE                 0x16

/* ===== DLMS CONFIGURATION ===== */
#define DLMS_MAX_SIZE             (MBUS_MAX_SIZE - MBUS_HEADER_LENGTH - MBUS_FOOTER_LENGTH)

/* This part can be different for other smartmeters */
#define DLMS_FRAME_COUNTER_OFFSET       15
#define DLMS_FRAME_COUNTER_SIZE         4

#define DLMS_FIRST_FRAME_OFFSET         (DLMS_FRAME_COUNTER_OFFSET + DLMS_FRAME_COUNTER_SIZE)
#define DLMS_SECONDARY_FRAME_OFFSET     2

#define GUE_KEY_LENGTH                  16

/**
 * @brief Initialize uart and dlms
 * 
 * @return esp_err_t 
 */
esp_err_t dlms_init();

#ifdef __cplusplus
} // extern "C"
#endif
