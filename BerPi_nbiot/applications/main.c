/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-19     RT-Thread    first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include "lcd_spi2_drv.h"
#include <arpa/inet.h>         /* 包含 ip_addr_t 等地址相关的头文件 */
#include <drv_common.h>
#include "led.h"
#include "rb_nb.h"

#define WDT_DEVICE_NAME    "wdt"    /* 看门狗设备名称 */
static rt_device_t wdg_dev; /* 看门狗设备句柄 */

//定义EVENT 事件
#define EVENT_NB_REBOOT     (1<<1)
#define EVENT_SEND_ERROR    (1<<2)
#define EVENT_NET_CLOSE     (1<<3)
#define EVENT_ONENET_CLOSE  (1<<4)
#define EVENT_UPDATE_ERROR  (1<<5)
//定义上报速率

//定义网络错误事件句柄
rt_event_t net_error = RT_NULL;

//定义线程控制句柄
static rt_thread_t data_up_thread_id = RT_NULL;
static rt_thread_t net_thread_id = RT_NULL;
static rt_thread_t send_data_thread_id = RT_NULL;

void net_entry(void *parameter);
void data_up_entry(void *parameter);
void send_data_entry(void *parameter);

//消息邮箱
rt_mailbox_t Send_data_mailbox = RT_NULL;

uint8_t NB_SendDataBuff[200];
int csq_pub;
int net_sta;
char * imei_info;
extern float humidity;
extern float temperature;
extern uint16_t lux_value;
extern uint8_t ppm;
extern uint8_t door_sta;
extern uint8_t ledflag;  // led开关状态
extern struct nb_device nb_device_table;

int main(void)
{
    WDG_Init();
    LCD_Init();
    rb_led_off(LED3);

    rt_thread_delay(1000);
    //LCD_Show_Image(0, 0, 240, 65, image_rttlogo);
    LCD_ShowChinese24x24(5, 10, "气", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24, 10, "体", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24 + 24, 10, "自", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24 + 24 + 24, 10, "检", GBLUE, WHITE, 24, 1);

    LCD_ShowChinese24x24(5, 10 + 26, "光", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24, 10 + 26, "度", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24 + 24, 10 + 26, "自", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24 + 24 + 24, 10 + 26, "检", GBLUE, WHITE, 24, 1);

    LCD_ShowChinese24x24(5, 10 + 26 + 26, "门", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24, 10 + 26 + 26, "禁", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24 + 24, 10 + 26 + 26, "状", GBLUE, WHITE, 24, 1);
    LCD_ShowChinese24x24(5 + 24 + 24 + 24, 10 + 26 + 26, "态", GBLUE, WHITE, 24, 1);
    LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24 + 26, 30, "OFF ", BLACK, RED, 24);

    LCD_Draw_ColorLine(0, 70 + 40, 240, 70 + 40, GRAY);
    LCD_ShowChinese48x48(20, 73 + 35, "语", GRAY, WHITE, 48, 1);
    LCD_ShowChinese48x48(20 + 48, 73 + 35, "音", GRAY, WHITE, 48, 1);
    LCD_ShowChinese48x48(20 + 48 + 48, 73 + 35, "助", GRAY, WHITE, 48, 1);
    LCD_ShowChinese48x48(20 + 48 + 48 + 48, 73 + 35, "手", GRAY, WHITE, 48, 1);

    for (int i = 0; i < 32; i += 4)
    {
        LCD_Draw_ColorCircle(120, 194, i, GRAY);
    }

    //创建事件
    net_error = rt_event_create("net_error", RT_IPC_FLAG_FIFO);
    if (net_error == RT_NULL)
    {
        rt_kprintf("create net error event failure\r\n");
    }

    //创建net 线程
    net_thread_id = rt_thread_create("net_th", net_entry,
    RT_NULL, 2048, 10, 15);
    if (net_thread_id != RT_NULL)
        rt_thread_startup(net_thread_id);
    else
        return -1;

    //创建data up 线程
    data_up_thread_id = rt_thread_create("data_th", data_up_entry,
    RT_NULL, 1024, 12, 15);
    if (data_up_thread_id == RT_NULL)
        return -1;

    //创建send data 线程
    send_data_thread_id = rt_thread_create("send_data_th", send_data_entry,
    RT_NULL,                //参数
            1024,           //栈大小
            12,             //优先级
            20);            //时间片

    if (send_data_thread_id != RT_NULL)
        rt_thread_startup(send_data_thread_id);
    else
        return -1;
}

void net_entry(void *parameter)
{
    rt_uint16_t recon_count = 0;
    rt_uint32_t event_status = 0;

    qsdk_mqtt_topic_t nb_test = NULL;

    net_recon:
    //nb-iot模块快快速初始化联网
    if (qsdk_nb_quick_connect() != RT_EOK)
    {
        rt_kprintf("module init failure\r\n");
        goto net_recon;
    }
    //如需测试PSM模式可以打开   进入PSM模式之后需要在finsh 输入qsdk_nb exit_psm 来退出PSM
    qsdk_nb_set_psm_mode(0, RT_NULL, RT_NULL);

    mqtt_recon: rt_thread_delay(100);
    nb_test = qsdk_mqtt_topic_init("rb_sub", 2, 0, 0, 0);   //Sub_topic：qs_l  --> 服务器下发
    rt_thread_delay(100);
    if (qsdk_mqtt_config() != RT_EOK)                                            //mqtt客户端配置：服务器，设备ID，产品APIKEY等信息
    {
        rt_kprintf("MQTT config failure\r\n");
    }
    rt_kprintf("MQTT config success\r\n");
    rt_thread_delay(100);

    if (qsdk_mqtt_open_ack_dis() != RT_EOK)
    {
        rt_kprintf("open MQTT ack failure\r\n");
    }
    rt_kprintf("open MQTT ack success\r\n");
    rt_thread_delay(100);

    if (qsdk_mqtt_set_ack_timeout(20) != RT_EOK)
    {
        rt_kprintf("Set MQTT timeout failure\r\n");
    }
    rt_kprintf("Set MQTT timeout success\r\n");
    rt_thread_delay(100);

    if (qsdk_mqtt_open() != RT_EOK)  //连接MQTT服务器
    {
        rt_kprintf("MQTT open failure\r\n");
    }
    rt_kprintf("MQTT open success\r\n");
    rt_thread_delay(100);

    if (qsdk_mqtt_unsub(nb_test) != RT_EOK)
    {
        rt_kprintf("MQTT unsub failure\r\n");
    }
    rt_kprintf("MQTT unsub success\r\n");
    rt_thread_delay(500);

    if (qsdk_mqtt_sub(nb_test) != RT_EOK)
    {
        rt_kprintf("MQTT Sub failure\r\n");
    }
    rt_kprintf("MQTT Sub success\r\n");
    rt_thread_delay(500);

    if (recon_count == 0)
        rt_thread_startup(data_up_thread_id);
    else
    {
        rt_thread_resume(data_up_thread_id);
        recon_count = 0;
    }
    while (1)
    {
        //等待网络错误事件
        if (rt_event_recv(net_error, EVENT_SEND_ERROR | EVENT_NB_REBOOT, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
        RT_WAITING_FOREVER, &event_status) == RT_EOK)
        {
            //首先挂起数据上报线程
            rt_thread_suspend(data_up_thread_id);
            //判断当前是否为数据发送错误或者更新在线时间错误
            if (event_status == EVENT_SEND_ERROR)
            {
                if (qsdk_nb_wait_connect() != RT_EOK)
                {
                    rt_kprintf("Failure to communicate with module now\r\n");
                    recon_count = 1;
                    qsdk_nb_reboot();
                    goto net_recon;
                }
                if (qsdk_nb_quick_connect() != RT_EOK)
                {
                    rt_kprintf("Now Internet has failed\r\n");
                    recon_count = 1;
                    qsdk_nb_reboot();
                    goto net_recon;
                }
                if (qsdk_mqtt_get_connect_status() != RT_EOK)
                {
                    rt_kprintf("Now the MQTT link is disconnected\r\n");
                    if (qsdk_mqtt_open() != RT_EOK)
                    {
                        rt_kprintf("Now MQTT reconnection\r\n");
                        qsdk_mqtt_close();
                        recon_count = 1;
                        goto mqtt_recon;
                    }
                }
            }
            //判断当前是否为模组异常重启
            else if (event_status == EVENT_NB_REBOOT)
            {
                if (qsdk_nb_get_net_connect_status() != RT_EOK)
                {
                    rt_kprintf("Now the nb-iot is reboot,will goto init\r\n");
                    recon_count = 1;
                    goto net_recon;
                }
            }
            rt_thread_resume(data_up_thread_id);
        }
    }
}

// /////////////////////////////////////////////SEND DATA TASK////////////////////////////////////////////////////////////////
void send_data_entry(void *parameter)
{
    /* 使用动态创建方法创建一个邮箱 */
    Send_data_mailbox = rt_mb_create("Send_data_mailbox", 4, RT_IPC_FLAG_FIFO); /* 采用FIFO方式进行线程等待 */
    /* 判断邮箱是否创建成功 */
    if (Send_data_mailbox != RT_NULL)
        rt_kprintf("Send_Data mailbox create succeed. \n");
    else
        rt_kprintf("Send_Data mailbox create failure. \n");

    while (1)
    {
        //csq质量
        csq_pub = nb_device_table.csq;
        net_sta = nb_device_table.net_connect_ok;
        imei_info = nb_device_table.imei;

        rt_sprintf((char *) NB_SendDataBuff,
                "{\"temperature\":%d.%d,\"humidity\":%d.%d,\"lux\":%d,\"MQ2\":%d,\"led_sta\":%d,\"csq_val\":%d"
                        ",\"imei\":%s,\"door_sta\":%d}", (int) temperature, (int) (temperature * 10) % 10,
                (int) humidity, (int) (humidity * 10) % 10, lux_value, (int) ppm, (int) ledflag, csq_pub, imei_info,
                door_sta);
        rt_mb_send(Send_data_mailbox, (rt_uint32_t) NB_SendDataBuff); //邮箱发送数据

        rt_thread_delay(3000);        //上报数据的频率
    }
}

void data_up_entry(void *parameter)
{
    /* 用以存放邮件 */
    char *receive_buf;

    char *buf = rt_calloc(1, 200);
    if (buf == RT_NULL)
    {
        rt_kprintf("net create buf error\r\n");
    }
    while (1)
    {
        //上报数据前首先判断当前mqtt 协议是否正常连接
        if (qsdk_mqtt_get_connect() == RT_EOK)
        {
            if (qsdk_nb_get_psm_status() == RT_EOK)
            {
                qsdk_nb_exit_psm();
            }
            //邮箱取出数据
            if (rt_mb_recv(Send_data_mailbox, (rt_ubase_t *) &receive_buf, RT_WAITING_FOREVER) == RT_EOK)
            {
                stat: if (qsdk_mqtt_pub_stream((char *) receive_buf, 1) != RT_EOK) //发布消息
                {
                    rt_kprintf("MQTT Publish Failure\r\n");
                    rt_event_send(net_error, EVENT_SEND_ERROR);
                    goto stat;
                }
                else
                {
                    rt_kprintf("MQTT Publish Success\r\n");
                }
            }
            else
            {
                rt_kprintf("no data .....\n");
            }
        }
        else
            rt_kprintf("Now MQTT Has No Connect\r\n");
        //如需测试PSM模式可以打开   进入PSM模式之后需要在finsh 输入qsdk_nb exit_psm 来退出PSM
        //qsdk_nb_enter_psm();
        rt_memset(buf, 0, sizeof(buf));
        rt_thread_delay(4000);        //上报数据的频率
    }
    /* 执行邮箱对象脱离 */
    rt_mb_detach(Send_data_mailbox);
}

// /////////////////////////////////////////////看门狗配置/////////////////////////////////////////////////////////////////
static void idle_hook(void)
{
    /* 在空闲线程的回调函数里喂狗 */
    rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
    //rt_kprintf("feed the dog!\n ");
}

int WDG_Init(void)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t timeout = 5; /* 溢出时间，单位：秒 */
    char device_name[RT_NAME_MAX] = WDT_DEVICE_NAME;

    /* 根据设备名称查找看门狗设备，获取设备句柄 */
    wdg_dev = rt_device_find(device_name);
    if (!wdg_dev)
    {
        rt_kprintf("find %s failed!\n", device_name);
        return RT_ERROR;
    }
    /* 初始化设备 */
    ret = rt_device_init(wdg_dev);
    if (ret != RT_EOK)
    {
        rt_kprintf("initialize %s failed!\n", device_name);
        return RT_ERROR;
    }

    /* 设置看门狗溢出时间 */
    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
    {
        rt_kprintf("set %s timeout failed!\n", device_name);
        return RT_ERROR;
    }
    /* 启动看门狗 */
    ret = rt_device_control(wdg_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("start %s failed!\n", device_name);
        return -RT_ERROR;
    }
    /* 设置空闲线程回调函数 */
    rt_thread_idle_sethook(idle_hook);
    return ret;
}

