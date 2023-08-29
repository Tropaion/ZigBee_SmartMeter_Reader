/**
 * @file general.h
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// Multiple inclusion protection
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ===== GENERAL CONFIGURATION ===== */
/* General buffer size for data handling */
#define DATA_BUFFER_SIZE                1024            

/* Parsing/Update interval of data */
/* e.g. smartmeter sends data every 5s * DATA_UPDATE_INTERVAL = data is sent via ZigBee every 10 seconds */
#define DATA_UPDATE_INTERVAL            2

/* Length of decryption key in bytes */
#define GUE_KEY_LENGTH                  16              

/* Decryption key */
static const uint8_t decryption_key[GUE_KEY_LENGTH] = {0x52, 0xFC, 0xF7, 0x0B, 0xC3, 0x94, 0x9B, 0x61, 0x3E, 0xC9, 0xE7, 0xB1, 0x5F, 0xAD, 0x55, 0xEF};

#ifdef __cplusplus
} // extern "C"
#endif
