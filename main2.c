/* main2.c: DEPRECATED duplicate of main.c - disabled to avoid duplicate symbols
 * The file is kept for reference only. All active logic lives in main.c.
 */
#if 0
/* * 优化: 添加了 pragma 指令。
 * 强制 Keil 编译器优先优化代码大小(SIZE)，而不是速度。
 */
#pragma OT(8, SIZE)

#include <reg51.h>
#include "lcd_ks0108.h"
#include "font.h"
#include "clock.h"
#include "key.h"
#include "stopwatch.h"
#include "display.h"
#include "buzzer.h"
#include "state.h"

// 全局变量 (未更改)
unsigned char data g_system_state = STATE_HOME;
unsigned char data g_menu_pos = 0; // 菜单位置: 0=日期, 1=时间, 2=秒表
unsigned char data g_edit_pos = 0; // 编辑位置: 用于逐位编辑
unsigned char data g_refresh_counter = 0;

// 光标超时相关变量 (未更改)
unsigned int data g_menu_cursor_timeout = 0;
bit g_menu_cursor_visible = 1;

// 临时设定变量 (未更改)
unsigned int data g_temp_year;
unsigned char data g_temp_month;
unsigned char data g_temp_day;
unsigned char data g_temp_hour;
unsigned char data g_temp_minute;
unsigned char data g_temp_second;

// 内部辅助函数 (未更改)
static void TempTime_Tick(void) {
    g_temp_second++;
    if (g_temp_second >= 60) {
        g_temp_second = 0;
        g_temp_minute++;
        if (g_temp_minute >= 60) {
            g_temp_minute = 0;
            g_temp_hour++;
            if (g_temp_hour >= 24)
            {
                g_temp_hour = 0;
            }
        }
    }
}

// 定时器0中断服务程序 (未更改)
void Timer0_ISR(void) interrupt 1
{
    TH0 = 0xB8; // 重新装载初值（22.1184MHz）
    TL0 = 0x00;

    Clock_Update();
    Stopwatch_Update();
    Key_Scan();
    Buzzer_Update();
    g_refresh_counter++;
}

// 处理主页按键 (未更改)
void Handle_HomePage_Keys(unsigned char key)
{
    if (key == KEY_VAL_4)
    {
        g_system_state = STATE_MENU;
        g_menu_pos = 0;
        Display_HomePage();
    }
}

/**
 * @brief 处理菜单选择按键
 * @note 优化: 将 if/else if 结构改为 switch 结构，提高可读性
 */
void Handle_Menu_Keys(unsigned char key)
{
    if (key == KEY_VAL_4)
    {
        // 切换菜单项
        g_menu_pos++;
        if (g_menu_pos > 2)
            g_menu_pos = 0;
        Display_HomePage();
    }
    else if (key == KEY_VAL_1)
    {
        // 确认进入选中的功能
        switch (g_menu_pos)
        {
        case 0: // 进入日期设定
            g_system_state = STATE_DATE_SET;
            g_temp_year = g_datetime.year;
            g_temp_month = g_datetime.month;
            g_temp_day = g_datetime.day;
            g_edit_pos = 0;
            Display_HomePage();
            break;
        case 1: // 进入时间设定
            g_system_state = STATE_TIME_SET;
            g_temp_hour = g_datetime.hour;
            g_temp_minute = g_datetime.minute;
            g_temp_second = g_datetime.second;
            g_edit_pos = 0;
            Display_HomePage();
            break;
        case 2: // 进入秒表
            g_system_state = STATE_STOPWATCH;
            Display_ResetStopwatch();
            Display_StopwatchPage();
            break;
        }
    }
}

/**
 * @brief (新增辅助函数) 获取年份编辑的位置权重
 * @note 优化: 提取了 Handle_DateSet_Keys 中 KEY_VAL_2 和 KEY_VAL_3 的重复 for 循环
 */
static unsigned int Get_Year_Digit_Weight(void)
{
    unsigned int year_digit = 1;
    unsigned char i;
    // g_edit_pos 4=个位(1), 5=十位(10), 6=百位(100), 7=千位(1000)
    for (i = 0; i < (g_edit_pos - 4); i++)
    {
        year_digit *= 10;
    }
    return year_digit;
}

/**
 * @brief 处理日期设定按键
 * @note 优化: 复用了 Get_Year_Digit_Weight() 辅助函数
 */
void Handle_DateSet_Keys(unsigned char key)
{
    unsigned char day_tens, day_ones, month_tens, month_ones;
    unsigned int year_digit; // 仅用于 KEY_VAL_3 的临时存储

    if (key == (KEY_VAL_1 | 0x10))
    {
        DateTime_SetDate(g_temp_year, g_temp_month, g_temp_day);
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        Display_HomePage();
        return;
    }

    if (key == KEY_VAL_1)
    {
        // KEY1: 切换到下一位编辑
        if (g_edit_pos >= 7)
        {
            DateTime_SetDate(g_temp_year, g_temp_month, g_temp_day);
            g_system_state = STATE_HOME;
            g_edit_pos = 0;
        }
        else
        {
            g_edit_pos++;
        }
        Display_HomePage();
    }
    else if (key == KEY_VAL_2)
    {
        // KEY2: 当前位+1
        if (g_edit_pos == 0)
        {
            day_tens = g_temp_day / 10;
            day_ones = g_temp_day % 10;
            day_ones++;
            if (day_ones > 9)
                day_ones = 0;
            g_temp_day = day_tens * 10 + day_ones;
            if (g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = 1;
        }
        else if (g_edit_pos == 1)
        {
            day_ones = g_temp_day % 10;
            day_tens = g_temp_day / 10;
            day_tens++;
            if (day_tens > 3)
                day_tens = 0;
            g_temp_day = day_tens * 10 + day_ones;
            if (g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = 1;
        }
        else if (g_edit_pos == 2)
        {
            month_tens = g_temp_month / 10;
            month_ones = g_temp_month % 10;
            month_ones++;
            if (month_ones > 9)
                month_ones = 0;
            g_temp_month = month_tens * 10 + month_ones;
            if (g_temp_month > 12 || g_temp_month == 0)
                g_temp_month = 1;
        }
        else if (g_edit_pos == 3)
        {
            month_ones = g_temp_month % 10;
            month_tens = g_temp_month / 10;
            month_tens++;
            if (month_tens > 1)
                month_tens = 0;
            g_temp_month = month_tens * 10 + month_ones;
            if (g_temp_month > 12 || g_temp_month == 0)
                g_temp_month = 1;
        }
        else if (g_edit_pos >= 4 && g_edit_pos <= 7)
        {
            /* 优化: 调用辅助函数，移除了重复的 for 循环 */
            g_temp_year = g_temp_year + Get_Year_Digit_Weight();
            if (g_temp_year > 2099)
                g_temp_year = 2000;
        }

        // 统一检查日期合法性（仅在月或年更改时）
        if (g_edit_pos >= 2)
        {
            if (g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        }
        Display_HomePage();
    }
    else if (key == KEY_VAL_3)
    {
        // KEY3: 当前位-1
        if (g_edit_pos == 0)
        {
            day_tens = g_temp_day / 10;
            day_ones = g_temp_day % 10;
            if (day_ones == 0)
                day_ones = 9;
            else
                day_ones--;
            g_temp_day = day_tens * 10 + day_ones;
            if (g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        }
        else if (g_edit_pos == 1)
        {
            day_ones = g_temp_day % 10;
            day_tens = g_temp_day / 10;
            if (day_tens == 0)
                day_tens = 3;
            else
                day_tens--;
            g_temp_day = day_tens * 10 + day_ones;
            if (g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        }
        else if (g_edit_pos == 2)
        {
            month_tens = g_temp_month / 10;
            month_ones = g_temp_month % 10;
            if (month_ones == 0)
                month_ones = 9;
            else
                month_ones--;
            g_temp_month = month_tens * 10 + month_ones;
            if (g_temp_month > 12 || g_temp_month == 0)
                g_temp_month = 12;
        }
        else if (g_edit_pos == 3)
        {
            month_ones = g_temp_month % 10;
            month_tens = g_temp_month / 10;
            if (month_tens == 0)
                month_tens = 1;
            else
                month_tens--;
            g_temp_month = month_tens * 10 + month_ones;
            if (g_temp_month > 12 || g_temp_month == 0)
                g_temp_month = 12;
        }
        else if (g_edit_pos >= 4 && g_edit_pos <= 7)
        {
            /* 优化: 调用辅助函数，移除了重复的 for 循环 */
            year_digit = Get_Year_Digit_Weight();
            if (g_temp_year < 2000 + year_digit)
                g_temp_year = 2099;
            else
                g_temp_year = g_temp_year - year_digit;
        }

        // 统一检查日期合法性（仅在月或年更改时）
        if (g_edit_pos >= 2)
        {
            if (g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        }
        Display_HomePage();
    }
    else if (key == KEY_VAL_4)
    {
        // KEY4: 取消并返回主页
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        // （注意：原代码在此处未重置 g_temp_year 等变量，保持该行为）
        Display_HomePage();
    }
}

/**
 * @brief 处理时间设定按键
 * @note 优化: 从 main() 函数中移入了 KEY_VAL_1 的特殊处理逻辑，
 * 使得 main() 的 switch 语句更干净。
 */
void Handle_TimeSet_Keys(unsigned char key)
{
    unsigned char tens, ones;

    if (key == (KEY_VAL_1 | 0x10))
    {
        DateTime_SetTime(g_temp_hour, g_temp_minute, g_temp_second);
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        Display_HomePage();
        return;
    }

    if (key == KEY_VAL_1)
    {
        /* * 优化: 这是从 main() 的 switch 中移入的特殊逻辑。
         * 作用：当编辑 "秒" (pos 0或1) 时，按 "下一位" (KEY_VAL_1)
         * 会直接跳到 "分钟" (pos 2)。
         */
        if (g_edit_pos == 0 || g_edit_pos == 1)
        {
            g_edit_pos = 2; // 跳过秒钟，直接到分钟
        }
        else if (g_edit_pos >= 5)
        {
            // 在最后一位（时钟十位）按 "下一位"，则保存并退出
            DateTime_SetTime(g_temp_hour, g_temp_minute, g_temp_second);
            g_system_state = STATE_HOME;
            g_edit_pos = 0;
        }
        else
        {
            // 正常情况：编辑位置+1
            g_edit_pos++;
        }
        Display_HomePage();
    }
    else if (key == KEY_VAL_2)
    {
        // KEY2: 当前位+1 (逻辑未更改)
        if (g_edit_pos <= 1)
        {
            g_temp_second = 0;
            g_temp_minute++;
            if (g_temp_minute >= 60)
            {
                g_temp_minute = 0;
                g_temp_hour++;
                if (g_temp_hour >= 24)
                {
                    g_temp_hour = 0;
                }
            }
        }
        else if (g_edit_pos == 2)
        {
            tens = g_temp_minute / 10;
            ones = g_temp_minute % 10;
            ones++;
            if (ones > 9)
                ones = 0;
            g_temp_minute = tens * 10 + ones;
            if (g_temp_minute >= 60)
                g_temp_minute = 0;
        }
        else if (g_edit_pos == 3)
        {
            ones = g_temp_minute % 10;
            tens = g_temp_minute / 10;
            tens++;
            if (tens > 5)
                tens = 0;
            g_temp_minute = tens * 10 + ones;
        }
        else if (g_edit_pos == 4)
        {
            tens = g_temp_hour / 10;
            ones = g_temp_hour % 10;
            ones++;
            if (ones > 9)
                ones = 0;
            g_temp_hour = tens * 10 + ones;
            if (g_temp_hour >= 24)
                g_temp_hour = 0;
        }
        else if (g_edit_pos == 5)
        {
            ones = g_temp_hour % 10;
            tens = g_temp_hour / 10;
            tens++;
            if (tens > 2)
                tens = 0;
            g_temp_hour = tens * 10 + ones;
            if (g_temp_hour >= 24)
                g_temp_hour = 0;
        }
        Display_HomePage();
    }
    else if (key == KEY_VAL_3)
    {
        // KEY3: 当前位-1 (逻辑未更改)
        if (g_edit_pos <= 1)
        {
            g_temp_second = 0;
        }
        else if (g_edit_pos == 2)
        {
            tens = g_temp_minute / 10;
            ones = g_temp_minute % 10;
            if (ones == 0)
                ones = 9;
            else
                ones--;
            g_temp_minute = tens * 10 + ones;
        }
        else if (g_edit_pos == 3)
        {
            ones = g_temp_minute % 10;
            tens = g_temp_minute / 10;
            if (tens == 0)
                tens = 5;
            else
                tens--;
            g_temp_minute = tens * 10 + ones;
        }
        else if (g_edit_pos == 4)
        {
            tens = g_temp_hour / 10;
            ones = g_temp_hour % 10;
            if (ones == 0)
                ones = 9;
            else
                ones--;
            g_temp_hour = tens * 10 + ones;
            if (g_temp_hour >= 24)
                g_temp_hour = 23;
        }
        else if (g_edit_pos == 5)
        {
            ones = g_temp_hour % 10;
            tens = g_temp_hour / 10;
            if (tens == 0)
                tens = 2;
            else
                tens--;
            g_temp_hour = tens * 10 + ones;
            if (g_temp_hour >= 24)
                g_temp_hour = 23;
        }
        Display_HomePage();
    }
    else if (key == KEY_VAL_4)
    {
        // KEY4: 取消并返回主页
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        // （注意：原代码在此处未重置 g_temp_hour 等变量，保持该行为）
        Display_HomePage();
    }
}

// 处理秒表按键 (未更改)
void Handle_Stopwatch_Keys(unsigned char key)
{
    if (key == (KEY_VAL_1 | 0x10))
    {
        Stopwatch_Clear();
        Display_ResetStopwatch();
        Display_StopwatchPage();
    }
    else if (key == KEY_VAL_1)
    {
        if (g_stopwatch.state == SW_RUNNING)
        {
            Stopwatch_Pause();
        }
        else
        {
            Stopwatch_Start();
        }
        Display_StopwatchPage();
    }
    else if (key == KEY_VAL_2)
    {
        Stopwatch_AddLap();
        Display_StopwatchPage();
    }
    else if (key == KEY_VAL_3)
    {
        g_system_state = STATE_LAP_VIEW;
        g_lap_view_page = 0;
        Display_LapViewPage();
    }
    else if (key == KEY_VAL_4)
    {
        g_system_state = STATE_HOME;
        Display_ResetStopwatch();
        Display_HomePage();
    }
}

// 处理计次查看按键 (未更改)
void Handle_LapView_Keys(unsigned char key)
{
    if (key == KEY_VAL_2)
    {
        unsigned char total_pages = (g_lap_count + 3) / 4;
        g_lap_view_page++;
        if (total_pages == 0)
        {
            total_pages = 1;
        }
        if (g_lap_view_page >= total_pages)
        {
            g_lap_view_page = 0;
        }
        Display_LapViewPage();
    }
    else if (key == KEY_VAL_3)
    {
        g_system_state = STATE_STOPWATCH;
        Display_ResetStopwatch();
        Display_StopwatchPage();
    }
    else if (key == KEY_VAL_4)
    {
        g_system_state = STATE_HOME;
        Display_ResetStopwatch();
        Display_HomePage();
    }
}

// 主函数
void main(void)
{
    unsigned char key;
    unsigned int i;

    // 初始化LCD
    LCD_Init();

    // 等待LCD稳定
    for (i = 0; i < 10000; i++)
        ;

    // 初始化各模块
    Key_Init();
    Stopwatch_Init();
    Buzzer_Init();
    BUZZER = BUZZER_OFF;

    Display_HomePage();

    // 最后启动时钟（定时器中断）
    Clock_Init();

    // 主循环
    while (1)
    {
        key = Key_GetPressed();

        // 菜单光标超时处理 (逻辑未更改)
        if (g_system_state == STATE_MENU)
        {
            if (key)
            {
                g_menu_cursor_timeout = 0;
                g_menu_cursor_visible = 1;
            }
            else
            {
                if (g_menu_cursor_visible)
                {
                    g_menu_cursor_timeout++;
                    if (g_menu_cursor_timeout >= 50000)
                    {
                        g_menu_cursor_visible = 0;
                        g_menu_pos = 0;
                        g_system_state = STATE_HOME;
                        Display_HomePage();
                    }
                }
            }
        }
        else
        {
            g_menu_cursor_visible = 1;
            g_menu_cursor_timeout = 0;
        }

        /* * 优化: 状态机 switch 语句
         * 将所有按键处理逻辑分发到各自的 Handle_* 函数中。
         * 移除了原有的针对 STATE_TIME_SET 的特殊处理，
         * 因为该逻辑已被移入 Handle_TimeSet_Keys() 函数中。
         * 这使得 main() 循环更干净，行数更少。
         */
        switch (g_system_state)
        {
        case STATE_TIME_SET:
            Handle_TimeSet_Keys(key);
            break;
        case STATE_STOPWATCH:
            Handle_Stopwatch_Keys(key);
            break;
        case STATE_LAP_VIEW:
            Handle_LapView_Keys(key);
            break;
        case STATE_DATE_SET:
            Handle_DateSet_Keys(key);
            break;
        case STATE_MENU:
            Handle_Menu_Keys(key);
            break;
        case STATE_HOME:
        default:
            Handle_HomePage_Keys(key);
            break;
        }

        /* * 优化: 定期刷新显示逻辑
         * 合并了重复的 Buzzer_Check() 和 Display_HomePage() 调用。
         * 功能完全不变，但代码行数减少，逻辑更清晰。
         */
        if (g_time_changed)
        {
            g_time_changed = 0;

            // 蜂鸣器检查在每次时间变化时都应执行
            Buzzer_Check();

            if (g_system_state == STATE_TIME_SET)
            {
                // 仅在时间设定界面，临时时间才需要“跳动”
                TempTime_Tick();
            }

            // 在所有显示主页的界面，统一刷新
            if (g_system_state == STATE_HOME ||
                g_system_state == STATE_MENU ||
                g_system_state == STATE_DATE_SET ||
                g_system_state == STATE_TIME_SET)
            {
                Display_HomePage();
            }
        }

        // 秒表页面的高速刷新 (逻辑未更改)
        if (g_system_state == STATE_STOPWATCH)
        {
            if (g_refresh_counter >= 2)
            { // 20ms刷新一次
                g_refresh_counter = 0;
                Display_StopwatchPage();
            }
        }
    }
}

#endif