#ifndef _BEEP_H
#define _BEEP_H

#include "system.h"

/* Buzzer clock, port and pin definitions */
#define BEEP_PORT 			GPIOB   
#define BEEP_PIN 			GPIO_Pin_8
#define BEEP_PORT_RCC		RCC_APB2Periph_GPIOB

#define BEEP PBout(8)

// Buzzer state definitions
#define BEEP_ON  1
#define BEEP_OFF 0

// 报警类型
typedef enum {
    ALARM_TYPE_NONE = 0,
    ALARM_TYPE_HIGH_TEMP,   // 高温
    ALARM_TYPE_LOW_TEMP,    // 低温
    ALARM_TYPE_HIGH_HUMI,   // 高湿
    ALARM_TYPE_LOW_HUMI,    // 低湿
    ALARM_TYPE_LOW_LIGHT,   // 光照不足
    ALARM_TYPE_SENSOR_ERROR // 传感器错误
} AlarmType_t;

// Buzzer function declarations
void BEEP_Init(void);                           // Buzzer initialization
void BEEP_Set(u8 state);
void BEEP_Short(void);
void BEEP_Long(void);
void BEEP_Alarm(u8 count);
void BEEP_Warning(void);                        // Warning sound
void BEEP_Success(void);                        // Success sound
void BEEP_Set_Alarm(AlarmType_t alarm_type);
void BEEP_Task(void); // 在主循环中调用

#endif
