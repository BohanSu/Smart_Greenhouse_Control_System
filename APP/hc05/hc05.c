#include "hc05.h"
#include "usart3.h"
#include "SysTick.h"
#include "greenhouse_control.h"
#include "stdio.h"
#include <string.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

/*******************************************************************************
* Function Name  : HC05_Init
* Description    : HC05 Bluetooth module initialization
* Input          : None
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// Enable GPIO clock
	RCC_APB2PeriphClockCmd(HC05_KEY_RCC | HC05_LED_RCC, ENABLE);
	
	// Configure HC05_KEY pin as push-pull output
	GPIO_InitStructure.GPIO_Pin = HC05_KEY_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(HC05_KEY_PORT, &GPIO_InitStructure);
	
	// Configure HC05_LED pin as input
	GPIO_InitStructure.GPIO_Pin = HC05_LED_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(HC05_LED_PORT, &GPIO_InitStructure);
	
	// Disable JTAG to free PA15 for GPIO use (CRITICAL for HC05!)
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	printf("HC05: JTAG disabled, PA15 freed for LED detection\\r\\n");
	
	USART3_Init(9600); // Initialize serial port, baud rate 9600
	
	HC05_KEY = 0; // Exit AT mode
	delay_ms(500); // Wait for module stable
	
	// Configure HC05 module with complete settings
	printf("HC05: Starting module configuration...\\r\\n");
	
	// Test AT command communication
	if(HC05_Test_AT() == 0) {
		printf("HC05: AT communication test - OK\\r\\n");
	} else {
		printf("HC05: AT communication test - FAILED\\r\\n");
	}
	
	// Set role to slave mode
	if(HC05_Set_Role(0) == 0) {
		printf("HC05: Role set to Slave mode\\r\\n");
	} else {
		printf("HC05: Failed to set role\\r\\n");
	}
	
	// Set device name to "SBH"
	if(HC05_Set_Name("SBH") == 0) {
		printf("HC05: Device name set to 'SBH'\\r\\n");
	} else {
		printf("HC05: Failed to set device name\\r\\n");
	}
	
	// Set pairing PIN to 1234 for security
	if(HC05_Set_Pin("1234") == 0) {
		printf("HC05: Pairing PIN set to '1234'\\r\\n");
	} else {
		printf("HC05: Failed to set PIN\\r\\n");
	}
	
	// Display final configuration
	{
		u8 role; // 声明变量在代码块开始处
		printf("\\r\\n=== HC05 Configuration Summary ===\\r\\n");
		printf("Device Name: ");
		HC05_Get_Name();
		printf("Device Role: ");
		role = HC05_Get_Role();
		if(role == 0) printf("Slave Mode\\r\\n");
		else if(role == 1) printf("Master Mode\\r\\n");
		else printf("Unknown\\r\\n");
		printf("===================================\\r\\n");
	}
	
	printf("HC05: Module configuration complete!\\r\\n");
	return 0;
}

/*******************************************************************************
* Function Name  : HC05_Get_Role
* Description    : Get HC05 role
* Input          : None
* Return         : 0-Slave 1-Master
*******************************************************************************/
u8 HC05_Get_Role(void)
{
	u8* p;
	u8 retry = 10;
	u8 temp, t;
	
	while(retry--)
	{
		HC05_KEY = 1; // Enter AT mode
		delay_ms(10);
		u3_printf("AT+ROLE?\r\n"); // Send AT+ROLE? command
		
		for(t = 0; t < 20; t++) // Wait up to 200ms for HC05 module response
		{
			delay_ms(10);
			if(USART3_RX_STA&0X8000) break; // Received data
		}
		
		HC05_KEY = 0; // Exit AT mode
		
		if(USART3_RX_STA&0X8000) // Received response
		{
			temp = USART3_RX_STA&0X7FFF; // Get data length
			USART3_RX_BUF[temp] = 0; // Add string terminator
			p = (u8*)strstr((const char*)USART3_RX_BUF, "+ROLE:");
			if(p)
			{
				p += 6; // Offset to after ':'
				temp = (*p) - '0'; // Get digit
				USART3_RX_STA = 0; // Clear
				return temp;
			}
		}
		USART3_RX_STA = 0; // Clear
	}
	return 0xFF; // Get failed
}

/*******************************************************************************
* Function Name  : HC05_Get_Name
* Description    : Get HC05 device name
* Input          : None
* Return         : None (prints name to serial port)
*******************************************************************************/
void HC05_Get_Name(void)
{
	HC05_CFG_CMD((u8*)"AT+NAME?");
}

/*******************************************************************************
* Function Name  : HC05_Get_Version
* Description    : Get HC05 firmware version
* Input          : None
* Return         : None (prints version to serial port)
*******************************************************************************/
void HC05_Get_Version(void)
{
	HC05_CFG_CMD((u8*)"AT+VERSION?");
}

/*******************************************************************************
* Function Name  : HC05_Get_Address
* Description    : Get HC05 device address
* Input          : None
* Return         : None (prints address to serial port)
*******************************************************************************/
void HC05_Get_Address(void)
{
	HC05_CFG_CMD((u8*)"AT+ADDR?");
}

/*******************************************************************************
* Function Name  : HC05_Set_Pin
* Description    : Set HC05 pairing PIN code
* Input          : pin - 4-digit PIN code string
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Set_Pin(char* pin)
{
	char cmd[32];
	sprintf(cmd, "AT+PSWD=%s", pin);
	return HC05_Set_Cmd((u8*)cmd);
}

/*******************************************************************************
* Function Name  : HC05_Get_Pin
* Description    : Get HC05 pairing PIN code
* Input          : None
* Return         : None (prints PIN to serial port)
*******************************************************************************/
void HC05_Get_Pin(void)
{
	HC05_CFG_CMD((u8*)"AT+PSWD?");
}

/*******************************************************************************
* Function Name  : HC05_Set_Baud
* Description    : Set HC05 UART baudrate
* Input          : baud - baudrate (9600, 38400, 115200, etc.)
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Set_Baud(u32 baud)
{
	char cmd[32];
	sprintf(cmd, "AT+UART=%ld,0,0", baud);
	return HC05_Set_Cmd((u8*)cmd);
}

/*******************************************************************************
* Function Name  : HC05_Get_Baud
* Description    : Get HC05 UART baudrate
* Input          : None
* Return         : None (prints baudrate to serial port)
*******************************************************************************/
void HC05_Get_Baud(void)
{
	HC05_CFG_CMD((u8*)"AT+UART?");
}

/*******************************************************************************
* Function Name  : HC05_Set_Role
* Description    : Set HC05 work mode (Master/Slave)
* Input          : role - 0:Slave, 1:Master
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Set_Role(u8 role)
{
	char cmd[16];
	sprintf(cmd, "AT+ROLE=%d", role);
	return HC05_Set_Cmd((u8*)cmd);
}

/*******************************************************************************
* Function Name  : HC05_Reset
* Description    : Reset HC05 module
* Input          : None
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Reset(void)
{
	return HC05_Set_Cmd((u8*)"AT+RESET");
}

/*******************************************************************************
* Function Name  : HC05_Test_AT
* Description    : Test AT command response
* Input          : None
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Test_AT(void)
{
	return HC05_Set_Cmd((u8*)"AT");
}

/*******************************************************************************
* Function Name  : HC05_Set_Class
* Description    : Set device class (COD)
* Input          : class_code - device class code
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Set_Class(u32 class_code)
{
	char cmd[32];
	sprintf(cmd, "AT+CLASS=%lX", class_code);
	return HC05_Set_Cmd((u8*)cmd);
}

/*******************************************************************************
* Function Name  : HC05_Get_Class
* Description    : Get device class (COD)
* Input          : None
* Return         : None (prints class to serial port)
*******************************************************************************/
void HC05_Get_Class(void)
{
	HC05_CFG_CMD((u8*)"AT+CLASS?");
}

/*******************************************************************************
* Function Name  : HC05_Set_Name
* Description    : Set HC05 device name
* Input          : name - device name string
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Set_Name(char* name)
{
	char cmd[32];
	sprintf(cmd, "AT+NAME=%s", name);
	return HC05_Set_Cmd((u8*)cmd);
}

/*******************************************************************************
* Function Name  : HC05_Set_Cmd
* Description    : Send AT command to HC05
* Input          : atstr - AT command string
* Return         : 0-Success 1-Failed
*******************************************************************************/
u8 HC05_Set_Cmd(u8* atstr)
{
	u8 retry = 5;
	u8 temp, t;
	
	while(retry--)
	{
		HC05_KEY = 1; // Enter AT mode
		delay_ms(10);
		u3_printf("%s\r\n", atstr); // Send AT command
		HC05_KEY = 0; // Exit AT mode
		
		for(t = 0; t < 20; t++) // Wait up to 200ms for HC05 module response
		{
			delay_ms(10);
			if(USART3_RX_STA&0X8000) break; // Received data
		}
		
		if(USART3_RX_STA&0X8000) // Received response
		{
			temp = USART3_RX_STA&0X7FFF; // Get data length
			USART3_RX_BUF[temp] = 0; // Add string terminator
			if(strstr((const char*)USART3_RX_BUF, "OK"))
			{
				USART3_RX_STA = 0;
				return 0;
			}
		}
		USART3_RX_STA = 0; // Clear
	}
	return 1; // Set failed
}

/*******************************************************************************
* Function Name  : HC05_CFG_CMD
* Description    : Configure HC05 parameters
* Input          : atstr - AT command string
* Return         : None
*******************************************************************************/
void HC05_CFG_CMD(u8* atstr)
{
	u8 temp, t;
	
	HC05_KEY = 1; // Enter configuration mode
	delay_ms(10);
	u3_printf("%s\r\n", atstr);
	
	for(t = 0; t < 50; t++) // Wait up to 500ms for HC05 module response
	{
		delay_ms(10);
		if(USART3_RX_STA&0X8000) break; // Received data
	}
	
	HC05_KEY = 0; // Exit configuration mode
	
	if(USART3_RX_STA&0X8000) // Received data
	{
		temp = USART3_RX_STA&0X7FFF; // Get data length
		USART3_RX_BUF[temp] = 0; // Add string terminator
		printf("%s\r\n", USART3_RX_BUF); // Send to serial port
	}
	USART3_RX_STA = 0;
}

/*******************************************************************************
* Function Name  : HC05_Send_Data
* Description    : Send data through HC05
* Input          : data - data to send
*                  len - data length
* Return         : None
*******************************************************************************/
void HC05_Send_Data(u8* data, u16 len)
{
    u16 i;
    
    for(i = 0; i < len; i++)
    {
        while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
        USART_SendData(USART3, data[i]);
    }
}

/*******************************************************************************
* Function Name  : HC05_Process_Data
* Description    : Process data received from HC05 (simplified version)
* Input          : None
* Return         : None
*******************************************************************************/
void HC05_Process_Data(void)
{
    u16 len;
    
    if(USART3_RX_STA & 0x8000)  // Received data
    {
        len = USART3_RX_STA & 0x3FFF;  // Get data length
        USART3_RX_BUF[len] = 0;       // Add string terminator
        
        // Echo received data to serial port for debugging
        printf("HC05 received: %s\r\n", USART3_RX_BUF);
        
        // Clear receive flag
        USART3_RX_STA = 0;
    }
} 


