/**
 * @file dlms.c
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <string.h>

/* Logging */
#include "esp_log.h"
static const char* TAG = "dlms";

/* Header */
#include "dlms.h"

/* FreeRTOS Libraries */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* UART Library */
#include "driver/uart.h"
#include "hal/uart_hal.h" /* for interrupt mask defines */

/* UART Event Queue */
static QueueHandle_t uart1_queue = NULL;

/* UART Event Handler */
static void uart_event_task(void *pvParameters)
{
    /* Store current event */
    uart_event_t event;

    /* Buffer for event data */
    uint8_t buff[UART_EVENT_BUFFER_SIZE];

    for(;;)
    {
        /* Loop infinitely and check for event */
        if(xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            /* Set buffer to zero */
            bzero(&buff, UART_EVENT_BUFFER_SIZE);

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

esp_err_t dlms_init()
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
        .intr_enable_mask = UART_INTR_RXFIFO_TOUT | UART_INTR_RXFIFO_FULL,
        .rxfifo_full_thresh = 20,
        .rx_timeout_thresh = 2,
    };

    /* Set interrupt configuration */
    //err = uart_intr_config(UART_PORT_NUMBER, &uart_intr);
    //if(err != ESP_OK){ return err; }

    /* Enable interrupts */
    //err = uart_enable_rx_intr(UART_PORT_NUMBER);
    //if(err != ESP_OK){ return err; }

    /* Set communication pins */
    err = uart_set_pin(UART_PORT_NUMBER, UART_TX_GPIO, UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if(err != ESP_OK){ return err; }

    /* Install driver */
    err = uart_driver_install(UART_PORT_NUMBER, UART_BUFFER_SIZE, 0, 20, &uart1_queue, 0);
    if(err != ESP_OK){ return err; }

    /* Create a task to handle events */
    xTaskCreate(uart_event_task, "uart_event_task", UART_EVENT_BUFFER_SIZE + 1024, NULL, 12, NULL);
    return err;
}