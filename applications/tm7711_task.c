#include "tm7711.h"

#define AVG_TIME            5

#define TM7711_TICK			2
#define TM7711_PRIORITY		6
static struct rt_thread tm7711_thread;
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t tm7711_stack[ 512 ];

static void task(void * parameter)
{
    TM7711_Init();
    while(1)
    {
        uint32_t sum = 0;
        for(int i = 0; i < AVG_TIME; i++)
        {
            uint32_t v;
            TM7711_GetAD(&v, TM7711_MODE_AD_10HZ);
            sum += v;
        }
        
        sum /= AVG_TIME;
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
}
