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

/* SmartMeter sends the data every 5s, so wait until all data send and then process it */
/* This parameter defines timeout threshold in uart symbol periods(time of sending one byte). */
/* 2400 Baud = 300 Byte/s -> Timout = 126/300 = 0,42s */
/* https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html#_CPPv419uart_set_rx_timeout11uart_port_tK7uint8_t */
/* https://www.esp32.com/viewtopic.php?t=21294 */
#define RX_TIMEOUT_TRESHOLD             126 /* < The maximum value of threshold is 126 */

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

#ifdef __cplusplus
} // extern "C"
#endif
