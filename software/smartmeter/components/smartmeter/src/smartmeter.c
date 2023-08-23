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
    /* Check for obis data integrity */
    if(obis_data[OBIS_INTEGRITY_VAL1_OFFSET] != OBIS_INTEGRITY_VAL1 || obis_data[OBIS_INTEGRITY_VAL2_OFFSET] != OBIS_INTEGRITY_VAL2)
    {
        ESP_LOGE(TAG, "OBIS: Decrypted obis data invalid");
        return ESP_FAIL;
    }

    /* Decode data */
    int curr_offset = 0;



    /* COPIED FROM SOMEWHERE ELSE!!!!!!!!!!! */
    ESP_LOGV(TAG, "Decoding payload");

    int currentPosition = DECODER_START_OFFSET;

    do
    {
        if(plaintext[currentPosition + OBIS_TYPE_OFFSET] != DataType::OctetString)
        {
            ESP_LOGE(TAG, "OBIS: Unsupported OBIS header type");
            return abort();
        }

        uint8_t obisCodeLength = plaintext[currentPosition + OBIS_LENGTH_OFFSET];

        if(obisCodeLength != 0x06)
        {
            ESP_LOGE(TAG, "OBIS: Unsupported OBIS header length");
            return abort();
        }

        uint8_t obisCode[obisCodeLength];
        memcpy(&obisCode[0], &plaintext[currentPosition + OBIS_CODE_OFFSET], obisCodeLength); // Copy OBIS code to array

        currentPosition += obisCodeLength + 2; // Advance past code, position and type

        uint8_t dataType = plaintext[currentPosition];
        currentPosition++; // Advance past data type

        uint8_t dataLength = 0x00;

        CodeType codeType = CodeType::Unknown;

        if(obisCode[OBIS_A] == Medium::Electricity)
        {
            // Compare C and D against code
            if(memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L1, 2) == 0)
            {
                codeType = CodeType::VoltageL1;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L2, 2) == 0)
            {
                codeType = CodeType::VoltageL2;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L3, 2) == 0)
            {
                codeType = CodeType::VoltageL3;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L1, 2) == 0)
            {
                codeType = CodeType::CurrentL1;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L2, 2) == 0)
            {
                codeType = CodeType::CurrentL2;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L3, 2) == 0)
            {
                codeType = CodeType::CurrentL3;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_POWER_PLUS, 2) == 0)
            {
                codeType = CodeType::ActivePowerPlus;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_POWER_MINUS, 2) == 0)
            {
                codeType = CodeType::ActivePowerMinus;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_ENERGY_PLUS, 2) == 0)
            {
                codeType = CodeType::ActiveEnergyPlus;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_ENERGY_MINUS, 2) == 0)
            {
                codeType = CodeType::ActiveEnergyMinus;
            }

            else if(memcmp(&obisCode[OBIS_C], ESPDM_REACTIVE_ENERGY_PLUS, 2) == 0)
            {
                codeType = CodeType::ReactiveEnergyPlus;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_REACTIVE_ENERGY_MINUS, 2) == 0)
            {
                codeType = CodeType::ReactiveEnergyMinus;
            }
            else
            {
                ESP_LOGW(TAG, "OBIS: Unsupported OBIS code");
            }
        }
        else if(obisCode[OBIS_A] == Medium::Abstract)
        {
            if(memcmp(&obisCode[OBIS_C], ESPDM_TIMESTAMP, 2) == 0)
            {
                codeType = CodeType::Timestamp;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_SERIAL_NUMBER, 2) == 0)
            {
                codeType = CodeType::SerialNumber;
            }
            else if(memcmp(&obisCode[OBIS_C], ESPDM_DEVICE_NAME, 2) == 0)
            {
                codeType = CodeType::DeviceName;
            }
            else
            {
                ESP_LOGW(TAG, "OBIS: Unsupported OBIS code");
            }
        }
        else
        {
            ESP_LOGE(TAG, "OBIS: Unsupported OBIS medium");
            return abort();
        }

        uint8_t uint8Value;
        uint16_t uint16Value;
        uint32_t uint32Value;
        float floatValue;

        switch(dataType)
        {
            case DataType::DoubleLongUnsigned:
                dataLength = 4;

                memcpy(&uint32Value, &plaintext[currentPosition], 4); // Copy bytes to integer
                uint32Value = swap_uint32(uint32Value); // Swap bytes

                floatValue = uint32Value; // Ignore decimal digits for now

                if(codeType == CodeType::ActivePowerPlus && this->active_power_plus != NULL && this->active_power_plus->state != floatValue)
                    this->active_power_plus->publish_state(floatValue);
                else if(codeType == CodeType::ActivePowerMinus && this->active_power_minus != NULL && this->active_power_minus->state != floatValue)
                    this->active_power_minus->publish_state(floatValue);

                else if(codeType == CodeType::ActiveEnergyPlus && this->active_energy_plus != NULL && this->active_energy_plus->state != floatValue)
                    this->active_energy_plus->publish_state(floatValue);
                else if(codeType == CodeType::ActiveEnergyMinus && this->active_energy_minus != NULL && this->active_energy_minus->state != floatValue)
                    this->active_energy_minus->publish_state(floatValue);

                else if(codeType == CodeType::ReactiveEnergyPlus && this->reactive_energy_plus != NULL && this->reactive_energy_plus->state != floatValue)
                    this->reactive_energy_plus->publish_state(floatValue);
                else if(codeType == CodeType::ReactiveEnergyMinus && this->reactive_energy_minus != NULL && this->reactive_energy_minus->state != floatValue)
                    this->reactive_energy_minus->publish_state(floatValue);

                break;
            case DataType::LongUnsigned:
                dataLength = 2;

                memcpy(&uint16Value, &plaintext[currentPosition], 2); // Copy bytes to integer
                uint16Value = swap_uint16(uint16Value); // Swap bytes

                if(plaintext[currentPosition + 5] == Accuracy::SingleDigit)
                    floatValue = uint16Value / 10.0; // Divide by 10 to get decimal places
                else if(plaintext[currentPosition + 5] == Accuracy::DoubleDigit)
                    floatValue = uint16Value / 100.0; // Divide by 100 to get decimal places
                else
                    floatValue = uint16Value; // No decimal places

                if(codeType == CodeType::VoltageL1 && this->voltage_l1 != NULL && this->voltage_l1->state != floatValue)
                    this->voltage_l1->publish_state(floatValue);
                else if(codeType == CodeType::VoltageL2 && this->voltage_l2 != NULL && this->voltage_l2->state != floatValue)
                    this->voltage_l2->publish_state(floatValue);
                else if(codeType == CodeType::VoltageL3 && this->voltage_l3 != NULL && this->voltage_l3->state != floatValue)
                    this->voltage_l3->publish_state(floatValue);

                else if(codeType == CodeType::CurrentL1 && this->current_l1 != NULL && this->current_l1->state != floatValue)
                    this->current_l1->publish_state(floatValue);
                else if(codeType == CodeType::CurrentL2 && this->current_l2 != NULL && this->current_l2->state != floatValue)
                    this->current_l2->publish_state(floatValue);
                else if(codeType == CodeType::CurrentL3 && this->current_l3 != NULL && this->current_l3->state != floatValue)
                    this->current_l3->publish_state(floatValue);

                break;
            case DataType::OctetString:
                dataLength = plaintext[currentPosition];
                currentPosition++; // Advance past string length

                if(codeType == CodeType::Timestamp) // Handle timestamp generation
                {
                    char timestamp[21]; // 0000-00-00T00:00:00Z

                    uint16_t year;
                    uint8_t month;
                    uint8_t day;

                    uint8_t hour;
                    uint8_t minute;
                    uint8_t second;

                    memcpy(&uint16Value, &plaintext[currentPosition], 2);
                    year = swap_uint16(uint16Value);

                    memcpy(&month, &plaintext[currentPosition + 2], 1);
                    memcpy(&day, &plaintext[currentPosition + 3], 1);

                    memcpy(&hour, &plaintext[currentPosition + 5], 1);
                    memcpy(&minute, &plaintext[currentPosition + 6], 1);
                    memcpy(&second, &plaintext[currentPosition + 7], 1);

                    sprintf(timestamp, "%04u-%02u-%02uT%02u:%02u:%02uZ", year, month, day, hour, minute, second);

                    this->timestamp->publish_state(timestamp);
                }

                break;
            default:
                ESP_LOGE(TAG, "OBIS: Unsupported OBIS data type");
                return abort();
                break;
        }

        currentPosition += dataLength; // Skip data length

        currentPosition += 2; // Skip break after data

        if(plaintext[currentPosition] == 0x0F) // There is still additional data for this type, skip it
               currentPosition += 6; // Skip additional data and additional break; this will jump out of bounds on last frame
    }
    while (currentPosition <= messageLength); // Loop until arrived at end

    this->receiveBuffer.clear(); // Reset buffer

    ESP_LOGI(TAG, "Received valid data");

    /* PRINT TO MQTT, CHANGE TO ZIGBEE*/
    if(this->mqtt_client != NULL)
    {
        this->mqtt_client->publish_json(this->topic, [=](JsonObject root)
        {
            if(this->voltage_l1 != NULL)
            {
                root["voltage_l1"] = this->voltage_l1->state;
                root["voltage_l2"] = this->voltage_l2->state;
                root["voltage_l3"] = this->voltage_l3->state;
            }
            if(this->current_l1 != NULL)
            {
                root["current_l1"] = this->current_l1->state;
                root["current_l2"] = this->current_l2->state;
                root["current_l3"] = this->current_l3->state;
            }
            if(this->active_power_plus != NULL)
            {
                root["active_power_plus"] = this->active_power_plus->state;
                root["active_power_minus"] = this->active_power_minus->state;
            }
            if(this->active_energy_plus != NULL)
            {
                root["active_energy_plus"] = this->active_energy_plus->state;
                root["active_energy_minus"] = this->active_energy_minus->state;
            }
            if(this->reactive_energy_plus != NULL)
            {
                root["reactive_energy_plus"] = this->reactive_energy_plus->state;
                root["reactive_energy_minus"] = this->reactive_energy_minus->state;
            }
            if(this->timestamp != NULL)
            {
                root["timestamp"] = this->timestamp->state;
            }
        });
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