/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-20     rbbb       the first version
 */
#ifndef _LED_H_
#define _LED_H_

#include "rtthread.h"
#include "board.h"
#include <rtdevice.h>

#define LED3 GET_PIN(A, 0)
#define SUO  GET_PIN(B, 12)
#define Fengshan GET_PIN(B, 8)

#define LED_ON  PIN_HIGH
#define LED_OFF PIN_LOW

//定义led 状态结构体
struct led_state_type
{
    int current_state;
    int last_state;
};

void rb_led_on(rt_base_t pin);
void rb_led_off(rt_base_t pin);
void rb_door_on(rt_base_t pin);
void rb_door_off(rt_base_t pin);
void rb_fengshan_on(rt_base_t pin);
void rb_fengshan_off(rt_base_t pin);

#endif /* PACKAGES_HARDWARE_DEVICE_LED_H_ */
