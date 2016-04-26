#include "zigbee_task.h"

#define REMOTE_TICK			5
#define REMOTE_PRIORITY		5

static unsigned char msg_buf[256];
static rt_device_t uart;
static struct rt_thread remote_thread;
static struct rt_semaphore uart_rx_sem;
static struct rt_mutex remote_tx_mut;
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t remote_stack[ 768 ];

unsigned char local_id;

static uint32_t value_table[REMOTE_VALUE_COUNT];

void get_chip_id(handshack_t * h)
{
	uint32_t * base = (uint32_t *)0x1FFFF7E8;
	h->id0=*(base++);
	h->id1=*(base++);
	h->id2=*(base++);
}
static rt_err_t send_msg(struct remote_msg * msg)
{
    if(local_id == 0 && msg->head.cmd != REMOTE_HANDSHAKE)
        return -RT_ERROR;
    
    if(rt_mutex_take(&remote_tx_mut, 20) == RT_EOK)
    {
        rt_device_write(uart,0,&msg->head,3);
        rt_device_write(uart,0,msg->content.raw,msg->head.len);
        rt_device_write(uart,0,&msg->sum,1);
        return RT_EOK;
    }
    else
        return -RT_EBUSY;
}
uint8_t get_sum(unsigned char * data,int size)
{
	uint8_t sum=0;
	for(int i=0;i<size;i++)
	{
		sum += data[i];
	}
	return sum;
}
static rt_err_t handshack_handle(struct remote_msg * msg)
{
	handshack_t id;
	get_chip_id(&id);
	if(id.id0 == msg->content.handshack_re->id0 &&
	   id.id1 == msg->content.handshack_re->id1 &&
	   id.id2 == msg->content.handshack_re->id2)
	{
		local_id = msg->content.handshack_re->id;
		return RT_EOK;
	}
	return -RT_ERROR;
}
static rt_err_t ping_handle(struct remote_msg * msg)
{
    if(msg->content.ping->is_ans)
    {
        //TODO: update alive state
    }
    else
    {
        ping_t ping = {1};
        struct remote_msg ans =
        {
            REMOTE_HANDSHAKE_SIZE,
            local_id,
            REMOTE_HANDSHAKE,
            (uint8_t *)&ping,
            get_sum((uint8_t *)&ping,sizeof(ping_t)),
        };
        send_msg(&ans);
    }
    return RT_EOK;
}
static rt_err_t get_value_handle(struct remote_msg * msg)
{
    uint8_t id = msg->content.get_value->id;
    if(id < REMOTE_VALUE_COUNT)
    {
        get_value_re_t get_value_re = {id, value_table[id]};
        struct remote_msg ans =
        {
            REMOTE_HANDSHAKE_SIZE,
            local_id,
            REMOTE_HANDSHAKE,
            (uint8_t *)&get_value_re,
            get_sum((uint8_t *)&get_value_re,sizeof(get_value_re_t)),
        };
        send_msg(&ans);
    }
    return -RT_ENOSYS;
}
static const msg_handle handles[REMOTE_CMD_COUNT] =
{
	RT_NULL,
	handshack_handle,
    ping_handle,
    RT_NULL,
    get_value_handle,
};


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
rt_err_t readf(unsigned char * buffer,int size)
{
	for(int i=0;i<size;i++)
	{
		buffer[i] = getc();
	}
	return RT_EOK;
}
static rt_err_t receive_msg(struct remote_msg * msg)
{
	while(1)
	{
		readf((uint8_t*)&msg->head,3);
		
		if(!IS_REMOTE_CMD(msg->head.cmd))
			return -RT_ERROR;
		
		readf(msg_buf, msg->head.len);
		msg->content.raw = msg_buf;
		msg->sum = getc();
		
		if(msg->head.id!=local_id)
			continue;
		if(msg->sum!=get_sum(msg->content.raw,msg->head.len))
			return -RT_ERROR;
		
		return handles[msg->head.cmd](msg);
	}
}
rt_err_t remote_handshack()
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
	
	err = receive_msg(&msg);
	return err;
}
rt_err_t remote_error(uint8_t code)
{
    error_t error = {code};
	struct remote_msg msg=
	{
		REMOTE_HANDSHAKE_SIZE,
		local_id,
		REMOTE_HANDSHAKE,
		(uint8_t *)&error,
		get_sum((uint8_t *)&error,sizeof(error_t)),
	};
	return send_msg(&msg);
}

void remote_set_value(uint8_t id, uint32_t value)
{
    if(id < REMOTE_VALUE_COUNT)
    {
        value_table[id] = value;
    }
}

static void task(void * parameter)
{
	struct remote_msg msg;
	while(remote_handshack()!=RT_EOK)
	{
		rt_thread_delay(500);
	}
	
	while(1)
	{
		receive_msg(&msg);
		rt_thread_delay(5);
	}
}
void remote_task_init(const char * uart_name)
{
	rt_sem_init(&uart_rx_sem,"uart_rx",0,RT_IPC_FLAG_FIFO);
	rt_mutex_init(&remote_tx_mut,"remo_tx", RT_IPC_FLAG_FIFO);
	
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
