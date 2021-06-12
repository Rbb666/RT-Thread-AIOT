/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-28     rbbb       the first version
 */
#ifndef __RB_KEY__H_
#define __RB_KEY__H_

#include "button.h"
#include <board.h>

#define KEY_0       GET_PIN(B,2)
#define KEY_1       GET_PIN(B,3)

#define KEY_PRESS   PIN_HIGH
#define KEY_RELEASE PIN_LOW

//定义key 状态结构体
struct key_state_type
{
    int down_state;
    int double_state;
    int long_state;
};

void rb_key_process(void);


#endif /* PACKAGES_HARDWARE_DEVICE_RB_KEY_H_ */
