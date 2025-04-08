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
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc_cal.h"
#include "esp_adc/adc_cali.h"

#define ADC_CHANNEL   ADC_CHANNEL_0   // GPIO pin connected to ADC
#define ADC_UNIT      ADC_UNIT_1      // ADC unit
#define DEFAULT_VREF  3.3f            // Default reference voltage in mV

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;

void init_pwm(void)
{
    // Configure the LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 10000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configure the LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = 18,  // GPIO pin to output PWM signal
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0, 
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // Optionally, fade functionality can be added here
}

void init_adc(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_11, // 11dB attenuation for 0-3.3V range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &channel_config);
}

void calibrate_adc(void)
{
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_WIDTH_BIT_12,
    };

    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle) == ESP_OK) {
        printf("ADC calibration initialized successfully\n");
    } else {
        printf("Failed to initialize ADC calibration\n");
        adc_cali_handle = NULL;
    }
}

float read_gpio_voltage(void)
{
    // Take multiple readings for stability
    const int NUM_SAMPLES = 100;
    int total = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        int raw_value = 0;
        adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_value);
        total += raw_value;
        // Small delay between readings
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    int avg_raw = total / NUM_SAMPLES;

    // Convert raw ADC value to calibrated voltage in mV
    int voltage_mv = 0;  // Changed from uint32_t to int
    if (adc_cali_handle) {
        adc_cali_raw_to_voltage(adc_cali_handle, avg_raw, &voltage_mv);
    } else {
        printf("ADC calibration not available, using raw value\n");
        voltage_mv = (avg_raw * DEFAULT_VREF * 1000) / 4095; // Fallback calculation
    }

    return voltage_mv / 1000.0f; // Convert to volts
}

void set_pwm(float percent)
{
    uint32_t duty = percent * 1023 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void app_main(void)
{
    init_pwm();
    init_adc();
    calibrate_adc();

    const int delta = 10;
    
    for(int i=0; i<=100/delta; i++){
        int percent = i * delta;
        set_pwm(percent);
        vTaskDelay(pdMS_TO_TICKS(1000)); 
        float voltage = read_gpio_voltage();
        printf("Duty percent %d, voltage: %.2f V\n", percent, voltage);
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Read voltage every second
    }
}
