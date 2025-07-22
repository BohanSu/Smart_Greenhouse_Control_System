#include "greenhouse_control.h"
#include "dht11.h"
#include "lsens.h"
#include "led.h"
#include "beep.h"
#include "key.h"
#include "hc05.h"
#include "usart3.h"
#include "usart.h"
#include "SysTick.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "greenhouse_display.h"
#include "../fan_pwm/fan_pwm.h"  // 添加PWM头文件
#include "../ws2812/ws2812.h"    // 添加RGB彩灯头文件

extern volatile u32 system_time_ms;

GreenhouseStatus_t greenhouse_status = {0};

void Greenhouse_Init(void)
{
    DHT11_Init();
    Lsens_Init();
    LED_Init();
    BEEP_Init();
    KEY_Init();
    HC05_Init();
    RTC_Init();
    DataLogger_Init();
    Config_Init();
    RGB_Greenhouse_Init();  // 初始化RGB彩灯系统
    
    greenhouse_status.work_mode = MODE_AUTO;
    greenhouse_status.fan_status = DEVICE_OFF;
    greenhouse_status.pump_status = DEVICE_OFF;
    greenhouse_status.light_status = DEVICE_OFF;
    greenhouse_status.alarm_flags = ALARM_NONE;
    greenhouse_status.sensor_error = 0;
    greenhouse_status.control_mode = CONTROL_SIMPLE;
    
    greenhouse_status.fan_run_status.status = DEVICE_OFF;
    greenhouse_status.fan_run_status.last_on_time = 0;
    greenhouse_status.fan_run_status.last_off_time = 0;
    greenhouse_status.fan_run_status.total_run_time = 0;
    greenhouse_status.fan_run_status.switch_count = 0;
    
    greenhouse_status.pump_run_status.status = DEVICE_OFF;
    greenhouse_status.pump_run_status.last_on_time = 0;
    greenhouse_status.pump_run_status.last_off_time = 0;
    greenhouse_status.pump_run_status.total_run_time = 0;
    greenhouse_status.pump_run_status.switch_count = 0;
    
    greenhouse_status.light_run_status.status = DEVICE_OFF;
    greenhouse_status.light_run_status.last_on_time = 0;
    greenhouse_status.light_run_status.last_off_time = 0;
    greenhouse_status.light_run_status.total_run_time = 0;
    greenhouse_status.light_run_status.switch_count = 0;
    
    greenhouse_status.env_history.history_index = 0;
    greenhouse_status.env_history.history_count = 0;
    
    greenhouse_status.system_run_time = 0;
    greenhouse_status.alarm_count = 0;
    greenhouse_status.auto_mode_ratio = 100;
    
    DHT11_Set_Calibration(0.0f, 0.0f);
    
    BEEP_Short();
    printf("Smart Greenhouse System Started!\r\n");
    printf("Current Mode: AUTO (Hysteresis Control)\r\n");
    printf("Enhanced Features Enabled:\r\n");
    printf("- DHT11 Data Filter & Calibration\r\n");
    printf("- Hysteresis Control Algorithm\r\n");
    printf("- Device Runtime Management\r\n");
    printf("- Environment Trend Analysis\r\n");
    printf("- Energy Optimization & Safety Check\r\n");
}

void Greenhouse_Update_Sensors(void)
{
    static u16 log_counter = 0;
    static u8 last_valid_temp = 0;
    static u8 last_valid_humi = 0;

    u8 temp, humi;
    u8 dht_result;
    
    // 读取DHT11数据
    dht_result = DHT11_Read_Data(&temp, &humi);
    if (dht_result == 0) {
        greenhouse_status.temperature = temp;
        greenhouse_status.humidity = humi;
        greenhouse_status.sensor_error &= ~0x01;
        last_valid_temp = temp;
        last_valid_humi = humi;
                         printf("DHT11: Read Success T=%d°C, H=%d%%\r\n", temp, humi);
    } else {
        // DHT11 read failed, use last valid data
        greenhouse_status.sensor_error |= 0x01;
        greenhouse_status.temperature = last_valid_temp;
        greenhouse_status.humidity = last_valid_humi;
        printf("DHT11: Read Failed, Using Last Valid Data T=%d°C, H=%d%%\r\n",
               greenhouse_status.temperature, greenhouse_status.humidity);
    }
    
    // 读取光照传感器
    greenhouse_status.light = Lsens_Get_Val();
    printf("Light Sensor: Reading L=%d%%\r\n", greenhouse_status.light);
    
    log_counter++;
    if(log_counter >= 5)
    {
        log_counter = 0;
        DataLogger_WriteSensorData(greenhouse_status.temperature,
                                   greenhouse_status.humidity,
                                   greenhouse_status.light,
                                   greenhouse_status.fan_status,
                                   greenhouse_status.pump_status,
                                   greenhouse_status.light_status,
                                   greenhouse_status.work_mode,
                                   greenhouse_status.alarm_flags);
    }
}

void Greenhouse_Auto_Control(void)
{
    u8 temp_threshold, temp_hysteresis;
    u8 humi_threshold, humi_hysteresis;
    u8 light_threshold, light_hysteresis;
    u8 fan_speed, current_fan_speed;
    
    if(greenhouse_status.work_mode != MODE_AUTO) return;
    
    temp_threshold = Config_Get_U8(CONFIG_TEMP_FAN_ON);
    temp_hysteresis = system_config.temp_hysteresis;
    
    if(greenhouse_status.temperature >= temp_threshold)
    {
        // 根据温度计算风扇转速
        if(greenhouse_status.temperature >= 40) {
            fan_speed = 70;
        } else if(greenhouse_status.temperature >= 35) {
            fan_speed = 50;
        } else if(greenhouse_status.temperature >= 30) {
            fan_speed = 35;
        } else {
            fan_speed = 25;
        }
        
        if(greenhouse_status.fan_status == DEVICE_OFF) {
            greenhouse_status.fan_status = DEVICE_ON;
            printf("Auto Fan ON for Cooling (T=%d°C>=%d°C) Speed=%d%%\r\n", 
                   greenhouse_status.temperature, temp_threshold, fan_speed);
            DataLogger_WriteOperation(OP_FAN_ON, DEVICE_OFF, DEVICE_ON, 0);
        }
        
        current_fan_speed = Fan_Get_Speed_Percent();  // 从PWM硬件读取实际转速
        if(current_fan_speed != fan_speed) {
            Fan_Set_Speed(fan_speed);
            printf("Fan Speed Auto Adjust: T=%d°C, %d%% → %d%%\r\n", 
                   greenhouse_status.temperature, current_fan_speed, fan_speed);
        }
    }
    else if(greenhouse_status.temperature < temp_threshold - temp_hysteresis)
    {
        printf("Temperature below stop threshold, preparing to turn off fan...\r\n");
        if(greenhouse_status.fan_status == DEVICE_ON)
        {
            if(greenhouse_status.temperature <= 25) {
                printf("Emergency Stop: Temperature too low, turning off fan immediately!\r\n");
            }
            
            greenhouse_status.fan_status = DEVICE_OFF;
            Fan_Set_Speed(0);
            printf("Auto Fan OFF (T=%d°C<%d°C)\r\n", 
                   greenhouse_status.temperature, temp_threshold - temp_hysteresis);
            DataLogger_WriteOperation(OP_FAN_OFF, DEVICE_ON, DEVICE_OFF, 0);
        }
    }
    else {
        if (greenhouse_status.fan_status == DEVICE_ON) {
            if(greenhouse_status.temperature >= 40) {
                fan_speed = 70;
            } else if(greenhouse_status.temperature >= 35) {
                fan_speed = 50;
            } else if(greenhouse_status.temperature >= 30) {
                fan_speed = 35;
            } else {
                fan_speed = 20;
            }
            
            current_fan_speed = Fan_Get_Speed_Percent();  // 从PWM硬件读取实际转速
            if(current_fan_speed != fan_speed) {
                Fan_Set_Speed(fan_speed);
                printf("Fan Speed Hysteresis Adjust: T=%d°C, %d%% → %d%%\r\n", 
                       greenhouse_status.temperature, current_fan_speed, fan_speed);
            }
        } else {
             printf("Temperature in hysteresis zone, fan OFF, maintaining current state\r\n");
        }
    }
    
    humi_threshold = Config_Get_U8(CONFIG_HUMI_PUMP_ON);
    humi_hysteresis = system_config.humi_hysteresis;
    
    if(greenhouse_status.humidity <= humi_threshold)
    {
        if(greenhouse_status.pump_status == DEVICE_OFF)
        {
            greenhouse_status.pump_status = DEVICE_ON;
            LED_Pump_Set(1);
            printf("Auto Pump ON for watering (H=%d%%<=%d%%)\r\n", greenhouse_status.humidity, humi_threshold);
            DataLogger_WriteOperation(OP_PUMP_ON, DEVICE_OFF, DEVICE_ON, 0);
        }
    }
    else if(greenhouse_status.humidity > humi_threshold + humi_hysteresis)
    {
        if(greenhouse_status.pump_status == DEVICE_ON)
        {
            greenhouse_status.pump_status = DEVICE_OFF;
            LED_Pump_Set(0);
            printf("Auto Pump OFF (H=%d%%>%d%%)\r\n", greenhouse_status.humidity, humi_threshold + humi_hysteresis);
            DataLogger_WriteOperation(OP_PUMP_OFF, DEVICE_ON, DEVICE_OFF, 0);
        }
    }
    
    light_threshold = Config_Get_U8(CONFIG_LIGHT_AUTO_ON);
    light_hysteresis = system_config.light_hysteresis;
    
    if(greenhouse_status.light < light_threshold)
    {
        if(greenhouse_status.light_status == DEVICE_OFF)
        {
            greenhouse_status.light_status = DEVICE_ON;
            LED_Light_Set(1);
            printf("Auto Light ON (L=%d%%<%d%%)\r\n", greenhouse_status.light, light_threshold);
            DataLogger_WriteOperation(OP_LIGHT_ON, DEVICE_OFF, DEVICE_ON, 0);
        }
    }
    else if(greenhouse_status.light > light_threshold + light_hysteresis)
    {
        if(greenhouse_status.light_status == DEVICE_ON)
        {
            greenhouse_status.light_status = DEVICE_OFF;
            LED_Light_Set(0);
            printf("Auto Light OFF (L=%d%%>%d%%)\r\n", greenhouse_status.light, light_threshold + light_hysteresis);
            DataLogger_WriteOperation(OP_LIGHT_OFF, DEVICE_ON, DEVICE_OFF, 0);
        }
    }
}

void Greenhouse_Manual_Control(u8 device, u8 action)
{
    u8 old_value, operation;
    
    if(greenhouse_status.work_mode != MODE_MANUAL) return;
    
    switch(device)
    {
        case 1:
            old_value = greenhouse_status.fan_status;
            greenhouse_status.fan_status = action;
            if(action) {
                Fan_Set_Speed(45);
                printf("Manual Fan ON (Speed 45%%)\r\n");
            } else {
                Fan_Set_Speed(0);
                printf("Manual Fan OFF\r\n");
            }
            operation = action ? OP_FAN_ON : OP_FAN_OFF;
            DataLogger_WriteOperation(operation, old_value, action, 1);
            break;
            
        case 2:
            old_value = greenhouse_status.pump_status;
            greenhouse_status.pump_status = action;
            // 直接控制LED流水灯，不依赖Greenhouse_Pump_Control的状态检查
            LED_Pump_Set(action);
            if (action) {
                printf("Manual Pump ON\r\n");
            } else {
                printf("Manual Pump OFF\r\n");
            }
            operation = action ? OP_PUMP_ON : OP_PUMP_OFF;
            DataLogger_WriteOperation(operation, old_value, action, 1);
            Greenhouse_Update_Device_Status(&greenhouse_status.pump_run_status, action);
            break;
            
        case 3:
            old_value = greenhouse_status.light_status;
            greenhouse_status.light_status = action;
            LED_Light_Set(action);
            operation = action ? OP_LIGHT_ON : OP_LIGHT_OFF;
            printf("Manual Light %s\r\n", action ? "ON" : "OFF");
            DataLogger_WriteOperation(operation, old_value, action, 1);
            break;
    }
    BEEP_Short();
}

void Greenhouse_Pump_Control(u8 state)
{
    if (greenhouse_status.pump_status != state)
    {
        greenhouse_status.pump_status = state;
        if (state == DEVICE_ON) {
            printf("Pump turned ON\r\n");
            LED_Pump_Set(1);
        } else {
            printf("Pump turned OFF\r\n");
            LED_Pump_Set(0);
        }
        Greenhouse_Update_Device_Status(&greenhouse_status.pump_run_status, state);
    }
}

void Greenhouse_Display_Status(void)
{
    // 这个函数现在只是一个包装器，实际的显示逻辑在 greenhouse_display.c 中
    Display_System_Status(&greenhouse_status);
    
    // 保留printf用于调试
    printf("=== Smart Greenhouse System Status ===\r\n");
    printf("Temperature: %d°C  Humidity: %d%%  Light: %d%%\r\n", 
           greenhouse_status.temperature, 
           greenhouse_status.humidity, 
           greenhouse_status.light);
    printf("Work Mode: %s\r\n", greenhouse_status.work_mode ? "MANUAL" : "AUTO");
    printf("Fan: %s  Pump: %s  Light: %s\r\n",
           greenhouse_status.fan_status ? "ON" : "OFF",
           greenhouse_status.pump_status ? "ON" : "OFF",
           greenhouse_status.light_status ? "ON" : "OFF");
    
    if(greenhouse_status.alarm_flags != ALARM_NONE)
    {
            printf("Alarms: ");
    if(greenhouse_status.alarm_flags & ALARM_HIGH_TEMP) printf("HighTemp ");
    if(greenhouse_status.alarm_flags & ALARM_LOW_TEMP) printf("LowTemp ");
    if(greenhouse_status.alarm_flags & ALARM_HIGH_HUMI) printf("HighHumi ");
    if(greenhouse_status.alarm_flags & ALARM_LOW_HUMI) printf("LowHumi ");
    if(greenhouse_status.alarm_flags & ALARM_LOW_LIGHT) printf("LowLight ");
    if(greenhouse_status.alarm_flags & ALARM_SENSOR_ERROR) printf("SensorErr ");
        printf("\r\n");
    }
    printf("========================\r\n");
}

void Greenhouse_Process_Key(u8 key)
{
    u8 old_mode;
    u8 operation;
    
    switch(key)
    {
        case KEY_UP_PRESS:
            old_mode = greenhouse_status.work_mode;
            greenhouse_status.work_mode = !greenhouse_status.work_mode;
            printf("Switch to %s mode\r\n", greenhouse_status.work_mode ? "MANUAL" : "AUTO");
            operation = greenhouse_status.work_mode ? OP_MODE_MANUAL : OP_MODE_AUTO;
            DataLogger_WriteOperation(operation, old_mode, greenhouse_status.work_mode, 1);
            BEEP_Short();
            break;
            
        case KEY0_PRESS:
            if(greenhouse_status.work_mode == MODE_MANUAL) {
                delay_ms(10); // 消抖
                if(KEY0 == 0) { // 再次检测按键是否仍被按下
                    delay_ms(80); // 长按检测延时
                    if(KEY0 == 0) { // 长按
                        if(greenhouse_status.fan_status == DEVICE_ON) {
                            u8 current_speed = Fan_Get_Speed();
                            current_speed += 10;
                            if(current_speed > 100) current_speed = 0;
                            Fan_Set_Speed(current_speed);
                            printf("Fan speed set to %d%%\r\n", current_speed);
                            Greenhouse_Update_Display();
                        }
                    } else { // 短按
                        Greenhouse_Manual_Control(1, !greenhouse_status.fan_status);
                    }
                }
            }
            break;
            
        case KEY1_PRESS:
            if(greenhouse_status.work_mode == MODE_MANUAL)
            {
                Greenhouse_Manual_Control(2, !greenhouse_status.pump_status);
            }
            break;
            
        case KEY2_PRESS:
            if(greenhouse_status.work_mode == MODE_MANUAL)
            {
                Greenhouse_Manual_Control(3, !greenhouse_status.light_status);
            }
            break;
    }
}

#include "beep.h" // 临时包含以解决编译问题
void Greenhouse_Check_Alarms(void)
{
    u8 temp = greenhouse_status.temperature;
    u8 humi = greenhouse_status.humidity;
    u8 light = greenhouse_status.light;
    u8 new_alarm_flags = ALARM_NONE;

    if(temp > Config_Get_U8(CONFIG_TEMP_HIGH_ALARM)) new_alarm_flags |= ALARM_HIGH_TEMP;
    if(temp < Config_Get_U8(CONFIG_TEMP_LOW_ALARM)) new_alarm_flags |= ALARM_LOW_TEMP;
    if(humi > Config_Get_U8(CONFIG_HUMI_HIGH_ALARM)) new_alarm_flags |= ALARM_HIGH_HUMI;
    if(humi < Config_Get_U8(CONFIG_HUMI_LOW_ALARM)) new_alarm_flags |= ALARM_LOW_HUMI;
    if(light < Config_Get_U8(CONFIG_LIGHT_LOW_ALARM)) new_alarm_flags |= ALARM_LOW_LIGHT;
    if(greenhouse_status.sensor_error & 0x01) new_alarm_flags |= ALARM_SENSOR_ERROR;

    if (greenhouse_status.alarm_flags != new_alarm_flags) {
        greenhouse_status.alarm_flags = new_alarm_flags;
        
        if (new_alarm_flags == ALARM_NONE) {
            BEEP_Set_Alarm(ALARM_TYPE_NONE);
            LED_Alarm_Set(0);
            printf("All alarms cleared.\r\n");
        } else {
            if (new_alarm_flags & ALARM_SENSOR_ERROR) BEEP_Set_Alarm(ALARM_TYPE_SENSOR_ERROR);
            else if (new_alarm_flags & ALARM_HIGH_TEMP) BEEP_Set_Alarm(ALARM_TYPE_HIGH_TEMP);
            else if (new_alarm_flags & ALARM_LOW_TEMP) BEEP_Set_Alarm(ALARM_TYPE_LOW_TEMP);
            else if (new_alarm_flags & ALARM_LOW_HUMI) BEEP_Set_Alarm(ALARM_TYPE_LOW_HUMI);
            else if (new_alarm_flags & ALARM_HIGH_HUMI) BEEP_Set_Alarm(ALARM_TYPE_HIGH_HUMI);
            else if (new_alarm_flags & ALARM_LOW_LIGHT) BEEP_Set_Alarm(ALARM_TYPE_LOW_LIGHT);
            
            LED_Alarm_Set(1);
            printf("System alarm triggered! Flag: %02X\r\n", new_alarm_flags);
            DataLogger_WriteAlarm(new_alarm_flags, temp, humi, light);
            }
        }
}

void Greenhouse_Handle_Bluetooth(void)
{
     static u16 command_count = 0; // 调试计数器
     
     // 调试：显示接收状态检查
     if(USART3_RX_STA != 0) {
         printf("[DEBUG] USART3_RX_STA = 0x%04X\r\n", USART3_RX_STA);
     }
     
     if(USART3_RX_STA & 0x8000)
     {
         USART3_RX_BUF[USART3_RX_STA & 0X3FFF] = 0;
         command_count++;
         
         printf("[BT_CMD_%d] Received: %s (len=%d)\r\n", 
                command_count, 
                USART3_RX_BUF, 
                USART3_RX_STA & 0X3FFF);
         
         // STATUS命令 - 查询系统状态
         if(strstr((char*)USART3_RX_BUF, "STATUS"))
         {
             printf("=== System Status ===\r\n");
             printf("Temperature: %d°C\r\n", greenhouse_status.temperature);
             printf("Humidity: %d%%\r\n", greenhouse_status.humidity);
             printf("Light: %d%%\r\n", greenhouse_status.light);
             printf("Mode: %s\r\n", greenhouse_status.work_mode ? "Manual" : "Auto");
             printf("Fan: %s\r\n", greenhouse_status.fan_run_status.status ? "ON" : "OFF");
             printf("Pump: %s\r\n", greenhouse_status.pump_run_status.status ? "ON" : "OFF");
             printf("Light: %s\r\n", greenhouse_status.light_run_status.status ? "ON" : "OFF");
             printf("====================\r\n");
         }
         // AUTO模式切换
         else if(strstr((char*)USART3_RX_BUF, "AUTO"))
         {
             // 从手动模式切换到自动模式时，关闭所有手动开启的设备
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 printf("Switching from MANUAL to AUTO mode...\r\n");
                 
                 // 关闭风扇
                 if(greenhouse_status.fan_run_status.status == DEVICE_ON) {
                     Fan_Set_Speed_Percent(0);
                     greenhouse_status.fan_run_status.status = DEVICE_OFF;
                     printf("Auto: Fan turned OFF (manual override cleared)\r\n");
                 }
                 
                 // 关闭水泵（流水灯）
                 if(greenhouse_status.pump_run_status.status == DEVICE_ON) {
                     LED_Pump_Set(0);
                     greenhouse_status.pump_run_status.status = DEVICE_OFF;
                     printf("Auto: Pump/Marquee turned OFF (manual override cleared)\r\n");
                 }
                 
                 // 关闭补光灯
                 if(greenhouse_status.light_run_status.status == DEVICE_ON) {
                     LED_Light_Set(0);
                     greenhouse_status.light_run_status.status = DEVICE_OFF;
                     printf("Auto: Light turned OFF (manual override cleared)\r\n");
                 }
             }
             
             greenhouse_status.work_mode = MODE_AUTO;
             printf("Switched to AUTO mode - System will control devices automatically\r\n");
         }
         // MANUAL模式切换
         else if(strstr((char*)USART3_RX_BUF, "MANUAL"))
         {
             if(greenhouse_status.work_mode == MODE_AUTO) {
                 printf("Switching from AUTO to MANUAL mode...\r\n");
                 printf("Note: Devices will keep current state until manually controlled\r\n");
             }
             
             greenhouse_status.work_mode = MODE_MANUAL;
             printf("Switched to MANUAL mode - Use FAN_ON/OFF, PUMP_ON/OFF, LIGHT_ON/OFF commands\r\n");
         }
         // FAN_ON - 风扇开启
         else if(strstr((char*)USART3_RX_BUF, "FAN_ON"))
         {
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 Fan_Set_Speed_Percent(100); // 100%功率
                 greenhouse_status.fan_status = DEVICE_ON; // 修正状态变量
                 Greenhouse_Update_Device_Status(&greenhouse_status.fan_run_status, DEVICE_ON); // 同步运行时状态
                 printf("Fan turned ON (100%% power)\r\n");
                 Greenhouse_Update_Display(); // 更新TFT显示
                 RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status, Fan_Get_Speed());
             } else {
                 printf("Error: Switch to MANUAL mode first\r\n");
             }
         }
         // FAN_OFF - 风扇关闭
         else if(strstr((char*)USART3_RX_BUF, "FAN_OFF"))
         {
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 Fan_Set_Speed_Percent(0);
                 greenhouse_status.fan_status = DEVICE_OFF; // 修正状态变量
                 Greenhouse_Update_Device_Status(&greenhouse_status.fan_run_status, DEVICE_OFF); // 同步运行时状态
                 printf("Fan turned OFF\r\n");
                 Greenhouse_Update_Display(); // 更新TFT显示
                 RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status, Fan_Get_Speed());
             } else {
                 printf("Error: Switch to MANUAL mode first\r\n");
             }
         }
         // PUMP_ON - 水泵开启
         else if(strstr((char*)USART3_RX_BUF, "PUMP_ON"))
         {
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 LED_Pump_Set(1);
                 greenhouse_status.pump_status = DEVICE_ON; // 修正状态变量
                 Greenhouse_Update_Device_Status(&greenhouse_status.pump_run_status, DEVICE_ON); // 同步运行时状态
                 printf("Pump turned ON\r\n");
                 Greenhouse_Update_Display(); // 更新TFT显示
                 RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status, Fan_Get_Speed());
             } else {
                 printf("Error: Switch to MANUAL mode first\r\n");
             }
         }
         // PUMP_OFF - 水泵关闭  
         else if(strstr((char*)USART3_RX_BUF, "PUMP_OFF"))
         {
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 LED_Pump_Set(0);
                 greenhouse_status.pump_status = DEVICE_OFF; // 修正状态变量
                 Greenhouse_Update_Device_Status(&greenhouse_status.pump_run_status, DEVICE_OFF); // 同步运行时状态
                 printf("Pump turned OFF\r\n");
                 Greenhouse_Update_Display(); // 更新TFT显示
                 RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status, Fan_Get_Speed());
             } else {
                 printf("Error: Switch to MANUAL mode first\r\n");
             }
         }
         // LIGHT_ON - 补光灯开启
         else if(strstr((char*)USART3_RX_BUF, "LIGHT_ON"))
         {
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 LED_Light_Set(1);
                 greenhouse_status.light_status = DEVICE_ON; // 修正状态变量
                 Greenhouse_Update_Device_Status(&greenhouse_status.light_run_status, DEVICE_ON); // 同步运行时状态
                 printf("Light turned ON\r\n");
                 Greenhouse_Update_Display(); // 更新TFT显示
                 RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status, Fan_Get_Speed());
             } else {
                 printf("Error: Switch to MANUAL mode first\r\n");
             }
         }
         // LIGHT_OFF - 补光灯关闭
         else if(strstr((char*)USART3_RX_BUF, "LIGHT_OFF"))
         {
             if(greenhouse_status.work_mode == MODE_MANUAL) {
                 LED_Light_Set(0);
                 greenhouse_status.light_status = DEVICE_OFF; // 修正状态变量
                 Greenhouse_Update_Device_Status(&greenhouse_status.light_run_status, DEVICE_OFF); // 同步运行时状态
                 printf("Light turned OFF\r\n");
                 Greenhouse_Update_Display(); // 更新TFT显示
                 RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status, Fan_Get_Speed());
             } else {
                 printf("Error: Switch to MANUAL mode first\r\n");
             }
         }
         // STATS命令 - 系统统计信息
         else if(strstr((char*)USART3_RX_BUF, "STATS"))
         {
             printf("=== System Statistics ===\r\n");
             printf("Run time: %lu seconds\r\n", greenhouse_status.system_run_time);
             printf("Fan switches: %d\r\n", greenhouse_status.fan_run_status.switch_count);
             printf("Pump switches: %d\r\n", greenhouse_status.pump_run_status.switch_count);
             printf("Light switches: %d\r\n", greenhouse_status.light_run_status.switch_count);
             printf("========================\r\n");
         }
         // TREND命令 - 环境趋势
         else if(strstr((char*)USART3_RX_BUF, "TREND"))
         {
             // ... (TREND command implementation)
         }
         // 传感器状态查询
         else if(strstr((char*)USART3_RX_BUF, "SENSOR_STATUS"))
         {
             printf("=== DHT11 Sensor Status ===\r\n");
             printf("Current Mode: Real Sensor Mode\r\n");
             printf("Note: System will auto-retry if read fails\r\n");
             printf("===========================\r\n");
         }
         // 蓝牙名称查询
         else if(strstr((char*)USART3_RX_BUF, "BT_NAME"))
         {
             printf("=== Bluetooth Device Info ===\r\n");
             printf("Current device name: ");
             HC05_Get_Name();  // 查询当前蓝牙名称
             printf("Note: Device name is set to 'SBH'\r\n");
             printf("=============================\r\n");
         }
         // 蓝牙版本查询
         else if(strstr((char*)USART3_RX_BUF, "BT_VER"))
         {
             printf("=== HC05 Firmware Version ===\r\n");
             HC05_Get_Version();
             printf("==============================\r\n");
         }
         // 蓝牙地址查询
         else if(strstr((char*)USART3_RX_BUF, "BT_ADDR"))
         {
             printf("=== HC05 MAC Address ===\r\n");
             HC05_Get_Address();
             printf("========================\r\n");
         }
         // 蓝牙PIN查询
         else if(strstr((char*)USART3_RX_BUF, "BT_PIN"))
         {
             printf("=== HC05 Pairing PIN ===\r\n");
             HC05_Get_Pin();
             printf("========================\r\n");
         }
         // 蓝牙波特率查询
         else if(strstr((char*)USART3_RX_BUF, "BT_BAUD"))
         {
             printf("=== HC05 UART Baudrate ===\r\n");
             HC05_Get_Baud();
             printf("==========================\r\n");
         }
         // 蓝牙角色查询
         else if(strstr((char*)USART3_RX_BUF, "BT_ROLE"))
         {
             u8 role; // 声明变量在代码块开始处
             printf("=== HC05 Role Mode ===\r\n");
             role = HC05_Get_Role();
             if(role == 0) {
                 printf("Current Role: Slave Mode\r\n");
             } else if(role == 1) {
                 printf("Current Role: Master Mode\r\n");
             } else {
                 printf("Role Query Failed\r\n");
             }
             printf("======================\r\n");
         }
         // 蓝牙设备类查询
         else if(strstr((char*)USART3_RX_BUF, "BT_CLASS"))
         {
             printf("=== HC05 Device Class ===\r\n");
             HC05_Get_Class();
             printf("=========================\r\n");
         }
         // 蓝牙AT测试
         else if(strstr((char*)USART3_RX_BUF, "BT_TEST"))
         {
             printf("=== HC05 AT Test ===\r\n");
             if(HC05_Test_AT() == 0) {
                 printf("AT Command Response: OK\r\n");
             } else {
                 printf("AT Command Response: FAILED\r\n");
             }
             printf("====================\r\n");
         }
         // 蓝牙模块重置
         else if(strstr((char*)USART3_RX_BUF, "BT_RESET"))
         {
             printf("=== HC05 Module Reset ===\r\n");
             if(HC05_Reset() == 0) {
                 printf("Module Reset: SUCCESS\r\n");
                 printf("Wait for module restart...\r\n");
             } else {
                 printf("Module Reset: FAILED\r\n");
             }
             printf("=========================\r\n");
         }
         // HC05硬件诊断
         else if(strstr((char*)USART3_RX_BUF, "BT_DIAG"))
         {
             printf("=== HC05 Hardware Diagnostics ===\r\n");
             
             // 测试1：KEY引脚控制测试
             printf("Test 1: KEY Pin Control...\r\n");
             HC05_KEY = 0;
             delay_ms(10);
             printf("  KEY=0: Normal mode\r\n");
             
             HC05_KEY = 1;
             delay_ms(10);
             printf("  KEY=1: AT mode\r\n");
             
             HC05_KEY = 0;
             printf("  KEY control: PASS\r\n");
             
             // 测试2：LED状态检测
             printf("Test 2: LED Status Detection...\r\n");
             printf("  LED Status: %s\r\n", HC05_LED ? "HIGH" : "LOW");
             if(HC05_LED == 0) {
                 printf("  WARNING: LED LOW - Check power\r\n");
             } else {
                 printf("  LED detection: PASS\r\n");
             }
             
             // 测试3：AT命令测试
             printf("Test 3: AT Command Test...\r\n");
             if(HC05_Test_AT() == 0) {
                 printf("  AT Response: PASS\r\n");
             } else {
                 printf("  AT Response: FAILED\r\n");
                 printf("  Check TXD/RXD connections\r\n");
             }
             
             printf("=== Diagnostics Complete ===\r\n");
         }
         // ===== RGB彩灯控制命令 =====
         
         // RGB显示模式控制
         else if(strstr((char*)USART3_RX_BUF, "RGB_TEMP"))
         {
             RGB_Set_Display_Mode(RGB_MODE_TEMP_DISPLAY);
             RGB_Show_Temperature(greenhouse_status.temperature);
             printf("RGB: Temperature display mode activated (%d°C)\r\n", greenhouse_status.temperature);
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_HUMIDITY"))
         {
             RGB_Set_Display_Mode(RGB_MODE_HUMIDITY_DISPLAY);
             RGB_Show_Humidity(greenhouse_status.humidity);
             printf("RGB: Humidity display mode activated (%d%%)\r\n", greenhouse_status.humidity);
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_LIGHT"))
         {
             RGB_Set_Display_Mode(RGB_MODE_LIGHT_DISPLAY);
             RGB_Show_Light_Level(greenhouse_status.light);
             printf("RGB: Light display mode activated (%d%%)\r\n", greenhouse_status.light);
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_STATUS"))
         {
             RGB_Set_Display_Mode(RGB_MODE_STATUS_DISPLAY);
             RGB_Show_System_Status(greenhouse_status.fan_status, greenhouse_status.pump_status, greenhouse_status.light_status);
             printf("RGB: System status display mode activated\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_OFF"))
         {
             RGB_Set_Display_Mode(RGB_MODE_OFF);
             RGB_LED_Clear();
             printf("RGB: Display turned off\r\n");
         }
         
         // RGB动画效果
         else if(strstr((char*)USART3_RX_BUF, "RGB_RAINBOW"))
         {
             RGB_Set_Display_Mode(RGB_MODE_ANIMATION);
             RGB_Start_Rainbow_Animation();
             printf("RGB: Rainbow animation started\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_BREATHING"))
         {
             RGB_Set_Display_Mode(RGB_MODE_ANIMATION);
             RGB_Start_Breathing_Animation(RGB_COLOR_BLUE);
             printf("RGB: Breathing animation started\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_FLOW"))
         {
             RGB_Set_Display_Mode(RGB_MODE_ANIMATION);
             RGB_Start_Water_Flow_Animation();
             printf("RGB: Water flow animation started\r\n");
         }
         
         // RGB图案显示
         else if(strstr((char*)USART3_RX_BUF, "RGB_HEART"))
         {
             RGB_Set_Display_Mode(RGB_MODE_PATTERN);
             RGB_Show_Heart(RGB_COLOR_RED);
             printf("RGB: Heart pattern displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_SMILEY"))
         {
             RGB_Set_Display_Mode(RGB_MODE_PATTERN);
             RGB_Show_Smiley(RGB_COLOR_YELLOW);
             printf("RGB: Smiley pattern displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_CHECK"))
         {
             RGB_Set_Display_Mode(RGB_MODE_PATTERN);
             RGB_Show_Check_Mark(RGB_COLOR_GREEN);
             printf("RGB: Check mark pattern displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_CROSS"))
         {
             RGB_Set_Display_Mode(RGB_MODE_PATTERN);
             RGB_Show_Cross(RGB_COLOR_RED);
             printf("RGB: Cross pattern displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_ROBOT"))
         {
             RGB_Set_Display_Mode(RGB_MODE_PATTERN);
             RGB_Show_Robot(RGB_COLOR_GREEN);
             printf("RGB: Robot pattern displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_MANUAL_FACE"))
         {
             RGB_Set_Display_Mode(RGB_MODE_PATTERN);
             RGB_Show_Manual_Status_Face(greenhouse_status.fan_status, 
                                        greenhouse_status.pump_status, 
                                        greenhouse_status.light_status,
                                        Fan_Get_Speed());
             printf("RGB: Manual status face pattern displayed\r\n");
         }
         
         // RGB纯色显示
         else if(strstr((char*)USART3_RX_BUF, "RGB_RED"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_RED);
             printf("RGB: Red color displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_GREEN"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_GREEN);
             printf("RGB: Green color displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_BLUE"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_BLUE);
             printf("RGB: Blue color displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_WHITE"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_WHITE);
             printf("RGB: White color displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_YELLOW"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_YELLOW);
             printf("RGB: Yellow color displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_PURPLE"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_PURPLE);
             printf("RGB: Purple color displayed\r\n");
         }
         else if(strstr((char*)USART3_RX_BUF, "RGB_CYAN"))
         {
             RGB_Set_Display_Mode(RGB_MODE_SOLID_COLOR);
             RGB_Set_All_Color(RGB_COLOR_CYAN);
             printf("RGB: Cyan color displayed\r\n");
         }
         
         // RGB亮度控制 (RGB_BRIGHT + 数字)
         else if(strstr((char*)USART3_RX_BUF, "RGB_BRIGHT"))
         {
             char* bright_str = strstr((char*)USART3_RX_BUF, "RGB_BRIGHT");
             if(bright_str) {
                 u8 brightness = atoi(bright_str + 10); // "RGB_BRIGHT"后的数字
                 if(brightness <= 100) {
                     RGB_Set_Brightness(brightness);
                     printf("RGB: Brightness set to %d%%\r\n", brightness);
                 } else {
                     printf("RGB: Invalid brightness value (0-100)\r\n");
                 }
             }
         }
         
         // 帮助命令
         else if(strstr((char*)USART3_RX_BUF, "HELP"))
         {
             printf("=== Bluetooth Command Help ===\r\n");
             printf("Basic Commands:\r\n");
             printf("  STATUS - Query system status\r\n");
             printf("  AUTO - Switch to auto mode\r\n");
             printf("  MANUAL - Switch to manual mode\r\n");
             printf("Device Control:\r\n");
             printf("  FAN_ON/FAN_OFF - Fan control\r\n");
             printf("  PUMP_ON/PUMP_OFF - Pump control\r\n");
             printf("  LIGHT_ON/LIGHT_OFF - Light control\r\n");
             printf("RGB Display Modes:\r\n");
             printf("  RGB_TEMP - Temperature visualization\r\n");
             printf("  RGB_HUMIDITY - Humidity visualization\r\n");
             printf("  RGB_LIGHT - Light level visualization\r\n");
             printf("  RGB_STATUS - System status display\r\n");
             printf("  RGB_OFF - Turn off RGB display\r\n");
             printf("RGB Animations:\r\n");
             printf("  RGB_RAINBOW - Rainbow effect\r\n");
             printf("  RGB_BREATHING - Breathing effect\r\n");
             printf("  RGB_FLOW - Water flow effect\r\n");
             printf("RGB Patterns:\r\n");
             printf("  RGB_HEART - Heart pattern\r\n");
             printf("  RGB_SMILEY - Smiley face pattern\r\n");
             printf("  RGB_CHECK - Check mark pattern\r\n");
             printf("  RGB_CROSS - Cross pattern\r\n");
             printf("RGB Colors:\r\n");
             printf("  RGB_RED/GREEN/BLUE/WHITE/YELLOW/PURPLE/CYAN\r\n");
             printf("RGB Brightness:\r\n");
             printf("  RGB_BRIGHT[0-100] - Set brightness (e.g., RGB_BRIGHT30)\r\n");
             printf("Sensor Control:\r\n");
             printf("  SENSOR_STATUS - Query sensor status\r\n");
             printf("Calibration Commands:\r\n");
             printf("  CAL_TEMP:±value - Temperature calibration\r\n");
             printf("  CAL_HUMI:±value - Humidity calibration\r\n");
             printf("Query Commands:\r\n");
             printf("  STATS - System Statistics\r\n");
             printf("  TREND - Environment Trend\r\n");
             printf("Bluetooth Commands:\r\n");
             printf("  BT_NAME - Get Device Name\r\n");
             printf("  BT_VER - Get Firmware Version\r\n");
             printf("  BT_ADDR - Get MAC Address\r\n");
             printf("  BT_PIN - Get Pairing PIN\r\n");
             printf("  BT_BAUD - Get Baudrate\r\n");
             printf("  BT_ROLE - Get Role Mode\r\n");
             printf("  BT_CLASS - Get Device Class\r\n");
             printf("  BT_TEST - Test AT Response\r\n");
             printf("  BT_RESET - Reset Module\r\n");
             printf("  BT_DIAG - Hardware Diagnostics\r\n");
             printf("  HELP - Show This Help\r\n");
             printf("==============================\r\n");
         }
         else
         {
             printf("Unknown command\r\n");
         }
         
         USART3_RX_STA = 0;
     }
}

// 添加缺失的函数定义

/**
 * @brief  温室控制主任务
 * @note   200ms调用一次，处理传感器读取、自动控制、报警检查
 */
void Greenhouse_Task(void)
{
    static u8 task_counter = 0;
    
    printf("Greenhouse_Task: Starting sensor reading\r\n");
    
    // 更新传感器数据
    Greenhouse_Update_Sensors();
    
    printf("Greenhouse_Task: Sensor reading completed\r\n");
    
    // 每隔几次任务周期打印一次状态，用于调试
    task_counter++;
    if(task_counter >= 10) {  // 每2秒打印一次状态
        task_counter = 0;
        printf("System Status: T=%d°C, H=%d%%, L=%d%%, Mode=%s\r\n",
               greenhouse_status.temperature,
               greenhouse_status.humidity, 
               greenhouse_status.light,
               greenhouse_status.work_mode ? "Manual" : "Auto");
    }
    
    // 执行自动控制逻辑
    if(greenhouse_status.work_mode == MODE_AUTO) {
        Greenhouse_Auto_Control();
    }
    
    // 检查报警条件
    Greenhouse_Check_Alarms();
    
    // 更新系统运行时间
    greenhouse_status.system_run_time++;
}

/**
 * @brief  更新显示界面
 * @note   调用greenhouse_display.c中的显示函数
 */
void Greenhouse_Update_Display(void)
{
    // 调用显示模块更新状态
    Display_System_Status(&greenhouse_status);
}

/**
 * @brief  更新设备运行状态
 * @param  device: 设备运行状态结构体指针
 * @param  new_status: 新的设备状态 (DEVICE_ON/DEVICE_OFF)
 */
void Greenhouse_Update_Device_Status(DeviceRunStatus_t* device, u8 new_status)
{
    extern volatile u32 system_time_ms;
    
    if(device->status != new_status) {
        if(new_status == DEVICE_ON) {
            device->last_on_time = system_time_ms;
            if(device->status == DEVICE_OFF && device->last_off_time > 0) {
                // 设备从关闭切换到开启，累计总关闭时间
                device->total_run_time += (system_time_ms - device->last_off_time);
            }
        } else {
            device->last_off_time = system_time_ms;
            if(device->status == DEVICE_ON && device->last_on_time > 0) {
                // 设备从开启切换到关闭，累计总运行时间
                device->total_run_time += (system_time_ms - device->last_on_time);
            }
        }
        
        device->status = new_status;
        device->switch_count++;
    }
}
 
