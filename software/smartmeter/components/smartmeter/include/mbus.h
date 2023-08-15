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

/* ===== MBUS CHIP CONFIGURATION ===== */
#define POWER_FAIL_SIGNAL_GPIO          21

/* ===== UART CONFIGURATION ===== */
#define UART_PORT_NUMBER                UART_NUM_1
#define UART_RX_GPIO                    22
#define UART_TX_GPIO                    23
#define UART_BAUD_RATE                  2400

/* For example the Sagemcom T210-D sends two frames every 10 seconds */
/* After receiving the first byte, all bytes are collected in a buffer */
/* If no new bytes is receive for a time of UART_RX_TIMEOUT, data will be parsed */
#define UART_RX_TIMEOUT                 1000        /* < Time to wait before received bytes are processed */

/* === M-BUS PARSER CONFIGURATION === */
#define MBUS_MAX_SIZE                   256

#define MBUS_HEADER_LENGTH              7           /* < Number of bytes before MBUS data */
#define MBUS_FOOTER_LENGTH              2           /* < Number of bytes after MBUS data */

#define MBUS_START_VALUE                0x68        /* < Value of MBUS start indicator */
#define MBUS_START1_OFFSET              0           /* < Position of first MBUS Start-Value*/
#define MBUS_START2_OFFSET              3           /* < Position of second MBUS Start-Value*/

#define MBUS_LENGTH1_OFFSET             1           /* < Position of first MBUS Length of Frame Value */
#define MBUS_LENGTH2_OFFSET             2           /* < Position of second MBUS Length of Frame Value */

#define MBUS_USER_DATA_SIZE_OFFSET      3           /* < C-Field, A-Field and CI-Field */
#define MBUS_USER_DATA_OFFSET           7           /* < Position where MBUS Data starts */

#define MBUS_STOP_OFFSET                2           /* < Offset added to the position of last data byte */
#define MBUS_STOP_VALUE                 0x16        /* < Value of MBUS stop indicator */

#ifdef __cplusplus
} // extern "C"
#endif
