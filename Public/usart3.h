#ifndef __USART3_H
#define __USART3_H

#include "system.h"

#define USART3_REC_LEN      200  // Define maximum receive bytes 200
#define USART3_MAX_SEND_LEN 200	 // Maximum send buffer bytes
#define EN_USART3_RX 			1		// Enable (1), Disable (0) USART3 receive

extern u8  USART3_RX_BUF[USART3_REC_LEN]; // Receive buffer, max USART3_REC_LEN bytes, last byte is newline
extern u16 USART3_RX_STA;         		// Receive status flag

void USART3_Init(u32 bound);
void u3_printf(char* fmt,...);

#endif
