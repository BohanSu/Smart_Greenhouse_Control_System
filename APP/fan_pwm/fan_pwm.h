#ifndef __FAN_PWM_H
#define __FAN_PWM_H

#include "stm32f10x.h"

void Fan_PWM_Init(u16 per, u16 psc);
void Fan_Set_Speed_Percent(u8 percent);
u8 Fan_Get_Speed_Percent(void);

#endif
