#ifndef __LCD_KS0108_H__
#define __LCD_KS0108_H__

#include <reg51.h>

// LCD端口定义
#define LCD_DATA P1         // DB0-7
sbit LCD_CS1 = P0^0;        // CS1 (左半屏)
sbit LCD_CS2 = P0^1;        // CS2 (右半屏)
sbit LCD_DI  = P0^2;        // D/I (数据/指令选择)
sbit LCD_RW  = P0^3;        // R/W (读写控制)
sbit LCD_E   = P0^4;        // E (使能)
sbit LCD_RST = P0^5;        // RST (复位)

// LCD显示尺寸
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_PAGES  8        // 64/8=8页

// LCD指令定义
#define LCD_CMD_DISPLAY_ON   0x3F  // 显示开
#define LCD_CMD_DISPLAY_OFF  0x3E  // 显示关
#define LCD_CMD_SET_Y        0x40  // 设置Y地址(列) | Y
#define LCD_CMD_SET_X        0xB8  // 设置X地址(页) | X
#define LCD_CMD_START_LINE   0xC0  // 设置起始行 | line

// 函数声明
void LCD_Init(void);
void LCD_Delay(void);
void LCD_CheckBusy(unsigned char side);
void LCD_WriteCmd(unsigned char cmd, unsigned char side);
void LCD_WriteData(unsigned char dat, unsigned char side);
unsigned char LCD_ReadData(unsigned char side);
void LCD_SetPos(unsigned char page, unsigned char col);
void LCD_Clear(void);
void LCD_ClearArea(unsigned char page_start, unsigned char page_end, unsigned char col_start, unsigned char col_end);
void LCD_DrawByte(unsigned char page, unsigned char col, unsigned char dat);
void LCD_Test(void);  // 测试函数
// void LCD_DrawPoint(unsigned char x, unsigned char y, unsigned char color);  // 未使用，已注释

#endif
