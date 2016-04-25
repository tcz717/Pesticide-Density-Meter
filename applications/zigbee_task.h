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
#define REMOTE_CLOSE				7

#define REMOTE_HANDSHAKE_SIZE		12
#define REMOTE_CMD_COUNT			8
#define IS_REMOTE_CMD(C) ((C) <= REMOTE_CLOSE)

typedef struct 
{
	uint32_t id0;
	uint32_t id1;
	uint32_t id2;
} handshack_t; 
typedef struct 
{
	uint32_t id0;
	uint32_t id1;
	uint32_t id2;
	uint8_t id;
} handshack_re_t;
struct remote_msg
{
	struct
	{
		uint8_t len;
		uint8_t id;
		uint8_t cmd;
	}head;
	union 
	{
		uint8_t * raw;
		handshack_t *handshack;
		handshack_re_t *handshack_re;
	} content;
	uint8_t sum;
};

typedef rt_err_t (*msg_handle)(struct remote_msg *);

extern unsigned char local_id;
void remote_task_init(const char *);
#endif
