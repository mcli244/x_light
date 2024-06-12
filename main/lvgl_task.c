#include "lvgl_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "key.h"

#define TAG "lvgl_task"
#define LV_TICK_PERIOD_MS 1

static lv_indev_t *indev_encoder;
SemaphoreHandle_t xGuiSemaphore;
lv_obj_t *slider_label = NULL;

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
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

void guiTask(void *pvParameter) 
{

    (void) pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
    lvgl_driver_init();

    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);

    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
	my_key_init();
	lv_indev_drv_init( &indev_drv);
	indev_drv.type = LV_INDEV_TYPE_ENCODER;
	indev_drv.read_cb = my_key_read;
	indev_encoder = lv_indev_drv_register( &indev_drv );

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


int lvgl_task_init(void)
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
    return 0;
}
