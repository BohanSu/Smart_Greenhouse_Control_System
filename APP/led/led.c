#include "led.h"
#include <stdio.h>  // 添加printf声明
#include "../greenhouse_control/greenhouse_control.h"  // 添加温室控制头文件，获取GreenhouseStatus_t类型
#include "../fan_pwm/fan_pwm.h"

// --- 跑马灯状态变量 ---
static u8 is_alarm_active = 0;      // 警报闪烁状态
static u8 is_pump_active = 0;       // 水泵流水灯状态
static u8 marquee_state = 0;        // 用于控制动画帧
static u8 marquee_counter = 0;      // 用于控制更新频率

// LED initialization
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(LED_SYS_RCC | LED_LIGHT_RCC | MARQUEE_RCC, ENABLE);
    
    // 配置系统LED (PB5) 和灯光LED (PE5)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_InitStructure.GPIO_Pin = LED_SYS_PIN;
    GPIO_Init(LED_SYS_PORT, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = LED_LIGHT_PIN;
    GPIO_Init(LED_LIGHT_PORT, &GPIO_InitStructure);
    
    // 配置跑马灯引脚 (PC0-PC7)
    GPIO_InitStructure.GPIO_Pin = MARQUEE_PINS;
    GPIO_Init(MARQUEE_PORT, &GPIO_InitStructure);
    
    // 初始化状态: 关闭所有LED
    LED_System_Set(0);
    LED_Light_Set(0);
    GPIO_Write(MARQUEE_PORT, 0xFFFF); // 关闭所有跑马灯 (高电平)
}

// Control system LED
void LED_System_Set(u8 state)
{
    LED_SYS = state ? LED_ON : LED_OFF;
}

// Toggle system LED
void LED_System_Toggle(void)
{
    LED_SYS = !LED_SYS;
}

// 启动/停止警报闪烁
void LED_Alarm_Set(u8 state)
{
    is_alarm_active = state;
    if (!state) {
        // 如果停止，则关闭所有跑马灯
        GPIO_Write(MARQUEE_PORT, 0xFFFF);
    }
}

// Control light LED
void LED_Light_Set(u8 state)
{
    LED_LIGHT = state ? LED_ON : LED_OFF;
}

// Turn on light LED
void LED_Light_ON(void)
{
    LED_LIGHT = LED_ON;
}

// Turn off light LED
void LED_Light_OFF(void)
{
    LED_LIGHT = LED_OFF;
}

// 启动/停止水泵流水灯
void LED_Pump_Set(u8 state)
{
    printf("LED_Pump_Set called with state=%d\r\n", state);
    is_pump_active = state;
    if (!state) {
        // 如果停止，则关闭所有跑马灯
        GPIO_Write(MARQUEE_PORT, 0xFFFF);
        printf("LED: Pump marquee stopped, all LEDs OFF\r\n");
    } else {
        printf("LED: Pump marquee started\r\n");
    }
}

// 跑马灯效果更新函数，应在主循环中以一定频率调用
void LED_Marquee_Update(void)
{
    // 控制更新频率 - 每40次调用更新一次 (约200ms间隔)
    marquee_counter++;
    if(marquee_counter < 40) {
        return;  // 还没到更新时间
    }
    marquee_counter = 0;  // 重置计数器
    
    if (is_alarm_active) {
        // 警报：全体闪烁
        marquee_state = (marquee_state + 1) % 2;
        if (marquee_state) {
            GPIO_Write(MARQUEE_PORT, 0x0000); // 全亮
            printf("LED: Alarm flash ON\r\n");
        } else {
            GPIO_Write(MARQUEE_PORT, 0xFFFF); // 全灭
            printf("LED: Alarm flash OFF\r\n");
        }
    } else if (is_pump_active) {
        // 水泵：流水灯
        u16 led_pattern = ~(1 << marquee_state); // 低电平点亮
        GPIO_Write(MARQUEE_PORT, led_pattern);
        printf("LED: Pump marquee step %d, pattern=0x%04X\r\n", marquee_state, led_pattern);
        marquee_state = (marquee_state + 1) % 8;
    } else {
        // 无活动，确保所有灯关闭
        GPIO_Write(MARQUEE_PORT, 0xFFFF);
        marquee_state = 0;
    }
}


// 风扇速度控制相关变量（仅用于PWM控制，无LED指示）
static u8 fan_speed = 0;           // 当前风扇速度 0-100%

// 风扇PWM调速控制 - 纯硬件PWM控制，无LED指示
void Fan_Set_Speed(u8 speed)
{
    u8 old_speed = fan_speed;  // 记录旧转速
    
    if(speed > 100) speed = 100;    // 限制最大速度
    fan_speed = speed;
    
    printf("Fan_Set_Speed called: setting %d%% (Hardware PWM)\r\n", speed);  
    
    // 使用硬件PWM控制真实风扇速度（PA6引脚）
    Fan_Set_Speed_Percent(speed);
    
    // 打印转速变化，立即反馈
    if(old_speed != speed) {
        printf("Fan speed updated: %d%% -> %d%% (PWM applied to PA6)\r\n", old_speed, speed);
    }
    
    // 验证设置结果
    printf("Fan_Get_Speed now returns: %d%%\r\n", Fan_Get_Speed());
}

// 获取当前风扇速度
u8 Fan_Get_Speed(void)
{
    return fan_speed;
}
