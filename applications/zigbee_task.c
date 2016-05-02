#include "zigbee_task.h"
#include "string.h"

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
rt_bool_t connected;

static int32_t value_table[REMOTE_VALUE_COUNT];
static int32_t param_table[REMOTE_PARAM_COUNT];

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
        if(msg->head.len > 0)
        {
            rt_device_write(uart,0,msg->content.raw,msg->head.len);
        }
        rt_device_write(uart,0,&msg->sum,1);
        
        rt_mutex_release(&remote_tx_mut);
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
        connected = RT_TRUE;
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
            REMOTE_PING_SIZE,
            local_id,
            REMOTE_PING,
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
    get_value_re_t get_value_re = {id};
    if(id < REMOTE_VALUE_COUNT)
    {
        get_value_re.value = value_table[id];
    }
    else
    {
        get_value_re.value = 0;
    }
    
    struct remote_msg ans =
    {
        REMOTE_GET_VALUE_RE_SIZE,
        local_id,
        REMOTE_GET_VALUE_RESPONSE,
        (uint8_t *)&get_value_re,
        get_sum((uint8_t *)&get_value_re,sizeof(get_value_re_t)),
    };
    send_msg(&ans);
    return RT_EOK;
}
static rt_err_t set_param_handle(struct remote_msg * msg)
{
    uint8_t id = msg->content.set_param->id;
    if(id < REMOTE_PARAM_COUNT)
    {
        param_table[id] = msg->content.set_param->value;
        return RT_EOK;
    }
    else
    {
        return -RT_ENOSYS;
    }
}
static rt_err_t close_handle(struct remote_msg * msg)
{
    connected = RT_FALSE;
    return RT_EOK;
}
static const msg_handle handles[REMOTE_CMD_COUNT] =
{
	RT_NULL,
	handshack_handle,
    ping_handle,
    RT_NULL,
    get_value_handle,
    RT_NULL,
    set_param_handle,
    close_handle,
    RT_NULL,
};


static rt_err_t rx_handle(rt_device_t device, rt_size_t size)
{
    rt_sem_release(&uart_rx_sem);
	return RT_EOK;
}
rt_bool_t getc(uint8_t * ch)
{
    if (rt_sem_take(&uart_rx_sem, 5) != RT_EOK) return RT_FALSE;
    while (rt_device_read(uart, 0, ch, 1) != 1);
	return RT_TRUE;
}
rt_err_t readf(unsigned char * buffer,int size)
{
	for(int i=0;i<size;i++)
	{
		if(!getc(&buffer[i]))
            return -RT_ETIMEOUT;
	}
	return RT_EOK;
}
static rt_err_t receive_msg(struct remote_msg * msg)
{
	while(1)
	{
		if(readf((uint8_t*)&msg->head,3)!=RT_EOK)
            return -RT_ETIMEOUT;
		
		if(!IS_REMOTE_CMD(msg->head.cmd))
			return -RT_ERROR;
		
		if(readf(msg_buf, msg->head.len)!=RT_EOK)
            return -RT_ETIMEOUT;
		msg->content.raw = msg_buf;
		if(!getc(&msg->sum))
            return -RT_ETIMEOUT;
		
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
rt_err_t remote_ping()
{
    ping_t ping = {0};
    struct remote_msg ans =
    {
        REMOTE_PING_SIZE,
        local_id,
        REMOTE_PING,
        (uint8_t *)&ping,
        get_sum((uint8_t *)&ping,sizeof(ping_t)),
    };
	return send_msg(&ans);
}
rt_err_t remote_error(uint8_t code)
{
    error_t error = {code};
	struct remote_msg msg=
	{
		REMOTE_ERROR_SIZE,
		local_id,
		REMOTE_ERROR,
		(uint8_t *)&error,
		get_sum((uint8_t *)&error,sizeof(error_t)),
	};
	return send_msg(&msg);
}

rt_err_t remote_close()
{
	struct remote_msg msg=
	{
		REMOTE_CLOSE_SIZE,
		local_id,
		REMOTE_CLOSE,
		RT_NULL,
		get_sum(RT_NULL,REMOTE_CLOSE_SIZE),
	};
	return send_msg(&msg);
}
rt_err_t remote_debug(char * str)
{
    uint8_t len = strlen(str);
	struct remote_msg msg=
	{
		len,
		local_id,
		REMOTE_DEBUG,
		(uint8_t *)str,
		get_sum((uint8_t *)str,len),
	};
	return send_msg(&msg);
}

void remote_set_value(uint8_t id, int32_t value)
{
    if(id < REMOTE_VALUE_COUNT)
    {
        value_table[id] = value;
    }
}
int32_t remote_get_value(uint8_t id)
{
    if(id < REMOTE_VALUE_COUNT)
    {
        return value_table[id];
    }
    return 0;
}
void remote_set_param(uint8_t id, int32_t value)
{
    if(id < REMOTE_PARAM_COUNT)
    {
        param_table[id] = value;
    }
}
int32_t remote_get_param(uint8_t id)
{
    if(id < REMOTE_PARAM_COUNT)
    {
        return param_table[id];
    }
    return 0;
}

static void task(void * parameter)
{
    USART_GetFlagStatus(USART1, USART_FLAG_TC);
    while(1)
    {
        struct remote_msg msg;
        uint32_t retry = 0;
        connected = RT_FALSE;
        while(remote_handshack() != RT_EOK)
        {
            rt_thread_delay(50);
        }
        
        remote_debug("Handshack OK");
        
        while(connected)
        {
            if(receive_msg(&msg) != RT_EOK)
                retry++;
            else
                retry = 0;
            
            if(retry > 2002)
                connected = RT_FALSE;
            else if(retry > 2000)
                remote_ping();
            
            rt_thread_delay(5);
        }
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
    rt_device_open(uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX | \
                       RT_DEVICE_FLAG_STREAM);
	rt_device_set_rx_indicate(uart,rx_handle);
        
    rt_thread_startup(&remote_thread);
}
