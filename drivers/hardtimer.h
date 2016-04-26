#ifndef __HARDTIMER_H__
#define __HARDTIMER_H__
#include "stm32f10x.h"                  // Device header

void Timer4_init(void);
float Timer4_GetSec(void);
void timer_init(TIM_TypeDef * tim);
void delay_us(TIM_TypeDef * tim,u16 us);
#endif
