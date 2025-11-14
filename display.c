#include "display.h"
#include "clock.h"
#include "stopwatch.h"
#include "state.h"
#include <reg51.h>
extern bit g_menu_cursor_visible;

typedef struct {
    unsigned char top[16];
    unsigned char bottom[16];
} Hanzi16;

static const Hanzi16 code g_hanzi_table[] = {
    {   // "秒"
        {0x24,0x24,0xA4,0xFE,0x23,0x22,0x00,0xC0,0x38,0x00,0xFF,0x00,0x08,0x10,0x60,0x00},
        {0x08,0x06,0x01,0xFF,0x01,0x06,0x81,0x80,0x40,0x40,0x27,0x10,0x0C,0x03,0x00,0x00}
    },
    {   // "表"
        {0x00,0x04,0x24,0x24,0x24,0x24,0x24,0xFF,0x24,0x24,0x24,0x24,0x24,0x04,0x00,0x00},
        {0x21,0x21,0x11,0x09,0xFD,0x83,0x41,0x23,0x05,0x09,0x11,0x29,0x25,0x41,0x41,0x00}
    }
};

enum {
    HANZI_MIAO = 0,
    HANZI_BIAO = 1
};

#define DISPLAY_SCREEN_NONE      0xFF
#define DISPLAY_SCREEN_HOME      0
#define DISPLAY_SCREEN_STOPWATCH 2
#define DISPLAY_SCREEN_LAP       2

static unsigned char idata g_last_screen = DISPLAY_SCREEN_NONE;
/* removed accidental global `col`/`band_offset` to avoid conflicts with local variables */
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

static void Display_Hanzi16(unsigned char page, unsigned char col, unsigned char index) {
    unsigned char i;
    for(i = 0; i < 16; i++) {
        LCD_DrawByte(page, col + i, g_hanzi_table[index].top[i]);
        LCD_DrawByte(page + 1, col + i, g_hanzi_table[index].bottom[i]);
    }
}

static void Display_DrawMenuCursor(unsigned char line, bit visible)
{
    unsigned char base_page;
    unsigned char col;
    unsigned char total_width;
    unsigned char time_col;
    unsigned char i;

    // 与原逻辑一致：行号 0 -> page0，1 -> page3，2 -> page6
    if (line == 0)
    {
        base_page = 0;
    }
    else if (line == 1)
    {
        base_page = 3;
    }
    else
    {
        base_page = 6;
    }

    // 计算与主页时间居中一致的起始列，并在其左侧留 10 像素放光标（位置不变）
    {
        total_width = 16 + (8 - 1) * 12; // 100
        time_col = (128 - total_width) / 2;
        col = (time_col > 10) ? (time_col - 10) : 0;
    }

    if (visible)
    {
        // ▶ 实心右三角
        static const unsigned char code arrow_bottom[8] = {
            0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
        static const unsigned char code arrow_top[8] = {
            0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};
        for (i = 0; i < 8; i++)
        {
            LCD_DrawByte(base_page, col + i, arrow_top[i]);
            LCD_DrawByte(base_page + 1, col + i, arrow_bottom[i]);
        }
    }
    else
    {
        // 清除该位置光标（宽 8 列）
        LCD_ClearArea(base_page, base_page + 1, col, col + 7);
    }
}

static void Display_Char_8x16_Custom(unsigned char page, unsigned char col, char c, bit underline)
{
    unsigned char i;
    unsigned char index = GetCharIndex(c);
    unsigned char data_byte;

    for(i = 0; i < 8; i++) {
        data_byte = Font_8x16[index][i];
        LCD_DrawByte(page, col + i, data_byte);

        data_byte = Font_8x16[index][i + 8];
        if(underline) {
            data_byte |= 0x80;
        }
        LCD_DrawByte(page + 1, col + i, data_byte);
    }
}

static void Display_Char_16x16_Custom(unsigned char page, unsigned char col, char c, bit underline) {
    unsigned char i;
    unsigned char index = GetCharIndex(c);
    unsigned char data_byte;

    for(i = 0; i < 16; i++) {
        data_byte = Font_16x16[index][i];
        LCD_DrawByte(page, col + i, data_byte);

        data_byte = Font_16x16[index][i + 16];
        if(underline) {
            data_byte |= 0x80;
        }
        LCD_DrawByte(page + 1, col + i, data_byte);
    }
}

void Display_Char_8x16(unsigned char page, unsigned char col, char c) {
    /* 复用 Custom 版本以减少重复代码 */
    Display_Char_8x16_Custom(page, col, c, 0);
}

void Display_Char_16x16(unsigned char page, unsigned char col, char c) {
    /* 复用 Custom 版本以减少重复代码 */
    Display_Char_16x16_Custom(page, col, c, 0);
}

void Display_Char_24x32(unsigned char page, unsigned char col, char c) {
    unsigned char i;
    unsigned char index = GetCharIndex(c);

    for(i = 0; i < 24; i++) {
        LCD_DrawByte(page, col + i, Font_24x32[index][i]);
    }
    for(i = 0; i < 24; i++) {
        LCD_DrawByte(page + 1, col + i, Font_24x32[index][i + 24]);
    }
    for(i = 0; i < 24; i++) {
        LCD_DrawByte(page + 2, col + i, Font_24x32[index][i + 48]);
    }
    for(i = 0; i < 24; i++) {
        LCD_DrawByte(page + 3, col + i, Font_24x32[index][i + 72]);
    }
}

/* 已移除：Display_String_* 与 Display_Number_* 的实现
   使用更小粒度的 Display_Char_* 接口在调用处拼装字符串/数字，
   以减少未使用代码体积。 */

void Display_HomePage(void) {
    unsigned int year_display;
    unsigned char month_display;
    unsigned char day_display;
    unsigned char total_width;
    unsigned char time_col;
    bit editing_date;
    bit editing_time;
    unsigned char date_highlight;
    unsigned char hour_display;
    unsigned char minute_display;
    unsigned char second_display;
    bit highlight_hour_tens;
    bit highlight_hour_units;
    bit highlight_minute_tens;
    bit highlight_minute_units;
    bit highlight_second_pair;
    bit menu_active;
    unsigned char page;
    unsigned char col;
    bit first_draw = (g_last_screen != DISPLAY_SCREEN_HOME);

    g_last_screen = DISPLAY_SCREEN_HOME;

    if(first_draw) {
        LCD_ClearArea(0, 7, 0, 127);
    }

    /* 计算当前是否处于编辑日期/时间/菜单的状态（由主程序的系统状态控制） */
    editing_date = (g_system_state == STATE_DATE_SET);
    editing_time = (g_system_state == STATE_TIME_SET);
    menu_active = (g_system_state == STATE_MENU);

    if(editing_date) {
        year_display = g_temp_year;
        month_display = g_temp_month;
        day_display = g_temp_day;
        date_highlight = 7 - g_edit_pos;
    } else {
        year_display = g_datetime.year;
        month_display = g_datetime.month;
        day_display = g_datetime.day;
    }

    if(editing_time) {
        hour_display = g_temp_hour;
        minute_display = g_temp_minute;
        second_display = g_temp_second;
    } else {
        hour_display = g_datetime.hour;
        minute_display = g_datetime.minute;
        second_display = g_datetime.second;
    }

    highlight_hour_tens = editing_time && (g_edit_pos == 5);
    highlight_hour_units = editing_time && (g_edit_pos == 4);
    highlight_minute_tens = editing_time && (g_edit_pos == 3);
    highlight_minute_units = editing_time && (g_edit_pos == 2);
    highlight_second_pair = editing_time && (g_edit_pos == 0 || g_edit_pos == 1);

    /* 菜单光标将稍后绘制，放在时间/标签之后以避免被覆盖 */

    col = 24;
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

    /* 时间显示跨第2行和第3行中间显示（起始于 page 3，绘制 page3/page4） */
    page = 3; /* 起始页 (page3/page4) */
    /* 计算居中起始列：每字符步进为 12，字符宽度取 16，总字符数 8 (HH:MM:SS) */
    {
        total_width = 16 + (8 - 1) * 12; /* 16 + 7*12 = 100 */
        col = (128 - total_width) / 2; /* 居中起始列 */
    }

    /* 将主页时间间距进一步缩小为步进 12（更紧凑但避免重叠） */
        Display_Char_16x16_Custom(page, col, (hour_display / 10) + '0', highlight_hour_tens);
    col += 12;
        Display_Char_16x16_Custom(page, col, (hour_display % 10) + '0', highlight_hour_units);
    col += 12;
    Display_Char_16x16(page, col, ':');
    col += 12;
        Display_Char_16x16_Custom(page, col, (minute_display / 10) + '0', highlight_minute_tens);
    col += 12;
        Display_Char_16x16_Custom(page, col, (minute_display % 10) + '0', highlight_minute_units);
    col += 12;
    Display_Char_16x16(page, col, ':');
    col += 12;
        Display_Char_16x16_Custom(page, col, (second_display / 10) + '0', highlight_second_pair);
    col += 12;
        Display_Char_16x16_Custom(page, col, (second_display % 10) + '0', highlight_second_pair);

    /* 在第四行显示“秒表”二字 */
    Display_Hanzi16(6, 48, HANZI_MIAO);
    Display_Hanzi16(6, 64, HANZI_BIAO);

    /* 管理并绘制菜单光标：先清除先前光标（如果存在且菜单仍激活），再绘制当前光标 */
    if(menu_active) {
        if(g_prev_menu_pos != 0xFF && g_prev_menu_pos != g_menu_pos) {
            Display_DrawMenuCursor(g_prev_menu_pos, 0);
        }
        if(g_menu_cursor_visible) {
            Display_DrawMenuCursor(0, g_menu_pos == 0);
            Display_DrawMenuCursor(1, g_menu_pos == 1);
            Display_DrawMenuCursor(2, g_menu_pos == 2);
        } else {
            Display_DrawMenuCursor(0, 0);
            Display_DrawMenuCursor(1, 0);
            Display_DrawMenuCursor(2, 0);
        }
        g_prev_menu_pos = g_menu_pos;
    } else {
        if(g_prev_menu_pos != 0xFF) {
            Display_DrawMenuCursor(g_prev_menu_pos, 0);
            g_prev_menu_pos = 0xFF;
        }
    }
}

void Display_ResetStopwatch(void) {
    unsigned char idata i;

    g_stopwatch_initialized = 0;
    g_prev_stop_hour = 0xFF;
    g_prev_stop_minute = 0xFF;
    g_prev_stop_second = 0xFF;
    g_prev_stop_decisecond = 0xFF;
    for(i = 0; i < 3; i++) {
        g_prev_lap_index[i] = 0xFF;
        g_prev_lap_hour[i] = 0xFF;
        g_prev_lap_minute[i] = 0xFF;
        g_prev_lap_second[i] = 0xFF;
        g_prev_lap_decisecond[i] = 0xFF;
    }
}

void Display_StopwatchPage(void) {
    unsigned char idata hour;
    unsigned char idata min;
    unsigned char idata sec;
    unsigned char idata ds;
    unsigned char idata col;
    unsigned char idata i;
    unsigned char di;
    unsigned char idata page;
    unsigned char idata lap_index;
    unsigned char idata lap_no;
    bit first_draw = (!g_stopwatch_initialized) || (g_last_screen != DISPLAY_SCREEN_STOPWATCH);

    if(first_draw) {
        LCD_ClearArea(0, 7, 0, 127);
        g_stopwatch_initialized = 1;
        g_prev_stop_hour = 0xFF;
        g_prev_stop_minute = 0xFF;
        g_prev_stop_second = 0xFF;
        g_prev_stop_decisecond = 0xFF;
        for(i = 0; i < 3; i++) {
            g_prev_lap_index[i] = 0xFF;
            g_prev_lap_hour[i] = 0xFF;
            g_prev_lap_minute[i] = 0xFF;
            g_prev_lap_second[i] = 0xFF;
            g_prev_lap_decisecond[i] = 0xFF;
        }
    }

    g_last_screen = DISPLAY_SCREEN_STOPWATCH;

    Stopwatch_GetTime(&hour, &min, &sec, &ds);

    /* 恢复秒表页面为标准紧凑步进 8（8x16 字体，步进 8）以避免重叠 */
    col = 16;
    if(first_draw || g_prev_stop_hour != hour) {
        Display_Char_8x16(0, col, hour / 10 + '0');
        Display_Char_8x16(0, col + 8, hour % 10 + '0');
        g_prev_stop_hour = hour;
    }
    /* 始终绘制小时/分钟分隔符，避免在部分刷新后丢失 */
    Display_Char_8x16(0, col + 16, ':');
    if(first_draw || g_prev_stop_minute != min) {
        Display_Char_8x16(0, col + 24, min / 10 + '0');
        Display_Char_8x16(0, col + 32, min % 10 + '0');
        g_prev_stop_minute = min;
    }
    /* 始终绘制分钟/秒分隔符 */
    Display_Char_8x16(0, col + 40, ':');
    if(first_draw || g_prev_stop_second != sec) {
        Display_Char_8x16(0, col + 48, sec / 10 + '0');
        Display_Char_8x16(0, col + 56, sec % 10 + '0');
        g_prev_stop_second = sec;
    }
    /* 强制使用自定义像素点作为秒与毫秒的分隔符（始终绘制） */
    {
        /* 绘制分隔点，使用共用辅助函数 */
        unsigned char dot_base = col + 64;
        unsigned char di_local;
        for(di_local = 0; di_local < 8; di_local++) {
            LCD_DrawByte(0, dot_base + di_local, 0x00);
            LCD_DrawByte(1, dot_base + di_local, (di_local == 3 || di_local == 4) ? 0x18 : 0x00);
        }
    }
    if(first_draw || g_prev_stop_decisecond != ds) {
        Display_Char_8x16(0, col + 72, ds + '0');
        g_prev_stop_decisecond = ds;
    }

    for(i = 0; i < 3; i++) {
        page = 2 + i * 2;
        if(i < g_lap_count) {
            lap_index = g_lap_count - 1 - i;
            lap_no = lap_index + 1;
            if(first_draw || g_prev_lap_index[i] != lap_index ||
               g_prev_lap_hour[i] != g_laps[lap_index].hour ||
               g_prev_lap_minute[i] != g_laps[lap_index].minute ||
               g_prev_lap_second[i] != g_laps[lap_index].second ||
               g_prev_lap_decisecond[i] != g_laps[lap_index].decisecond) {
                unsigned char lap_col = 0;
                unsigned char base_col = 16 + 16; /* 与主秒表起点 16 对齐并向右平移 16 列用于序号 */
                LCD_ClearArea(page, page + 1, 0, 127);
                /* 保留左侧 L 和序号 */
                Display_Char_8x16(page, lap_col, 'L');
                lap_col += 8;
                Display_Char_8x16(page, lap_col, (lap_no >= 10) ? (lap_no / 10 + '0') : ' ');
                lap_col += 8;
                Display_Char_8x16(page, lap_col, lap_no % 10 + '0');

                /* 使用与主秒表相同的相对布局，但起点为 base_col */
                Display_Char_8x16(page, base_col + 0, g_laps[lap_index].hour / 10 + '0');
                Display_Char_8x16(page, base_col + 8, g_laps[lap_index].hour % 10 + '0');
                Display_Char_8x16(page, base_col + 16, ':');
                Display_Char_8x16(page, base_col + 24, g_laps[lap_index].minute / 10 + '0');
                Display_Char_8x16(page, base_col + 32, g_laps[lap_index].minute % 10 + '0');
                Display_Char_8x16(page, base_col + 40, ':');
                Display_Char_8x16(page, base_col + 48, g_laps[lap_index].second / 10 + '0');
                Display_Char_8x16(page, base_col + 56, g_laps[lap_index].second % 10 + '0');
                /* 绘制与主秒表相同的点和毫秒 */
                {
                    unsigned char dot_base = base_col + 64;
                    unsigned char di2_local;
                    for(di2_local = 0; di2_local < 8; di2_local++) {
                        LCD_DrawByte(page, dot_base + di2_local, 0x00);
                        LCD_DrawByte(page + 1, dot_base + di2_local, (di2_local == 3 || di2_local == 4) ? 0x18 : 0x00);
                    }
                    Display_Char_8x16(page, base_col + 72, g_laps[lap_index].decisecond + '0');
                }

                g_prev_lap_index[i] = lap_index;
                g_prev_lap_hour[i] = g_laps[lap_index].hour;
                g_prev_lap_minute[i] = g_laps[lap_index].minute;
                g_prev_lap_second[i] = g_laps[lap_index].second;
                g_prev_lap_decisecond[i] = g_laps[lap_index].decisecond;
            }
        } else {
            if(first_draw || g_prev_lap_index[i] != 0xFF) {
                LCD_ClearArea(page, page + 1, 0, 127);
                g_prev_lap_index[i] = 0xFF;
                g_prev_lap_hour[i] = 0xFF;
                g_prev_lap_minute[i] = 0xFF;
                g_prev_lap_second[i] = 0xFF;
                g_prev_lap_decisecond[i] = 0xFF;
            }
        }
    }
}

void Display_LapViewPage(void) {
    unsigned char idata i;
    unsigned char idata j;
    unsigned char idata start_index;
    unsigned char idata page;
    unsigned char idata lap_no;
    unsigned char dot_col;
    unsigned char base_col;

    if(g_last_screen != DISPLAY_SCREEN_LAP) {
        LCD_Clear();
    }
    g_last_screen = DISPLAY_SCREEN_LAP;

    start_index = g_lap_view_page * 4;

    for(i = 0; i < 4; i++) {
        page = i * 2;
        if((start_index + i) < g_lap_count) {
            j = start_index + i;
            lap_no = j + 1;
            LCD_ClearArea(page, page + 1, 0, 127);
            /* 左侧显示序号 */
            Display_Char_8x16(page, 0, (lap_no >= 10) ? (lap_no / 10 + '0') : ' ');
            Display_Char_8x16(page, 8, (lap_no % 10) + '0');
            /* time area: same spacing as main stopwatch, shifted right by 16 for sequence */
            base_col = 16 + 16; /* main start(16) + lap number width(16) */
            Display_Char_8x16(page, base_col + 0, g_laps[j].hour / 10 + '0');
            Display_Char_8x16(page, base_col + 8, g_laps[j].hour % 10 + '0');
            Display_Char_8x16(page, base_col + 16, ':');
            Display_Char_8x16(page, base_col + 24, g_laps[j].minute / 10 + '0');
            Display_Char_8x16(page, base_col + 32, g_laps[j].minute % 10 + '0');
            Display_Char_8x16(page, base_col + 40, ':');
            Display_Char_8x16(page, base_col + 48, g_laps[j].second / 10 + '0');
            Display_Char_8x16(page, base_col + 56, g_laps[j].second % 10 + '0');
            /* 绘制点区和毫秒，与主界面一致 */
            {
                unsigned char dot_base = base_col + 64;
                unsigned char di3_local;
                for(di3_local = 0; di3_local < 8; di3_local++) {
                    LCD_DrawByte(page, dot_base + di3_local, 0x00);
                    LCD_DrawByte(page + 1, dot_base + di3_local, (di3_local == 3 || di3_local == 4) ? 0x18 : 0x00);
                }
                Display_Char_8x16(page, base_col + 72, g_laps[j].decisecond + '0');
            }
        } else {
            LCD_ClearArea(page, page + 1, 0, 127);
        }
    }
}
