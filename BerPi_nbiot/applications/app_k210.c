#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <ulog.h>
#include <string.h>
#include "led.h"
#include "lcd_spi2_drv.h"
#include "rb_nb.h"
#define SAMPLE_UART_NAME     "uart2"      /* 串口设备名称 */

extern float humidity;
extern float temperature;
extern uint16_t lux_value;
extern uint8_t ppm;
char* mask_flag = "1";
char* unmask_flag = "0";
char* opnled_flag = "openled";
char* dwnled_flag = "closeled";
uint8_t door_sta;
uint8_t UART_SendDataBuff[60];

/* 串口接收消息结构*/
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
/* 串口设备句柄 */
rt_device_t serial;

/* 初始化配置参数 */
struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

/* 消息队列控制块 */
static struct rt_messagequeue rx_mq;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL)
    {
        /* 消息队列满 */
        rt_kprintf("message queue full！\n");
    }
    return result;
}

static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[RT_SERIAL_RB_BUFSZ + 1];

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        /* 从消息队列中读取消息*/
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';
            /* 通过串口设备 serial 输出读取到的消息 */
            //rt_device_write(serial, 0, rx_buffer, rx_length);
            /* 打印数据 */
            //rt_kprintf("%s\n", rx_buffer);
            if (strcmp(rx_buffer, mask_flag) == RT_EOK)
            {
                rt_kprintf("@open the door--\n");
                rb_door_on(SUO);
                door_sta = 1;
                LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24 + 26, 30, "On ", BLACK, GREEN, 24);
                rt_thread_delay(5000);
                rb_door_off(SUO);
                door_sta = 0;
                rt_kprintf("door_sta ------ %d--\n",door_sta);
                LCD_ShowCharStr(5 + 24 + 24 + 24 + 24 + 24, 10 + 24 + 26, 30, "OFF ", BLACK, RED, 24);
            }
        }
    }
}

static int uart_dma_sample()
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    static char msg_pool[256];

    /* 查找串口设备 */
    serial = rt_device_find(SAMPLE_UART_NAME);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    /* 初始化消息队列 */
    rt_mq_init(&rx_mq, "rx_mq", msg_pool, /* 存放消息的缓冲区 */
    sizeof(struct rx_msg), /* 一条消息的最大长度 */
    sizeof(msg_pool), /* 存放消息的缓冲区大小 */
    RT_IPC_FLAG_FIFO); /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

    /* 修改串口配置参数 */
    config.baud_rate = BAUD_RATE_115200;      //修改波特率为 9600
    config.data_bits = DATA_BITS_8;           //数据位 8
    config.stop_bits = STOP_BITS_1;           //停止位 1
    config.bufsz = 128;                       //修改缓冲区 buff size 为 128
    config.parity = PARITY_NONE;              //无奇偶校验位

    /*控制串口设备。通过控制接口传入命令控制字，与控制参数 */
    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 14, 15);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }
    return ret;
}
INIT_APP_EXPORT(uart_dma_sample);
