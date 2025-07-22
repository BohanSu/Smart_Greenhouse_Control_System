#include "usart3.h"
#include "stdarg.h"	 	 
#include "stdio.h"	 	 
#include "string.h"

// USART3 interrupt service routine
// Note: reading USARTx->SR can avoid unknown errors   	
u8 USART3_RX_BUF[USART3_REC_LEN];     // receive buffer, max USART_REC_LEN bytes
// receive status
// bit15: receive complete flag
// bit14: received 0x0d
// bit13~0: number of valid bytes received
u16 USART3_RX_STA=0;       // receive status flag

void USART3_IRQHandler(void)                	// USART3 interrupt service routine
{
	u8 Res;
#if SYSTEM_SUPPORT_OS 		// if SYSTEM_SUPPORT_OS is true, need to support OS.
	OSIntEnter();    
#endif
	if(USART3->SR & USART_IT_RXNE)  // receive interrupt (received data must end with 0x0d 0x0a)
	{
		Res =USART3->DR;			// read received data
		
		if((USART3_RX_STA&0x8000)==0)// receive not complete
		{
			if(USART3_RX_STA&0x4000)// received 0x0d
			{
				if(Res!=0x0a)USART3_RX_STA=0;// receive error, restart
				else USART3_RX_STA|=0x8000;	// receive complete
			}
			else // haven't received 0X0D yet
			{	
				if(Res==0x0d)USART3_RX_STA|=0x4000;
				else
				{
					USART3_RX_BUF[USART3_RX_STA&0X3FFF]=Res ;
					USART3_RX_STA++;
					if(USART3_RX_STA>(USART3_REC_LEN-1))USART3_RX_STA=0;// receive data error, restart  
				}		 
			}
		}   		 
	} 
#if SYSTEM_SUPPORT_OS 	// if SYSTEM_SUPPORT_OS is true, need to support OS.
	OSIntExit();  											 
#endif
} 

// Initialize IO and USART3
// bound: baud rate
void USART3_Init(u32 bound){
	// GPIO port settings
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	// Enable USART3, GPIOB clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	// USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // PB.10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	// alternate function push-pull output
	GPIO_Init(GPIOB, &GPIO_InitStructure);// initialize GPIOB.10
   
	// USART3_RX	  GPIOB.11 initialize
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;// PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;// floating input
	GPIO_Init(GPIOB, &GPIO_InitStructure);// initialize GPIOB.11

	// Usart3 NVIC configuration
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;// preemption priority 3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		// sub priority 3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			// IRQ channel enable
	NVIC_Init(&NVIC_InitStructure);	// initialize VIC register according to specified parameters

	// USART initialization settings
	USART_InitStructure.USART_BaudRate = bound;// serial baud rate
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;// word length is 8-bit data format
	USART_InitStructure.USART_StopBits = USART_StopBits_1;// one stop bit
	USART_InitStructure.USART_Parity = USART_Parity_No;// no parity check bit
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;// no hardware data flow control
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	// receive and transmit mode

	USART_Init(USART3, &USART_InitStructure); // initialize serial port
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);// enable serial receive interrupt
	USART_Cmd(USART3, ENABLE);                    // enable USART3

}

// USART3, printf function
// ensure that the data sent at one time does not exceed USART3_MAX_SEND_LEN bytes
void u3_printf(char* fmt,...)  
{  
	u16 i,j; 
	va_list ap; 
	va_start(ap,fmt);
	vsprintf((char*)USART3_RX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)USART3_RX_BUF);		// length of data to be sent this time
	for(j=0;j<i;j++)							// loop to send data
	{
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC)==RESET); // loop send until transmission complete  
		USART_SendData(USART3,USART3_RX_BUF[j]); 
	} 
}
