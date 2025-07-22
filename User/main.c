#include <stdio.h>
#include "system.h"
#include "SysTick.h"
#include "usart3.h"
#include "tftlcd.h"
#include "dht11.h"
#include "../APP/greenhouse_control/greenhouse_control.h"
#include "../APP/greenhouse_control/greenhouse_display.h"
#include "../APP/data_logger/data_logger.h"
#include "../APP/led/led.h"
#include "../APP/key/key.h"
#include "../APP/ws2812/ws2812.h"  // 添加RGB彩灯支持
#include "stm32f10x_gpio.h"
#include "usart.h"

// 添加RTC函数声明
void RTC_Process_Interrupt(void);

/**
 * @brief  智能温室控制系统主程序
 * @note   基于STM32F103ZET6的智能温室控制系统
 *         包含温湿度监测、光照检测、自动控制风扇/水泵/补光灯等功能
 * @date   2025年7月
 */

// 系统时间计数器
volatile u32 system_time_ms = 0;

// 定时器1秒标志
u8 timer_1s_flag = 0;

/**
 * @brief  系统时钟初始化
 */
void System_Clock_Init(void)
{
	SystemInit();  // 系统时钟初始化，配置为72MHz
}

/**
 * @brief  外设初始化
 */
void Peripheral_Init(void)
{
	SysTick_Init(72);      // 系统滴答定时器初始化，1ms中断
	USART1_Init(115200);   // 串口1初始化，用于调试输出
	LED_Init();            // 初始化LED
	
	// 初始化风扇PWM控制 - 2KHz频率，平滑电机控制
	Fan_PWM_Init(500, 72-1);  // 周期500，预分频72-1，产生2KHz PWM频率
	printf("Fan PWM: Initialized at 2KHz (PA6 pin)\r\n");
	
	// 初始化RGB彩灯模块
	RGB_Greenhouse_Init();  // 初始化RGB彩灯系统
	printf("RGB: WS2812 LED matrix initialized (PE5)\r\n");
	
	Display_Init();        // TFT LCD显示屏初始化
	Greenhouse_Init();     // 智能温室控制系统初始化
	
	// 显示系统标题
	Display_Title();
	
	// 启动显示欢迎画面 - 显示绿色爱心2秒
	RGB_Show_Heart(RGB_COLOR_GREEN);
	delay_ms(2000);
	RGB_LED_Clear();
}

/**
 * @brief  主函数
 */
int main(void)
{	
	u8 key;
	u16 task_counter = 0;
	u16 led_counter = 0;
	u16 rtc_counter = 0;
	u16 rgb_counter = 0;           // RGB更新计数器
	u16 rgb_mode_counter = 0;      // RGB模式切换计数器
	
	// 系统初始化
	System_Clock_Init();
	Peripheral_Init();
	
	printf("\r\n");
	printf("====================================\r\n");
	printf("    Smart Greenhouse System v1.0\r\n");
	printf("    Based on STM32F103ZET6\r\n");
	printf("====================================\r\n");
	printf("System Features:\r\n");
	printf("- DHT11 Temperature & Humidity\r\n");
	printf("- Light sensor monitoring\r\n");
	printf("- Auto/Manual mode switch\r\n");
	printf("- Smart Fan/Pump/Light control\r\n");
	printf("- UART & Bluetooth support\r\n");
	printf("- Multi-level alarm system\r\n");
	printf("- 5X5 RGB LED Matrix Display\r\n");
	printf("====================================\r\n");
	printf("GPIO Assignment:\r\n");
	printf("DHT11: PG11\r\n");
	printf("Light sensor: PF8 (ADC3_IN6)\r\n");
	printf("System LED: PB5\r\n");
	printf("Light LED (DS1): PE5\r\n");
	printf("RGB Matrix: PE6 (WS2812)\r\n");
	printf("Light LED: PC6\r\n");
	printf("Fan PWM: PA6 (TIM3_CH1) - Real motor control\r\n");
	printf("Pump LED: PC8\r\n");
	printf("Buzzer: PB8\r\n");
	printf("Keys: PE2,PE3,PE4,PA0\r\n");
	printf("====================================\r\n");
	printf("Key Functions:\r\n");
	printf("KEY_UP: Switch Auto/Manual mode\r\n");
	printf("KEY0: Manual Fan control\r\n");
	printf("KEY1: Manual Pump control\r\n");
	printf("KEY2: Manual Light control\r\n");
	printf("====================================\r\n");
	printf("System starting...\r\n\r\n");
	
	// 硬件诊断测试
	printf("=== Hardware Diagnostic Test ===\r\n");
	
	// 测试DHT11初始化 - 参考基础案例的简单方式
	printf("DHT11: Testing sensor connection...\r\n");
	if(DHT11_Init() == 0) {
		printf("DHT11: Sensor OK\r\n");
	} else {
		printf("DHT11: Sensor Error\r\n");
	}
	
	// 测试RTC功能
	printf("RTC: Testing time update mechanism...\r\n");
	RTC_Process_Interrupt(); // 手动调用一次测试
	
	// 测试GPIO状态
	printf("GPIO: Testing pin states...\r\n");
	printf("PG11 (DHT11): %d\r\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_11));
	printf("PF8 (Light): %d\r\n", GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_8));
	
	// 测试RGB彩灯
	printf("RGB: Testing LED matrix...\r\n");
	RGB_Set_All_Color(RGB_COLOR_RED);
	delay_ms(300);
	RGB_Set_All_Color(RGB_COLOR_GREEN);
	delay_ms(300);
	RGB_Set_All_Color(RGB_COLOR_BLUE);
	delay_ms(300);
	RGB_LED_Clear();
	printf("RGB: LED matrix test complete\r\n");
	
	printf("=== Diagnostic Complete ===\r\n\r\n");
	
	delay_ms(2000);  // 等待系统稳定
	
	printf("System started, running...\r\n\r\n");
	
	// 主循环
	
	while(1)
	{
		task_counter++;
		led_counter++;
		rtc_counter++;
		rgb_counter++;
		rgb_mode_counter++;
		
		// 按键处理
		key = KEY_Scan(0);
		if(key) {
			Greenhouse_Process_Key(key);
		}
		
		// RTC时间更新 - 每200次循环调用一次 (约1秒)
		if(rtc_counter >= 200) {
			rtc_counter = 0;
			RTC_Process_Interrupt();  // 更新时间，模拟1秒时钟
		}
		
		// RGB彩灯处理 - 每2次循环更新一次 (约10ms)
		if(rgb_counter >= 2) {
			rgb_counter = 0;

			// 每秒更新一次常规显示
			if(rgb_mode_counter >= 200) { // 200 * 5ms = 1s
				rgb_mode_counter = 0;
				
				if(greenhouse_status.work_mode == MODE_AUTO) {
					// 自动模式下，显示机器人头像
					RGB_Show_Robot(RGB_COLOR_GREEN);
				} else {
					// 手动模式下，显示状态人脸，并实时反映风扇速度
					RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, 
										   greenhouse_status.pump_status, 
										   greenhouse_status.light_status,
										   Fan_Get_Speed());
				}
			}
		}
		
		// 任务调度 - 每50次循环执行一次 (约500ms)
		if(task_counter >= 50)
		{
			task_counter = 0;
			printf("Main task executing: counter=%d\r\n", task_counter);
			
			Greenhouse_Task();          // 温室控制主任务
			Greenhouse_Update_Display(); // 更新显示
		}
		
		// 系统状态LED闪烁 - 每30次循环切换一次
		if (led_counter >= 30) { 
			led_counter = 0;
			LED_System_Toggle();
		}
		
		// 实时任务
		BEEP_Task();                // 处理蜂鸣器任务
		LED_Marquee_Update();       // 更新跑马灯状态
		
		// 处理蓝牙命令 - 实时处理
		Greenhouse_Handle_Bluetooth();
		
		// 循环延时5ms - 提高响应速度
		delay_ms(5);
	}
}
