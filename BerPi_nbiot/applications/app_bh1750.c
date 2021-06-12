#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "sensor.h"
#include "led.h"
#include "lcd_spi2_drv.h"

#define led_auto 1
#define led_hand 0

/* 定义一个BH1750线程句柄结构体指针 */
static rt_thread_t app_bh1750_thread = RT_NULL;
uint16_t lux_value;
//设置光照值
extern uint16_t set_lux;
extern uint8_t auto_flag;
extern uint8_t ledflag;                    // led开关状态
uint8_t warn_lux = 0;

static void app_bh1750_thread_entry(void *parameter)
{
    rt_device_t dev = RT_NULL;
    struct rt_sensor_data data;
    rt_size_t res;

    /* 查找系统中的传感器设备 */
    dev = rt_device_find("li_bh1750");
    if (dev == RT_NULL)
    {
        rt_kprintf("Can't find device:li_bh1750\n");
        return;
    }

    /* 以轮询模式打开传感器设备 */
    if (rt_device_open(dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
    {
        rt_kprintf("open device failed!");
        return;
    }

    while (1)
    {

        /* 从传感器读取一个数据 */
        res = rt_device_read(dev, 0, &data, 1);
        if (res != 1)
        {
            rt_kprintf("read data failed!size is %d", res);
        }
        else
        {
            lux_value = (&data)->data.light / 10;
        }
        if (auto_flag == led_auto)
        {
            if (lux_value <= set_lux)
            {
                warn_lux++;
                if (warn_lux >= 200)
                {
                    warn_lux = 1;
                }
            }
            else
            {
                if (warn_lux == 0)
                {
                    warn_lux = 0;
                    rb_led_off(LED3);   //灭
                    ledflag = 0;
                    LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24, 30, "OFF", BLACK, GREEN, 24);
                }
                else
                {
                    warn_lux--;
                }
            }
            if (warn_lux >= 5)
            {
                LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24, 30, "On ", BLACK, RED, 24);
                rb_led_on(LED3);        //亮
                ledflag = 1;
            }
        }
        //rt_kprintf("%d \n", warn_lux);
        rt_thread_mdelay(1000);
    }
    /* 关闭传感器设备 */
    rt_device_close(dev);
}

static int app_bh1750_init(void)
{
    rt_err_t rt_err;

    /* 创建BH1750线程*/
    app_bh1750_thread = rt_thread_create("app_bh1750", app_bh1750_thread_entry, RT_NULL, 1024, 15, 20);
    /* 如果获得线程控制块，启动这个线程 */
    if (app_bh1750_thread != RT_NULL)
        rt_err = rt_thread_startup(app_bh1750_thread);
    else
        rt_kprintf("app_bh1750_thread create failure !!! \n");

    /* 判断线程是否启动成功 */
    if (rt_err == RT_EOK)
        rt_kprintf("app_bh1750_thread startup ok. \n");
    else
        rt_kprintf("app_bh1750_thread startup err. \n");

    return rt_err;
}
INIT_APP_EXPORT(app_bh1750_init);
