#ifndef _HC05_H_
#define _HC05_H_

#include "system.h"
#include "usart3.h"

// HC05 Bluetooth module pin definitions
#define HC05_KEY_PORT 		GPIOA
#define HC05_KEY_PIN		GPIO_Pin_4
#define HC05_KEY_RCC		RCC_APB2Periph_GPIOA

#define HC05_LED_PORT 		GPIOA
#define HC05_LED_PIN		GPIO_Pin_15
#define HC05_LED_RCC		RCC_APB2Periph_GPIOA

// HC05 control pins
#define HC05_KEY		PAout(4)  // HC05 enable pin
#define HC05_LED		PAin(15)  // HC05 status pin

// HC05 module work mode
#define HC05_SLAVE_MODE		0	// Slave mode
#define HC05_MASTER_MODE	1	// Master mode

// Function declarations
u8 HC05_Init(void);
u8 HC05_Get_Role(void);
u8 HC05_Set_Role(u8 role);
void HC05_Get_Name(void);
u8 HC05_Set_Name(char* name);
void HC05_Get_Version(void);
void HC05_Get_Address(void);
u8 HC05_Set_Pin(char* pin);
void HC05_Get_Pin(void);
u8 HC05_Set_Baud(u32 baud);
void HC05_Get_Baud(void);
u8 HC05_Reset(void);
u8 HC05_Test_AT(void);
u8 HC05_Set_Class(u32 class_code);
void HC05_Get_Class(void);
u8 HC05_Set_Cmd(u8* atstr);
void HC05_CFG_CMD(u8* atstr);
void HC05_Send_Data(u8* data, u16 len);
void HC05_Process_Data(void);

#endif
