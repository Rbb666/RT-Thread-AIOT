/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-01     rbbb       the first version
 */
#include "sht3x.h"
#include "rb_nb.h"
#include <drv_soft_i2c.h>

static rt_thread_t sht30_thread_id = RT_NULL;

void sht30_entry(void *parameter);

//定义温湿度缓存变量
float humidity;
float temperature;

//定义sht30 句柄
sht3x_device_t sht30_dev;

//创建sht30_thread
static int sht30_init(void)
{
    //创建sht30 线程
    sht30_thread_id = rt_thread_create("sht30_th",    //名称
            sht30_entry,            //线程代码
            RT_NULL,                //参数
            1024,                   //栈大小
            15,                     //优先级
            20);                    //时间片
    /*如果线程创建成功，启动这个线程*/
    if (sht30_thread_id != RT_NULL)
        rt_thread_startup(sht30_thread_id);
    else
        rt_kprintf("sht30_thread create failure !!! \n");
    return RT_EOK;
}

INIT_APP_EXPORT(sht30_init);

//sht30任务
void sht30_entry(void *parameter)
{
    //定义温湿度器件地址
    rt_uint8_t sht_addr = SHT3X_ADDR_PD;
    sht30_dev = sht3x_init("i2c1", sht_addr);

    if (sht30_dev == RT_NULL)
        rt_kprintf("no find sht30 device\r\n");
    while (1)
    {
        if (RT_EOK == sht3x_read_singleshot(sht30_dev))
        {
            humidity = sht30_dev->humidity;
            temperature = sht30_dev->temperature;
        }
        else
        {
            humidity = 55.2;
            temperature = 23;
        }
        rt_thread_delay(1000);
    }
}
