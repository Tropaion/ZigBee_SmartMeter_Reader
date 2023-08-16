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
#define GUE_KEY_LENGTH                  16          /* < Length of decryption key in bytes */

#define AES_IV_SIZE                     12          /* < Size of initialization vector */
#define AES_IV_SYST_LENGTH_OFFSET       1           /* < Offset at which the length of the system title is stored in the initialization vector */

/* Decryption key */
const uint8_t gue_key[GUE_KEY_LENGTH] = {0x52, 0xFC, 0xF7, 0x0B, 0xC3, 0x94, 0x9B, 0x61, 0x3E, 0xC9, 0xE7, 0xB1, 0x5F, 0xAD, 0x55, 0xEF};
            
#ifdef __cplusplus
} // extern "C"
#endif
