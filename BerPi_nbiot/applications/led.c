#include "led.h"

int rb_led_init(void)
{
    rt_pin_mode(SUO, PIN_MODE_OUTPUT);
    rt_pin_write(SUO, PIN_LOW);

    rt_pin_mode(Fengshan, PIN_MODE_OUTPUT);
    rt_pin_write(Fengshan, PIN_LOW);

    rt_pin_mode(LED3, PIN_MODE_OUTPUT);
    rt_pin_write(LED3, PIN_LOW);

    return RT_EOK;
}

void rb_led_on(rt_base_t pin)
{
    rt_pin_write(pin, PIN_HIGH);
}

void rb_led_off(rt_base_t pin)
{
    rt_pin_write(pin, PIN_LOW);
}

void rb_door_on(rt_base_t pin)
{
    rt_pin_write(pin, PIN_HIGH);
}

void rb_door_off(rt_base_t pin)
{
    rt_pin_write(pin, PIN_LOW);
}

void rb_fengshan_on(rt_base_t pin)
{
    rt_pin_write(pin, PIN_HIGH);
}

void rb_fengshan_off(rt_base_t pin)
{
    rt_pin_write(pin, PIN_LOW);
}
INIT_APP_EXPORT(rb_led_init);
