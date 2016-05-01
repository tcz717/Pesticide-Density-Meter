#include "tm7711.h"
#include "zigbee_task.h"

#define TM7711_AD_AVE       0
#define TM7711_AD_CUR       1
#define TM7711_AD_MASS      2

#define AVE_TIME            5

#define TM7711_TICK			2
#define TM7711_PRIORITY		6
static struct rt_thread tm7711_thread;
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t tm7711_stack[ 512 ];

static void task(void * parameter)
{
    TM7711_Init();
    remote_debug("tm7711 init finished");
    while(1)
    {
        uint32_t sum = 0;
        uint32_t raw_mass;
        for(int i = 0; i < AVE_TIME; i++)
        {
            uint32_t v;
            while(TM7711_GetAD(&v, TM7711_MODE_AD_10HZ) != RT_EOK &&
                v != 0xFFFFFF)
                __nop();
            remote_set_value(TM7711_AD_CUR ,v);
            sum += v;
        }
        
        sum /= AVE_TIME;
        raw_mass = ((uint64_t)sum)*1000*2000>>23>>7;
        remote_set_value(TM7711_AD_AVE ,sum);
        remote_set_value(TM7711_AD_MASS ,((uint64_t)sum)*1000*2000>>23>>7);
//        remote_debug("tm7711 got value");
        rt_thread_delay(1);
    }
}
void tm7711_task_init()
{
	rt_thread_init(&tm7711_thread,
		"tm7711",
		task,
		RT_NULL,
		tm7711_stack,
		sizeof(tm7711_stack),
		TM7711_PRIORITY,
		TM7711_TICK);
    rt_thread_startup(&tm7711_thread);
}
