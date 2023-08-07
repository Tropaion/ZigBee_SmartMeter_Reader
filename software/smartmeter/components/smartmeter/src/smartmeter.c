/**
 * @file smartmeter.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <string.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "smartmeter";

/* Header */
#include "smartmeter.h"
#include "mbus.h"
#include "dlms.h"
#include "obis.h"

/* FreeRTOS Libraries */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* UART Library */
#include "driver/uart.h"
#include "hal/uart_hal.h" /* for interrupt mask defines */

/* Encryption Library */
#include "mbedtls/gcm.h"

/* UART Event Queue */
static QueueHandle_t uart1_queue = NULL;

/* SMALL INFODUMP */
/* Structure of data and how it's processed */
/* 1. Physical Layer -> UART */
/* 2. MBUS-Layer -> parse with "parse_mbus_long_frame_layer", supports multiple frames, returns user data */
/* 3. DLMS (Application)-Layer -> decrypt and parse with "todo" */

/* ===== Physical Layer (UART) ===== */
/* UART Event Handler */
//TODO: Timeout Event
static void uart_event_task(void *pvParameters)
{
    /* Store current event */
    uart_event_t event;

    /* Buffer for event data */
    uint8_t buff[DATA_BUFFER_SIZE];

    for(;;)
    {
        /* Loop infinitely and check for event */
        if(xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            /* Set buffer to zero */
            bzero(&buff, DATA_BUFFER_SIZE);

            /* Switch according to event type */
            switch(event.type)
            {
                /* Received data */
                case UART_DATA:
                    /* Receive bytes from uart */
                    uart_read_bytes(UART_PORT_NUMBER, &buff, event.size, portMAX_DELAY);

                    /* Dump bytes to console */
                    ESP_LOG_BUFFER_HEX(TAG, &buff, event.size);
                    break;
                /* Other events */
                default:
                    /* Write event to log */
                    ESP_LOGI(TAG, "UART Event Type: %d", event.type);
                    break;
            }
        }
    }
    /* Delete this task */
    vTaskDelete(NULL);
}

/* ===== M-BUS-Layer ===== */
/**
 * @brief Parser for MBUS-Layer, only long frames!
 * 
 * @param payload data from physical layer
 * @param payload_size size of data from physical layer
 * @param user_data extracted user data
 * @param user_data_size size of extracted user data
 */
static esp_err_t parse_mbus_long_frame_layer(uint8_t* payload, uint16_t payload_size, uint8_t* user_data, uint16_t* user_data_size)
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
            ESP_LOGE(TAG, "MBUS: Invalid start bytes!");
            return ESP_FAIL;
        }
        
        /* Check L-fields integrity (length) */
        if(payload[curr_offset + MBUS_LENGTH1_OFFSET] != payload[curr_offset + MBUS_LENGTH2_OFFSET])
        {
            ESP_LOGE(TAG, "MBUS: Length byes do not match!");
            return ESP_FAIL;
        }
        
        /* Get user data size (l-field) */
        uint8_t l_field = payload[curr_offset + MBUS_LENGTH1_OFFSET] - MBUS_USER_DATA_SIZE_OFFSET;
        
        /* Check Stop-field integrity */
        if(payload[curr_offset + MBUS_HEADER_LENGTH + l_field + MBUS_FOOTER_LENGTH - 1] != MBUS_STOP_VALUE)
        {
            ESP_LOGE(TAG, "MBUS: Invalid stop byte!");
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

/* ===== DLMS Layer ===== */
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
static esp_err_t parse_dlms_layer(uint8_t* user_data, uint16_t user_data_size, uint8_t* decrypted_data, uint16_t* decrypted_data_size, uint8_t* gue_key)
{
    /* Encrypted data buffer */
    uint8_t encrypted_data[DATA_BUFFER_SIZE];
    uint16_t encrypted_data_size = 0;

    /* ===== GET RELEVANT DATA AND COMBINE FRAMES ===== */
    /* This part can be different for other smartmeters */

    /* === HANDLE FIRST FRAME OF DLMS DATA === */
    /* Check for data packet start value */
    if((user_data[0] != DLMS_DATA_START_VAL1) || (user_data[1] != DLMS_DATA_START_VAL2))
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
        if((user_data[i] != DLMS_DATA_START_VAL1) || (user_data[i + 1] != DLMS_DATA_START_VAL2))
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

    printf("Encrypted data Size: %d\n", encrypted_data_size);
    for (int i = 0; i < encrypted_data_size; i++)
    {
        printf("%02X", encrypted_data[i]);
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

    printf("\n\nDecrypted data size: %d\n", *decrypted_data_size);
    for (int i = 0; i < *decrypted_data_size; i++)
    {
        printf("%02X", decrypted_data[i]);
    }
    printf("\n");

    return ESP_OK;
}

/* ===== OBIS Layer ===== */
static esp_err_t parse_obis(uint8_t* obis_data, uint16_t obis_data_size)
{
    /* Check for obis data integrity */
    if(obis_data[OBIS_INTEGRITY_VAL1_OFFSET] != OBIS_INTEGRITY_VAL1 || obis_data[OBIS_INTEGRITY_VAL2_OFFSET] != OBIS_INTEGRITY_VAL2)
    {
        ESP_LOGE(TAG, "OBIS: Decrypted obis data invalid");
        return ESP_FAIL;
    }

    /* Decode data */
    int curr_offset = 0;

    return ESP_OK;
}

esp_err_t smartmeter_init()
{
    /* === CONFIGURE UART ===*/
    /* Create basic configuration */
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* Set configuration */
    esp_err_t err = uart_param_config(UART_PORT_NUMBER, &uart_config);
    if(err != ESP_OK){ return err; }

    /* Create interrupt configuration */
    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_INTR_RXFIFO_TOUT,
        .rx_timeout_thresh = RX_TIMEOUT_TRESHOLD,
    };

    /* Set interrupt configuration */
    err = uart_intr_config(UART_PORT_NUMBER, &uart_intr);
    if(err != ESP_OK){ return err; }

    /* Enable interrupts */
    err = uart_enable_rx_intr(UART_PORT_NUMBER);
    if(err != ESP_OK){ return err; }

    /* Set communication pins */
    err = uart_set_pin(UART_PORT_NUMBER, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if(err != ESP_OK){ return err; }

    /* Install driver */
    err = uart_driver_install(UART_PORT_NUMBER, DATA_BUFFER_SIZE, 0, 20, &uart1_queue, 0);
    if(err != ESP_OK){ return err; }

    /* Create a task to handle events */
    xTaskCreate(uart_event_task, "uart_event_task", DATA_BUFFER_SIZE + 2048, NULL, 12, NULL);

    /* Configuration values */
    uint16_t payload_size = 282;
    uint8_t gue_key[GUE_KEY_LENGTH] = {0x36, 0xC6, 0x66, 0x39, 0xE4, 0x8A, 0x8C, 0xA4, 0xD6, 0xBC, 0x8B, 0x28, 0x2A, 0x79, 0x3B, 0xBB};
    uint8_t payload[] = {0x68, 0xFA, 0xFA, 0x68, 0x53, 0xFF, 0x00, 0x01, 0x67, 0xDB, 0x08, 0x4B, 0x46, 0x4D, 0x67, 0x50, 0x00, 0x00, 0x09, 0x81, 0xF8, 0x20, 0x00, 0x00, 0x00, 0x23, 0x88, 0xD5, 0xAB, 0x4F, 0x97, 0x51, 0x5A, 0xAF, 0xC6, 0xB8, 0x8D, 0x2F, 0x85, 0xDA, 0xA7, 0xA0, 0xE3, 0xC0, 0xC4, 0x0D, 0x00, 0x45, 0x35, 0xC3, 0x97, 0xC9, 0xD0, 0x37, 0xAB, 0x7D, 0xBD, 0xA3, 0x29, 0x10, 0x76, 0x15, 0x44, 0x48, 0x94, 0xA1, 0xA0, 0xDD, 0x7E, 0x85, 0xF0, 0x2D, 0x49, 0x6C, 0xEC, 0xD3, 0xFF, 0x46, 0xAF, 0x5F, 0xB3, 0xC9, 0x22, 0x9C, 0xFE, 0x8F, 0x3E, 0xE4, 0x60, 0x6A, 0xB2, 0xE1, 0xF4, 0x09, 0xF3, 0x6A, 0xAD, 0x2E, 0x50, 0x90, 0x0A, 0x43, 0x96, 0xFC, 0x6C, 0x2E, 0x08, 0x3F, 0x37, 0x32, 0x33, 0xA6, 0x96, 0x16, 0x95, 0x07, 0x58, 0xBF, 0xC7, 0xD6, 0x3A, 0x9E, 0x9B, 0x6E, 0x99, 0xE2, 0x1B, 0x2C, 0xBC, 0x2B, 0x93, 0x47, 0x72, 0xCA, 0x51, 0xFD, 0x4D, 0x69, 0x83, 0x07, 0x11, 0xCA, 0xB1, 0xF8, 0xCF, 0xF2, 0x5F, 0x0A, 0x32, 0x93, 0x37, 0xCB, 0xA5, 0x19, 0x04, 0xF0, 0xCA, 0xED, 0x88, 0xD6, 0x19, 0x68, 0x74, 0x3C, 0x84, 0x54, 0xBA, 0x92, 0x2E, 0xB0, 0x00, 0x38, 0x18, 0x2C, 0x22, 0xFE, 0x31, 0x6D, 0x16, 0xF2, 0xA9, 0xF5, 0x44, 0xD6, 0xF7, 0x5D, 0x51, 0xA4, 0xE9, 0x2A, 0x1C, 0x4E, 0xF8, 0xAB, 0x19, 0xA2, 0xB7, 0xFE, 0xAA, 0x32, 0xD0, 0x72, 0x6C, 0x0E, 0xD8, 0x02, 0x29, 0xAE, 0x6C, 0x0F, 0x76, 0x21, 0xA4, 0x20, 0x92, 0x51, 0xAC, 0xE2, 0xB2, 0xBC, 0x66, 0xFF, 0x03, 0x27, 0xA6, 0x53, 0xBB, 0x68, 0x6C, 0x75, 0x6B, 0xE0, 0x33, 0xC7, 0xA2, 0x81, 0xF1, 0xD2, 0xA7, 0xE1, 0xFA, 0x31, 0xC3, 0x98, 0x3E, 0x15, 0xF8, 0xFD, 0x16, 0xCC, 0x57, 0x87, 0xE6, 0xF5, 0x17, 0x16, 0x68, 0x14, 0x14, 0x68, 0x53, 0xFF, 0x11, 0x01, 0x67, 0x41, 0x9A, 0x3C, 0xFD, 0xA4, 0x4B, 0xE4, 0x38, 0xC9, 0x6F, 0x0E, 0x38, 0xBF, 0x83, 0xD9, 0x83, 0x16};

    uint16_t user_data_size = 0;
    uint8_t user_data[DATA_BUFFER_SIZE];

    parse_mbus_long_frame_layer(&payload[0], payload_size, &user_data[0], &user_data_size);

    uint16_t obis_data_size = 0;
    uint8_t obis_data[DATA_BUFFER_SIZE];

    parse_dlms_layer(&user_data[0], user_data_size, &obis_data[0], &obis_data_size, &gue_key[0]);

    return err;
}