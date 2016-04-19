#ifndef __REMOTE_TASK
#define __REMOTE_TASK
#include <rtthread.h>
#include <components.h>
#include <board.h>


#define REMOTE_HANDSHAKE			0
#define REMOTE_HANDSHAKE_RESPONSE	1
#define REMOTE_PING					2
#define REMOTE_ERROR				3
#define REMOTE_GET_VALUE			4
#define REMOTE_GET_VALUE_RESPONSE	5
#define REMOTE_SET_PARAM			6
#define REMOTE_CLOSE				0xff

#define IS_REMOTE_CMD(C) ((C) <= REMOTE_SET_PARAM || (C) == REMOTE_CLOSE)

struct remote_msg
{
	uint8_t len;
	uint8_t id;
	uint8_t cmd;
	union 
	{
		uint8_t * raw;
	} content;
	uint8_t sum;
};

extern unsigned char local_id;
void remote_task_init(const char *);
#endif
