#ifndef __GREENHOUSE_DISPLAY_H__
#define __GREENHOUSE_DISPLAY_H__

#include "stm32f10x.h"
#include "../tftlcd/tftlcd.h"
#include "greenhouse_control.h"
#include "../rtc/rtc.h"  // 包含RTC头文件，使用其中定义的RTC_Time_t

// 外部变量声明
extern RTC_Time_t current_time;

// 颜色定义
#define WHITE           0xFFFF
#define BLACK           0x0000
#define TITLE_COLOR     0x001F    // 蓝色
#define NORMAL_COLOR    0x0000    // 黑色
#define VALUE_COLOR     0x7800    // 橙色
#define OK_COLOR        0x07E0    // 绿色
#define ALARM_COLOR     0xF800    // 红色

// 布局定义
#define TITLE_Y         10
#define STATUS_START_Y  70
#define LINE_HEIGHT     20
#define VALUE_X         120

// 函数声明
void Display_Init(void);
void Display_Clear(void);
void Display_Title(void);
void Display_System_Status(GreenhouseStatus_t* status);
void Display_Message(u16 x, u16 y, char* msg, u16 color);

#endif /* __GREENHOUSE_DISPLAY_H__ */
