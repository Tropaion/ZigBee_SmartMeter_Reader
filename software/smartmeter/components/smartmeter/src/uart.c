/**
 * @file uart.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <string.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "UART";

/* Header */
#include "smartmeter.h"
#include "general.h"
#include "uart.h"

/* Layer Parsers */
#include "mbus.h"
#include "dlms.h"
#include "obis.h"

/* FreeRTOS Libraries */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* UART Library */
#include "driver/uart.h"

/* UART Event Queue */
static QueueHandle_t uart1_queue = NULL;

/* SMALL INFODUMP */
/* Structure of data and how it's processed */
/* 1. Physical Layer -> UART */
/* 2. MBUS-Layer -> parse with "parse_mbus_long_frame_layer", supports multiple frames, returns user data */
/* 3. DLMS (Application)-Layer -> decrypt and parse with "todo" */

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
                    ESP_LOGI(TAG, "FIFO overflow");

                    /* Flush ringbuffer */
                    uart_flush_input(UART_PORT_NUMBER);

                    /* Reset event queue */
                    xQueueReset(uart1_queue);
                    break;

                /* Ringbuffer full */
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "Ringbuffer full");
                    
                    /* Flush ringbuffer */
                    uart_flush_input(UART_PORT_NUMBER);

                    /* Reset event queue */
                    xQueueReset(uart1_queue);
                    break;

                /* RX break detected */
                case UART_BREAK:
                    ESP_LOGI(TAG, "RX Break");
                    break;

                /* Parity check error */
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "Parity Error");
                    break;

                /* Frame error */
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "Frame Error");
                    break;

                /* Other events */
                default:
                    /* Write event to log */
                    ESP_LOGI(TAG, "Event Type: %d", event.type);
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
                err = parse_dlms_layer(&buff1[0], buff1_size, &buff0[0], &buff0_size, &decryption_key[0]);
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
                //err = parse_obis(&buff0[0], buff0_size);
            }
            
            /* Check if parsing failed */
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Parsing failed.");
                
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