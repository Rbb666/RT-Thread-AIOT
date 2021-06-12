/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-10     rbbb       the first version
 */
#include "rb_nb.h"
#include "stdio.h"
#include "stdlib.h"
#include "cJSON.h"
#include "lcd_spi2_drv.h"

#ifdef RT_USING_ULOG
#define LOG_TAG              "[QSDK/CALLBACK]"
#ifdef QSDK_USING_LOG
#define LOG_LVL              LOG_LVL_DBG
#else
#define LOG_LVL              LOG_LVL_INFO
#endif
#include <ulog.h>
#else
#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME              "[QSDK/CALLBACK]"
#ifdef QSDK_USING_LOG
#define DBG_LEVEL                      DBG_LOG
#else
#define DBG_LEVEL                      DBG_INFO
#endif /* QSDK_DEBUG */

#include <rtdbg.h>
#endif

#include "stdio.h"
#include "stdlib.h"
#include "led.h"
#include <stdbool.h>

//定义EVENT 事件
#define NB_REBOOT     (1<<1)
#define SEND_ERROR    (1<<2)
#define NET_CLOSE     (1<<3)
#define ONENET_CLOSE  (1<<4)
#define UPDATE_ERROR  (1<<5)

#define NET_STA        200

static uint8_t is_auto = 1;         // 设置LED的亮灭是否由BH1750自动控制,默认自动控制
static uint8_t light_sw = 0;        // 若不是自动控制，则通过这个值来设定亮灭，若自动控制，则无效
static uint8_t sm_light = 0;
static uint16_t lux_value;          // 自动控制下，光照强度大于这个值，LED灭，小于则亮
static uint8_t mq_value;            // 有害气体浓度
static uint8_t dool = 0;
extern uint16_t set_lux;
uint8_t fan;
uint8_t k210_flag = 22;
uint8_t connect_sta;                // NB重连标志
uint8_t ledflag;                    // led开关状态
uint8_t auto_flag = 1;              // 手自动状态
uint16_t pwm_led;
uint8_t usart_data[3] = {0xaa,0x00,0xbb};
//引用错误事件句柄
extern rt_event_t net_error;
/* 串口设备句柄 */
extern rt_device_t serial;
extern uint8_t door_sta;
/****************************************************
 * 函数名称： qsdk_rtc_set_time_callback
 *
 * 函数作用： RTC设置回调函数
 *
 * 入口参数： year：年份     month: 月份       day: 日期
 *
 *                           hour: 小时        min: 分钟     sec: 秒      week: 星期
 *
 * 返回值： 0 处理成功   1 处理失败
 *****************************************************/
void qsdk_rtc_set_time_callback(int year, char month, char day, char hour, char min, char sec, char week)
{
#ifdef RT_USING_RTC
    set_date(year, month, day);
    set_time(hour, min, sec);
#endif
}

/*************************************************************
 *   函数名称：   qsdk_net_close_callback
 *
 *   函数功能：   TCP异常断开回调函数
 *
 *   入口参数：   无
 *
 *   返回参数：   无
 *
 *   说明：
 *************************************************************/
void qsdk_net_close_callback(void)
{
    LOG_E("now the network is abnormally disconnected\r\n");
}
/****************************************************
 * 函数名称： qsdk_mqtt_data_callback
 *
 * 函数作用： MQTT 服务器下发数据回调函数
 *
 * 入口参数：topic：主题    mesg：平台下发消息    mesg_len：下发消息长度   @@@@@@@@@@@ 这里处理下发的json数据
 *
 * 返回值：  0 处理成功  1 处理失败
 *****************************************************/
int qsdk_mqtt_data_callback(char *topic, char *mesg, int mesg_len)
{
    rt_kprintf("@@enter mqtt callback  mesg:%s\r\n", mesg);
    //JSON提取值
    cJSON * root = NULL;
    cJSON * object = NULL;

    /* 解析整段JSON数据 */
    root = cJSON_Parse(mesg);
    if (root == NULL)
    {
        rt_kprintf("No memory for cJSON root!\n");
        cJSON_Delete(root); //释放json内存
    }
    else
    {
        object = cJSON_GetObjectItem(root, "is_auto");
        if (object->type == cJSON_Number)
            is_auto = object->valueint;

        object = cJSON_GetObjectItem(root, "light_sw");
        if (object->type == cJSON_Number)
            light_sw = object->valueint;

        object = cJSON_GetObjectItem(root, "sm_light");
        if (object->type == cJSON_Number)
            sm_light = object->valueint;

        object = cJSON_GetObjectItem(root, "set_lux");
        if (object->type == cJSON_Number)
            lux_value = object->valueint;

        object = cJSON_GetObjectItem(root, "set_adc");
        if (object->type == cJSON_Number)
            mq_value = object->valueint;

        object = cJSON_GetObjectItem(root, "dool_op");       //锁
        if (object->type == cJSON_Number)
            dool = object->valueint;

        object = cJSON_GetObjectItem(root, "k210_flag");       //锁
        if (object->type == cJSON_Number)
            k210_flag = object->valueint;

        object = cJSON_GetObjectItem(root, "fengshan_sw");   //fan
        if (object->type == cJSON_Number)
            fan = object->valueint;

        object = cJSON_GetObjectItem(root, "pwm_led");       //fan
        if (object->type == cJSON_Number)
            pwm_led = object->valueint;

        object = cJSON_GetObjectItem(root, "nb_connect");
        if (object->type == cJSON_Number)
            connect_sta = object->valueint;

        cJSON_Delete(root);                                  //释放json内存

        if (!is_auto || sm_light == 1)                       //收到  "is_auto=0"  topic 手动控制led
        {
            auto_flag = 0;
            if (light_sw == 1 || sm_light == 1)
            {
                rb_led_on(LED3);
                ledflag = 1;
                rt_kprintf("led0_on\r\n");
                LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24, 30, "On ", BLACK, RED, 24);
            }
            else
            {
                rb_led_off(LED3);
                rt_kprintf("led0_off\r\n");
                LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24, 30, "OFF", BLACK, GREEN, 24);
                ledflag = 0;
            }
        }
        else                                                //自动模式
        {
            auto_flag = 1;
            if (sm_light == 2)
            {
                rb_led_off(LED3);
                ledflag = 0;
            }
        }

        if (lux_value)
        {
            set_lux = lux_value;
        }

        if (dool == 1)
        {
            rb_door_on(SUO);
            door_sta = 1;
            LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24 + 26, 30, "On ", BLACK, GREEN, 24);
            rt_thread_delay(6000);
            rb_door_off(SUO);
            door_sta = 0;
        }
        else if(dool == 0)
        {
            rb_door_off(SUO);
            door_sta = 0;
            LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24 + 26, 30, "OFF ", BLACK, RED, 24);
        }

        if (k210_flag == 1)
        {
            usart_data[1] = 0x01;
            rt_kprintf("usart_data:%d",usart_data[1]);
            rt_device_write(serial, 0, usart_data, sizeof(usart_data));
        }
        else if (k210_flag == 0)
        {
            usart_data[1] = 0x00;
            rt_kprintf("usart_data:%d",usart_data[1]);
            rb_door_off(SUO);
            door_sta = 0;
            LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24 + 26, 30, "OFF ", BLACK, RED, 24);
        }

        if (fan == 1)
        {
            rb_fengshan_on(Fengshan);
        }
        else if (fan == 0)
        {
            rb_fengshan_off(Fengshan);
        }

        if (connect_sta == NET_STA)
        {
            connect_sta = 0;
            if (qsdk_nb_quick_connect() != RT_EOK)
            {
                rt_kprintf("reboot fail...\n");
            }
            rt_kprintf("reboot Success!!!\n");
        }
    }
    return RT_EOK;
}
/****************************************************
 * 函数名称： qsdk_nb_reboot_callback
 *
 * 函数作用： nb-iot模组意外复位回调函数
 *
 * 入口参数：无
 *
 * 返回值：  无
 *****************************************************/
void qsdk_nb_reboot_callback(void)
{
    rt_kprintf("enter reboot callback\r\n");

    if (rt_event_send(net_error, NB_REBOOT) == RT_ERROR)
        rt_kprintf("event reboot send error\r\n");
}

