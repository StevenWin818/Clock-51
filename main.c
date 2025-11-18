#include <reg51.h>
#include "lcd_ks0108.h"
#include "font.h"
#include "clock.h"
#include "key.h"
#include "stopwatch.h"
#include "display.h"
#include "buzzer.h"
#include "state.h"

// 全局变量
unsigned char data g_system_state = STATE_HOME;
unsigned char data g_menu_pos = 0;     // 菜单位置: 0=日期, 1=时间, 2=秒表
unsigned char data g_edit_pos = 0;     // 编辑位置: 用于逐位编辑
unsigned char data g_refresh_counter = 0;

// 光标超时相关变量
unsigned int data g_menu_cursor_timeout = 0; // 菜单光标超时计数（单位：10ms）
bit g_menu_cursor_visible = 1;               // 菜单光标是否显示

// 临时设定变量
unsigned int data g_temp_year;
unsigned char data g_temp_month;
unsigned char data g_temp_day;
unsigned char data g_temp_hour;
unsigned char data g_temp_minute;
unsigned char data g_temp_second;

static void TempTime_Tick(void) {
    g_temp_second++;
    if(g_temp_second >= 60) {
        g_temp_second = 0;
        g_temp_minute++;
        if(g_temp_minute >= 60) {
            g_temp_minute = 0;
            g_temp_hour++;
            if(g_temp_hour >= 24) {
                g_temp_hour = 0;
            }
        }
    }
}

// 定时器0中断服务程序 (合并所有10ms任务)
void Timer0_ISR(void) interrupt 1 {
    TH0 = 0xB8;    // 重新装载初值（22.1184MHz）
    TL0 = 0x00;
    
    // 时钟更新
    Clock_Update();
    
    // 秒表更新
    Stopwatch_Update();
    
    // 按键扫描
    Key_Scan();
    
    // 蜂鸣器更新
    Buzzer_Update();
    
    // 刷新计数
    g_refresh_counter++;
}

// 处理主页按键
void Handle_HomePage_Keys(unsigned char key) {
    if(key == KEY_VAL_4) {
        // 进入菜单选择模式
        g_system_state = STATE_MENU;
        g_menu_pos = 0;
        Display_HomePage();
    }
}

// 处理菜单选择按键
void Handle_Menu_Keys(unsigned char key) {
    if(key == KEY_VAL_4) {
        // 切换菜单项
        g_menu_pos++;
        if(g_menu_pos > 2) g_menu_pos = 0;
        Display_HomePage();
    } else if(key == KEY_VAL_1) {
        // 确认进入选中的功能
        if(g_menu_pos == 0) {
            // 进入日期设定
            g_system_state = STATE_DATE_SET;
            g_temp_year = g_datetime.year;
            g_temp_month = g_datetime.month;
            g_temp_day = g_datetime.day;
            g_edit_pos = 0;  // 从第一位开始编辑
            Display_HomePage();
        } else if(g_menu_pos == 1) {
            // 进入时间设定
            g_system_state = STATE_TIME_SET;
            g_temp_hour = g_datetime.hour;
            g_temp_minute = g_datetime.minute;
            g_temp_second = g_datetime.second;
            g_edit_pos = 0;  // 从第一位开始编辑
            Display_HomePage();
        } else if(g_menu_pos == 2) {
            // 进入秒表
            g_system_state = STATE_STOPWATCH;
            Display_ResetStopwatch();
            Display_StopwatchPage();
        }
    }
}

// 处理日期设定按键 - 逐位编辑模式
// edit_pos: 0-7 对应 YYYY-MM-DD 的8位数字（从右到左）
//           7654 32 10
void Handle_DateSet_Keys(unsigned char key) {
    unsigned char day_tens, day_ones, month_tens, month_ones;
    unsigned int year_digit;

    if(key == (KEY_VAL_1 | 0x10)) {
        DateTime_SetDate(g_temp_year, g_temp_month, g_temp_day);
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        Display_HomePage();
        return;
    }
    if(key == KEY_VAL_1) {
        // KEY1: 切换到下一位编辑
        if(g_edit_pos >= 7) {
            // 最后一位确认后保存并返回主页
            DateTime_SetDate(g_temp_year, g_temp_month, g_temp_day);
            g_system_state = STATE_HOME;
            g_edit_pos = 0;
            Display_HomePage();
        } else {
            g_edit_pos++;
            Display_HomePage();
        }
    } else if(key == KEY_VAL_2) {
        // KEY2: 当前位+1（逐位编辑；个位溢出向十位进位，但不影响更高位）
        if(g_edit_pos == 0) {
            // 日的个位 +1, 溢出进十位
            day_tens = g_temp_day / 10;
            day_ones = g_temp_day % 10;
            day_ones++;
            if(day_ones > 9) {
                day_ones = 0;
                if(day_tens >= 3) day_tens = 0; else day_tens++;
            }
            g_temp_day = day_tens * 10 + day_ones;
            EnsureTempDayValid();
        } else if(g_edit_pos == 1) {
            // 日的十位 +1
            day_ones = g_temp_day % 10;
            day_tens = g_temp_day / 10;
            day_tens++;
            if(day_tens > 3) day_tens = 0;
            g_temp_day = day_tens * 10 + day_ones;
            EnsureTempDayValid();
        } else if(g_edit_pos == 2) {
            // 月的个位 +1, 溢出进十位
            month_tens = g_temp_month / 10;
            month_ones = g_temp_month % 10;
            month_ones++;
            if(month_ones > 9) {
                month_ones = 0;
                if(month_tens >= 1) month_tens = 0; else month_tens++;
            }
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 1;
            EnsureTempDayValid();
        } else if(g_edit_pos == 3) {
            // 月的十位 +1
            month_ones = g_temp_month % 10;
            month_tens = g_temp_month / 10;
            month_tens++;
            if(month_tens > 1) month_tens = 0;
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 1;
            EnsureTempDayValid();
        } else if(g_edit_pos >= 4 && g_edit_pos <= 7) {
            // 年份的各位 (个位到千位) 保持原有行为
            year_digit = 1;
            for(month_ones = 0; month_ones < (g_edit_pos - 4); month_ones++) {
                year_digit *= 10;
            }
            g_temp_year = g_temp_year + year_digit;
            if(g_temp_year > 2099) g_temp_year = 2000;
            EnsureTempDayValid();
        }
        Display_HomePage();
    } else if(key == KEY_VAL_3) {
        // KEY3: 当前位-1（逐位编辑；个位借位向十位借位，但不影响更高位）
        if(g_edit_pos == 0) {
            // 日的个位 -1, 借位到十位
            day_tens = g_temp_day / 10;
            day_ones = g_temp_day % 10;
            if(day_ones == 0) {
                day_ones = 9;
                if(day_tens == 0) day_tens = 3; else day_tens--;
            } else {
                day_ones--;
            }
            g_temp_day = day_tens * 10 + day_ones;
            EnsureTempDayValid();
        } else if(g_edit_pos == 1) {
            // 日的十位 -1
            day_ones = g_temp_day % 10;
            day_tens = g_temp_day / 10;
            if(day_tens == 0) day_tens = 3;
            else day_tens--;
            g_temp_day = day_tens * 10 + day_ones;
            EnsureTempDayValid();
        } else if(g_edit_pos == 2) {
            // 月的个位 -1
            month_tens = g_temp_month / 10;
            month_ones = g_temp_month % 10;
            if(month_ones == 0) {
                month_ones = 9;
                if(month_tens == 0) month_tens = 1; else month_tens--;
            } else {
                month_ones--;
            }
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 12;
            EnsureTempDayValid();
        } else if(g_edit_pos == 3) {
            // 月的十位 -1
            month_ones = g_temp_month % 10;
            month_tens = g_temp_month / 10;
            if(month_tens == 0) month_tens = 1;
            else month_tens--;
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 12;
            EnsureTempDayValid();
        } else if(g_edit_pos >= 4 && g_edit_pos <= 7) {
            year_digit = 1;
            for(month_ones = 0; month_ones < (g_edit_pos - 4); month_ones++) {
                year_digit *= 10;
            }
            if(g_temp_year < 2000 + year_digit) g_temp_year = 2099;
            else g_temp_year = g_temp_year - year_digit;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        }
        Display_HomePage();
    } else if(key == KEY_VAL_4) {
        // KEY4: 取消并返回主页
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        g_temp_year = g_datetime.year;
        g_temp_month = g_datetime.month;
        g_temp_day = g_datetime.day;
        Display_HomePage();
    }
}

// 处理时间设定按键 - 逐位编辑模式
// edit_pos: 0-5 对应 HH:MM:SS 的6位数字（从右到左）
//           54 32 10
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
        /* 优化: 当编辑 "秒" (pos 0或1) 时，按 "下一位" (KEY_VAL_1)
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
        // KEY2: 当前位+1 (按位进位，进位仅在同字段内传播)
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
            // 分钟 个位 +1，溢出则十位+1（不影响小时）
            tens = g_temp_minute / 10;
            ones = g_temp_minute % 10;
            ones++;
            if (ones > 9) {
                ones = 0;
                if (tens >= 5) tens = 0; else tens++;
            }
            g_temp_minute = tens * 10 + ones;
        }
        else if (g_edit_pos == 3)
        {
            // 分钟 十位 +1（0..5 循环），不影响小时
            ones = g_temp_minute % 10;
            tens = g_temp_minute / 10;
            tens++;
            if (tens > 5)
                tens = 0;
            g_temp_minute = tens * 10 + ones;
        }
        else if (g_edit_pos == 4)
        {
            // 小时 个位 +1，溢出则十位+1（不影响日期）
            tens = g_temp_hour / 10;
            ones = g_temp_hour % 10;
            ones++;
            if (ones > 9) {
                ones = 0;
                if (tens >= 2) tens = 0; else tens++;
            }
            g_temp_hour = tens * 10 + ones;
            if (g_temp_hour >= 24)
                g_temp_hour = 0;
        }
        else if (g_edit_pos == 5)
        {
            // 小时 十位 +1（0..2 循环），不影响日期
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
        // KEY3: 当前位-1 (按位借位，借位仅在同字段内传播)
        if (g_edit_pos <= 1)
        {
            g_temp_second = 0;
        }
        else if (g_edit_pos == 2)
        {
            // 分钟 个位 -1，借位到十位
            tens = g_temp_minute / 10;
            ones = g_temp_minute % 10;
            if (ones == 0) {
                ones = 9;
                if (tens == 0) tens = 5; else tens--;
            } else {
                ones--;
            }
            g_temp_minute = tens * 10 + ones;
        }
        else if (g_edit_pos == 3)
        {
            // 分钟 十位 -1
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
            // 小时 个位 -1，借位到十位
            tens = g_temp_hour / 10;
            ones = g_temp_hour % 10;
            if (ones == 0) {
                ones = 9;
                if (tens == 0) tens = 2; else tens--;
            } else {
                ones--;
            }
            g_temp_hour = tens * 10 + ones;
            if (g_temp_hour >= 24)
                g_temp_hour = 23;
        }
        else if (g_edit_pos == 5)
        {
            // 小时 十位 -1
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

// 处理秒表按键
void Handle_Stopwatch_Keys(unsigned char key) {
    if(key == (KEY_VAL_1 | 0x10)) {
        // 长按确认键：清零
        Stopwatch_Clear();
        Display_ResetStopwatch();
        Display_StopwatchPage();
    } else if(key == KEY_VAL_1) {
        // 短按确认键：启动/暂停
        if(g_stopwatch.state == SW_RUNNING) {
            Stopwatch_Pause();
        } else {
            Stopwatch_Start();
        }
        Display_StopwatchPage();
    } else if(key == KEY_VAL_2) {
        // 增加键：计次
        Stopwatch_AddLap();
        Display_StopwatchPage();
    } else if(key == KEY_VAL_3) {
        // 减少键：进入计次查看
        g_system_state = STATE_LAP_VIEW;
        g_lap_view_page = 0;
        Display_LapViewPage();
    } else if(key == KEY_VAL_4) {
        // 模式切换：返回主页
        g_system_state = STATE_HOME;
        Display_ResetStopwatch();
        Display_HomePage();
    }
}

// 处理计次查看按键
void Handle_LapView_Keys(unsigned char key) {
    if(key == KEY_VAL_2) {
        // 翻页
        unsigned char total_pages;
        g_lap_view_page++;
        total_pages = (g_lap_count + 3) / 4;
        if(total_pages == 0) {
            total_pages = 1;
        }
        if(g_lap_view_page >= total_pages) {
            g_lap_view_page = 0;  // 循环
        }
        Display_LapViewPage();
    } else if(key == KEY_VAL_3) {
        // 返回秒表主界面
        g_system_state = STATE_STOPWATCH;
        Display_ResetStopwatch();
        Display_StopwatchPage();
    } else if(key == KEY_VAL_4) {
        // 退出秒表模式
        g_system_state = STATE_HOME;
        Display_ResetStopwatch();
        Display_HomePage();
    }
}

// 主函数
void main(void) {
    unsigned char key;
    unsigned int i;
    
    // 初始化LCD
    LCD_Init();
    
    // 等待LCD稳定
    for(i = 0; i < 10000; i++);
    

    Key_Init();
    Stopwatch_Init();
    Buzzer_Init();
    BUZZER = BUZZER_OFF; // 强制关闭蜂鸣器，防止开机误响（使用板级宏）
    
    // 显示主页
    Display_HomePage();
    
    // 最后启动时钟（定时器中断）
    Clock_Init();
    
    // 主循环
    while(1) {
        // 获取按键
        key = Key_GetPressed();

        // 如果在报时窗口（55秒 - 0秒）按下任意键，则请求取消本次整点报时
        // 窗口判断：当分钟为59且秒在55~59，或分钟为0且秒为0（整点瞬间）
        if (key) {
            if ((g_datetime.minute == 59 && g_datetime.second >= 55 && g_datetime.second <= 59) ||
                (g_datetime.minute == 0 && g_datetime.second == 0)) {
                Buzzer_RequestCancelCurrentTop();
            }
        }

        // 菜单光标超时处理：有按键且在菜单状态则重置计数和显示
        if(g_system_state == STATE_MENU) {
            if(key) {
                g_menu_cursor_timeout = 0;
                g_menu_cursor_visible = 1;
            } else {
                if(g_menu_cursor_visible) {
                    g_menu_cursor_timeout++;
                    if(g_menu_cursor_timeout >= 50000) { 
                        g_menu_cursor_visible = 0;
                        g_menu_pos = 0;
                        g_system_state = STATE_HOME; // 超时后直接回主页
                        Display_HomePage();
                    }
                }
            }
        } else {
            g_menu_cursor_visible = 1;
            g_menu_cursor_timeout = 0;
        }

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

        // 定期刷新显示
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

// 确保 g_temp_day 在当前 g_temp_year/g_temp_month 的合法范围内
static void EnsureTempDayValid(void) {
    unsigned char md = GetDaysInMonth(g_temp_year, g_temp_month);
    if (g_temp_day == 0) {
        g_temp_day = 1;
    } else if (g_temp_day > md) {
        g_temp_day = md;
    }
}