#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include <string.h>

/* 串口设备句柄 */
rt_device_t lpserial;

#define LPUART_NAME   "lpuart1"      /* 串口设备名称 */
uint8_t UART_SendDataBuff[56];

extern float humidity;
extern float temperature;
extern uint16_t lux_value;
extern uint8_t ppm;

static void lpuart1_serial_thread(void *parameter)
{

    while (1)
    {
        rt_memset(UART_SendDataBuff, 0x00, sizeof(UART_SendDataBuff));
        rt_sprintf((char *) UART_SendDataBuff,
                "{\"temperature\":%d.%d,\"humidity\":%d.%d,\"lux\":%d,\"ppm\":%d}",
                    (int) temperature, (int) (temperature * 10) % 10,
                        (int) humidity, (int) (humidity * 10) % 10,
                            lux_value, (int) ppm);

        rt_device_write(lpserial, 0, &UART_SendDataBuff, (sizeof(UART_SendDataBuff) - 1));
        rt_thread_delay(2500); //上报数据的频率

    }
}

static int uart_sample(void)
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];

    /* 查找串口设备 */
    lpserial = rt_device_find(LPUART_NAME);
    if (!lpserial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    rt_device_open(lpserial, RT_DEVICE_FLAG_INT_RX);

    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("lpuart1_serial", lpuart1_serial_thread, RT_NULL, 1024, 14, 25);
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
/* 导出到 msh 命令列表中 */
INIT_APP_EXPORT(uart_sample);
