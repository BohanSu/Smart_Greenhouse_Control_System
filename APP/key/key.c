#include "key.h"
#include "SysTick.h"

// Key initialization function
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // Enable GPIO clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOE, ENABLE);
    
    // Configure KEY_UP (PA0) as pull-down input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // Configure KEY0-KEY2 (PE2-PE4) as pull-up input
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
}

// Key scan function, returns key value
u8 KEY_Scan(u8 mode)
{
    static u8 key_up = 1; // Key release flag
    
    if(mode) key_up = 1; // Support continuous press
    
    if(key_up && (KEY_UP == 1 || KEY0 == 0 || KEY1 == 0 || KEY2 == 0))
    {
        delay_ms(10); // Debounce
        key_up = 0;
        
        if(KEY_UP == 1) return KEY_UP_PRESS;
        else if(KEY0 == 0) return KEY0_PRESS;
        else if(KEY1 == 0) return KEY1_PRESS;
        else if(KEY2 == 0) return KEY2_PRESS;
    }
    else if(KEY_UP == 0 && KEY0 == 1 && KEY1 == 1 && KEY2 == 1)
    {
        key_up = 1; // All keys released
    }
    
    return KEY_NONE; // No key pressed
} 
