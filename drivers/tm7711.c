#include "tm7711.h"
#include "stm32f10x.h"                  // Device header

#define DIN_PIN                     GPIO_Pin_6
#define DIN_NVIC                    EXTI9_5_IRQn
#define DIN_EXTI_LINE               EXTI_Line6
#define DIN_PortSource              GPIO_PortSourceGPIOB
#define DIN_PinSource               GPIO_PinSource6

#define CLK_PIN GPIO_Pin_7

#define TM7711_GPIO                 GPIOB
#define TM7711_RCC_GPIO             RCC_APB2Periph_GPIOB

#define TIMER                       TIM6
#define TIMER_WAIT                  5 

#define timer_init(a)
#define delay_us(a,us)                for(int _i=0;_i<(us);i++) {__nop();}

static struct rt_semaphore din_sem;
static rt_bool_t handled = RT_TRUE;

static void GPIO_Configure()
{
	GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(TM7711_RCC_GPIO, ENABLE);
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Pin = DIN_PIN;
	GPIO_Init(TM7711_GPIO,&gpio);
	
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin = CLK_PIN;
	GPIO_Init(TM7711_GPIO,&gpio);
	
	GPIO_EXTILineConfig(DIN_PortSource, DIN_PinSource);
}

static void EXTI_Configure()
{
	EXTI_InitTypeDef exti;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	exti.EXTI_Line = DIN_EXTI_LINE;
	exti.EXTI_Trigger = EXTI_Trigger_Falling;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);
}

static void NVIC_Configure()
{
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = DIN_NVIC;
	nvic.NVIC_IRQChannelPreemptionPriority = 2;
	nvic.NVIC_IRQChannelSubPriority = 2;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
}

void TM7711_Init()
{
    rt_sem_init(&din_sem,"tm_din",0,RT_IPC_FLAG_FIFO);
    
    GPIO_Configure();
    EXTI_Configure();
    NVIC_Configure();
    
    timer_init(TIMER);
}

void TM7711_Awake()
{
    GPIO_ResetBits(TM7711_GPIO, CLK_PIN);
}

static int32_t read24()
{
    int value = 0;
    for (int i = 0; i < 24; i++)
    {
        GPIO_SetBits(TM7711_GPIO, CLK_PIN);
        
        delay_us(TIMER, TIMER_WAIT);
        
        value <<= 1;
        value |= GPIO_ReadInputDataBit(TM7711_GPIO, DIN_PIN) != 0;
        
        GPIO_ResetBits(TM7711_GPIO, CLK_PIN);
    }
    return value;
}

void set_mode(uint8_t mode)
{
    RT_ASSERT(mode > 0 && mode <= 3);
    for (int i = 0; i < mode; i++)
    {
        GPIO_SetBits(TM7711_GPIO, CLK_PIN);
        delay_us(TIMER, TIMER_WAIT);
        GPIO_ResetBits(TM7711_GPIO, CLK_PIN);
    }
}

rt_err_t TM7711_GetAD(uint32_t * result, uint8_t mode)
{
    TM7711_Awake();
	if(rt_sem_take(&din_sem,RT_WAITING_FOREVER)==RT_EOK)
	{
        *result = read24();
        set_mode(mode);
        handled = RT_TRUE;
        return RT_EOK;
	}
    handled = RT_TRUE;
    return RT_EIO;
}

void EXTI6_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    if(handled)
    {
        handled = RT_FALSE;
        rt_sem_release(&din_sem);
    }
    /* Clear the DM9000A EXTI line pending bit */
    EXTI_ClearITPendingBit(DIN_EXTI_LINE);

    /* leave interrupt */
    rt_interrupt_leave();
}
