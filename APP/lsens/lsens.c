#include "lsens.h"
#include "SysTick.h"

// Initialize light sensor
void Lsens_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    
    // Enable GPIO and ADC clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF | RCC_APB2Periph_ADC3, ENABLE);
    
    // Configure PF8 as analog input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOF, &GPIO_InitStructure);
    
    ADC_DeInit(ADC3);
    
    // Configure ADC3
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC3, &ADC_InitStructure);
    
    // Enable ADC3
    ADC_Cmd(ADC3, ENABLE);
    
    // ADC3 calibration
    ADC_ResetCalibration(ADC3);
    while(ADC_GetResetCalibrationStatus(ADC3));
    ADC_StartCalibration(ADC3);
    while(ADC_GetCalibrationStatus(ADC3));
}

// Get ADC value
u16 Lsens_Get_Adc(void)
{
    // Set specified ADC regular group channel, one sequence, sampling time
    ADC_RegularChannelConfig(ADC3, ADC_Channel_6, 1, ADC_SampleTime_239Cycles5);
    
    ADC_SoftwareStartConvCmd(ADC3, ENABLE);
    while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC));
    
    return ADC_GetConversionValue(ADC3);
}

// Get light intensity (0-100%)
u8 Lsens_Get_Val(void)
{
    u16 temp_val = 0;
    u8 i;
    
    for(i = 0; i < 10; i++)
    {
        temp_val += Lsens_Get_Adc();
    }
    temp_val /= 10;
    
    // Convert to percentage (光敏电阻：光照越强，电阻越小，ADC值越小)
    // 反转逻辑：ADC值越小，光照强度百分比越高
    return (u8)(100 - ((temp_val * 100) / 4095));
} 
