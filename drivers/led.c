/*
 * File      : led.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */
#include <rtthread.h>
#include <stm32f10x.h>
#include "zigbee_task.h"

#define LED_MASS_STD        2
#define LED_MODE            3

#define LED_MASS_MODE       0
#define LED_FLOW_MODE       1

#define LED_MASS_STD_DEFAULT    80
#define LED_MODE_DEFAULT        LED_MASS_MODE

#define TM7711_AD_RMASS     3

static GPIO_TypeDef * led_gpio[16] = 
{
    GPIOA,  
    GPIOA, 
    GPIOA, 
    GPIOA, 
    GPIOA, 
    GPIOA, 
    GPIOA, 
    GPIOB, 
    
    GPIOB, 
    GPIOB, 
    GPIOB, 
    GPIOB, 
    GPIOB, 
    GPIOB, 
    GPIOB, 
    GPIOA, 
};
static const uint16_t led_pin[16] = 
{
    GPIO_Pin_1,  
    GPIO_Pin_2, 
    GPIO_Pin_3, 
    GPIO_Pin_4, 
    GPIO_Pin_5, 
    GPIO_Pin_6, 
    GPIO_Pin_7, 
    GPIO_Pin_0, 
    
    GPIO_Pin_1, 
    GPIO_Pin_10, 
    GPIO_Pin_11, 
    GPIO_Pin_12, 
    GPIO_Pin_13, 
    GPIO_Pin_14, 
    GPIO_Pin_15, 
    GPIO_Pin_8, 
};

static rt_thread_t led_thread;

void rt_hw_led_on(rt_uint32_t led)
{
    if(led < 16)
        GPIO_SetBits(led_gpio[led], led_pin[led]);
}
void rt_hw_led_off(rt_uint32_t led)
{
    if(led < 16)
        GPIO_ResetBits(led_gpio[led], led_pin[led]);
}

static void flow_mode()
{
    for(int i = 0; i < 16; i++)
    {
        int prev = (i - 1 + 16) % 16;
        int next = (i + 1 + 16) % 16;
        rt_hw_led_on(next);
        rt_hw_led_off(prev);
        
        rt_thread_delay(1);
    }
}

static void mass_mode()
{
    int std = remote_get_param(LED_MASS_STD);
    int rmass = remote_get_value(TM7711_AD_RMASS);
    int det = std / 20;
    int n = (rmass - std) / det + 7;
    if (n > 14)
        n = 14;
    else if (n < 0)
        n = 0;
    
    for(int i = 0; i < 16; i++)
        rt_hw_led_off(i);
    
    rt_hw_led_on(n);
    rt_hw_led_on(n + 1);
    
    rt_thread_delay(2);
}

static void led_main(void * param)
{
    while(1)
    {
        switch(remote_get_param(LED_MODE))
        {
            case LED_MASS_MODE:
                mass_mode();
                break;
            case LED_FLOW_MODE:
                flow_mode();
                break;
        }
    }
}

void rt_hw_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB,ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    for(int i = 0; i < 16; i++)
    {
        GPIO_InitStructure.GPIO_Pin = led_pin[i];
        GPIO_Init(led_gpio[i], &GPIO_InitStructure);
        GPIO_ResetBits(led_gpio[i], led_pin[i]);
    }
    
    remote_set_param(LED_MASS_STD, LED_MASS_STD_DEFAULT);
    remote_set_param(LED_MODE, LED_MODE_DEFAULT);
    
    led_thread = rt_thread_create("led",led_main,RT_NULL,512,7,1);
    rt_thread_startup(led_thread);
}


