#ifndef _ws2812_H
#define _ws2812_H

#include "system.h"

// RGB LED硬件配置
#define RGB_LED 		GPIO_Pin_6
#define	RGB_LED_HIGH	(GPIO_SetBits(GPIOE,RGB_LED))
#define RGB_LED_LOW		(GPIO_ResetBits(GPIOE,RGB_LED))

// RGB LED矩阵尺寸
#define RGB_LED_XWIDTH	5
#define RGB_LED_YHIGH	5
#define RGB_LED_COUNT   (RGB_LED_XWIDTH * RGB_LED_YHIGH)

// RGB颜色定义 (GRB格式)
#define RGB_COLOR_RED		0x00FF00
#define RGB_COLOR_GREEN		0xFF0000
#define RGB_COLOR_BLUE		0x0000FF
#define RGB_COLOR_WHITE		0xFFFFFF
#define RGB_COLOR_YELLOW	0xFFFF00
#define RGB_COLOR_PURPLE	0x00FFFF
#define RGB_COLOR_CYAN		0xFF00FF
#define RGB_COLOR_ORANGE	0x80FF00
#define RGB_COLOR_PINK		0x40FF80
#define RGB_COLOR_OFF		0x000000

// RGB显示模式定义
typedef enum {
    RGB_MODE_OFF = 0,               // 关闭显示
    RGB_MODE_TEMP_DISPLAY,          // 温度显示
    RGB_MODE_HUMIDITY_DISPLAY,      // 湿度显示
    RGB_MODE_LIGHT_DISPLAY,         // 光照显示
    RGB_MODE_STATUS_DISPLAY,        // 状态显示
    RGB_MODE_ANIMATION,             // 动画模式
    RGB_MODE_PATTERN,               // 图案模式
    RGB_MODE_SOLID_COLOR            // 纯色模式
} RGB_DisplayMode_t;

// RGB动画类型定义
typedef enum {
    RGB_ANIM_NONE = 0,
    RGB_ANIM_RAINBOW,
    RGB_ANIM_BREATHING,
    RGB_ANIM_WATER_FLOW
} RGB_AnimationType_t;

// RGB温室系统状态结构
typedef struct {
    RGB_DisplayMode_t display_mode;
    RGB_AnimationType_t animation_type;
    u8 brightness;                  // 亮度 (0-100)
    u8 animation_step;              // 动画步骤
    u32 animation_color;            // 动画颜色
    u8 auto_cycle_enabled;          // 自动循环使能
    u32 mode_change_timer;          // 模式切换计时器
} RGB_GreenhouseStatus_t;

// 基础RGB LED控制函数
void RGB_LED_Init(void);
void RGB_LED_Reset(void);
void RGB_LED_Write_24Bits(uint8_t green, uint8_t red, uint8_t blue);
void RGB_LED_Clear(void);
void RGB_Clear_Buffer(void);
void RGB_LED_Update(void);

// RGB矩阵控制函数
void RGB_DrawDotColor(u8 x, u8 y, u8 status, u32 color);
void RGB_DrawLine_Color(u16 x1, u16 y1, u16 x2, u16 y2, u32 color);
void RGB_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u32 color);
void RGB_Draw_Circle(u16 x0, u16 y0, u8 r, u32 color);
void RGB_ShowCharNum(u8 num, u32 color);
void RGB_Set_All_Color(u32 color);
void RGB_Set_Pixel(u8 x, u8 y, u32 color);

// 温室系统专用RGB函数
void RGB_Greenhouse_Init(void);
void RGB_Set_Display_Mode(RGB_DisplayMode_t mode);
void RGB_Set_Brightness(u8 brightness);

// 传感器数据可视化函数
void RGB_Show_Temperature(u8 temperature);
void RGB_Show_Humidity(u8 humidity);
void RGB_Show_Light_Level(u8 light_level);
void RGB_Show_System_Status(u8 fan_status, u8 pump_status, u8 light_status);

// 动画效果函数
void RGB_Process_Animation(void);
void RGB_Start_Rainbow_Animation(void);
void RGB_Start_Breathing_Animation(u32 color);
void RGB_Start_Water_Flow_Animation(void);

// 图案显示函数
void RGB_Show_Heart(u32 color);
void RGB_Show_Smiley(u32 color);
void RGB_Show_Check_Mark(u32 color);
void RGB_Show_Cross(u32 color);
void RGB_Show_Arrow(u32 color);
void RGB_Show_Robot(u32 color);
void RGB_Show_Manual_Status_Face(u8 fan_status, u8 pump_status, u8 light_status, u8 fan_speed);

// 基础颜色控制函数
void RGB_LED_Red(void);
void RGB_LED_Green(void);
void RGB_LED_Blue(void);

// 辅助函数
u32 RGB_HSV_to_RGB(u16 hue, u8 saturation, u8 value);
u32 RGB_Blend_Colors(u32 color1, u32 color2, u8 blend_factor);
void RGB_Test_Pattern(void);
void RGB_Demo_All_Patterns(void);

// 全局状态变量声明
extern RGB_GreenhouseStatus_t rgb_greenhouse_status;

#endif
