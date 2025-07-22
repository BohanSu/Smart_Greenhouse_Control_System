/**
  ******************************************************************************
  * @file    智能温室控制系统/User/stm32f10x_it.h 
  * @author  Intelligent Greenhouse Control System
  * @version V1.0.0
  * @date    2024-12-19
  * @brief   This file contains the headers of the interrupt handlers.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2024 Intelligent Greenhouse</center></h2>
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F10x_IT_H
#define __STM32F10x_IT_H

#ifdef __cplusplus
 extern "C" {
#endif 

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* External interrupt handlers for greenhouse system */
void EXTI0_IRQHandler(void);    // KEY_UP button interrupt
void EXTI2_IRQHandler(void);    // KEY0 button interrupt  
void EXTI3_IRQHandler(void);    // KEY1 button interrupt
void EXTI4_IRQHandler(void);    // KEY2 button interrupt

/* USART interrupt handlers */
void USART1_IRQHandler(void);   // Serial communication
void USART3_IRQHandler(void);   // HC05 Bluetooth communication

/* ADC interrupt handler */
void ADC1_2_IRQHandler(void);   // Light sensor ADC conversion

/* RTC interrupt handler */
void RTC_IRQHandler(void);      // Real-time clock interrupt

/* Greenhouse control functions called in interrupts */
void Greenhouse_Toggle_Mode(void);
void Greenhouse_Manual_Fan_Toggle(void);
void Greenhouse_Manual_Pump_Toggle(void);
void Greenhouse_Manual_Light_Toggle(void);
void Greenhouse_Process_Bluetooth_Data(u8 ch);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F10x_IT_H */

/******************* (C) COPYRIGHT 2024 Intelligent Greenhouse *****END OF FILE****/
