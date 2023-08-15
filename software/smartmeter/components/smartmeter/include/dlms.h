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

/* ===== DLMS PARSER CONFIGURATION ===== */
/* == INFO: OFFSETS ARE ALWAYS CALCULATED BEGINNING FROM THE RELEVANT LAYER == */
#define DLMS_MAX_SIZE                   (MBUS_MAX_SIZE - MBUS_HEADER_LENGTH - MBUS_FOOTER_LENGTH)

/* This part can be different for other smartmeters */
#define DLMS_FRAME_COUNTER_OFFSET       15
#define DLMS_FRAME_COUNTER_SIZE         4

#define DLMS_DATA_START_OFFSET          2
#define DLMS_DATA_START_VAL1            0x01
#define DLMS_DATA_START_VAL2            0x67

/* Only for first user data packet */
#define DLMS_ENCRYPTION_TYPE_OFFSET     2
#define DLMS_ENCRYPTION_TYPE_VALUE      0xDB        /* < General-Glo-Cyphering */

#define DLMS_SYSTEM_TITLE_LENGTH_OFFSET 3
#define DLMS_SYSTEM_TITLE_OFFSET        4

#define DLMS_UNKNOWN_SIZE               3           /* < 3 Bytes after system title, always 0x81F820 */
#define DLMS_FRAME_COUNTER_SIZE         4

/* ===== DECRYPTION CONFIGURATION ===== */
#define GUE_KEY_LENGTH                  16

#define AES_IV_SIZE                     12
#define AES_IV_SYST_LENGTH_OFFSET       1           /* < Offset at which length of system title is stored */

/* Decryption key */
const uint8_t gue_key[GUE_KEY_LENGTH] = {0x52, 0xFC, 0xF7, 0x0B, 0xC3, 0x94, 0x9B, 0x61, 0x3E, 0xC9, 0xE7, 0xB1, 0x5F, 0xAD, 0x55, 0xEF};
            
#ifdef __cplusplus
} // extern "C"
#endif
