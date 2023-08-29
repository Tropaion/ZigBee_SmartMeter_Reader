/**
 * @file mbus.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <string.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "MBUS";

/* Header */
#include "general.h"
#include "mbus.h"

/* ===== M-BUS-Layer ===== */
esp_err_t parse_mbus_long_frame_layer(uint8_t* payload, size_t payload_size, uint8_t* user_data, size_t* user_data_size)
{
    /* Offset if multiple frames need to be parsed */
    uint16_t curr_offset = 0;
    
    /* New data, set user data size to zero */
    *user_data_size = 0;
    
    /* Loop throught payload and that minimum frame size does not exceed rest of payload */
    while(curr_offset + MBUS_HEADER_LENGTH + MBUS_FOOTER_LENGTH < payload_size)
    {
        /* Check start fields integrity */
        if((payload[curr_offset + MBUS_START1_OFFSET] != MBUS_START_VALUE) || (payload[curr_offset + MBUS_START2_OFFSET] != MBUS_START_VALUE))
        {
            ESP_LOGE(TAG, "Invalid start bytes!");
            return ESP_FAIL;
        }
        
        /* Check L-fields integrity (length) */
        if(payload[curr_offset + MBUS_LENGTH1_OFFSET] != payload[curr_offset + MBUS_LENGTH2_OFFSET])
        {
            ESP_LOGE(TAG, "Length byes do not match!");
            return ESP_FAIL;
        }
        
        /* Get user data size (l-field) */
        uint8_t l_field = payload[curr_offset + MBUS_LENGTH1_OFFSET] - MBUS_USER_DATA_SIZE_OFFSET;
        
        /* Check Stop-field integrity */
        if(payload[curr_offset + MBUS_HEADER_LENGTH + l_field + MBUS_FOOTER_LENGTH - 1] != MBUS_STOP_VALUE)
        {
            ESP_LOGE(TAG, "Invalid stop byte!");
            return ESP_FAIL;
        }
        
        /* Frame check passed, everything ok, copy user data to buffer */
        memcpy(&user_data[*user_data_size], &payload[curr_offset + MBUS_USER_DATA_OFFSET], l_field);
     
        /* Set offset to next frame */
        curr_offset += MBUS_HEADER_LENGTH + l_field + MBUS_FOOTER_LENGTH;
        
        /* Increase user data size */
        *user_data_size += l_field;
    }
    return ESP_OK;
}