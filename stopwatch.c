#include "stopwatch.h"
#include "clock.h"

// 全局秒表变量 - 优化内存分配
Stopwatch idata g_stopwatch = {0UL, SW_STOPPED};
LapRecord idata g_laps[MAX_LAP_COUNT];
unsigned char idata g_lap_count = 0;
unsigned char idata g_lap_view_page = 0;

// 秒表初始化
void Stopwatch_Init(void) {
    g_stopwatch.ticks = 0UL;
    g_stopwatch.state = SW_STOPPED;
    g_lap_count = 0;
    g_lap_view_page = 0;
}

// 秒表更新 (每10ms调用一次)
void Stopwatch_Update(void) {
    if(g_stopwatch.state == SW_RUNNING) {
        g_stopwatch.ticks++;
    }
}

// 启动秒表
void Stopwatch_Start(void) {
    g_stopwatch.state = SW_RUNNING;
}

// 暂停秒表
void Stopwatch_Pause(void) {
    g_stopwatch.state = SW_PAUSED;
}

// 清零秒表
void Stopwatch_Clear(void) {
    g_stopwatch.ticks = 0UL;
    g_stopwatch.state = SW_STOPPED;
    g_lap_count = 0;
    g_lap_view_page = 0;
}

// 添加计次
void Stopwatch_AddLap(void) {
    unsigned char hour, min, sec, ds;
    
    if(g_lap_count >= MAX_LAP_COUNT) {
        return;  // 已达最大计次数
    }
    
    Stopwatch_GetTime(&hour, &min, &sec, &ds);
    
    g_laps[g_lap_count].hour = hour;
    g_laps[g_lap_count].minute = min;
    g_laps[g_lap_count].second = sec;
    g_laps[g_lap_count].decisecond = ds;
    g_lap_count++;
}

// 获取秒表时间
void Stopwatch_GetTime(unsigned char *hour, unsigned char *min, unsigned char *sec, unsigned char *ds) {
    unsigned long value = g_stopwatch.ticks;   // 百分之一秒

    *ds = (unsigned char)((value / 10UL) % 10UL);  // 0.1秒
    value /= 100UL;                                 // 秒
    *sec = (unsigned char)(value % 60UL);
    value /= 60UL;                                  // 分
    *min = (unsigned char)(value % 60UL);
    value /= 60UL;                                  // 时
    if(value > 99UL) {
        value = 99UL;
    }
    *hour = (unsigned char)value;
}
