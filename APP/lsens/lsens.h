#ifndef __LSENS_H
#define __LSENS_H

#include "system.h"  

// Light sensor ADC configuration
#define LSENS_ADC_CH    ADC_Channel_6   // Corresponds to PF8
#define LSENS_ADC       ADC3
#define LSENS_GPIO      GPIOF
#define LSENS_PIN       GPIO_Pin_8

// Light sensor function declarations
void Lsens_Init(void);          // Initialize light sensor
u8 Lsens_Get_Val(void);         // Get light sensor value (0-100)
u16 Get_ADC3(u8 ch);            // Get ADC conversion value

#endif
