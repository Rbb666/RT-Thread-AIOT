#include "rb_nb.h"

#include <at.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef RT_USING_ULOG
#define LOG_TAG "[QSDK/MQTT]"
#ifdef QSDK_USING_LOG
#define LOG_LVL LOG_LVL_DBG
#else
#define LOG_LVL LOG_LVL_INFO
#endif
#include <ulog.h>
#else
#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME "[QSDK/MQTT]"
#ifdef QSDK_USING_LOG
#define DBG_LEVEL DBG_LOG
#else
#define DBG_LEVEL DBG_INFO
#endif /* QSDK_DEBUG */

#include <rtdbg.h>
#endif

//引用AT命令部分
extern at_response_t nb_resp;
extern at_client_t nb_client;

typedef struct
{
    char *suback_topic;
    char *puback_topic;
    char *pub_mesg;
    int connect_status;
    int error_type;
    int suback_status;
    int unsuback_status;
    int puback_status;
    int puback_open;
    int pub_mesg_len;
    int pubrec_status;
    int pubcomp_status;
    int packid;
} MQTT_DEVICE;

MQTT_DEVICE mqtt_device;
static struct mqtt_stream mqtt_stream_table[MQTT_TOPIC_MAX] = {0};

/*************************************************************
 *   函数名称：   qsdk_mqtt_config
 *
 *   函数功能：   MQTT客户端配置
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_config(void)
{
    memset(&mqtt_device, 0, sizeof(mqtt_device));
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTCFG=\"%s\",%s,\"%s\",%d,\"%s\",\"%s\",1\r\n", MQTT_ADDRESS, MQTT_PORT, MQTT_CLIENT_ID, MQTT_KeepAlive,
          MQTT_USER, MQTT_PASSWD);
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTCFG=\"%s\",%s,\"%s\",%d,\"%s\",\"%s\",1", MQTT_ADDRESS, MQTT_PORT,
                        MQTT_CLIENT_ID, MQTT_KeepAlive, MQTT_USER, MQTT_PASSWD) != RT_EOK)
    {
        LOG_E("mqtt config failure\r\n");
        return RT_ERROR;
    }
    return RT_EOK;
}
/*************************************************************
 *   函数名称：   qsdk_mqtt_check_config
 *
 *   函数功能：   查询 MQTT客户端配置
 *
 *   入口参数：   cmd:命令      result:需要判断的响应
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_check_config(void)
{
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTCFG?\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTCFG?") != RT_EOK)
    {
        LOG_E("check mqtt config failure\r\n");
        return RT_ERROR;
    }
    if (at_resp_get_line_by_kw(nb_resp, MQTT_ADDRESS) != RT_NULL)
        return RT_EOK;
    else
        return RT_ERROR;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_open
 *
 *   函数功能：   连接MQTT服务器
 *
 *   入口参数：   cmd:命令      result:需要判断的响应
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
qsdk_mqtt_topic_t qsdk_mqtt_topic_init(const char *topic, int qos, int retain, int dup, int mesg_type)
{
    int num = 0;
    for (num = 0; num < MQTT_TOPIC_MAX && mqtt_stream_table[num].user_status; num++)
        ;
    if (num >= MQTT_TOPIC_MAX)
    {
        LOG_E("max error\r\n");
        return RT_NULL;
    }
    if (strcpy(mqtt_stream_table[num].topic, topic) != NULL)
    {
#ifdef QSDK_USING_DBUG
        LOG_D("topic=%s\r\n", topic);
        LOG_D("topic=%s   num=%d\r\n", mqtt_stream_table[num].topic, num);
#endif
        mqtt_stream_table[num].qos = qos;
        mqtt_stream_table[num].retain = retain;
        mqtt_stream_table[num].dup = dup;
        mqtt_stream_table[num].mesg_type = mesg_type;
        mqtt_stream_table[num].user_status = 1;
        return &mqtt_stream_table[num];
    }
    LOG_E("topic init error\r\n");
    return RT_NULL;
}
/*************************************************************
 *   函数名称：   qsdk_mqtt_open
 *
 *   函数功能：   连接MQTT服务器
 *
 *   入口参数：   cmd:命令      result:需要判断的响应
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_open(void)
{
    int count = 50;
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTOPEN=1,1,1,1,1,\"mywill\",\"001bye\"") != RT_EOK)
    {
        LOG_E("mqtt open failure\r\n");
        return RT_ERROR;
    }
    do
    {
        count--;
        rt_thread_delay(50);
    } while (count > 0 && mqtt_device.connect_status == 0);
    if (count <= 0)
        return RT_ERROR;
    else
        return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_get_connect_status
 *
 *   函数功能：   获取MQTT当前状态
 *
 *   入口参数：   无
 *
 *   返回参数：   0-5：成功   -1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_get_connect_status(void)
{
    int status = 0;
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTSTAT?\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTSTAT?") != RT_EOK)
    {
        LOG_E("get mqtt status failure\r\n");
        return -1;
    }
    at_resp_parse_line_args(nb_resp, 2, "+MQTTSTAT:%d", &status);
    return status;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_sub
 *
 *   函数功能：   订阅主题消息
 *
 *   入口参数：   topic:需要订阅的主题
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_sub(qsdk_mqtt_topic_t topic)
{
    int count = 50;
    mqtt_device.suback_status = 0x80;
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTSUB=\"%s\",%d,0\r\n", topic->topic, topic->qos);
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTSUB=\"%s\",%d,0", topic->topic, topic->qos) != RT_EOK)
    {
        LOG_E("mqtt sub topic failure\r\n");
        return RT_ERROR;
    }

    do
    {
        count--;
        rt_thread_delay(50);
    } while (count > 0 && mqtt_device.suback_status == 0x80);

    if (count <= 0)
        return RT_ERROR;
    topic->suback_ststus = 1;
    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_check_sub
 *
 *   函数功能：   查询订阅消息
 *
 *   入口参数：   topic：需要查询的主题
 *
 *   返回参数：   0：已订阅   1：未订阅
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_check_sub(qsdk_mqtt_topic_t topic)
{
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTSUB?\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTSUB?") != RT_EOK)
    {
        LOG_E("mqtt check sub failure\r\n");
        return -1;
    }
    if (at_resp_get_line_by_kw(nb_resp, topic->topic) != RT_NULL)
        return RT_EOK;

    LOG_E("no find topic\r\n");
    return RT_ERROR;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_pub_one_stream
 *
 *   函数功能：   发布消息
 *
 *   入口参数：   mesg:发布的消息      qos:信号质量
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_pub_stream(char *mesg, int qos)
{
    int count = 50;
    char *str = rt_malloc(rt_strlen(mesg) * 2);
    if (str == RT_NULL)
    {
        LOG_E("mqtt pub stream malloc failure");
        return RT_ERROR;
    }
    if (string_to_hex(mesg, strlen(mesg), str) != RT_EOK)
    {
        LOG_E("mesg to hex failure\r\n");
        return RT_ERROR;
    }
    mqtt_device.puback_status = 0;
    mqtt_device.pubcomp_status = 0;
    mqtt_device.pubrec_status = 0;
    //AT+MQTTPUB=$dp,1,1,0,0,"hello-5310a"
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTPUB=\"rb_pub\",%d,0,0,%d,%s", qos, strlen(str) / 2, str) != RT_EOK)
    {
        LOG_E("mqtt pub send failure\r\n");
        return RT_ERROR;
    }
    rt_free(str);
    if (qos == 1)
    {
        do
        {
            count--;
            rt_thread_delay(100);
        } while (count > 0 && mqtt_device.puback_status == 0);

        if (count <= 0)
        {
            LOG_E("mqtt puback failure\r\n");
            return RT_ERROR;
        }
    }
    else if (qos == 2)
    {
        do
        {
            count--;
            rt_thread_delay(100);
        } while (count > 0 && mqtt_device.pubrec_status == 0);

        if (count <= 0)
        {
            LOG_E("mqtt pubrec failure\r\n");
            return RT_ERROR;
        }
        count = 50;
        do
        {
            count--;
            rt_thread_delay(100);
        } while (count > 0 && mqtt_device.pubcomp_status == 0);

        if (count <= 0)
        {
            LOG_E("mqtt pubcomp failure\r\n");
            return RT_ERROR;
        }
    }

    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_unsub
 *
 *   函数功能：   取消订阅
 *
 *   入口参数：
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_unsub(qsdk_mqtt_topic_t topic)
{
    int count = 50;
    mqtt_device.unsuback_status = 0;
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTUNSUB=\"%s\"\r\n", topic->topic);
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTUNSUB=\"%s\"", topic->topic) != RT_EOK)
    {
        LOG_E("mqtt unsub send failure\r\n");
        return RT_ERROR;
    }

    do
    {
        count--;
        rt_thread_delay(50);
    } while (count > 0 && mqtt_device.unsuback_status == 0);

    if (count <= 0)
    {
        LOG_E("mqtt sub failure\r\n");
        return RT_ERROR;
    }
    topic->suback_ststus = 0;
    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_close
 *
 *   函数功能：   断开连接
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_close(void)
{
    int count = 50;
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTDISC\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTDISC") != RT_EOK)
    {
        LOG_E("mqtt close send failure\r\n");
        return RT_ERROR;
    }

    do
    {
        count--;
        rt_thread_delay(50);
    } while (count > 0 && mqtt_device.connect_status == 1);

    if (count <= 0)
    {
        LOG_E("mqtt close failure\r\n");
        return RT_ERROR;
    }

    return RT_EOK;
}
/*************************************************************
 *   函数名称：   qsdk_mqtt_delete
 *
 *   函数功能：   删除 MQTT客户端
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_delete(void)
{
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTDEL\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTDEL") != RT_EOK)
    {
        LOG_E("mqtt client delete send failure\r\n");
        return RT_ERROR;
    }
    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_set_ack_timeout
 *
 *   函数功能：   设置 ACK 超时时间
 *
 *   入口参数：   timeout: ACK超时时间
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_set_ack_timeout(int timeout)
{
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTTO=%d\r\n", timeout);
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTTO=%d", timeout) != RT_EOK)
    {
        LOG_E("mqtt set ack timeout failure\r\n");
        return RT_ERROR;
    }
    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_open_ack_dis
 *
 *   函数功能：   开启 MQTT客户端 ACK显示
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_open_ack_dis(void)
{
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTPING=1\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTPING=1") != RT_EOK)
    {
        LOG_E("mqtt open ack display failure\r\n");
        return RT_ERROR;
    }
    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_close_ack_dis
 *
 *   函数功能：   关闭 MQTT客户端 ACK显示
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_close_ack_dis(void)
{
#ifdef QSDK_USING_LOG
    LOG_D("AT+MQTTPING=0\r\n");
#endif
    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+MQTTPING=0") != RT_EOK)
    {
        LOG_E("mqtt close ack display failure\r\n");
        return RT_ERROR;
    }
    return RT_EOK;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_get_connect
 *
 *   函数功能：   关闭 MQTT客户端 ACK显示
 *
 *   入口参数：   无
 *
 *   返回参数：   0：连接成功   1：连接失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_get_connect(void)
{
    if (mqtt_device.connect_status)
        return RT_EOK;
    return RT_ERROR;
}
/*************************************************************
 *   函数名称：   qsdk_mqtt_get_error_type
 *
 *   函数功能：   查询MQTT错误类型
 *
 *   入口参数：   无
 *
 *   返回参数：   错误类型
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_get_error_type(void)
{
    return mqtt_device.error_type;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_event_func
 *
 *   函数功能：   MQTT 模块下发回调函数
 *
 *   入口参数：   无
 *
 *   返回参数：   错误类型
 *
 *   说明：
 *************************************************************/
void mqtt_event_func(char *event)
{
    char *result = NULL;
#ifdef QSDK_USING_LOG
    LOG_D("%s\r\n", event);
#endif
    if (rt_strstr(event, "+MQTTOPEN:") != RT_NULL)
    {
        if (rt_strstr(event, "+MQTTOPEN:OK") != RT_NULL)
            mqtt_device.connect_status = 1;
        else
            mqtt_device.connect_status = 0;
    }
    if (rt_strstr(event, "+MQTTSUBACK:") != RT_NULL)
    {
        char *id = NULL;
        char *code = NULL;
        /* 获取第一个子字符串 */
        result = strtok((char *)event, ":");
        /* 继续获取其他的子字符串 */
        id = strtok(NULL, ",");
        code = strtok(NULL, ",");
        mqtt_device.suback_topic = strtok(NULL, ",");
        mqtt_device.suback_status = atoi(code);
#ifdef QSDK_USING_DEBUG
        LOG_D("suback_topic=%s\r\n", mqtt_device.suback_topic);
        LOG_D("suback_status=%d\r\n", mqtt_device.suback_status);
#endif
    }
    if (rt_strstr(event, "+MQTTPUBACK:") != RT_NULL)
    {
        mqtt_device.puback_status = 1;
    }
    if (rt_strstr(event, "+MQTTUNSUBACK:") != RT_NULL)
    {
        mqtt_device.unsuback_status = 1;
    }
    if (rt_strstr(event, "+MQTTDISC:") != RT_NULL)
    {
        if (rt_strstr(event, "+MQTTDISC:OK") != RT_NULL)
            mqtt_device.connect_status = 0;
    }
    if (rt_strstr(event, "+MQTTPUBREC:") != RT_NULL)
    {
        mqtt_device.pubrec_status = 1;
    }
    if (rt_strstr(event, "+MQTTPUBCOMP:") != RT_NULL)
    {
        mqtt_device.pubcomp_status = 1;
    }
    if (rt_strstr(event, "+MQTTPUBLISH:") != RT_NULL)
    {
        char *dup = NULL;
        char *qos = NULL;
        char *retained = NULL;
        char *packId = NULL;
        result = strtok((char *)event, ":");
        dup = strtok(NULL, ",");
        qos = strtok(NULL, ",");
        retained = strtok(NULL, ",");
        mqtt_device.packid = atoi(strtok(NULL, ","));
        mqtt_device.puback_topic = strtok(NULL, ",");
        mqtt_device.pub_mesg_len = atoi(strtok(NULL, ","));
        mqtt_device.pub_mesg = strtok(NULL, ",");

        if (!mqtt_device.puback_open)
        {
#ifdef QSDK_USING_LOG
            LOG_D("enter mqtt callback\r\n");
#endif
            if (qsdk_mqtt_data_callback(mqtt_device.puback_topic, mqtt_device.pub_mesg,
                                        mqtt_device.pub_mesg_len) != RT_EOK)
            {
                LOG_E("mqtt topic callback failure\r\n");
            }
        }
    }
    if (rt_strstr(event, "+MQTTPINGRESP:") != RT_NULL)
    {
        mqtt_device.connect_status = 1;
    }
    if (rt_strstr(event, "+MQTTTO:") != RT_NULL)
    {
        char *type = NULL;
        result = strtok((char *)event, ":");
        type = strtok(NULL, ":");
        mqtt_device.error_type = atoi(type);
        if (mqtt_device.error_type == 5)
            mqtt_device.connect_status = 0;
    }
    if (rt_strstr(event, "+MQTTREC:") != RT_NULL)
    {
    }
}
