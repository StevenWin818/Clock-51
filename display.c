#include "display.h"
#include "clock.h"
#include "stopwatch.h"
#include "state.h"
#include <reg51.h>

/* 外部变量和原始数据 */
extern bit g_menu_cursor_visible;

typedef struct
{
    unsigned char top[16];
    unsigned char bottom[16];
} Hanzi16;

static const Hanzi16 code g_hanzi_table[] = {
    {// "秒"
     {0x24, 0x24, 0xA4, 0xFE, 0x23, 0x22, 0x00, 0xC0, 0x38, 0x00, 0xFF, 0x00, 0x08, 0x10, 0x60, 0x00},
     {0x08, 0x06, 0x01, 0xFF, 0x01, 0x06, 0x81, 0x80, 0x40, 0x40, 0x27, 0x10, 0x0C, 0x03, 0x00, 0x00}},
    {// "表"
     {0x00, 0x04, 0x24, 0x24, 0x24, 0x24, 0x24, 0xFF, 0x24, 0x24, 0x24, 0x24, 0x24, 0x04, 0x00, 0x00},
     {0x21, 0x21, 0x11, 0x09, 0xFD, 0x83, 0x41, 0x23, 0x05, 0x09, 0x11, 0x29, 0x25, 0x41, 0x41, 0x00}}};

enum
{
    HANZI_MIAO = 0,
    HANZI_BIAO = 1
};

// 英文星期缩写表 (0=Sun .. 6=Sat)
static const unsigned char code g_week_abbrev[7][4] = {
    "SUN",
    "MON",
    "TUE",
    "WED",
    "THU",
    "FRI",
    "SAT",
};

#define DISPLAY_SCREEN_NONE 0xFF
#define DISPLAY_SCREEN_HOME 0
#define DISPLAY_SCREEN_STOPWATCH 2
#define DISPLAY_SCREEN_LAP 2

/* 静态全局变量 */
static unsigned char idata g_last_screen = DISPLAY_SCREEN_NONE;
static bit g_stopwatch_initialized = 0;
static unsigned char idata g_prev_stop_hour = 0xFF;
static unsigned char idata g_prev_stop_minute = 0xFF;
static unsigned char idata g_prev_stop_second = 0xFF;
static unsigned char idata g_prev_stop_decisecond = 0xFF;
static unsigned char idata g_prev_lap_index[3] = {0xFF, 0xFF, 0xFF};
static unsigned char idata g_prev_lap_hour[3];
static unsigned char idata g_prev_lap_minute[3];
static unsigned char idata g_prev_lap_second[3];
static unsigned char idata g_prev_lap_decisecond[3];
static unsigned char idata g_prev_menu_pos = 0xFF;

//基础绘制函数
static void Display_Hanzi16(unsigned char page, unsigned char col, unsigned char index)
{
    unsigned char i;
    for (i = 0; i < 16; i++)
    {
        LCD_DrawByte(page, col + i, g_hanzi_table[index].top[i]);
        LCD_DrawByte(page + 1, col + i, g_hanzi_table[index].bottom[i]);
    }
}

static void Display_DrawMenuCursor(unsigned char line, bit visible)
{
    // 优化: 合并变量声明，减少行数
    unsigned char base_page, total_width, time_col, i, col;

    base_page = (line == 0) ? 0 : ((line == 1) ? 3 : 6);

    /* 计算光标位置 */
    {
        total_width = 16 + (8 - 1) * 12; // 100
        time_col = (128 - total_width) / 2;
        col = (time_col > 10) ? (time_col - 10) : 0;
    }

    if (visible)
    {
        /* 绘制箭头：已替换为自定义图形（前8字节为 top，后8字节为 bottom） */
        static const unsigned char code arrow_top[8] = {
            0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
        static const unsigned char code arrow_bottom[8] = {
            0x0F, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00};
        for (i = 0; i < 8; i++)
        {
            LCD_DrawByte(base_page, col + i, arrow_top[i]);
            LCD_DrawByte(base_page + 1, col + i, arrow_bottom[i]);
        }
    }
    else
    {
        // 清除箭头
        LCD_ClearArea(base_page, base_page + 1, col, col + 7);
    }
}

static void Display_Char_8x16_Custom(unsigned char page, unsigned char col, char c, bit underline)
{
    unsigned char i, index = GetCharIndex(c), data_byte;
    for (i = 0; i < 8; i++)
    {
        data_byte = Font_8x16[index][i];
        LCD_DrawByte(page, col + i, data_byte);
        data_byte = Font_8x16[index][i + 8];
        if (underline)
        {
            data_byte |= 0x80;
        }
        LCD_DrawByte(page + 1, col + i, data_byte);
    }
}

static void Display_Char_16x16_Custom(unsigned char page, unsigned char col, char c, bit underline)
{
    unsigned char i, index = GetCharIndex(c), data_byte;
    for (i = 0; i < 16; i++)
    {
        data_byte = Font_16x16[index][i];
        LCD_DrawByte(page, col + i, data_byte);
        data_byte = Font_16x16[index][i + 16];
        if (underline)
        {
            data_byte |= 0x80;
        }
        LCD_DrawByte(page + 1, col + i, data_byte);
    }
}

/* 公共接口 */
void Display_Char_8x16(unsigned char page, unsigned char col, char c)
{
    Display_Char_8x16_Custom(page, col, c, 0);
}

void Display_Char_16x16(unsigned char page, unsigned char col, char c)
{
    Display_Char_16x16_Custom(page, col, c, 0);
}

/* 绘制秒表的小数点 */
static void Display_DrawDecimalPoint(unsigned char page, unsigned char col)
{
    unsigned char idata i;
    for (i = 0; i < 8; i++)
    {
        LCD_DrawByte(page, col + i, 0x00);
        LCD_DrawByte(page + 1, col + i, (i == 3 || i == 4) ? 0x18 : 0x00);
    }
}

/* 绘制一行计次时间 (HH:MM:SS.D) (复用于 StopwatchPage 和 LapViewPage) */
static void Display_DrawLapTime(unsigned char page, unsigned char base_col, unsigned char lap_index)
{
    Display_Char_8x16(page, base_col + 0, g_laps[lap_index].hour / 10 + '0');
    Display_Char_8x16(page, base_col + 8, g_laps[lap_index].hour % 10 + '0');
    Display_Char_8x16(page, base_col + 16, ':');
    Display_Char_8x16(page, base_col + 24, g_laps[lap_index].minute / 10 + '0');
    Display_Char_8x16(page, base_col + 32, g_laps[lap_index].minute % 10 + '0');
    Display_Char_8x16(page, base_col + 40, ':');
    Display_Char_8x16(page, base_col + 48, g_laps[lap_index].second / 10 + '0');
    Display_Char_8x16(page, base_col + 56, g_laps[lap_index].second % 10 + '0');
    Display_DrawDecimalPoint(page, base_col + 64); /* 复用 */
    Display_Char_8x16(page, base_col + 72, g_laps[lap_index].decisecond + '0');
}

/* 重置指定索引的 "上一次" 计次缓存 */
static void Display_ResetPrevLap(unsigned char i)
{
    g_prev_lap_index[i] = 0xFF;
    g_prev_lap_hour[i] = 0xFF;
    g_prev_lap_minute[i] = 0xFF;
    g_prev_lap_second[i] = 0xFF;
    g_prev_lap_decisecond[i] = 0xFF;
}

/* 重置所有 "上一次" 的秒表和计次缓存变量 (复用于 ResetStopwatch 和 StopwatchPage) */
static void Display_Internal_ResetStopwatchVars(void)
{
    unsigned char idata i;
    g_prev_stop_hour = 0xFF;
    g_prev_stop_minute = 0xFF;
    g_prev_stop_second = 0xFF;
    g_prev_stop_decisecond = 0xFF;
    for (i = 0; i < 3; i++)
    {
        Display_ResetPrevLap(i); /* 复用 */
    }
}

/* 检查指定的计次是否需要更新 */
static bit NeedLapUpdate(unsigned char i, unsigned char lap_index)
{
    return (g_prev_lap_index[i] != lap_index) ||
           (g_prev_lap_hour[i] != g_laps[lap_index].hour) ||
           (g_prev_lap_minute[i] != g_laps[lap_index].minute) ||
           (g_prev_lap_second[i] != g_laps[lap_index].second) ||
           (g_prev_lap_decisecond[i] != g_laps[lap_index].decisecond);
}

/* 更新 "上一次" 计次缓存为当前值 */
static void Display_SetPrevLap(unsigned char i, unsigned char lap_index)
{
    g_prev_lap_index[i] = lap_index;
    g_prev_lap_hour[i] = g_laps[lap_index].hour;
    g_prev_lap_minute[i] = g_laps[lap_index].minute;
    g_prev_lap_second[i] = g_laps[lap_index].second;
    g_prev_lap_decisecond[i] = g_laps[lap_index].decisecond;
}

// 主显示函数
void Display_HomePage(void)
{
    // 优化: 合并变量声明，减少行数
    unsigned int year_display;
    unsigned char month_display, day_display, total_width, date_highlight;
    unsigned char hour_display, minute_display, second_display;
    bit editing_date, editing_time, menu_active, first_draw;
    bit highlight_hour_tens, highlight_hour_units, highlight_minute_tens;
    bit highlight_minute_units, highlight_second_pair;
    unsigned char page, col;

    first_draw = (g_last_screen != DISPLAY_SCREEN_HOME);
    g_last_screen = DISPLAY_SCREEN_HOME;

    if (first_draw)
    {
        LCD_ClearArea(0, 7, 0, 127);
    }

    // 状态计算
    editing_date = (g_system_state == STATE_DATE_SET);
    editing_time = (g_system_state == STATE_TIME_SET);
    menu_active = (g_system_state == STATE_MENU);

    if (editing_date)
    {
        year_display = g_temp_year;
        month_display = g_temp_month;
        day_display = g_temp_day;
        date_highlight = 7 - g_edit_pos;
    }
    else
    {
        year_display = g_datetime.year;
        month_display = g_datetime.month;
        day_display = g_datetime.day;
    }

    if (editing_time)
    {
        hour_display = g_temp_hour;
        minute_display = g_temp_minute;
        second_display = g_temp_second;
    }
    else
    {
        hour_display = g_datetime.hour;
        minute_display = g_datetime.minute;
        second_display = g_datetime.second;
    }

    highlight_hour_tens = editing_time && (g_edit_pos == 5);
    highlight_hour_units = editing_time && (g_edit_pos == 4);
    highlight_minute_tens = editing_time && (g_edit_pos == 3);
    highlight_minute_units = editing_time && (g_edit_pos == 2);
    highlight_second_pair = editing_time && (g_edit_pos == 0 || g_edit_pos == 1);

    // 日期绘制
    col = 12;
    Display_Char_8x16_Custom(0, col, (year_display / 1000) + '0', editing_date && date_highlight == 0);
    col += 8;
    Display_Char_8x16_Custom(0, col, ((year_display / 100) % 10) + '0', editing_date && date_highlight == 1);
    col += 8;
    Display_Char_8x16_Custom(0, col, ((year_display / 10) % 10) + '0', editing_date && date_highlight == 2);
    col += 8;
    Display_Char_8x16_Custom(0, col, (year_display % 10) + '0', editing_date && date_highlight == 3);
    col += 8;
    Display_Char_8x16(0, col, '-');
    col += 8;
    Display_Char_8x16_Custom(0, col, (month_display / 10) + '0', editing_date && date_highlight == 4);
    col += 8;
    Display_Char_8x16_Custom(0, col, (month_display % 10) + '0', editing_date && date_highlight == 5);
    col += 8;
    Display_Char_8x16(0, col, '-');
    col += 8;
    Display_Char_8x16_Custom(0, col, (day_display / 10) + '0', editing_date && date_highlight == 6);
    col += 8;
    Display_Char_8x16_Custom(0, col, (day_display % 10) + '0', editing_date && date_highlight == 7);

    // 绘制英文星期缩写 (例如: 2025-11-14    Fri)
    {
        unsigned char dow = GetDayOfWeek(year_display, month_display, day_display);
        unsigned char wk_col = 100; /* 安全位置，避免与时间区域重叠 */
        /* 直接逐列绘制三个字符，避免通过通用接口出现混乱 */
        unsigned char ch_idx, i, idx_ext;
        for (ch_idx = 0; ch_idx < 3; ch_idx++) {
            unsigned char c = g_week_abbrev[dow][ch_idx];
            /* 直接用 A-Z 索引 */
            idx_ext = c - 'A';
            if (idx_ext < 26) {
                for (i = 0; i < 8; i++) {
                    LCD_DrawByte(0, wk_col + ch_idx * 8 + i, Font_8x16_ext[idx_ext][i]);
                    LCD_DrawByte(1, wk_col + ch_idx * 8 + i, Font_8x16_ext[idx_ext][i + 8]);
                }
            } else {
                /* fallback: 空格 */
                for (i = 0; i < 8; i++) {
                    LCD_DrawByte(0, wk_col + ch_idx * 8 + i, Font_8x16[13][i]);
                    LCD_DrawByte(1, wk_col + ch_idx * 8 + i, Font_8x16[13][i + 8]);
                }
            }
        }
    }

    // 时间绘制
    page = 3;
    {
        total_width = 14 + (8 - 1) * 14;
        col = (128 - total_width) / 2;
    }
    Display_Char_16x16_Custom(page, col, (hour_display / 10) + '0', highlight_hour_tens);
    col += 14;
    Display_Char_16x16_Custom(page, col, (hour_display % 10) + '0', highlight_hour_units);
    col += 14;
    Display_Char_16x16(page, col, ':');
    col += 14;
    Display_Char_16x16_Custom(page, col, (minute_display / 10) + '0', highlight_minute_tens);
    col += 14;
    Display_Char_16x16_Custom(page, col, (minute_display % 10) + '0', highlight_minute_units);
    col += 14;
    Display_Char_16x16(page, col, ':');
    col += 14;
    Display_Char_16x16_Custom(page, col, (second_display / 10) + '0', highlight_second_pair);
    col += 14;
    Display_Char_16x16_Custom(page, col, (second_display % 10) + '0', highlight_second_pair);

    // 汉字绘制
    Display_Hanzi16(6, 48, HANZI_MIAO);
    Display_Hanzi16(6, 64, HANZI_BIAO);

    // 菜单光标绘制
    if (g_prev_menu_pos != 0xFF && (g_prev_menu_pos != g_menu_pos || !menu_active))
    {
        Display_DrawMenuCursor(g_prev_menu_pos, 0);
        g_prev_menu_pos = 0xFF; /* 标记为已清除 */
    }
    if (menu_active)
    {
        Display_DrawMenuCursor(0, g_menu_cursor_visible && (g_menu_pos == 0));
        Display_DrawMenuCursor(1, g_menu_cursor_visible && (g_menu_pos == 1));
        Display_DrawMenuCursor(2, g_menu_cursor_visible && (g_menu_pos == 2));
        g_prev_menu_pos = g_menu_pos; /* 记录绘制的位置 */
    }
}

void Display_ResetStopwatch(void)
{
    g_stopwatch_initialized = 0;
    Display_Internal_ResetStopwatchVars(); // 复用: 重置所有g_prev_*变量
}

void Display_StopwatchPage(void)
{
    // 优化: 合并变量声明，减少行数
    unsigned char idata hour, min, sec, ds, col, i, page, lap_index, lap_no;
    bit first_draw = (!g_stopwatch_initialized) || (g_last_screen != DISPLAY_SCREEN_STOPWATCH);

    if (first_draw)
    {
        LCD_ClearArea(0, 7, 0, 127);
        Display_Internal_ResetStopwatchVars(); /* 复用: 重置所有g_prev_*变量 */
        g_stopwatch_initialized = 1;
    }
    g_last_screen = DISPLAY_SCREEN_STOPWATCH;

    Stopwatch_GetTime(&hour, &min, &sec, &ds);

    // 主秒表绘制
    col = 16;
    if (first_draw || g_prev_stop_hour != hour)
    {
        Display_Char_8x16(0, col, hour / 10 + '0');
        Display_Char_8x16(0, col + 8, hour % 10 + '0');
        g_prev_stop_hour = hour;
    }
    Display_Char_8x16(0, col + 16, ':');
    if (first_draw || g_prev_stop_minute != min)
    {
        Display_Char_8x16(0, col + 24, min / 10 + '0');
        Display_Char_8x16(0, col + 32, min % 10 + '0');
        g_prev_stop_minute = min;
    }
    Display_Char_8x16(0, col + 40, ':');
    if (first_draw || g_prev_stop_second != sec)
    {
        Display_Char_8x16(0, col + 48, sec / 10 + '0');
        Display_Char_8x16(0, col + 56, sec % 10 + '0');
        g_prev_stop_second = sec;
    }

    Display_DrawDecimalPoint(0, col + 64); // 复用: 绘制小数点

    if (first_draw || g_prev_stop_decisecond != ds)
    {
        Display_Char_8x16(0, col + 72, ds + '0');
        g_prev_stop_decisecond = ds;
    }

    // 计次列表绘制
    for (i = 0; i < 3; i++)
    {
        page = 2 + i * 2;
        if (i < g_lap_count)
        {
            lap_index = g_lap_count - 1 - i;
            lap_no = lap_index + 1;

            /* 复用: NeedLapUpdate 辅助函数 */
            if (first_draw || NeedLapUpdate(i, lap_index))
            {
                unsigned char lap_col = 0;
                unsigned char base_col = 16 + 16;
                LCD_ClearArea(page, page + 1, 0, 127);

                Display_Char_8x16(page, lap_col, 'L');
                lap_col += 8;
                Display_Char_8x16(page, lap_col, (lap_no >= 10) ? (lap_no / 10 + '0') : ' ');
                lap_col += 8;
                Display_Char_8x16(page, lap_col, lap_no % 10 + '0');

                Display_DrawLapTime(page, base_col, lap_index); // 复用: 绘制计次时间
                Display_SetPrevLap(i, lap_index);               // 复用: 更新缓存
            }
        }
        else
        {
            if (first_draw || g_prev_lap_index[i] != 0xFF)
            {
                LCD_ClearArea(page, page + 1, 0, 127);
                Display_ResetPrevLap(i); /* 复用: 重置缓存 */
            }
        }
    }
}

void Display_LapViewPage(void)
{
    unsigned char idata i, j, start_index, page, lap_no, base_col;

    if (g_last_screen != DISPLAY_SCREEN_LAP)
    {
        LCD_Clear();
    }
    g_last_screen = DISPLAY_SCREEN_LAP;

    start_index = g_lap_view_page * 4;

    for (i = 0; i < 4; i++)
    {
        page = i * 2;
        if ((start_index + i) < g_lap_count)
        {
            j = start_index + i;
            lap_no = j + 1;
            LCD_ClearArea(page, page + 1, 0, 127);

            Display_Char_8x16(page, 0, (lap_no >= 10) ? (lap_no / 10 + '0') : ' ');
            Display_Char_8x16(page, 8, (lap_no % 10) + '0');
            base_col = 16 + 16;

            Display_DrawLapTime(page, base_col, j); // 复用: 绘制计次时间
        }
        else
        {
            LCD_ClearArea(page, page + 1, 0, 127);
        }
    }
}