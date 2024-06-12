#ifndef __LED_CTRL_H__
#define __LED_CTRL_H__

#include "driver/ledc.h"

// 冷色灯
#define CLED_IO                 (8) // Define the output GPIO
#define CLED_CHANNEL            LEDC_CHANNEL_0

// 暖色灯
#define WLED_IO                 (18) // Define the output GPIO
#define WLED_CHANNEL            LEDC_CHANNEL_1

typedef enum {
    CLED = CLED_CHANNEL,
    WLED = WLED_CHANNEL,
} led_channel_t;

void led_ctl_init(void);

#endif /*__LED_CTRL_H__*/