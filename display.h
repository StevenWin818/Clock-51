#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <reg51.h>
#include "lcd_ks0108.h"
#include "font.h"

// 外部变量声明（定义在main.c中）
extern unsigned int data g_temp_year;
extern unsigned char data g_temp_month;
extern unsigned char data g_temp_day;
extern unsigned char data g_temp_hour;
extern unsigned char data g_temp_minute;
extern unsigned char data g_temp_second;
extern unsigned char data g_edit_pos;
extern unsigned char data g_menu_pos;
extern unsigned char data g_system_state;

// 显示函数声明
void Display_Char_8x16(unsigned char page, unsigned char col, char c);
void Display_Char_16x16(unsigned char page, unsigned char col, char c);
/* 24x32 字体渲染函数已移除（未被调用），保留 Display_Char_16x16/8x16 作为基础接口 */

// UI显示函数
void Display_HomePage(void);
void Display_StopwatchPage(void);
void Display_LapViewPage(void);
void Display_ResetStopwatch(void);  // 重置秒表页面状态

#endif
