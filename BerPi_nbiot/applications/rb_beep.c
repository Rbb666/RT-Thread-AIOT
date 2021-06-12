/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-31     rbbb       the first version
 */
#include "rb_beep.h"

int rb_beep_init(void)
{
    rt_pin_mode(BEEP,PIN_MODE_OUTPUT);
    rt_pin_write(BEEP,PIN_LOW);

    return RT_EOK;
}

void rb_beep_on(void)
{
    rt_pin_write(BEEP,PIN_HIGH);
}

void rb_beep_off(void)
{
    rt_pin_write(BEEP,PIN_LOW);
}
