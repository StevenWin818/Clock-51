#ifndef __STOPWATCH_H__
#define __STOPWATCH_H__

#include <reg51.h>

// 秒表状态
#define SW_STOPPED  0
#define SW_RUNNING  1
#define SW_PAUSED   2

// 最大计次数 - 减少到8条以节省RAM
#define MAX_LAP_COUNT  8

// 秒表结构 - 优化内存使用
typedef struct {
    unsigned long ticks;        // 0.01秒为单位，支持长时间计时
    unsigned char state;        // 状态
} Stopwatch;

// 计次记录
typedef struct {
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
    unsigned char decisecond;   // 0.1秒
} LapRecord;

// 全局秒表变量
extern Stopwatch idata g_stopwatch;
extern LapRecord idata g_laps[MAX_LAP_COUNT];
extern unsigned char idata g_lap_count;
extern unsigned char idata g_lap_view_page;

// 函数声明
void Stopwatch_Init(void);
void Stopwatch_Update(void);
void Stopwatch_Start(void);
void Stopwatch_Pause(void);
void Stopwatch_Clear(void);
void Stopwatch_AddLap(void);
void Stopwatch_GetTime(unsigned char *hour, unsigned char *min, unsigned char *sec, unsigned char *ds);

#endif
