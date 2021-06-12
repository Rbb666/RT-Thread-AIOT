/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-01     rbbb       the first version
 */
#ifndef APPLICATIONS_RB_BEEP_H_
#define APPLICATIONS_RB_BEEP_H_

#include "rtthread.h"
#include <rtdevice.h>
#include "board.h"

#define BEEP GET_PIN(A,0)

void rb_beep_on(void);
void rb_beep_off(void);

#endif /* APPLICATIONS_RB_BEEP_H_ */
