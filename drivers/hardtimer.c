#include "hardtimer.h"

void timer_init(TIM_TypeDef * tim)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	if(tim==TIM4)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	else if(tim==TIM6)
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	
	TIM_TimeBaseStructure.TIM_Period = 0xffff; 
	TIM_TimeBaseStructure.TIM_Prescaler = 71; 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; 
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
	TIM_TimeBaseInit(tim, &TIM_TimeBaseStructure); 
	
	TIM_ClearFlag(tim,TIM_FLAG_Update);
 
	TIM_Cmd(tim, ENABLE); 
}

void delay_us(TIM_TypeDef * tim,u16 us)
{
	TIM_SetAutoreload(tim,us);
	TIM_SetCounter(tim,0);
	TIM_ClearFlag(tim,TIM_FLAG_Update);
	while(TIM_GetFlagStatus(tim,TIM_FLAG_Update)!=SET);
}
float timer_GetSec(TIM_TypeDef * tim)
{
	float t=TIM_GetCounter(tim)/1000000.0;
	TIM_SetCounter(tim,0);
	return t;
}

