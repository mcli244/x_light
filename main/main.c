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

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "lvgl_helpers.h"

#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    #if defined CONFIG_LV_USE_DEMO_WIDGETS
        #include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
    #elif defined CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER
        #include "lv_examples/src/lv_demo_keypad_encoder/lv_demo_keypad_encoder.h"
    #elif defined CONFIG_LV_USE_DEMO_BENCHMARK
        #include "lv_examples/src/lv_demo_benchmark/lv_demo_benchmark.h"
    #elif defined CONFIG_LV_USE_DEMO_STRESS
        #include "lv_examples/src/lv_demo_stress/lv_demo_stress.h"
    #else
        #error "No demo application selected."
    #endif
#endif

/*********************
 *      DEFINES
 *********************/
#define TAG "demo"
#define LV_TICK_PERIOD_MS 1

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);
static void guiTask(void *pvParameter);
static void create_demo_application(void);

#define STA_LED_PIN     8           // 状态指示灯
#define STA_LED_ON      0  
#define STA_LED_OFF     1 

#define KEY1_PIN        9
#define KEY2_PIN        10
#define KEY3_PIN        11


void state_led_init(void)
{
    gpio_set_direction(STA_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(STA_LED_PIN, STA_LED_OFF);
}

void state_led_ctr(uint8_t sta)
{
    gpio_set_level(STA_LED_PIN, sta);
}


void ledTask( void *pvParameters )
{	
    state_led_init();
    uint8_t flag = 0;
	for( ;; )
	{
        if(flag)    state_led_ctr(STA_LED_ON);
        else        state_led_ctr(STA_LED_OFF);
        flag = !flag;
		vTaskDelay( pdMS_TO_TICKS( 1000UL ) );
	}
}

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
static bool my_key_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
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
/**********************
 *   APPLICATION MAIN
 **********************/
void app_main() 
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
    xTaskCreate(ledTask, "led", 1024, NULL, 0, NULL);
}


static lv_indev_t *indev_encoder;
void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
	my_key_init();
	lv_indev_drv_init( &indev_drv);
	indev_drv.type = LV_INDEV_TYPE_ENCODER;
	indev_drv.read_cb = my_key_read;
	indev_encoder = lv_indev_drv_register( &indev_drv );
    if(indev_encoder == NULL)
        printf("indev_encoder is NULL");
}

/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;
lv_obj_t *slider_label = NULL;

static void guiTask(void *pvParameter) {

    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);

    /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

    static lv_disp_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

#if defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_IL3820         \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_JD79653A    \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_UC8151D     \
    || defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_SSD1306

    /* Actual size in pixels, not bytes. */
    size_in_px *= 8;
#endif

    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

#if defined CONFIG_DISPLAY_ORIENTATION_PORTRAIT || defined CONFIG_DISPLAY_ORIENTATION_PORTRAIT_INVERTED
    disp_drv.rotated = 1;
#endif

    /* When using a monochrome display we need to register the callbacks:
     * - rounder_cb
     * - set_px_cb */
#ifdef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    disp_drv.rounder_cb = disp_driver_rounder;
    disp_drv.set_px_cb = disp_driver_set_px;
#endif

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);
#endif

    lv_port_indev_init();

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    /* Create the demo application */
    create_demo_application();

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }

    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}


static void btn_event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked\n");
    }
    else if(event == LV_EVENT_VALUE_CHANGED) {
        printf("Toggled\n");
    }
}

static void slider_event_cb(lv_obj_t * slider, lv_event_t event)
{

    if(event == LV_EVENT_VALUE_CHANGED)
    {
        lv_label_set_text_fmt(slider_label, "%d", lv_slider_get_value(slider));
    }
    
}

static void create_demo_application(void)
{
 
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);

    //新建一个组
    lv_group_t *g = lv_group_create();
    

    /*Create a Label on the currently active screen*/
    lv_obj_t * label1 =  lv_label_create(scr, NULL);
    lv_obj_t * label2 =  lv_label_create(scr, NULL);
    lv_obj_t * label3 =  lv_label_create(scr, NULL);

     /*使能recolor*/
    lv_label_set_recolor(label1,true);
    lv_label_set_recolor(label2,true);
    lv_label_set_recolor(label3,true);

    /*设置对应文本颜色*/
    lv_label_set_text(label1,"#FF0000 color# Red");
    lv_label_set_text(label2,"#00FF00 color# Green");
    lv_label_set_text(label3,"#0000FF color# Bule");

    lv_obj_align(label1, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_obj_align(label2, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 16);
    lv_obj_align(label3, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 32);
    
    //创建对象
    lv_obj_t * button1 = lv_btn_create(scr, NULL);
    lv_obj_t * button2 = lv_btn_create(scr, NULL);

    lv_obj_t *btn_label1 = lv_label_create(button1, NULL);
    lv_obj_t *btn_label2 = lv_label_create(button2, NULL);

    lv_label_set_text(btn_label1, "OK");
    lv_obj_set_size(button1, 40, 20);
    lv_obj_align(button1, NULL, LV_ALIGN_CENTER, 20, -25);
    lv_obj_set_event_cb(button1, btn_event_handler);

    lv_label_set_text(btn_label2, "PASS");
    lv_obj_set_size(button2, 40, 20);
    lv_obj_align(button2, NULL, LV_ALIGN_CENTER, 20, 0);
    lv_obj_set_event_cb(button2, btn_event_handler);

    lv_obj_t *slider = lv_slider_create(scr, NULL);   //创建个滑条
    lv_obj_align(slider, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 55);
    lv_obj_set_size(slider, 100, 10);
    lv_obj_set_event_cb(slider, slider_event_cb); 

    slider_label = lv_label_create(scr, NULL);    //标签
    lv_label_set_text(slider_label, "0%");
    lv_obj_align_mid(slider_label, slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);  //放在滑条的右边


    lv_group_add_obj(g, button1);
    lv_group_add_obj(g, button2);
    lv_group_add_obj(g, slider);

    lv_indev_set_group(indev_encoder, g);
    lv_group_set_click_focus(g, true);
}

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}
