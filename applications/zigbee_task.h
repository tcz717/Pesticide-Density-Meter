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
#define REMOTE_DEBUG				8
#define REMOTE_CMD_COUNT			9

#define REMOTE_HANDSHAKE_SIZE		12
#define REMOTE_PING_SIZE		    1
#define REMOTE_GET_VALUE_RE_SIZE    5
#define REMOTE_ERROR_SIZE		    1
#define REMOTE_CLOSE_SIZE		    0
#define IS_REMOTE_CMD(C) ((C) <= REMOTE_CLOSE)

#define REMOTE_VALUE_COUNT			8
#define REMOTE_PARAM_COUNT			8

#pragma pack(push)
#pragma pack(1)
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
typedef struct 
{
	uint8_t is_ans;
} ping_t;
typedef struct 
{
	uint8_t code;
} error_t;
typedef struct 
{
	uint8_t id;
} get_value_t;
typedef struct 
{
	uint8_t id;
    int32_t value;
} get_value_re_t;
typedef struct 
{
	uint8_t id;
    int32_t value;
} set_param_t;
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
        ping_t *ping;
        error_t *error;
        get_value_t *get_value;
        get_value_re_t *get_value_re;
        set_param_t *set_param;
	} content;
	uint8_t sum;
};

typedef rt_err_t (*msg_handle)(struct remote_msg *);

extern unsigned char local_id;
void remote_task_init(const char *);
int32_t remote_get_value(uint8_t id);
void remote_set_value(uint8_t id, int32_t value);
int32_t remote_get_param(uint8_t id);
void remote_set_param(uint8_t id, int32_t value);

rt_err_t remote_handshack(void);
rt_err_t remote_error(uint8_t code);
rt_err_t remote_close(void);
rt_err_t remote_debug(char * str);
#endif
