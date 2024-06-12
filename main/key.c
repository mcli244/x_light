#include "key.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

void my_key_init(void)
{
    gpio_set_direction(KEY1_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(KEY2_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(KEY3_PIN, GPIO_MODE_INPUT);
}


uint8_t Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1;

    if(mode) key_up = 1;

    if(key_up && 
        ((0 == gpio_get_level(KEY1_PIN))
        || (0 == gpio_get_level(KEY2_PIN))
        || (0 == gpio_get_level(KEY3_PIN))))
    {
        vTaskDelay( pdMS_TO_TICKS( 10UL ) );
        key_up = 0;
        if(0 == gpio_get_level(KEY1_PIN))           return 1;
        else if(0 == gpio_get_level(KEY2_PIN))      return 2;
        else if(0 == gpio_get_level(KEY3_PIN))      return 3;
    }
    else if(0 == key_up 
        && 1 == gpio_get_level(KEY1_PIN)
        && 1 == gpio_get_level(KEY2_PIN)
        && 1 == gpio_get_level(KEY3_PIN))
    {
        key_up = 1;
    }

    return 0;
}


// 按键状态更新函数
bool my_key_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
	static uint32_t last_key = 0;
	uint8_t act_enc = Key_Scan(1);
	
	if(act_enc != 0) {
	    switch(act_enc) {
	        case 1:
	            act_enc = LV_KEY_ENTER;
	            data->state = LV_INDEV_STATE_PR;	
	            break;
	        case 2:
	            act_enc = LV_KEY_RIGHT;
	            data->state = LV_INDEV_STATE_PR;
	            data->enc_diff++;
	            break;
	        case 3:
	            act_enc = LV_KEY_LEFT;
	            data->state = LV_INDEV_STATE_PR;
	            data->enc_diff--;
	            break;
	    }
	    last_key = (uint32_t)act_enc;        
	}
	data->key = last_key;

    return false;
}

