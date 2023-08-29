/**
 * @file dlms.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <string.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "DLMS";

/* Header */
#include "general.h"
#include "dlms.h"

/* Encryption Library */
#include "mbedtls/gcm.h"

/* ===== DLMS Layer ===== */
esp_err_t parse_dlms_layer(uint8_t* user_data, size_t user_data_size, uint8_t* decrypted_data, size_t* decrypted_data_size, const uint8_t* gue_key)
{
    /* Encrypted data buffer */
    static uint8_t encrypted_data[DATA_BUFFER_SIZE];
    uint16_t encrypted_data_size = 0;

    /* ===== GET RELEVANT DATA AND COMBINE FRAMES ===== */
    /* This part can be different for other smartmeters */

    /* === HANDLE FIRST FRAME OF DLMS DATA === */
    /* Check for data packet start value */
    if((user_data[0] != DLMS_START_VAL1) || (user_data[1] != DLMS_START_VAL2))
    {
        ESP_LOGE(TAG, "DLMS: Invalid packet start value");
        return ESP_FAIL;
    }

    /* Check encryption type */
    if(user_data[DLMS_ENCRYPTION_TYPE_OFFSET] != DLMS_ENCRYPTION_TYPE_VALUE)
    {
        ESP_LOGE(TAG, "DLMS: Encryption type not supported");
        return ESP_FAIL;
    }

    /* Get system title length */
    uint8_t title_length = user_data[DLMS_SYSTEM_TITLE_LENGTH_OFFSET];

    /* Calculate offset via title length */
    uint8_t curr_offset = DLMS_SYSTEM_TITLE_OFFSET + title_length + DLMS_UNKNOWN_SIZE + DLMS_FRAME_COUNTER_SIZE;

    /* Calculate size to copy */
    if(user_data_size < DLMS_MAX_SIZE)
    {
        encrypted_data_size = user_data_size - curr_offset;
    }
    else
    {
        encrypted_data_size = DLMS_MAX_SIZE - curr_offset;
    }

    /* Copy first frame */
    memcpy(&encrypted_data[0], &user_data[DLMS_SYSTEM_TITLE_OFFSET + title_length + DLMS_UNKNOWN_SIZE + DLMS_FRAME_COUNTER_SIZE], encrypted_data_size);

    /* === HANDLE SUBSEQUENT FRAMES === */
    for(int i = DLMS_MAX_SIZE; user_data_size > i; i += DLMS_MAX_SIZE)
    {
        /* Check for data packet start value */
        if((user_data[i] != DLMS_START_VAL1) || (user_data[i + 1] != DLMS_START_VAL2))
        {
            ESP_LOGE(TAG, "DLMS: Invalid packet start value");
            return ESP_FAIL;
        }

        /* Calculate frame size */
        uint16_t frame_size;
        if(user_data_size > (i + DLMS_MAX_SIZE))
        {
            frame_size = DLMS_MAX_SIZE - DLMS_DATA_START_OFFSET;
        }
        else
        {
            frame_size = user_data_size - i - DLMS_DATA_START_OFFSET;
        }
        /* Copy frame */
        memcpy(&encrypted_data[encrypted_data_size], &user_data[i + DLMS_DATA_START_OFFSET], frame_size);
        
        /* Increment data size */
        encrypted_data_size += frame_size;
    }

    /* ===== DECRYPT DATA ===== */
    /* Create initialization vector */
    uint8_t iv[AES_IV_SIZE];

    /* Copy system title to the beginning of iv */
    memcpy(&iv[0], &user_data[DLMS_SYSTEM_TITLE_OFFSET], title_length);

    /* Copy frame counter to the end of iv */
    memcpy(&iv[AES_IV_SIZE - DLMS_FRAME_COUNTER_SIZE], &user_data[DLMS_FRAME_COUNTER_OFFSET], DLMS_FRAME_COUNTER_SIZE);

    /* Create context structure */
    mbedtls_gcm_context aes;

    /* Initialize context structure */
    mbedtls_gcm_init(&aes);

    /* Set decryption key */
    mbedtls_gcm_setkey(&aes, MBEDTLS_CIPHER_ID_AES, gue_key, GUE_KEY_LENGTH * 8);

    /* Decrypt data */
    mbedtls_gcm_auth_decrypt(&aes, encrypted_data_size, &iv[0], AES_IV_SIZE, NULL, 0, NULL, 0, &encrypted_data[0], decrypted_data);
    *decrypted_data_size = encrypted_data_size;

    /* Free contest structure */
    mbedtls_gcm_free(&aes);

    return ESP_OK;
}