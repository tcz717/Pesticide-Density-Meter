#include "zigbee_task.h"
#include "stdio.h"

#define REMOTE_TICK			5
#define REMOTE_PRIORITY		5

static unsigned char msg_buf[256];
#if 0
static rt_device_t uart;
static struct rt_thread remote_thread;
static struct rt_semaphore uart_rx_sem;
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t remote_stack[ 1024 ];
#endif // 0

unsigned char local_id;
rt_err_t handshack_handle(struct remote_msg * msg)
{
	local_id = msg->content.handshack_re->id;
	return RT_EOK;
}
static const msg_handle handles[REMOTE_CMD_COUNT] =
{
	RT_NULL,
	handshack_handle,
};

#if 0
static rt_err_t rx_handle(rt_device_t device, rt_size_t size)
{
	while(size)
	{
		rt_sem_release(&uart_rx_sem);
		size--;
	}
	return RT_EOK;
}
unsigned char getc()
{
	if(rt_sem_take(&uart_rx_sem,RT_WAITING_FOREVER)==RT_EOK)
	{
		unsigned char ch;
		rt_device_read(uart,0,&ch,1);
		return ch;
	}
	return 0;
}
#else // 0
uint8_t * input;
int len;
int uart=0;
void set_input(uint8_t * i,int l)
{
    input=i;
    len=l;
}
void rt_device_write(int dump,int pos,void* d,int size)
{
    for(int i =0;i<size;i++)
    {
        printf("%02X ",((uint8_t*)d)[pos+i] );
    }
}
unsigned char igetc()
{
    if(len)
    {
        len--;
        return *(input++);
    }
	return 0;
}
#endif
rt_err_t readf(unsigned char * buffer,int size)
{
	for(int i=0;i<size;i++)
	{
		buffer[i] = igetc();
	}
	return RT_EOK;
}

uint8_t get_sum(unsigned char * data,int size)
{
	uint8_t sum=0;
	for(int i=0;i<size;i++)
	{
		sum+=data[i];
	}
	return sum;
}
static rt_err_t send_msg(struct remote_msg * msg)
{
	rt_device_write(uart,0,&msg->head,3);
	rt_device_write(uart,0,msg->content.raw,msg->head.len);
	rt_device_write(uart,0,&msg->sum,1);
	return RT_EOK;
}
static rt_err_t receive_msg(struct remote_msg * msg)
{
	while(1)
	{
		readf((uint8_t*)&msg->head,3);

		if(!IS_REMOTE_CMD(msg->head.cmd))
			return RT_ERROR;

		readf(msg_buf, msg->head.len);
		msg->content.raw=msg_buf;
		msg->sum = igetc();

		if(msg->head.id!=local_id)
			continue;
		if(msg->sum!=get_sum(msg->content.raw,msg->head.len))
			return RT_ERROR;

		return handles[msg->head.cmd](msg);
	}
}
void get_chip_id(handshack_t * h)
{
//	uint32_t * base = (uint32_t *)0x1FFFF7E8;
//	h->id0=*(base++);
//	h->id1=*(base++);
//	h->id2=*(base++);
	h->id0=0x111111ab;
	h->id1=0x222222cd;
	h->id2=0x333333ef;
}
rt_err_t handshack()
{
	rt_err_t err;
	handshack_t h;
	get_chip_id(&h);

	struct remote_msg msg=
	{
		REMOTE_HANDSHAKE_SIZE,
		0,
		REMOTE_HANDSHAKE,
		(uint8_t *)&h,
		get_sum((uint8_t *)&h,sizeof(handshack_t)),
	};
	send_msg(&msg);

	err=receive_msg(&msg);
	return err;
}
#if 0
void remote_task_init(const char * uart_name)
{
	rt_sem_init(&uart_rx_sem,"uart_rx",0,RT_IPC_FLAG_FIFO);

	rt_thread_init(&remote_thread,
		"remote",
		task,
		RT_NULL,
		remote_stack,
		sizeof(remote_stack),
		REMOTE_PRIORITY,
		REMOTE_TICK);

	uart = rt_device_find(uart_name);
	rt_device_set_rx_indicate(uart,rx_handle);
}
#endif
