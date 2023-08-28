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

/* ===== OBIS Layer ===== */
static esp_err_t parse_obis(uint8_t* obis_data, size_t obis_data_size)
{
    size_t curr_offset = 0;
    
    /* === CHECK OBIS HEADER === */
    /* Check for obis start byte */
    if(obis_data[curr_offset] != OBIS_HEADER_START)
    {
        ESP_LOGE(TAG, "OBIS: start byte invalid");
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
        ESP_LOGE(TAG, "OBIS: header date time length invalid");
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
    /* parse_obis_data_type */
    
    /* Check which data type to expect */
    switch(obis_data[curr_offset])
    {
        case NullData:
            /* No following data, skip */
            curr_offset++;
            break;
        case Array:
            /* Check how many entries are in array */
            uint8_t len = obis_data[curr_offset + 1];
            /* Loop through array */
            //TODO: Recursive loop?
            break;
        case OctetString:
            /* Check length of octet string */
            if(obis_data[curr_offset + 1] == OBIS_DATE_TIME_LENGTH)
            {
                /* String is <DateTime Value>, skip */
                curr_offset += 2 + OBIS_DATE_TIME_LENGTH;
                ESP_LOGI(TAG, "OBIS: Skipped <DateTime Value>");
            }
            else if(obis_data[curr_offset + 1] == OBIS_CODE_LENGTH)
            {
                /* Important, this is what we want */
                //TODO: Parse obis code
            }
            else
            {
                /* Unknown octet string, skip */
                curr_offset += 2 + obis_data[curr_offset + 1];
            }
            break;
        default:
            ESP_LOGE(TAG, "OBIS: Unsupported data type");
            return ESP_FAIL;
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
static esp_err_t parse_dlms_layer(uint8_t* user_data, size_t user_data_size, uint8_t* decrypted_data, size_t* decrypted_data_size, const uint8_t* gue_key)
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

/* ===== M-BUS-Layer ===== */
/**
 * @brief Parser for MBUS-Layer, only long frames!
 * 
 * @param payload data from physical layer
 * @param payload_size size of data from physical layer
 * @param user_data extracted user data
 * @param user_data_size size of extracted user data
 */
static esp_err_t parse_mbus_long_frame_layer(uint8_t* payload, size_t payload_size, uint8_t* user_data, size_t* user_data_size)
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

/* ===== Physical Layer (UART) ===== */
/* UART Event Handler */
static void uart_event_task(void *pvParameters)
{
    /* Store current event */
    uart_event_t event;

    /* First buffer for processing data */
    static uint8_t buff0[DATA_BUFFER_SIZE];

    /* Second buffer for processing data */
    static uint8_t buff1[DATA_BUFFER_SIZE];

    /* Current measurement interval */
    static uint8_t curr_interval = DATA_UPDATE_INTERVAL;

    /* Ticks to wait before received bytes are processed */
    const TickType_t timeout_ticks = UART_RX_TIMEOUT / portTICK_PERIOD_MS;

    for(;;)
    {
        /* Remember if first data after timeout was received */
        bool data_received = false;

        /* Loop infinitely and check for event */
        /* Wait indefinitely for the first byte(s), but not more than timeout_ms for all other bytes */
        while(xQueueReceive(uart1_queue, (void *)&event, data_received ? timeout_ticks : portMAX_DELAY))
        {
            /* Switch according to event type */
            switch(event.type)
            {
                /* Received data */
                case UART_DATA:
                    /* Set data received to true */
                    data_received = true;
                    break;

                /* FIFO Overflow */
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "UART: FIFO overflow");

                    /* Flush ringbuffer */
                    uart_flush_input(UART_PORT_NUMBER);

                    /* Reset event queue */
                    xQueueReset(uart1_queue);
                    break;

                /* Ringbuffer full */
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "UART: Ringbuffer full");
                    
                    /* Flush ringbuffer */
                    uart_flush_input(UART_PORT_NUMBER);

                    /* Reset event queue */
                    xQueueReset(uart1_queue);
                    break;

                /* RX break detected */
                case UART_BREAK:
                    ESP_LOGI(TAG, "UART: RX Break");
                    break;

                /* Parity check error */
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "UART: Parity Error");
                    break;

                /* Frame error */
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "UART: Frame Error");
                    break;

                /* Other events */
                default:
                    /* Write event to log */
                    ESP_LOGI(TAG, "UART: Event Type: %d", event.type);
                    break;
            }
        }

        /* Increment measurement interval */
        curr_interval++;

        /* Set buffer sizes */
        size_t buff0_size = 0;
        size_t buff1_size = 0;

        /* No new data received within the given time UART_RX_TIMEOUT */
        /* Check how many bytes were received */ 
        uart_get_buffered_data_len(UART_PORT_NUMBER, &buff0_size);

        /* Check if received bytes are more than minimum of MBUS frame and if a new measurement should be made */
        if(buff0_size >= (MBUS_HEADER_LENGTH + MBUS_FOOTER_LENGTH) && curr_interval >= DATA_UPDATE_INTERVAL)
        {
            /* Write bytes to buffer0 */
            uart_read_bytes(UART_PORT_NUMBER, &buff0, buff0_size, portMAX_DELAY);

            /* Process received data from buffer0 and write to buffer1 */
            esp_err_t err = parse_mbus_long_frame_layer(&buff0[0], buff0_size, &buff1[0], &buff1_size);
            
            /* Check if mbus parsing was successfull */
            if(err == ESP_OK)
            {
                /* Process mbus data from buffer1 and write to buffer0 */
                err = parse_dlms_layer(&buff1[0], buff1_size, &buff0[0], &buff0_size, &gue_key[0]);
            }

            //TEST: PRINT DATA
            printf("Decrypted data size: %d\n", buff0_size);
        	for (int i = 0; i < buff0_size; i++)
        	{
        	    printf("%02X", buff0[i]);
        	}
            printf("\n");

            /* Check if dlms parsing was successfull */
            if(err == ESP_OK)
            {
                /* Process dlms data from buffer0 */
                err = parse_obis(&buff0[0], buff0_size);
            }
            
            /* Check if parsing failed */
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "UART: Parsing failed.");
                
                /* Flush ringbuffer */
                uart_flush_input(UART_PORT_NUMBER);

                /* Reset event queue */
                xQueueReset(uart1_queue);
            }
            else
            {
                /* Measurement successfull, reset interval */
                curr_interval = 0;
            }
        }
    }
    /* Delete this task */
    vTaskDelete(NULL);
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

    /* Set communication pins */
    err = uart_set_pin(UART_PORT_NUMBER, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if(err != ESP_OK){ return err; }

    /* Install driver */
    err = uart_driver_install(UART_PORT_NUMBER, DATA_BUFFER_SIZE, 0, 20, &uart1_queue, 0);
    if(err != ESP_OK){ return err; }

    /* Create a task to handle events */
    xTaskCreate(uart_event_task, "uart_event_task", ((3 * DATA_BUFFER_SIZE) + 2048), NULL, 12, NULL);

    return err;
}