/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-29     rbbb       the first version
 */
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <math.h>
#include <rtdbg.h>
#include <stdio.h>
#include "rb_beep.h"
#include "led.h"
#include "lcd_spi2_drv.h"

#define ADC_DEV_NAME        "adc1"      /* ADC 设备名称 */
#define ADC_DEV_CHANNEL     3           /* ADC 通道 */
#define REFER_VOLTAGE       330         /* 参考电压 3.3V,数据精度乘以100保留2位小数*/
#define CONVERT_BITS        (1 << 12)   /* 转换位数为12位 */
extern uint8_t set_adc;
extern uint8_t fan;
rt_adc_device_t adc_dev; /* ADC 设备句柄 */
rt_uint32_t value;
uint8_t ppm;
uint8_t ppm_warn;

/* 定义一个adc线程句柄结构体指针 */
static rt_thread_t app_adc_thread = RT_NULL;

static void app_adc_thread_entry(void *parameter)
{
    rt_err_t ret = RT_EOK;

    /* 查找设备 */
    adc_dev = (rt_adc_device_t) rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return;
    }
    /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_DEV_CHANNEL);

    while (1)
    {
        /* 读取采样值 */
        value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);
        /* 读取采样值 */
        value = rt_adc_read(adc_dev, ADC_DEV_CHANNEL);

        /* 转换为对应电压值 */
        ppm = value * REFER_VOLTAGE / CONVERT_BITS;
        //rt_kprintf("ppm-------%d\n", ppm);

        if (ppm >= set_adc)
        {
            ppm_warn += 1;
            if (ppm_warn == 200)
            {
                ppm_warn = 1;
            }
        }
        else                //safe
        {
            ppm_warn = 0;
            LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10, 40, "Safe", BLACK, GREEN, 24);
            if (fan == 1)
            {
                //rb_fengshan_on(Fengshan);
            }
            else if (fan == 0)
            {
                //rb_fengshan_off(Fengshan);
            }
        }
        if (ppm_warn >= 5)  //warning
        {
            //rb_fengshan_on(Fengshan);
            LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10, 50, "Warn", BLACK, RED, 24);
        }
        //rt_kprintf("ppm_warn %d\n", ppm_warn);
        rt_thread_mdelay(1000);
    }

    /* 关闭通道 */
    ret = rt_adc_disable(adc_dev, ADC_DEV_CHANNEL);
    return ret;
}

static int app_adc_init(void)
{
    rt_err_t rt_err;

    /* 创建ADC线程*/
    app_adc_thread = rt_thread_create("app_adc", app_adc_thread_entry, RT_NULL, 512 + 200, 15, 10);
    /* 如果获得线程控制块，启动这个线程 */
    if (app_adc_thread != RT_NULL)
        rt_err = rt_thread_startup(app_adc_thread);
    else
        rt_kprintf("app_adc_thread create failure !!! \n");

    /* 判断线程是否启动成功 */
    if (rt_err == RT_EOK)
        rt_kprintf("app_adc_thread startup ok. \n");
    else
        rt_kprintf("app_adc_thread startup err. \n");

    return rt_err;
}
INIT_APP_EXPORT(app_adc_init);
