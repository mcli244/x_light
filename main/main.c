/* LVGL Example project
 *
 * Basic project to test LVGL on ESP32 based projects.
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "lvgl_task.h"
#include "led_ctrl.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "demo"
#define LV_TICK_PERIOD_MS 1


void app_main() 
{
    lvgl_task_init();
    led_ctl_init();
    while (1) 
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
