#ifndef __TM7711_H
#define __TM7711_H
#include <rtthread.h>
#include <components.h>
#include <board.h>

#define TM7711_MODE_AD_10HZ         1
#define TM7711_MODE_T_40HZ          2
#define TM7711_MODE_AD_40HZ         3

void TM7711_Init(void);
rt_err_t TM7711_GetAD(int32_t * result, uint8_t mode);
#endif
