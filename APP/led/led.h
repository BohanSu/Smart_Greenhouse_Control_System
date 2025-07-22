#ifndef _led_H
#define _led_H

#include "system.h"

// --- 保留的LED定义 ---
#define LED_SYS_PORT    GPIOB   // System status LED
#define LED_SYS_PIN     GPIO_Pin_5
#define LED_SYS_RCC     RCC_APB2Periph_GPIOB

#define LED_LIGHT_PORT  GPIOE   // Fill light LED (DS1)
#define LED_LIGHT_PIN   GPIO_Pin_5
#define LED_LIGHT_RCC   RCC_APB2Periph_GPIOE


// --- 跑马灯模块 (用于警报和水泵) ---
#define MARQUEE_PORT    GPIOC
#define MARQUEE_PINS    (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7)
#define MARQUEE_RCC     RCC_APB2Periph_GPIOC


// --- LED控制宏 ---
#define LED_SYS     PBout(5)    // System status LED
#define LED_LIGHT   PEout(5)    // Fill light LED


// LED status definitions
#define LED_ON      0   // LED on (low level active)
#define LED_OFF     1   // LED off

void LED_Init(void);
void LED_System_Set(u8 state);
void LED_System_Toggle(void);
void LED_Alarm_Set(u8 state); // 启动/停止警报闪烁
void LED_Light_Set(u8 state);
void LED_Pump_Set(u8 state);  // 启动/停止水泵流水灯
void LED_Light_ON(void);
void LED_Light_OFF(void);
void LED_Marquee_Update(void); // 新增：跑马灯效果更新函数

// 风扇速度控制函数
void Fan_Set_Speed(u8 speed);
u8 Fan_Get_Speed(void);

// 兼容旧的API，但标记为弃用
void LED_Fan_Set_Speed(u8 speed) __attribute__((deprecated("use Fan_Set_Speed instead")));


// 硬件PWM控制函数（集成到LED模块中）
void Fan_PWM_Init(u16 per, u16 psc);     // PWM初始化
void Fan_Set_Speed_Percent(u8 percent);  // 设置PWM占空比

#endif
