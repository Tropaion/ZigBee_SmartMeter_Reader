/**
 * @file mbus.h
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

/* === M-BUS PARSER CONFIGURATION === */
#define MBUS_MAX_SIZE                   256         /* < Maximum size of an MBUS frame */

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

/**
 * @brief Parser for MBUS-Layer, only long frames!
 * 
 * @param payload data from physical layer
 * @param payload_size size of data from physical layer
 * @param user_data extracted user data
 * @param user_data_size size of extracted user data
 */
esp_err_t parse_mbus_long_frame_layer(uint8_t* payload, size_t payload_size, uint8_t* user_data, size_t* user_data_size);

#ifdef __cplusplus
} // extern "C"
#endif
