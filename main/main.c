/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/gpio.h"

void configure_gpio(void)
{
    // Configure GPIO pin as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 18),  // GPIO pin 18
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Set initial state to OFF
    gpio_set_level(18, 0);
}

void app_main(void)
{
    configure_gpio();

    while (1) {
        // Toggle GPIO pin to control MOSFET
        printf("Turning MOSFET ON\n");
        gpio_set_level(18, 1);  // Turn MOSFET ON
        vTaskDelay(pdMS_TO_TICKS(5000));  // Delay 1 second
        printf("Turning MOSFET OFF\n");
        gpio_set_level(18, 0);  // Turn MOSFET OFF
        vTaskDelay(pdMS_TO_TICKS(5000));  // Delay 1 second
    }
}
