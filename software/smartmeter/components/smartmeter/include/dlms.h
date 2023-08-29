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

/* ===== DLMS PARSER CONFIGURATION ===== */
/* == INFO: OFFSETS ARE ALWAYS CALCULATED FROM THE BEGINNING OF THE RELEVANT LAYER == */
#define DLMS_MAX_SIZE                   247         /* < Maximum size of one dlms frame = (MBUS_MAX_SIZE - MBUS_HEADER_LENGTH - MBUS_FOOTER_LENGTH) */

/* This part can be different for other smartmeters */
#define DLMS_FRAME_COUNTER_OFFSET       15          /* < Position of first frame counter byte */
#define DLMS_FRAME_COUNTER_SIZE         4           /* < Size of frame counter in bytes */

#define DLMS_START_VAL1                 0x01        /* < Value of first byte of dlms start indicator */
#define DLMS_START_VAL2                 0x67        /* < Value of second byte of dlms start indicator */

/* Only for first user data packet */
#define DLMS_ENCRYPTION_TYPE_OFFSET     2
#define DLMS_ENCRYPTION_TYPE_VALUE      0xDB        /* < General-Glo-Cyphering */

#define DLMS_SYSTEM_TITLE_LENGTH_OFFSET 3           /* < Position of system title length byte */
#define DLMS_SYSTEM_TITLE_OFFSET        4           /* < Position of system title */

#define DLMS_UNKNOWN_SIZE               3           /* < 3 Bytes after system title, always 0x81F820 */

/* Only for subsequent user data packets */
#define DLMS_DATA_START_OFFSET          2           /* < Offset where user data begins when receiving subsequent frames after first one */

/* ===== DECRYPTION CONFIGURATION ===== */
#define AES_IV_SIZE                     12          /* < Size of initialization vector */
#define AES_IV_SYST_LENGTH_OFFSET       1           /* < Offset at which the length of the system title is stored in the initialization vector */

/**
 * @brief Parser for DLMS-Layer
 * 
 * @param user_data user data from mbus layer
 * @param user_data_size size of user data
 * @param decrypted_data decrypted user data
 * @param decrypted_data_size size of decrypted user data
 * @param gue_key key used for decryption
 * @return esp_err_t 
 */
esp_err_t parse_dlms_layer(uint8_t* user_data, size_t user_data_size, uint8_t* decrypted_data, size_t* decrypted_data_size, const uint8_t* gue_key);
            
#ifdef __cplusplus
} // extern "C"
#endif
