/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

const int LED_PIN_B = 8;
const int LED_PIN_Y = 13;

typedef struct input {
    int num_led1;
    int num_led2;
} input_t;

QueueHandle_t xQueueInput;

/**
 * NÃO MEXER!
 */
void input_task(void* p) {
    input_t test_case;

    test_case.num_led1 = 3;
    test_case.num_led2 = 4;
    xQueueSend(xQueueInput, &test_case, 0);

    test_case.num_led1 = 0;
    test_case.num_led2 = 2;
    xQueueSend(xQueueInput, &test_case, 0);

    while (true) {

    }
}

/**
 * Seu código vem aqui!
 */

QueueHandle_t xQueueLed1;
QueueHandle_t xQueueLed2;
SemaphoreHandle_t xSemaphoreLed2;

void led_1_task(void *p) {
    gpio_init(LED_PIN_B);
    gpio_set_dir(LED_PIN_B, GPIO_OUT);

    int n_blinks = 0;
    while(true) {
        if (xQueueReceive(xQueueLed1, &n_blinks, portMAX_DELAY)) {
            for (int i=0; i < n_blinks; i++){
                gpio_put(LED_PIN_B, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_put(LED_PIN_B, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            xSemaphoreGive(xSemaphoreLed2);
        }
    }
}

void led_2_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    int n_blinks = 0;
    while(true) {
        if (xQueueReceive(xQueueLed2, &n_blinks, portMAX_DELAY)) {
            if (xSemaphoreTake(xSemaphoreLed2, portMAX_DELAY) == pdTRUE) {
                for (int i=0; i < n_blinks; i++) {
                    gpio_put(LED_PIN_Y, 1);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    gpio_put(LED_PIN_Y, 0);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
            }
        }
    }
}

void main_task(void *p) {
    input_t received_data;
    while(true) {
        if (xQueueReceive(xQueueInput, &received_data, portMAX_DELAY)) {
            (xQueueSend(xQueueLed1, &received_data.num_led1, portMAX_DELAY));
            (xQueueSend(xQueueLed2, &received_data.num_led2, portMAX_DELAY));
        }
    }
}


int main() {
    stdio_init_all();

    /**
     * manter essas duas linhas!
     */
    xQueueInput = xQueueCreate(32, sizeof(input_t));
    xTaskCreate(input_task, "Input", 256, NULL, 1, NULL);

    /**
     * Seu código vem aqui!
     */

    xQueueLed1 = xQueueCreate(16, sizeof(int));
    xQueueLed2 = xQueueCreate(16, sizeof(int));

    xSemaphoreLed2 = xSemaphoreCreateBinary();

    xTaskCreate(main_task, "Main_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_1_task, "LED1_Task", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED2_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {}

    return 0;
}
