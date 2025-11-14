#include "buzzer.h"
#include "clock.h"

/* ===== 共享状态：在 ISR 与主线程之间传递，建议 volatile ===== */
volatile bit g_buzzer_active = 0;
/* buzzer_data: 低6位 = 剩余tick(0~63)，高6位 = last_second(0~59) */
static volatile unsigned char buzzer_data = 0;

/* 工具宏：存取低/高 6 位 */
#define BUZZ_REMAINING_GET() (buzzer_data & 0x3F)
#define BUZZ_REMAINING_SET(v) (buzzer_data = (buzzer_data & 0xC0) | ((v) & 0x3F))
#define BUZZ_LASTSEC_GET() ((buzzer_data >> 6) & 0x3F)
#define BUZZ_LASTSEC_SET(sec) (buzzer_data = (buzzer_data & 0x3F) | (((sec) & 0x3F) << 6))

/* 若硬件是低电平有效，请改成：#define BUZZER_ON  0 / #define BUZZER_OFF 1 */
#define BUZZER_ON 1
#define BUZZER_OFF 0

/* 统一启动蜂鸣器，durationTicks 以 10ms 为单位 */
static void Buzzer_Start(unsigned char durationTicks)
{
    if (durationTicks == 0)
        return;
    BUZZER = BUZZER_ON;
    g_buzzer_active = 1;
    BUZZ_REMAINING_SET(durationTicks); /* 锁定时长，后续只递减，不看当前时间 */
}

/* 初始化 */
void Buzzer_Init(void)
{
    BUZZER = BUZZER_OFF; /* 初始静音 */
    g_buzzer_active = 0;
    buzzer_data = 0x00;     /* remaining=0, last_second=0 */
    BUZZ_LASTSEC_SET(0x3F); /* 设成无效秒，避免上电立即触发 */
}

/* 每 10ms 在定时器 ISR 中调用一次 */
void Buzzer_Update(void)
{
    unsigned char rem;

    if (!g_buzzer_active)
        return;

    rem = BUZZ_REMAINING_GET();
    if (rem == 0)
    {
        /* 容错：剩余为0但仍 active */
        BUZZER = BUZZER_OFF;
        g_buzzer_active = 0;
        return;
    }

    rem--;
    BUZZ_REMAINING_SET(rem);
    if (rem == 0)
    {
        BUZZER = BUZZER_OFF;
        g_buzzer_active = 0;
    }
}

/* 通用脉冲（ticks 单位为 10ms）——供短/长响调用 */
static void Buzzer_Pulse(unsigned char ticks)
{
    if (g_datetime.year >= 2000 && g_datetime.year <= 2099 &&
        g_datetime.month >= 1 && g_datetime.month <= 12)
    {
        Buzzer_Start(ticks);
    }
}

/* 100ms 短响（10 tick） */
void Buzzer_Short(void)
{
    Buzzer_Pulse(10);
}

/* 500ms 长响（50 tick） */
void Buzzer_Long(void)
{
    Buzzer_Pulse(50);
}

/* 报时触发逻辑：55~59 秒短响；0 秒长响 */
void Buzzer_Check(void)
{
    unsigned char now_sec = g_datetime.second & 0x3F;
    unsigned char last_sec = BUZZ_LASTSEC_GET();

    /* 用 last_second 防抖，确保每秒只触发一次 */
    if (now_sec == last_sec)
    {
        return;
    }
    BUZZ_LASTSEC_SET(now_sec);

    /* 59分的 55~59 秒短响（每秒一次） */
    if (g_datetime.minute == 59 && now_sec >= 55 && now_sec <= 59)
    {
        Buzzer_Short();
        return;
    }

    /* 整点：第 0 秒长响（仅一次） */
    if (g_datetime.minute == 0 && now_sec == 0)
    {
        Buzzer_Long();
        return;
    }

    /* 其余时间不触发 */
}