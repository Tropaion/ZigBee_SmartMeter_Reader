/**
 * @file uart.h
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
#define POWER_FAIL_SIGNAL_GPIO          21          /* < GPIO Pin where MBUS-to-UART-Converter indicates power fail */

/* ===== UART CONFIGURATION ===== */
#define UART_PORT_NUMBER                UART_NUM_1  /* < UART Port from which data is read */
#define UART_RX_GPIO                    22          /* < GPIO Pin where UART receives data */
#define UART_TX_GPIO                    23          /* < GPIO Pin where UART sends data (unused) */
#define UART_BAUD_RATE                  2400        /* < Baudrate of the connected meter */

/* For example the Sagemcom T210-D sends two frames every 5 seconds */
/* After receiving the first byte, all bytes are collected in a buffer */
/* If no new bytes is receive for a time of UART_RX_TIMEOUT, data will be parsed */
#define UART_RX_TIMEOUT                 1000        /* < Time to wait before received bytes are processed */

#ifdef __cplusplus
} // extern "C"
#endif
