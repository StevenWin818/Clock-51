#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <reg51.h>
#include "board_config.h"

// 蜂鸣器引脚
sbit BUZZER = P2^0;

// 按板级配置映射 BUZZER_ON / BUZZER_OFF
#if BUZZER_ACTIVE_LOW
#define BUZZER_ON  0
#define BUZZER_OFF 1
#else
#define BUZZER_ON  1
#define BUZZER_OFF 0
#endif

// 蜂鸣器状态
extern bit g_buzzer_active;

// 函数声明
void Buzzer_Init(void);
void Buzzer_Update(void);
void Buzzer_Short(void);  // 短响100ms
void Buzzer_Long(void);   // 长响500ms
void Buzzer_Check(void);  // 检查报时条件

// 请求取消本次（即即将到来的）整点报时（仅影响当前整点实例）
extern bit g_buzzer_suppressed;
void Buzzer_RequestCancelCurrentTop(void);
void Buzzer_CancelNow(void);

#endif
