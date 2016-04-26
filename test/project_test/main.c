#include <stdio.h>
#include <stdlib.h>
#include "zigbee_task.h"
#include "stdbool.h"
int _a,_l;
#define assert(x)                                   \
    if(!( _a=(x))) {_l=__LINE__;                    \
        printf("line %d\t%s %s\n",_l,__func__,#x);  \
        return false;                               \
    }

rt_err_t handshack(void);
void set_input(uint8_t * i,int l);
unsigned char data[] = {0x01,0x00,REMOTE_HANDSHAKE_RESPONSE,0x01,0x01};
bool zigbee_handshack_test()
{
    int re;
    printf("req:\t");
    set_input(data,sizeof(data));
    re=handshack();

    printf("\n");

    printf("res:\t");
    for(int i=0;i<sizeof(data);i++)
        printf("%02X ",data[i]);
    printf("\n");
    assert(local_id == 0x01);
    assert(re == RT_EOK);
    return true;
}

int main()
{
    printf("zigbee_handshack [%s]\n",zigbee_handshack_test()?"OK":"FAIL");

    system("pause");
    return 0;
}
