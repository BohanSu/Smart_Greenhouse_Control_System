#include "beep.h"
#include "SysTick.h"

// Buzzer initialization
void BEEP_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    BEEP = 0; // Initialize buzzer off
}

// Set buzzer state
void BEEP_Set(u8 state)
{
    BEEP = state;
}

// Short beep sound
void BEEP_Short(void)
{
    BEEP = 1;
    delay_ms(100);
    BEEP = 0;
}

// Long beep sound
void BEEP_Long(void)
{
    BEEP = 1;
    delay_ms(500);
    BEEP = 0;
}

// Alarm sound (multiple short beeps)
void BEEP_Alarm(u8 count)
{
    u8 i;
    for(i = 0; i < count; i++)
    {
        BEEP = 1;
        delay_ms(200);
        BEEP = 0;
        delay_ms(200);
    }
}

// Warning prompt sound - three short beeps
void BEEP_Warning(void)
{
    BEEP_Alarm(3);
}

// Success prompt sound - one long beep
void BEEP_Success(void)
{
    BEEP_Long();
} 

static AlarmType_t current_alarm = ALARM_TYPE_NONE;
static u32 beep_timer = 0;

extern volatile u32 system_time_ms; // 使用在 main.c 中定义的全局变量

void BEEP_Set_Alarm(AlarmType_t alarm_type) {
    current_alarm = alarm_type;
    if (alarm_type != ALARM_TYPE_NONE) {
        beep_timer = system_time_ms; // 记录开始报警的时间
    }
}

void BEEP_Task(void) {
    u32 elapsed_time;

    if (current_alarm == ALARM_TYPE_NONE) {
        BEEP = 0;
        return;
    }

    elapsed_time = system_time_ms - beep_timer;

    switch (current_alarm) {
        case ALARM_TYPE_HIGH_TEMP: // 急促的双短音
            if ((elapsed_time % 400 < 100) || (elapsed_time % 400 > 200 && elapsed_time % 400 < 300)) BEEP = 1;
            else BEEP = 0;
            break;
        case ALARM_TYPE_LOW_TEMP: // 长音警告
            if (elapsed_time % 1000 < 500) BEEP = 1;
            else BEEP = 0;
            break;
        case ALARM_TYPE_HIGH_HUMI: // 间断的长音
            if (elapsed_time % 1500 < 700) BEEP = 1;
            else BEEP = 0;
            break;
        case ALARM_TYPE_LOW_HUMI: // 急促的连续短音
            if (elapsed_time % 200 < 100) BEEP = 1;
            else BEEP = 0;
            break;
        case ALARM_TYPE_LOW_LIGHT: // 较慢的单音
            if (elapsed_time % 800 < 200) BEEP = 1;
            else BEEP = 0;
            break;
        case ALARM_TYPE_SENSOR_ERROR: // 持续长音
            BEEP = 1;
            break;
        default:
            BEEP = 0;
            break;
    }
} 
