#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <reg51.h>

// 时间结构
typedef struct {
    unsigned int year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char tick_count;  // 10ms tick计数，改用char减少1字节
} DateTime;

// 全局时间变量
extern DateTime idata g_datetime;
extern bit g_time_changed;

// 函数声明
void Clock_Init(void);
void Clock_Update(void);
unsigned char IsLeapYear(unsigned int year);
unsigned char GetDaysInMonth(unsigned int year, unsigned char month);
void DateTime_AddSecond(void);
void DateTime_SetDate(unsigned int year, unsigned char month, unsigned char day);
void DateTime_SetTime(unsigned char hour, unsigned char minute, unsigned char second);
// 返回星期：0=Sunday, 1=Monday, ..., 6=Saturday
unsigned char GetDayOfWeek(unsigned int year, unsigned char month, unsigned char day);

#endif
