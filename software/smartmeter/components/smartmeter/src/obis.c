/**
 * @file obis.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <string.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "OBIS";

/* Header */
#include "obis.h"

/* ===== OBIS Layer ===== */
esp_err_t parse_obis_data_type(uint8_t* obis_data, size_t* curr_offset)
{
    /* Check which data type to expect */
    switch(obis_data[*curr_offset])
    {
        case NullData:
            /* No following data, skip */
            (*curr_offset)++;
            break;
        case Array:
            /* Check how many entries are in array */
            uint8_t len = obis_data[*curr_offset + 1];
            /* Loop through array */
            //TODO: Recursive loop?
            break;
        case OctetString:
            /* Check length of octet string */
            if(obis_data[*curr_offset + 1] == OBIS_DATE_TIME_LENGTH)
            {
                /* String is <DateTime Value>, skip */
                *curr_offset += 2 + OBIS_DATE_TIME_LENGTH;
                ESP_LOGI(TAG, "Skipped <DateTime Value>");
            }
            else if(obis_data[*curr_offset + 1] == OBIS_CODE_LENGTH)
            {
                /* Important, this is what we want */
                //TODO: Parse obis code and save in structure?
                //Structure could be uint32 + divisor, when divisor = 1 -> ganzzahlig
            }
            else
            {
                /* Unknown octet string, skip */
                *curr_offset += 2 + obis_data[*curr_offset + 1];
            }
            break;
        default:
            ESP_LOGE(TAG, "Unsupported data type");
            return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t parse_obis(uint8_t* obis_data, size_t obis_data_size)
{
    size_t curr_offset = 0;
    
    /* === CHECK OBIS HEADER === */
    /* Check for obis start byte */
    if(obis_data[curr_offset] != OBIS_HEADER_START)
    {
        ESP_LOGE(TAG, "start byte invalid");
        return ESP_FAIL;
    }
    else
    {
        /* next byte */
        curr_offset++;
    }

    /* Skip <LongInvokeIdAndPriority> data */
    curr_offset += OBIS_HEADER_LONG_INVOKE_ID_PRIO_BYTES;

    /* Check if size of following DateTime string is correct */
    if(obis_data[curr_offset] != OBIS_DATE_TIME_LENGTH)
    {
        ESP_LOGE(TAG, "header date time length invalid");
        return ESP_FAIL;
    }
    else
    {
        /* next byte */
        curr_offset++;
    }

    /* Skip <DateTime Value> */
    curr_offset += OBIS_DATE_TIME_LENGTH;

    /* === CHECK OBIS NOTIFICATION BODY === */
    //TODO: Parse until footer?
    parse_obis_data_type(obis_data, &curr_offset);

    return ESP_OK;
}