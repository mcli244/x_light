#ifndef __KEY_H__
#define __KEY_H__

#include <stdint.h>
#include "lvgl.h"


#define KEY1_PIN        9
#define KEY2_PIN        10
#define KEY3_PIN        11

void my_key_init(void);
bool my_key_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data );

#endif /*__KEY_H__*/