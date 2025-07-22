#include "greenhouse_display.h"
#include "greenhouse_control.h"
#include "../led/led.h"
#include "../fan_pwm/fan_pwm.h"  // 添加PWM头文件以获取Fan_Get_Speed_Percent函数
#include <stdio.h>
#include <stdlib.h>  // 添加stdlib.h以获取abs函数
#include "stm32f10x.h"
#include "../tftlcd/tftlcd.h"
#include "greenhouse_control.h"
#include "../rtc/rtc.h"  // 包含RTC头文件，使用其中定义的RTC_Time_t
#include <string.h>     // 包含string.h以使用strcat

// 显示屏初始化
void Display_Init(void)
{
    TFTLCD_Init();
    LCD_Clear(WHITE);
}

// 清屏
void Display_Clear(void)
{
    LCD_Clear(WHITE);
}

// 显示标题
void Display_Title(void)
{
    FRONT_COLOR = TITLE_COLOR;
    BACK_COLOR = WHITE;
    LCD_ShowString(60, 10, 200, 24, 24, (u8*)"Smart Greenhouse");
    
    // 显示初始界面框架
    FRONT_COLOR = NORMAL_COLOR;
    LCD_ShowString(10, 70, 100, 16, 16, (u8*)"Mode:");
    LCD_ShowString(10, 100, 100, 16, 16, (u8*)"Temp:");
    LCD_ShowString(10, 130, 100, 16, 16, (u8*)"Humi:");
    LCD_ShowString(10, 160, 100, 16, 16, (u8*)"Light:");
    LCD_ShowString(10, 190, 100, 16, 16, (u8*)"Fan:");
    LCD_ShowString(10, 220, 100, 16, 16, (u8*)"Pump:");
    LCD_ShowString(10, 250, 100, 16, 16, (u8*)"Light:");
    
    // 显示初始值
    FRONT_COLOR = VALUE_COLOR;
    LCD_ShowString(80, 70, 100, 16, 16, (u8*)"---");
    LCD_ShowString(80, 100, 100, 16, 16, (u8*)"--- C");
    LCD_ShowString(80, 130, 100, 16, 16, (u8*)"---%");
    LCD_ShowString(80, 160, 100, 16, 16, (u8*)"---%");
    LCD_ShowString(80, 190, 100, 16, 16, (u8*)"---");
    LCD_ShowString(80, 220, 100, 16, 16, (u8*)"---");
    LCD_ShowString(80, 250, 100, 16, 16, (u8*)"---");
}

// 显示系统状态 - 高性能版本
void Display_System_Status(GreenhouseStatus_t* status)
{
    char buf[50];
    u8 current_fan_speed;  // 变量声明移到函数开始，符合C89标准
    static u8 last_temp = 255, last_humi = 255, last_light = 255;  // 缓存上次数值
    static u8 last_fan_status = 255, last_pump_status = 255, last_light_status = 255;
    static u8 last_work_mode = 255, last_fan_speed = 255;
    static u32 last_alarm_flags = 0xFFFFFFFF;
    static u8 first_display = 1;  // 首次显示标志
    static u8 fan_update_counter = 0;  // 风扇强制更新计数器
    char alarm_msg[50];
    
    BACK_COLOR = WHITE;
    
    // 调试信息：显示接收到的数据
    if(first_display) {
        printf("Display: First display status - T:%d, H:%d, L:%d, Mode:%d\r\n", 
               status->temperature, status->humidity, status->light, status->work_mode);
    }
    
    // 智能刷新：只更新变化的内容，减少不必要的重绘
    
    // 时间总是更新（每500ms更新一次，快速响应）
    LCD_Fill(160, 50, 280, 66, WHITE);
    FRONT_COLOR = NORMAL_COLOR;
    RTC_Format_Time(&current_time, buf);
    LCD_ShowString(160, 50, 120, 16, 16, (u8*)buf);
    
    LCD_Fill(10, 30, 200, 46, WHITE);
    RTC_Format_Date(&current_time, buf);
    LCD_ShowString(10, 30, 200, 16, 16, (u8*)buf);
    
    // 工作模式 - 只在变化时更新或首次显示
    if(last_work_mode != status->work_mode || first_display) {
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 70, 100, 16, 16, (u8*)"Mode:");
        LCD_Fill(80, 70, 180, 86, WHITE);
        FRONT_COLOR = VALUE_COLOR;
        sprintf(buf, "%s", status->work_mode == MODE_AUTO ? "AUTO" : "MANUAL");
        LCD_ShowString(80, 70, 100, 16, 16, (u8*)buf);
        last_work_mode = status->work_mode;
    }
    
    // 温度 - 只在变化时更新或首次显示
    if(last_temp != status->temperature || first_display) {
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 100, 100, 16, 16, (u8*)"Temp:");
        LCD_Fill(80, 100, 180, 116, WHITE);
        FRONT_COLOR = VALUE_COLOR;
        sprintf(buf, "%3d C", status->temperature);
        LCD_ShowString(80, 100, 100, 16, 16, (u8*)buf);
        last_temp = status->temperature;
        printf("Temperature display updated: %d C\r\n", status->temperature);
    }
    
    // 湿度 - 只在变化时更新或首次显示
    if(last_humi != status->humidity || first_display) {
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 130, 100, 16, 16, (u8*)"Humi:");
        LCD_Fill(80, 130, 180, 146, WHITE);
        FRONT_COLOR = VALUE_COLOR;
        sprintf(buf, "%3d%%", status->humidity);
        LCD_ShowString(80, 130, 100, 16, 16, (u8*)buf);
        last_humi = status->humidity;
    }
    
    // 光照 - 只在变化时更新或首次显示
    if(last_light != status->light || first_display) {
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 160, 100, 16, 16, (u8*)"Light:");
        LCD_Fill(80, 160, 180, 176, WHITE);
        FRONT_COLOR = VALUE_COLOR;
        sprintf(buf, "%3d%%", status->light);
        LCD_ShowString(80, 160, 100, 16, 16, (u8*)buf);
        last_light = status->light;
    }
    
    // 风扇状态和转速变化检测 - 直接从PWM硬件读取
    current_fan_speed = Fan_Get_Speed_Percent();  // 直接从PWM寄存器读取实际转速
    if(last_fan_status != status->fan_status) {
        printf("Fan status changed: %d -> %d\r\n", last_fan_status, status->fan_status);
    }
    if(last_fan_speed != current_fan_speed) {
        printf("Fan speed changed: %d%% -> %d%%\r\n", last_fan_speed, current_fan_speed);
    }
    
    // 风扇强制更新计数器 - 每5次调用强制更新一次
    fan_update_counter++;
    
    // 风扇状态显示 - 强制更新以确保转速及时显示
    if(last_fan_status != status->fan_status || last_fan_speed != current_fan_speed || first_display || (fan_update_counter >= 5)) {
        
        if(fan_update_counter >= 5) {
            fan_update_counter = 0;  // 重置计数器
            printf("Fan display: Force update (counter reset)\r\n");
        }
        
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 190, 100, 16, 16, (u8*)"Fan:");
        LCD_Fill(80, 190, 180, 206, WHITE);
        
        // 修正逻辑：严格根据fan_status判断ON/OFF
        if(status->fan_status == DEVICE_OFF) {
            FRONT_COLOR = NORMAL_COLOR;
            sprintf(buf, "OFF");
        } else {
            FRONT_COLOR = OK_COLOR;
            sprintf(buf, "ON %3d%%", current_fan_speed);
        }
        
        LCD_ShowString(80, 190, 100, 16, 16, (u8*)buf);
        
        printf("Fan display updated: %s (Speed: %d%%)\r\n", buf, current_fan_speed);
        
        last_fan_status = status->fan_status;
        last_fan_speed = current_fan_speed;
    }
    
    // 水泵状态 - 只在变化时更新或首次显示
    if(last_pump_status != status->pump_status || first_display) {
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 220, 100, 16, 16, (u8*)"Pump:");
        LCD_Fill(80, 220, 180, 236, WHITE);
        FRONT_COLOR = status->pump_status ? OK_COLOR : NORMAL_COLOR;
        sprintf(buf, "%s", status->pump_status ? "ON" : "OFF");
        LCD_ShowString(80, 220, 100, 16, 16, (u8*)buf);
        last_pump_status = status->pump_status;
    }
    
    // 补光灯状态 - 只在变化时更新或首次显示
    if(last_light_status != status->light_status || first_display) {
        FRONT_COLOR = NORMAL_COLOR;
        LCD_ShowString(10, 250, 100, 16, 16, (u8*)"Light:");
        LCD_Fill(80, 250, 180, 266, WHITE);
        FRONT_COLOR = status->light_status ? OK_COLOR : NORMAL_COLOR;
        sprintf(buf, "%s", status->light_status ? "ON" : "OFF");
        LCD_ShowString(80, 250, 100, 16, 16, (u8*)buf);
        last_light_status = status->light_status;
    }
    
    // 报警状态 - 只在变化时更新
    if (last_alarm_flags != status->alarm_flags)
    {
        if (status->alarm_flags != ALARM_NONE) {
            LCD_Fill(0, 280, 240, 320, ALARM_COLOR); // 屏幕底部显示红色背景
            
            strcpy(alarm_msg, "ALARM: ");
            if (status->alarm_flags & ALARM_HIGH_TEMP) strcat(alarm_msg, "HighTemp ");
            if (status->alarm_flags & ALARM_LOW_TEMP) strcat(alarm_msg, "LowTemp ");
            if (status->alarm_flags & ALARM_HIGH_HUMI) strcat(alarm_msg, "HighHumi ");
            if (status->alarm_flags & ALARM_LOW_HUMI) strcat(alarm_msg, "LowHumi ");
            if (status->alarm_flags & ALARM_LOW_LIGHT) strcat(alarm_msg, "LowLight ");
            if (status->alarm_flags & ALARM_SENSOR_ERROR) strcat(alarm_msg, "SensorErr ");
            
            Display_Message(10, 295, alarm_msg, WHITE);
        } else {
            // 清除报警区域
            LCD_Fill(0, 280, 240, 320, WHITE);
        }
        last_alarm_flags = status->alarm_flags;
    }
    
    // 重置首次显示标志
    if(first_display) {
        first_display = 0;
    }
}

// 显示消息
void Display_Message(u16 x, u16 y, char* msg, u16 color)
{
    FRONT_COLOR = color;
    BACK_COLOR = WHITE;
    LCD_ShowString(x, y, 200, 16, 16, (u8*)msg);
}
 
