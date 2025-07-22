#ifndef __KEY_H
#define __KEY_H
#include "stm32f10x.h"
#include "SysTick.h"

// Key GPIO configuration
#define KEY0_PIN            GPIO_Pin_2      // Key KEY0 pin (PE2)
#define KEY1_PIN            GPIO_Pin_3      // Key KEY1 pin (PE3)
#define KEY2_PIN            GPIO_Pin_4      // Key KEY2 pin (PE4)
#define KEY_UP_PIN          GPIO_Pin_0      // Key KEY_UP pin (PA0)

#define KEY_PORT            GPIOE           // Key port
#define KEY_UP_PORT         GPIOA           // Key port

// Use bit-band operation to read key status
#define KEY0        PEin(2)   // PE2
#define KEY1        PEin(3)   // PE3
#define KEY2        PEin(4)   // PE4
#define KEY_UP      PAin(0)   // PA0

// Key value definition after pressed
#define KEY_UP_PRESS    1
#define KEY0_PRESS      2
#define KEY1_PRESS      3
#define KEY2_PRESS      4
#define KEY_NONE        0

// Key function declarations
void KEY_Init(void);                // Key initialization
u8 KEY_Scan(u8 mode);              // Key scan function

#endif
