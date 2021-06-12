#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <U8g2lib.h>
#include <stdio.h>
#include <drv_common.h>

//定义温湿度缓存变量
extern float humidity;
extern float temperature;
extern uint16_t lux_value;
extern uint8_t set_temp;
extern uint16_t set_lux;
extern uint8_t set_adc;
extern uint8_t ppm;

static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

#define SUN 0
#define SUN_CLOUD  1
#define CLOUD      2
#define RAIN       3
#define THUNDER    4

static rt_thread_t oled_thread_id = RT_NULL;

void oled_entry(void *parameter);

static void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
    switch (symbol)
    {
    case SUN:
        u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
        u8g2.drawGlyph(x, y, 69);
        break;
    case SUN_CLOUD:
        u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
        u8g2.drawGlyph(x, y, 65);
        break;
    case CLOUD:
        u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
        u8g2.drawGlyph(x, y, 64);
        break;
    case RAIN:
        u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
        u8g2.drawGlyph(x, y, 67);
        break;
    case THUNDER:
        u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
        u8g2.drawGlyph(x, y, 67);
        break;
    }
}

static void drawWeather(uint8_t symbol, int degree)
{
    drawWeatherSymbol(0, 63, symbol);
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(55, 60);
    u8g2.print(degree);
    u8g2.print("C");
}
static void drawHumidity(uint8_t symbol, int humidity)
{
    drawWeatherSymbol(0, 63, symbol);
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(55, 60);
    u8g2.print(humidity);
    u8g2.print("%");
}
static void drawThunder(uint8_t symbol, int Thunder)
{
    drawWeatherSymbol(0, 63, symbol);
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(45, 60);
    u8g2.print(Thunder);
    u8g2.print("%");
}
static void drawMQ(int mq)
{
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(5, 55);
    u8g2.print("MQ:");
    u8g2.setCursor(56, 55);
    u8g2.print(mq);
    u8g2.print("P");
}
static void set_tempvalue(int temp)
{
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(50, 60);
    u8g2.print(temp);
    u8g2.print("C");
}
static void set_luxvalue(int lux)
{
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(50, 60);
    u8g2.print(lux);
    u8g2.print("l");
}
static void set_mqvalue(uint8_t MQ)
{
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(50, 60);
    u8g2.print(MQ);
    u8g2.print("P");
}

static void menu_setting(char *str)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.setCursor(50, 15);
    u8g2.print(str);
}

//创建oled_thread
static int oled_init(void)
{
    //创建oled 线程
    oled_thread_id = rt_thread_create("oled_th",    //名称
            oled_entry,             //线程代码
            RT_NULL,                //参数
            1024,                   //栈大小
            12,                     //优先级
            15);                    //时间片
    /*如果线程创建成功，启动这个线程*/
    if (oled_thread_id != RT_NULL)
        rt_thread_startup(oled_thread_id);
    else
        rt_kprintf("oled_thread create failure !!! \n");
    return RT_EOK;
}

INIT_APP_EXPORT(oled_init);

uint8_t menu = 0;
void oled_entry(void *parameter)
{
    u8g2.begin();
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_logisoso22_tf);
    u8g2.setCursor(0, 37);
    u8g2.print("RT-thread");               // requires enableUTF8Print()

    u8g2.setFont(u8g2_font_6x13_tr);    // choose a suitable font
    u8g2.drawStr(40, 60, "Rb_Stdio");   // write something to the internal memory

    u8g2.sendBuffer();
    rt_thread_delay(6000);

    int status = 0;
    int count = 0;
    char mstr[3];
    char hstr[3];
    time_t now;
    struct tm *p;
    int min = 0, hour = 0;
    while (1)
    {
        if (menu == 0)
        {
            if (status == 0)
            {
                if (count < 20)
                {
                    count++;
                    now = time(RT_NULL);
                    p = gmtime((const time_t*) &now);
                    hour = p->tm_hour;
                    min = p->tm_min;
                    sprintf(mstr, "%02d", min);
                    sprintf(hstr, "%02d", hour);
                    u8g2.firstPage();
                    do
                    {
                        u8g2.setFont(u8g2_font_logisoso42_tn);
                        u8g2.drawStr(0, 63, hstr);
                        u8g2.drawStr(50, 63, ":");
                        u8g2.drawStr(67, 63, mstr);
                    } while (u8g2.nextPage());
                    rt_thread_delay(50);
                }
                else
                {
                    count = 0;
                    status = 1;
                }
            }
            else if (status == 1)
            {
                if (count < 20)
                {
                    count++;
                    u8g2.clearBuffer();
                    drawWeather(SUN, (int) temperature);
                    u8g2.sendBuffer();
                    rt_thread_delay(50);
                }
                else
                {
                    count = 0;
                    status = 2;
                }
            }
            else if (status == 2)
            {
                if (count < 20)
                {
                    count++;
                    u8g2.clearBuffer();
                    drawHumidity(SUN_CLOUD, (int) humidity);
                    u8g2.sendBuffer();
                    rt_thread_delay(50);
                }
                else
                {
                    count = 0;
                    status = 3;
                }
            }
            else if (status == 3)
            {
                if (count < 20)
                {
                    count++;
                    u8g2.clearBuffer();
                    drawThunder(THUNDER, (int) lux_value);
                    u8g2.sendBuffer();
                    rt_thread_delay(50);
                }
                else
                {
                    count = 0;
                    status = 4;
                }
            }
            else if (status == 4)
            {
                if (count < 20)
                {
                    count++;
                    u8g2.clearBuffer();
                    drawMQ(ppm);
                    u8g2.sendBuffer();
                    rt_thread_delay(50);
                }
                else
                {
                    count = 0;
                    status = 0;
                }
            }
        }
        else if (menu == 1)     //设置主菜单
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_unifont_t_symbols);
            u8g2.drawGlyph(5, 13, 0x2603);

            u8g2.setFont(u8g2_font_6x13_tr);
            u8g2.setCursor(50, 15);
            u8g2.print("MENU");
            u8g2.setCursor(20, 30);
            u8g2.print("* Temp: set H/L");
            u8g2.setCursor(20, 45);
            u8g2.print("  LUX : set H/L");
            u8g2.setCursor(20, 60);
            u8g2.print("  MQ  : set H/L");
            u8g2.sendBuffer();
            rt_thread_delay(50);
        }
        else if (menu == 2)     //设置主菜单
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x13_tr);
            u8g2.setCursor(50, 15);
            u8g2.print("MENU");
            u8g2.setCursor(20, 30);
            u8g2.print("  Temp: set H/L");
            u8g2.setCursor(20, 45);
            u8g2.print("* LUX : set H/L");
            u8g2.setCursor(20, 60);
            u8g2.print("  MQ  : set H/L");
            u8g2.sendBuffer();
            rt_thread_delay(50);
        }
        else if (menu == 3)     //设置主菜单
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_6x13_tr);
            u8g2.setCursor(50, 15);
            u8g2.print("MENU");
            u8g2.setCursor(20, 30);
            u8g2.print("  Temp: set H/L");
            u8g2.setCursor(20, 45);
            u8g2.print("  LUX : set H/L");
            u8g2.setCursor(20, 60);
            u8g2.print("* MQ  : set H/L");
            u8g2.sendBuffer();
            rt_thread_delay(50);
        }
        else if (menu == 4)     //设置温度+/-
        {
            menu_setting("Setting temp");
            set_tempvalue(set_temp);
            u8g2.sendBuffer();
            rt_thread_delay(50);
        }
        else if (menu == 5)     //设置光照+/-
        {
            menu_setting("Setting lux");
            set_luxvalue(set_lux);
            u8g2.sendBuffer();
            rt_thread_delay(50);
        }
        else if (menu == 6)     //设置ADC+/-
        {
            menu_setting("Setting ADC");
            set_mqvalue(set_adc);
            u8g2.sendBuffer();
            rt_thread_delay(50);
        }
    }
}
