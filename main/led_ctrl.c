#include "led_ctrl.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4096) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz
#define LEDC_PWM_MAX            (1<<LEDC_DUTY_RES)


// 设置占空比 0~100 %
void led_set_duty(led_channel_t led_ch, uint8_t duty)
{
    float duty_set = duty * LEDC_PWM_MAX/100.0; 
    if(duty_set > LEDC_PWM_MAX)
        duty_set = LEDC_PWM_MAX;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, led_ch, duty_set));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, led_ch));
}

static void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t cled_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = CLED_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CLED_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&cled_channel));

    ledc_channel_config_t wled_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = WLED_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = WLED_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&wled_channel));

    led_set_duty(CLED, 0);
    led_set_duty(WLED, 0);
}

void ledTask( void *pvParameters )
{	
    uint8_t duty = 0;
	for( ;; )
	{
        led_set_duty(CLED, duty);
        led_set_duty(WLED, duty);
        duty += 10;
        if(duty >= 100) duty = 0;
		vTaskDelay( pdMS_TO_TICKS( 100UL ) );
	}
}

void led_ctl_init(void)
{
    example_ledc_init();

    xTaskCreate(ledTask, "led", 1024, NULL, 0, NULL);
}