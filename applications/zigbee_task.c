#include <board.h>
#include <rtthread.h>
#include <components.h>

#define ZIGBEE_TICK			5
#define ZIGBEE_PRIORITY		5
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t zigbee_stack[ 1024 ];
static struct rt_thread zigbee_thread;

static rt_device_t uart;
static struct rt_semaphore uart_rx_sem;
static rt_err_t rx_handle(rt_device_t device, rt_size_t size)
{
	char ch;
	while(size--)
	{
		rt_device_read(device,0,&ch,1);
		rt_sem_release(&uart_rx_sem);
	}
	return RT_EOK;
}
static void task(void * parameter)
{
	while(rt_sem_take(&uart_rx_sem,RT_WAITING_FOREVER)==RT_EOK)
	{
		
	}
}
void zigbee_task_init(const char * uart_name)
{
	rt_sem_init(&uart_rx_sem,"uart_rx",0,RT_IPC_FLAG_FIFO);
	
	rt_thread_init(&zigbee_thread,
		"zigbee",
		task,
		RT_NULL,
		zigbee_stack,
		sizeof(zigbee_stack),
		ZIGBEE_PRIORITY,
		ZIGBEE_TICK);
		
	uart = rt_device_find(uart_name);
	rt_device_set_rx_indicate(uart,rx_handle);
}
