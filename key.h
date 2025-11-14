#ifndef __KEY_H__
#define __KEY_H__

#include <reg51.h>

// 按键定义
sbit KEY1 = P3^0;  // 确认键
sbit KEY2 = P3^1;  // 增加键
sbit KEY3 = P3^2;  // 减少键
sbit KEY4 = P3^3;  // 模式切换键

// 按键状态
#define KEY_NONE    0
#define KEY_PRESS   1
#define KEY_LONG    2

// 按键值
#define KEY_VAL_NONE   0
#define KEY_VAL_1      1
#define KEY_VAL_2      2
#define KEY_VAL_3      3
#define KEY_VAL_4      4

// 长按时间阈值 (150个tick = 1.5秒)
#define KEY_LONG_PRESS_TICKS  150

// 按键结构
typedef struct {
    unsigned char key_val;
    unsigned char key_state;
    unsigned int press_ticks;
} KeyInfo;

// 全局按键变量
extern KeyInfo g_key_info;

// 函数声明
void Key_Init(void);
void Key_Scan(void);
unsigned char Key_GetPressed(void);

#endif
