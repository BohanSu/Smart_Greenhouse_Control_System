#include "ws2812.h"
#include "SysTick.h"
#include "stdio.h"

u8 g_rgb_databuf[3][RGB_LED_XWIDTH][RGB_LED_YHIGH];//RGB

// 全局RGB状态变量
RGB_GreenhouseStatus_t rgb_greenhouse_status = {
    RGB_MODE_OFF,        // display_mode
    RGB_ANIM_NONE,       // animation_type 
    50,                  // brightness (50%)
    0,                   // animation_step
    RGB_COLOR_OFF,       // animation_color
    0,                   // auto_cycle_enabled
    0                    // mode_change_timer
};

const u8 g_rgb_num_buf[][5]=
{
{0x70,0x88,0x88,0x88,0x70},//0
{0x00,0x48,0xF8,0x08,0x00},//1
{0x48,0x98,0xA8,0x48,0x00},//2
{0x00,0x88,0xA8,0x50,0x00},//3
{0x20,0x50,0x90,0x38,0x10},//4
{0x00,0xE8,0xA8,0xB8,0x00},//5
{0x00,0x70,0xA8,0xA8,0x30},//6
{0x80,0x98,0xA0,0xC0,0x00},//7
{0x50,0xA8,0xA8,0xA8,0x50},//8
{0x40,0xA8,0xA8,0xA8,0x70},//9
{0x38,0x50,0x90,0x50,0x38},//A
{0xF8,0xA8,0xA8,0x50,0x00},//B
{0x70,0x88,0x88,0x88,0x00},//C
{0xF8,0x88,0x88,0x50,0x20},//D
{0xF8,0xA8,0xA8,0xA8,0x00},//E
{0x00,0xF8,0xA0,0xA0,0x00},//F
};

void RGB_LED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
 	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);	 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		
	GPIO_Init(GPIOE, &GPIO_InitStructure);					
	GPIO_SetBits(GPIOE,GPIO_Pin_6);		
	
	RGB_LED_Clear();
}

void delay(u32 ns)//100ns
{
	while(ns--);
}

/********************************************************/
//
/********************************************************/
void RGB_LED_Write0(void)
{
	RGB_LED_HIGH;
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();
	RGB_LED_LOW;
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();
}

/********************************************************/
//
/********************************************************/

void RGB_LED_Write1(void)
{
	RGB_LED_HIGH;
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();
	RGB_LED_LOW;
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();
}

void RGB_LED_Reset(void)
{
	RGB_LED_LOW;
	delay_us(80);
	RGB_LED_HIGH;
}

void RGB_LED_Write_Byte(uint8_t byte)
{
	uint8_t i;

	for(i=0;i<8;i++)
	{
		if(byte&0x80)
		{
			RGB_LED_Write1();
		}
		else
		{
			RGB_LED_Write0();
		}
		byte <<= 1;
	}
}

void RGB_LED_Write_24Bits(uint8_t green,uint8_t red,uint8_t blue)
{
	RGB_LED_Write_Byte(green);
	RGB_LED_Write_Byte(red);
	RGB_LED_Write_Byte(blue);
}


//������ɫ�趨��������ɫ�Դ�����
void RGB_LED_Red(void)
{
	uint8_t i;
	//LEDȫ�ʵ�
	for(i=0;i<25;i++)
	{
		RGB_LED_Write_24Bits(0,0xff, 0);
	}
}

void RGB_LED_Green(void)
{
	uint8_t i;

	for(i=0;i<25;i++)
	{
		RGB_LED_Write_24Bits(0xff, 0, 0);
	}
}

void RGB_LED_Blue(void)
{
	uint8_t i;

	for(i=0;i<25;i++)
	{
		RGB_LED_Write_24Bits(0, 0, 0xff);
	}
}

void RGB_LED_Clear(void)
{
	u8 i;
	for(i=0;i<25;i++)
		RGB_LED_Write_24Bits(0,0,0);
	RGB_LED_Reset();
	delay_ms(10);
}

/**
 * @brief  清除RGB缓冲区
 */
void RGB_Clear_Buffer(void)
{
    u8 i, j;
    for(i = 0; i < RGB_LED_XWIDTH; i++) {
        for(j = 0; j < RGB_LED_YHIGH; j++) {
            g_rgb_databuf[0][i][j] = 0;  // red
            g_rgb_databuf[1][i][j] = 0;  // green  
            g_rgb_databuf[2][i][j] = 0;  // blue
        }
    }
}

//����
//x,y:����λ��
//status��1:������0:Ϩ��
//color��RGB��ɫ
void RGB_DrawDotColor(u8 x,u8 y,u8 status,u32 color)
{
	u8 i=0;
	u8 j=0;
	
	RGB_LED_Clear();
	if(status)
	{
		g_rgb_databuf[0][x][y]=color>>16;//r
		g_rgb_databuf[1][x][y]=color>>8;//g
		g_rgb_databuf[2][x][y]=color;//b
	}
	else
	{
		g_rgb_databuf[0][x][y]=0x00;
		g_rgb_databuf[1][x][y]=0x00;
		g_rgb_databuf[2][x][y]=0x00;
	}
		
	for(i=0;i<RGB_LED_YHIGH;i++)
	{
		for(j=0;j<RGB_LED_XWIDTH;j++)
			RGB_LED_Write_24Bits(g_rgb_databuf[1][j][i], g_rgb_databuf[0][j][i], g_rgb_databuf[2][j][i]);
	}
}

void RGB_DrawLine_Color(u16 x1, u16 y1, u16 x2, u16 y2,u32 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	delta_x=x2-x1; //������������ 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //���õ������� 
	else if(delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//ˮƽ�� 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //ѡȡ�������������� 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//������� 
	{  
		RGB_DrawDotColor(uRow,uCol,1,color);//���� 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
} 

//������	  
//(x1,y1),(x2,y2):���εĶԽ�����
void RGB_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u32 color)
{
	RGB_DrawLine_Color(x1,y1,x2,y1,color);
	RGB_DrawLine_Color(x1,y1,x1,y2,color);
	RGB_DrawLine_Color(x1,y2,x2,y2,color);
	RGB_DrawLine_Color(x2,y1,x2,y2,color);
}

//��ָ��λ�û�һ��ָ����С��Բ
//(x,y):���ĵ�
//r    :�뾶
void RGB_Draw_Circle(u16 x0,u16 y0,u8 r,u32 color)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //�ж��¸���λ�õı�־
	while(a<=b)
	{
		RGB_DrawDotColor(x0+a,y0-b,1,color);             //5
 		RGB_DrawDotColor(x0+b,y0-a,1,color);             //0           
		RGB_DrawDotColor(x0+b,y0+a,1,color);             //4               
		RGB_DrawDotColor(x0+a,y0+b,1,color);             //6 
		RGB_DrawDotColor(x0-a,y0+b,1,color);             //1       
 		RGB_DrawDotColor(x0-b,y0+a,1,color);             
		RGB_DrawDotColor(x0-a,y0-b,1,color);             //2             
  		RGB_DrawDotColor(x0-b,y0-a,1,color);             //7     	         
		a++;
		//ʹ��Bresenham�㷨��Բ     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
}

void RGB_ShowCharNum(u8 num,u32 color)
{
	u8 i=0,j=0;
	u8 x=0,y=0;
	u8 temp=0;
	
	for(j=0;j<5;j++)
	{
		temp=g_rgb_num_buf[num][j];
		for(i=0;i<5;i++)
		{
			if(temp&0x80)RGB_DrawDotColor(x,y,1,color);
			else RGB_DrawDotColor(x,y,0,color);
			temp<<=1;
			y++;
			if(y==RGB_LED_YHIGH)
			{
				y=0;
				x++;
				if(x==RGB_LED_XWIDTH)return;
			}
		}
	}
}

// ================== 温室系统专用RGB函数实现 ==================

/**
 * @brief  RGB温室系统初始化
 */
void RGB_Greenhouse_Init(void)
{
    RGB_LED_Init();
    
    // 初始化系统状态
    rgb_greenhouse_status.display_mode = RGB_MODE_OFF;
    rgb_greenhouse_status.animation_type = RGB_ANIM_NONE;
    rgb_greenhouse_status.brightness = 50;  // 默认50%亮度
    rgb_greenhouse_status.animation_step = 0;
    rgb_greenhouse_status.animation_color = RGB_COLOR_BLUE;
    rgb_greenhouse_status.auto_cycle_enabled = 1;
    rgb_greenhouse_status.mode_change_timer = 0;
    
    RGB_LED_Clear();
    printf("RGB: WS2812 LED matrix initialized successfully\r\n");
}

/**
 * @brief  设置所有LED为指定颜色
 */
void RGB_Set_All_Color(u32 color)
{
    u8 i, j;
    
    // 将颜色写入缓冲区
    for(i = 0; i < RGB_LED_XWIDTH; i++) {
        for(j = 0; j < RGB_LED_YHIGH; j++) {
            RGB_Set_Pixel(i, j, color);
        }
    }
    RGB_LED_Update();  // 统一更新到LED（含亮度调整）
}

/**
 * @brief  设置单个像素颜色
 */
void RGB_Set_Pixel(u8 x, u8 y, u32 color)
{
    if(x >= RGB_LED_XWIDTH || y >= RGB_LED_YHIGH) return;
    
    g_rgb_databuf[0][x][y] = (color >> 8) & 0xFF;   // red
    g_rgb_databuf[1][x][y] = (color >> 16) & 0xFF;  // green
    g_rgb_databuf[2][x][y] = color & 0xFF;          // blue
}

/**
 * @brief  更新LED显示
 */
void RGB_LED_Update(void)
{
    u8 i, j;
    u8 red, green, blue;
    
    for(i = 0; i < RGB_LED_YHIGH; i++) {
        for(j = 0; j < RGB_LED_XWIDTH; j++) {
            red = (g_rgb_databuf[0][j][i] * rgb_greenhouse_status.brightness) / 100;
            green = (g_rgb_databuf[1][j][i] * rgb_greenhouse_status.brightness) / 100;
            blue = (g_rgb_databuf[2][j][i] * rgb_greenhouse_status.brightness) / 100;
            
            RGB_LED_Write_24Bits(green, red, blue);
        }
    }
}

/**
 * @brief  设置显示模式
 */
void RGB_Set_Display_Mode(RGB_DisplayMode_t mode)
{
    rgb_greenhouse_status.display_mode = mode;
    if(mode == RGB_MODE_OFF) {
        RGB_LED_Clear();
    }
}

/**
 * @brief  设置亮度
 */
void RGB_Set_Brightness(u8 brightness)
{
    if(brightness > 100) brightness = 100;
    rgb_greenhouse_status.brightness = brightness;
}

/**
 * @brief  显示温度可视化（垂直条形图）
 */
void RGB_Show_Temperature(u8 temperature)
{
    u8 i, j;
    u32 color;
    u8 level = (temperature > 50) ? 5 : (temperature * 5) / 50;  // 0-5级别
    
    RGB_Clear_Buffer();  // 清除缓冲区而不是直接清除LED
    
    for(i = 0; i < RGB_LED_XWIDTH; i++) {
        for(j = 0; j < RGB_LED_YHIGH; j++) {
            if((5 - j) <= level) {
                // 温度颜色映射：蓝色->绿色->黄色->红色
                if(temperature < 15) color = RGB_COLOR_BLUE;
                else if(temperature < 25) color = RGB_COLOR_CYAN;
                else if(temperature < 30) color = RGB_COLOR_GREEN;
                else if(temperature < 35) color = RGB_COLOR_YELLOW;
                else color = RGB_COLOR_RED;
                
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 最后统一更新到LED
}

/**
 * @brief  显示湿度可视化（水平条形图）
 */
void RGB_Show_Humidity(u8 humidity)
{
    u8 i, j;
    u32 color;
    u8 level = (humidity > 100) ? 5 : (humidity * 5) / 100;  // 0-5级别
    
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < RGB_LED_XWIDTH; i++) {
        for(j = 0; j < RGB_LED_YHIGH; j++) {
            if(i < level) {
                // 湿度颜色映射：红色->黄色->绿色->蓝色
                if(humidity < 30) color = RGB_COLOR_RED;
                else if(humidity < 50) color = RGB_COLOR_YELLOW;
                else if(humidity < 70) color = RGB_COLOR_GREEN;
                else color = RGB_COLOR_BLUE;
                
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示光照级别（对角线图案）
 */
void RGB_Show_Light_Level(u8 light_level)
{
    u8 i, j;
    u32 color;
    u8 level = (light_level > 100) ? 10 : (light_level * 10) / 100;  // 0-10级别
    
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < RGB_LED_XWIDTH; i++) {
        for(j = 0; j < RGB_LED_YHIGH; j++) {
            if((i + j) < level) {
                // 光照颜色映射：紫色->青色->白色
                if(light_level < 30) color = RGB_COLOR_PURPLE;
                else if(light_level < 60) color = RGB_COLOR_CYAN;
                else color = RGB_COLOR_WHITE;
                
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示系统状态
 */
void RGB_Show_System_Status(u8 fan_status, u8 pump_status, u8 light_status)
{
    RGB_Clear_Buffer();  // 清除缓冲区
    
    // 风扇状态 - 青色（左上角）
    if(fan_status) {
        RGB_Set_Pixel(0, 0, RGB_COLOR_CYAN);
        RGB_Set_Pixel(1, 0, RGB_COLOR_CYAN);
        RGB_Set_Pixel(0, 1, RGB_COLOR_CYAN);
        RGB_Set_Pixel(1, 1, RGB_COLOR_CYAN);
    }
    
    // 水泵状态 - 蓝色（右上角）
    if(pump_status) {
        RGB_Set_Pixel(3, 0, RGB_COLOR_BLUE);
        RGB_Set_Pixel(4, 0, RGB_COLOR_BLUE);
        RGB_Set_Pixel(3, 1, RGB_COLOR_BLUE);
        RGB_Set_Pixel(4, 1, RGB_COLOR_BLUE);
    }
    
    // 补光灯状态 - 黄色（中心十字）
    if(light_status) {
        RGB_Set_Pixel(2, 2, RGB_COLOR_YELLOW);
        RGB_Set_Pixel(1, 2, RGB_COLOR_YELLOW);
        RGB_Set_Pixel(3, 2, RGB_COLOR_YELLOW);
        RGB_Set_Pixel(2, 1, RGB_COLOR_YELLOW);
        RGB_Set_Pixel(2, 3, RGB_COLOR_YELLOW);
    }
    
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  处理动画效果
 */
void RGB_Process_Animation(void)
{
    static u8 step = 0;
    static u8 direction = 0; // 0 for fade in, 1 for fade out
    u32 color;
    u8 brightness;

    if(rgb_greenhouse_status.display_mode != RGB_MODE_ANIMATION) return;

    switch(rgb_greenhouse_status.animation_type)
    {
        case RGB_ANIM_RAINBOW:
            // 彩虹效果
            rgb_greenhouse_status.animation_step = (rgb_greenhouse_status.animation_step + 1) % 360;
            RGB_Set_All_Color(RGB_HSV_to_RGB(rgb_greenhouse_status.animation_step, 255, rgb_greenhouse_status.brightness));
            break;
            
        case RGB_ANIM_BREATHING:
            // 呼吸灯效果
            if (direction == 0) {
                step++;
                if (step >= 100) direction = 1;
            } else {
                step--;
                if (step <= 0) direction = 0;
            }
            brightness = (u8)((float)step / 100.0f * rgb_greenhouse_status.brightness);
            color = RGB_Blend_Colors(RGB_COLOR_OFF, rgb_greenhouse_status.animation_color, brightness);
            RGB_Set_All_Color(color);
            break;
            
        case RGB_ANIM_WATER_FLOW:
            // 流水灯效果
            rgb_greenhouse_status.animation_step = (rgb_greenhouse_status.animation_step + 1) % 5;
            RGB_Clear_Buffer();
            RGB_Set_Pixel(rgb_greenhouse_status.animation_step, 0, rgb_greenhouse_status.animation_color);
            RGB_Set_Pixel(rgb_greenhouse_status.animation_step, 1, rgb_greenhouse_status.animation_color);
            RGB_Set_Pixel(rgb_greenhouse_status.animation_step, 2, rgb_greenhouse_status.animation_color);
            RGB_Set_Pixel(rgb_greenhouse_status.animation_step, 3, rgb_greenhouse_status.animation_color);
            RGB_Set_Pixel(rgb_greenhouse_status.animation_step, 4, rgb_greenhouse_status.animation_color);
            RGB_LED_Update();
            break;

        default:
            // 无动画
            break;
    }
}

/**
 * @brief  启动彩虹动画
 */
void RGB_Start_Rainbow_Animation(void)
{
    rgb_greenhouse_status.animation_type = RGB_ANIM_RAINBOW;
    rgb_greenhouse_status.animation_step = 0;
}

/**
 * @brief  启动呼吸灯动画
 * @param  color: 呼吸灯颜色
 */
void RGB_Start_Breathing_Animation(u32 color)
{
    rgb_greenhouse_status.animation_type = RGB_ANIM_BREATHING;
    rgb_greenhouse_status.animation_color = color;
    rgb_greenhouse_status.animation_step = 0;
}

/**
 * @brief  启动流水灯动画
 */
void RGB_Start_Water_Flow_Animation(void)
{
    rgb_greenhouse_status.animation_type = RGB_ANIM_WATER_FLOW;
    rgb_greenhouse_status.animation_step = 0;
}

/**
 * @brief  启动报警动画
 */
void RGB_Start_Alarm_Animation(void)
{
    // Do nothing
}

/**
 * @brief  停止报警动画
 */
void RGB_Stop_Alarm_Animation(void)
{
    // Do nothing
}

/**
 * @brief  显示心形图案
 */
void RGB_Show_Heart(u32 color)
{
    // 5x5心形图案
    const u8 heart_pattern[5][5] = {
        {0, 1, 0, 1, 0},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {0, 1, 1, 1, 0},
        {0, 0, 1, 0, 0}
    };
    
    u8 i, j;
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 5; j++) {
            if(heart_pattern[j][i]) {
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示笑脸图案
 */
void RGB_Show_Smiley(u32 color)
{
    // 5x5笑脸图案
    const u8 smiley_pattern[5][5] = {
        {0, 1, 1, 1, 0},
        {1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 1, 1, 0}
    };
    
    u8 i, j;
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 5; j++) {
            if(smiley_pattern[j][i]) {
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示对号图案
 */
void RGB_Show_Check_Mark(u32 color)
{
    // 5x5对号图案
    const u8 check_pattern[5][5] = {
        {0, 0, 0, 0, 1},
        {0, 0, 0, 1, 0},
        {1, 0, 1, 0, 0},
        {0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0}
    };
    
    u8 i, j;
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 5; j++) {
            if(check_pattern[j][i]) {
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示叉号图案
 */
void RGB_Show_Cross(u32 color)
{
    // 5x5叉号图案
    const u8 cross_pattern[5][5] = {
        {1, 0, 0, 0, 1},
        {0, 1, 0, 1, 0},
        {0, 0, 1, 0, 0},
        {0, 1, 0, 1, 0},
        {1, 0, 0, 0, 1}
    };
    
    u8 i, j;
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 5; j++) {
            if(cross_pattern[j][i]) {
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示箭头图案
 */
void RGB_Show_Arrow(u32 color)
{
    // 5x5箭头图案
    const u8 arrow_pattern[5][5] = {
        {0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0},
        {1, 0, 1, 0, 1},
        {0, 0, 1, 0, 0},
        {0, 0, 1, 0, 0}
    };
    
    u8 i, j;
    RGB_Clear_Buffer();  // 清除缓冲区
    
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 5; j++) {
            if(arrow_pattern[j][i]) {
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();  // 统一更新到LED
}

/**
 * @brief  显示机器人头像
 */
void RGB_Show_Robot(u32 color)
{
    const u8 robot_pattern[5][5] = {
        {1, 0, 1, 0, 1},
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {0, 1, 1, 1, 0},
        {1, 1, 0, 1, 1}
    };
    
    u8 i, j;
    RGB_Clear_Buffer();
    
    for(i = 0; i < 5; i++) {
        for(j = 0; j < 5; j++) {
            if(robot_pattern[j][i]) {
                RGB_Set_Pixel(i, j, color);
            }
        }
    }
    RGB_LED_Update();
}

/**
 * @brief  显示感叹号图案
 */
void RGB_Show_Exclamation_Marks(u8 count, u32 color)
{
    // Do nothing
}

/**
 * @brief  根据设备状态显示手动模式下的人脸图案
 * @param  fan_status: 风扇状态
 * @param  pump_status: 水泵状态
 * @param  light_status: 补光灯状态
 * @param  fan_speed: 风扇速度
 */
void RGB_Show_Manual_Status_Face(u8 fan_status, u8 pump_status, u8 light_status, u8 fan_speed)
{
    u32 left_eye_base_color = RGB_COLOR_BLUE;
    u32 right_eye_color = pump_status ? RGB_COLOR_BLUE : RGB_COLOR_WHITE;
    u32 mouth_color = light_status ? RGB_COLOR_BLUE : RGB_COLOR_WHITE;

    u32 left_eye_color;

    RGB_Clear_Buffer();

    // 左眼 (风扇) - 根据速度调整亮度
    if (fan_status) {
        u8 brightness = 20 + (fan_speed * 80 / 100); // 20% - 100% 亮度
        left_eye_color = RGB_Blend_Colors(RGB_COLOR_OFF, left_eye_base_color, brightness);
    } else {
        left_eye_color = RGB_COLOR_WHITE;
    }
    RGB_Set_Pixel(1, 1, left_eye_color);

    // 右眼 (水泵) - 蓝色
    RGB_Set_Pixel(3, 1, right_eye_color);

    // 嘴巴 (补光灯) - 蓝色
    RGB_Set_Pixel(1, 3, mouth_color);
    RGB_Set_Pixel(2, 3, mouth_color);
    RGB_Set_Pixel(3, 3, mouth_color);
    
    RGB_LED_Update();
}

/**
 * @brief  HSV转RGB颜色
 */
u32 RGB_HSV_to_RGB(u16 hue, u8 saturation, u8 value)
{
    u8 red, green, blue;
    u8 region, remainder, p, q, t;
    
    if(saturation == 0) {
        red = green = blue = value;
        return ((u32)green << 16) | ((u32)red << 8) | blue;
    }
    
    region = hue / 43;
    remainder = (hue - (region * 43)) * 6;
    
    p = (value * (255 - saturation)) >> 8;
    q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
    t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;
    
    switch(region) {
        case 0: red = value; green = t; blue = p; break;
        case 1: red = q; green = value; blue = p; break;
        case 2: red = p; green = value; blue = t; break;
        case 3: red = p; green = q; blue = value; break;
        case 4: red = t; green = p; blue = value; break;
        default: red = value; green = p; blue = q; break;
    }
    
    return ((u32)green << 16) | ((u32)red << 8) | blue;
}

/**
 * @brief  颜色混合
 */
u32 RGB_Blend_Colors(u32 color1, u32 color2, u8 blend_factor)
{
    u8 r1 = (color1 >> 8) & 0xFF, g1 = (color1 >> 16) & 0xFF, b1 = color1 & 0xFF;
    u8 r2 = (color2 >> 8) & 0xFF, g2 = (color2 >> 16) & 0xFF, b2 = color2 & 0xFF;
    
    u8 r = (r1 * (255 - blend_factor) + r2 * blend_factor) / 255;
    u8 g = (g1 * (255 - blend_factor) + g2 * blend_factor) / 255;
    u8 b = (b1 * (255 - blend_factor) + b2 * blend_factor) / 255;
    
    return ((u32)g << 16) | ((u32)r << 8) | b;
}

/**
 * @brief  测试图案
 */
void RGB_Test_Pattern(void)
{
    RGB_Set_All_Color(RGB_COLOR_RED);
    delay_ms(500);
    RGB_Set_All_Color(RGB_COLOR_GREEN);
    delay_ms(500);
    RGB_Set_All_Color(RGB_COLOR_BLUE);
    delay_ms(500);
    RGB_LED_Clear();
}

/**
 * @brief  RGB演示测试 - 展示所有模式和图案
 */
void RGB_Demo_All_Patterns(void)
{
    printf("=== RGB Demo: Testing All Patterns ===\r\n");
    
    // 1. 测试基础颜色
    printf("RGB: Testing basic colors...\r\n");
    RGB_Set_All_Color(RGB_COLOR_RED);
    delay_ms(1000);
    RGB_Set_All_Color(RGB_COLOR_GREEN);
    delay_ms(1000);
    RGB_Set_All_Color(RGB_COLOR_BLUE);
    delay_ms(1000);
    RGB_Set_All_Color(RGB_COLOR_YELLOW);
    delay_ms(1000);
    RGB_Set_All_Color(RGB_COLOR_PURPLE);
    delay_ms(1000);
    RGB_Set_All_Color(RGB_COLOR_CYAN);
    delay_ms(1000);
    RGB_Set_All_Color(RGB_COLOR_WHITE);
    delay_ms(1000);
    RGB_LED_Clear();
    delay_ms(500);
    
    // 2. 测试传感器数据图案
    printf("RGB: Testing sensor data patterns...\r\n");
    
    // 温度显示测试
    printf("RGB: Temperature pattern (15-40°C)...\r\n");
    RGB_Show_Temperature(15);  // 蓝色
    delay_ms(1500);
    RGB_Show_Temperature(25);  // 青色
    delay_ms(1500);
    RGB_Show_Temperature(30);  // 绿色
    delay_ms(1500);
    RGB_Show_Temperature(35);  // 黄色
    delay_ms(1500);
    RGB_Show_Temperature(40);  // 红色
    delay_ms(1500);
    
    // 湿度显示测试
    printf("RGB: Humidity pattern (20-80%%)...\r\n");
    RGB_Show_Humidity(20);   // 红色
    delay_ms(1500);
    RGB_Show_Humidity(40);   // 黄色
    delay_ms(1500);
    RGB_Show_Humidity(60);   // 绿色
    delay_ms(1500);
    RGB_Show_Humidity(80);   // 蓝色
    delay_ms(1500);
    
    // 光照显示测试
    printf("RGB: Light level pattern (20-80%%)...\r\n");
    RGB_Show_Light_Level(20);  // 紫色
    delay_ms(1500);
    RGB_Show_Light_Level(50);  // 青色
    delay_ms(1500);
    RGB_Show_Light_Level(80);  // 白色
    delay_ms(1500);
    
    // 系统状态显示测试
    printf("RGB: System status patterns...\r\n");
    RGB_Show_System_Status(1, 0, 0);  // 只有风扇
    delay_ms(1500);
    RGB_Show_System_Status(0, 1, 0);  // 只有水泵
    delay_ms(1500);
    RGB_Show_System_Status(0, 0, 1);  // 只有补光灯
    delay_ms(1500);
    RGB_Show_System_Status(1, 1, 1);  // 全部开启
    delay_ms(1500);
    
    // 3. 测试图案形状
    printf("RGB: Testing pattern shapes...\r\n");
    
    RGB_Show_Heart(RGB_COLOR_RED);
    delay_ms(2000);
    printf("RGB: Heart pattern displayed\r\n");
    
    RGB_Show_Smiley(RGB_COLOR_YELLOW);
    delay_ms(2000);
    printf("RGB: Smiley pattern displayed\r\n");
    
    RGB_Show_Check_Mark(RGB_COLOR_GREEN);
    delay_ms(2000);
    printf("RGB: Check mark pattern displayed\r\n");
    
    RGB_Show_Cross(RGB_COLOR_RED);
    delay_ms(2000);
    printf("RGB: Cross pattern displayed\r\n");
    
    RGB_Show_Arrow(RGB_COLOR_BLUE);
    delay_ms(2000);
    printf("RGB: Arrow pattern displayed\r\n");
    
    // 4. 测试动画效果
    printf("RGB: Testing animations...\r\n");
    
    // 彩虹动画
    printf("RGB: Rainbow animation...\r\n");
    RGB_Start_Rainbow_Animation();
    {
        u8 i;
        for(i = 0; i < 50; i++) {
            RGB_Process_Animation();
            delay_ms(100);
        }
    }
    
    // 呼吸灯动画
    printf("RGB: Breathing animation...\r\n");
    RGB_Start_Breathing_Animation(RGB_COLOR_BLUE);
    {
        u8 i;
        for(i = 0; i < 100; i++) {
            RGB_Process_Animation();
            delay_ms(50);
        }
    }
    
    RGB_LED_Clear();
    printf("=== RGB Demo Complete ===\r\n");
}

