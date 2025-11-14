#include "clock.h"

// 全局时间变量 - 使用idata(内部RAM间接寻址)
DateTime idata g_datetime = {2025, 11, 14, 17, 42, 0, 0};
bit g_time_changed = 0;

// 定时器0初始化 (10ms定时)
// 22.1184MHz晶振，12分频，定时器0工作在模式1
// 10ms定时值 = 65536 - (22118400/12/100) = 65536 - 18432 = 47104 = 0xB800
void Clock_Init(void) {
    TMOD &= 0xF0;  // 清除Timer0设置
    TMOD |= 0x01;  // Timer0模式1 (16位定时器)
    
    TH0 = 0xB8;    // 装载初值
    TL0 = 0x00;
    
    ET0 = 1;       // 使能Timer0中断
    EA = 1;        // 使能全局中断
    TR0 = 1;       // 启动Timer0
}

// 时钟更新 (每10ms调用一次)
void Clock_Update(void) {
    g_datetime.tick_count++;
    
    if(g_datetime.tick_count >= 100) {  // 100个tick = 1秒
        g_datetime.tick_count = 0;
        DateTime_AddSecond();
        g_time_changed = 1;
    }
}

// 闰年判断
unsigned char IsLeapYear(unsigned int year) {
    if(year % 400 == 0) {
        return 1;
    } else if(year % 100 == 0) {
        return 0;
    } else if(year % 4 == 0) {
        return 1;
    }
    return 0;
}

// 获取月份天数
unsigned char GetDaysInMonth(unsigned int year, unsigned char month) {
    unsigned char days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if(month == 2 && IsLeapYear(year)) {
        return 29;
    }
    return days_in_month[month - 1];
}

// 增加一秒
void DateTime_AddSecond(void) {
    g_datetime.second++;
    
    if(g_datetime.second >= 60) {
        g_datetime.second = 0;
        g_datetime.minute++;
        
        if(g_datetime.minute >= 60) {
            g_datetime.minute = 0;
            g_datetime.hour++;
            
            if(g_datetime.hour >= 24) {
                g_datetime.hour = 0;
                g_datetime.day++;
                
                if(g_datetime.day > GetDaysInMonth(g_datetime.year, g_datetime.month)) {
                    g_datetime.day = 1;
                    g_datetime.month++;
                    
                    if(g_datetime.month > 12) {
                        g_datetime.month = 1;
                        g_datetime.year++;
                    }
                }
            }
        }
    }
}

// 设置日期
void DateTime_SetDate(unsigned int year, unsigned char month, unsigned char day) {
    g_datetime.year = year;
    g_datetime.month = month;
    g_datetime.day = day;
}

// 设置时间
void DateTime_SetTime(unsigned char hour, unsigned char minute, unsigned char second) {
    g_datetime.hour = hour;
    g_datetime.minute = minute;
    g_datetime.second = second;
    g_datetime.tick_count = 0;
}
