#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <reg51.h>

// 蜂鸣器引脚
sbit BUZZER = P2^0;

// 蜂鸣器状态
extern bit g_buzzer_active;

// 函数声明
void Buzzer_Init(void);
void Buzzer_Update(void);
void Buzzer_Short(void);  // 短响100ms
void Buzzer_Long(void);   // 长响500ms
void Buzzer_Check(void);  // 检查报时条件

#endif
