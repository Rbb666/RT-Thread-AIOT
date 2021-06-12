/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-01     rbbb       the first version
 */
#include <rb_key.h>

/* 定义一个按键检测线程句柄结构体指针 */
static rt_thread_t key_thread = RT_NULL;

void key_thread_entry(void *parameter);

//定义key 控制块
struct key_state_type key0 = { 0 };
struct key_state_type key1 = { 0 };

extern uint8_t menu;

static int app_key_init(void)
{
    /* 创建按键检测线程*/
    key_thread = rt_thread_create("key thread", //名称
            key_thread_entry,        //线程代码
            RT_NULL,                 //参数
            350,                     //栈大小
            6,                       //优先级
            10);                     //时间片
    /* 如果获得线程控制块，启动这个线程 */
    if (key_thread != RT_NULL)
        rt_thread_startup(key_thread);
    else
        rt_kprintf("key thread create failure !!! \n");
    return RT_EOK;
}
INIT_APP_EXPORT(app_key_init);

uint8_t set_temp = 20;
uint16_t set_lux = 25;
uint8_t set_adc = 55;
uint8_t now_menu = 1;
#define main_menu  1
#define set_menu   2

//定义菜单 状态结构体
struct menu_state
{
    uint8_t current_menu;
    uint8_t pre_menu;
};

/* 按键检测线程入口函数*/
void key_thread_entry(void *parameter)
{
    struct menu_state sta_menu;
    while (1)
    {
        rb_key_process();
        if (key0.down_state == KEY_PRESS)       //短按1----选择菜单
        {
            rt_kprintf("key is pass\n");
            if (now_menu == main_menu)
            {
                menu += 1;
                if (menu > 3)
                {
                    menu = 1;
                }
                sta_menu.current_menu = menu;   //进入时设置当前的菜单
            }
            else
            {
                switch (menu)
                //温度/光照++
                {
                case 4:
                    set_temp++;
                    if (set_temp == 100)
                        set_temp = 0;
                    break;
                case 5:
                    set_lux++;
                    if (set_lux == 300)
                        set_lux = 0;
                    break;
                case 6:
                    set_adc++;
                    if (set_adc == 120)
                        set_adc = 0;
                    break;
                }
            }
            key0.down_state = KEY_RELEASE;
        }
        else if (key0.long_state == KEY_PRESS)  //长按1----退出主菜单
        {
            if (now_menu == main_menu)
            {
                menu = 0;
                now_menu = main_menu;
            }
            else if (now_menu == set_menu)
            {
                switch (menu)
                //温度/光照++
                {
                case 4:
                    set_temp++;
                    if (set_temp == 100)
                        set_temp = 0;
                    break;
                case 5:
                    set_lux++;
                    if (set_lux == 300)
                        set_lux = 0;
                    break;
                case 6:
                    set_adc++;
                    if (set_adc == 120)
                        set_adc = 0;
                    break;
                }
            }
            key0.long_state = KEY_RELEASE;
        }
//--------------------------------------------------------------------------------------------
        else if (key1.down_state == KEY_PRESS)  //短按2----设置+/-
        {
            if (now_menu == main_menu)
            {
                switch (menu)
                {
                case 1:
                    menu = 4;
                    break;
                case 2:
                    menu = 5;
                    break;
                case 3:
                    menu = 6;
                    break;
                }
                now_menu = set_menu;
            }
            else
            {
                switch (menu)
                //温度/光照--
                {
                case 4:
                    set_temp--;
                    if (set_temp <= 0)
                        set_temp = 99;
                    break;
                case 5:
                    set_lux--;
                    if (set_lux <= 0)
                        set_lux = 300;
                    break;
                case 6:
                    set_adc--;
                    if (set_adc <= 0)
                        set_adc = 120;
                    break;
                }
            }
            key1.down_state = KEY_RELEASE;
        }
        else if (key1.double_state == KEY_PRESS) //双击2----退出设置
        {
            if (now_menu == set_menu)
            {
                menu = sta_menu.current_menu;
                now_menu = main_menu;
            }
            key1.double_state = KEY_RELEASE;
        }
        else if (key1.long_state == KEY_PRESS)   //长按2----+/-
        {
            if (now_menu == set_menu)
            {
                switch (menu)
                //温度/光照--
                {
                case 4:
                    set_temp--;
                    if (set_temp <= 0)
                        set_temp = 99;
                    break;
                case 5:
                    set_lux--;
                    if (set_lux <= 1)
                        set_lux = 200;
                    break;
                case 6:
                    set_adc--;
                    if (set_adc <= 1)
                        set_adc = 120;
                    break;
                }
            }
            key1.long_state = KEY_RELEASE;
        }
        rt_thread_delay(10);
    }
}

