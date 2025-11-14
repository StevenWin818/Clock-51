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
        // KEY2: 当前位+1
        if(g_edit_pos == 0) {
            // 日的个位
            day_tens = g_temp_day / 10;
            day_ones = g_temp_day % 10;
            day_ones++;
            if(day_ones > 9) day_ones = 0;
            g_temp_day = day_tens * 10 + day_ones;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = 1;
        } else if(g_edit_pos == 1) {
            // 日的十位
            day_ones = g_temp_day % 10;
            day_tens = g_temp_day / 10;
            day_tens++;
            if(day_tens > 3) day_tens = 0;
            g_temp_day = day_tens * 10 + day_ones;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = 1;
        } else if(g_edit_pos == 2) {
            // 月的个位
            month_tens = g_temp_month / 10;
            month_ones = g_temp_month % 10;
            month_ones++;
            if(month_ones > 9) month_ones = 0;
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 1;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        } else if(g_edit_pos == 3) {
            // 月的十位
            month_ones = g_temp_month % 10;
            month_tens = g_temp_month / 10;
            month_tens++;
            if(month_tens > 1) month_tens = 0;
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 1;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        } else if(g_edit_pos >= 4 && g_edit_pos <= 7) {
            // 年份的各位 (个位到千位)
            year_digit = 1;
            for(month_ones = 0; month_ones < (g_edit_pos - 4); month_ones++) {
                year_digit *= 10;
            }
            g_temp_year = g_temp_year + year_digit;
            if(g_temp_year > 2099) g_temp_year = 2000;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        }
        Display_HomePage();
    } else if(key == KEY_VAL_3) {
        // KEY3: 当前位-1
        if(g_edit_pos == 0) {
            day_tens = g_temp_day / 10;
            day_ones = g_temp_day % 10;
            if(day_ones == 0) day_ones = 9;
            else day_ones--;
            g_temp_day = day_tens * 10 + day_ones;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        } else if(g_edit_pos == 1) {
            day_ones = g_temp_day % 10;
            day_tens = g_temp_day / 10;
            if(day_tens == 0) day_tens = 3;
            else day_tens--;
            g_temp_day = day_tens * 10 + day_ones;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month) || g_temp_day == 0)
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        } else if(g_edit_pos == 2) {
            month_tens = g_temp_month / 10;
            month_ones = g_temp_month % 10;
            if(month_ones == 0) month_ones = 9;
            else month_ones--;
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 12;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
        } else if(g_edit_pos == 3) {
            month_ones = g_temp_month % 10;
            month_tens = g_temp_month / 10;
            if(month_tens == 0) month_tens = 1;
            else month_tens--;
            g_temp_month = month_tens * 10 + month_ones;
            if(g_temp_month > 12 || g_temp_month == 0) g_temp_month = 12;
            if(g_temp_day > GetDaysInMonth(g_temp_year, g_temp_month))
                g_temp_day = GetDaysInMonth(g_temp_year, g_temp_month);
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
void Handle_TimeSet_Keys(unsigned char key) {
    unsigned char tens, ones;
    
    if(key == (KEY_VAL_1 | 0x10)) {
        DateTime_SetTime(g_temp_hour, g_temp_minute, g_temp_second);
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        Display_HomePage();
        return;
    }

    if(key == KEY_VAL_1) {
        // KEY1: 切换到下一位编辑
        if(g_edit_pos >= 5) {
            DateTime_SetTime(g_temp_hour, g_temp_minute, g_temp_second);
            g_system_state = STATE_HOME;
            g_edit_pos = 0;
            Display_HomePage();
        } else {
            g_edit_pos++;
            Display_HomePage();
        }
    } else if(key == KEY_VAL_2) {
        // KEY2: 当前位+1
        if(g_edit_pos <= 1) {
            // 调整秒钟时，分钟加一，秒钟归零
            g_temp_second = 0;
            g_temp_minute++;
            if(g_temp_minute >= 60) {
                g_temp_minute = 0;
                g_temp_hour++;
                if(g_temp_hour >= 24) {
                    g_temp_hour = 0;
                }
            }
        } else if(g_edit_pos == 2) {
            // 分的个位
            tens = g_temp_minute / 10;
            ones = g_temp_minute % 10;
            ones++;
            if(ones > 9) ones = 0;
            g_temp_minute = tens * 10 + ones;
            if(g_temp_minute >= 60) g_temp_minute = 0;
        } else if(g_edit_pos == 3) {
            // 分的十位
            ones = g_temp_minute % 10;
            tens = g_temp_minute / 10;
            tens++;
            if(tens > 5) tens = 0;
            g_temp_minute = tens * 10 + ones;
        } else if(g_edit_pos == 4) {
            // 时的个位
            tens = g_temp_hour / 10;
            ones = g_temp_hour % 10;
            ones++;
            if(ones > 9) ones = 0;
            g_temp_hour = tens * 10 + ones;
            if(g_temp_hour >= 24) g_temp_hour = 0;
        } else if(g_edit_pos == 5) {
            // 时的十位
            ones = g_temp_hour % 10;
            tens = g_temp_hour / 10;
            tens++;
            if(tens > 2) tens = 0;
            g_temp_hour = tens * 10 + ones;
            if(g_temp_hour >= 24) g_temp_hour = 0;
        }
        Display_HomePage();
    } else if(key == KEY_VAL_3) {
        // KEY3: 当前位-1
        if(g_edit_pos <= 1) {
            // 秒钟位直接归零
            g_temp_second = 0;
        } else if(g_edit_pos == 2) {
            tens = g_temp_minute / 10;
            ones = g_temp_minute % 10;
            if(ones == 0) ones = 9;
            else ones--;
            g_temp_minute = tens * 10 + ones;
        } else if(g_edit_pos == 3) {
            ones = g_temp_minute % 10;
            tens = g_temp_minute / 10;
            if(tens == 0) tens = 5;
            else tens--;
            g_temp_minute = tens * 10 + ones;
        } else if(g_edit_pos == 4) {
            tens = g_temp_hour / 10;
            ones = g_temp_hour % 10;
            if(ones == 0) ones = 9;
            else ones--;
            g_temp_hour = tens * 10 + ones;
            if(g_temp_hour >= 24) g_temp_hour = 23;
        } else if(g_edit_pos == 5) {
            ones = g_temp_hour % 10;
            tens = g_temp_hour / 10;
            if(tens == 0) tens = 2;
            else tens--;
            g_temp_hour = tens * 10 + ones;
            if(g_temp_hour >= 24) g_temp_hour = 23;
        }
        Display_HomePage();
    } else if(key == KEY_VAL_4) {
        // KEY4: 取消并返回主页
        g_system_state = STATE_HOME;
        g_edit_pos = 0;
        g_temp_hour = g_datetime.hour;
        g_temp_minute = g_datetime.minute;
        g_temp_second = g_datetime.second;
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
    
    // === 调试选项：取消注释以测试LCD ===
    // LCD_Test();  // 全屏填充测试
    // while(1);    // 停在这里观察
    
    // === 简单绘图测试 ===
    // LCD_DrawByte(0, 0, 0xFF);    // 左上角
    // LCD_DrawByte(0, 10, 0xFF);   // 第二个位置
    // LCD_DrawByte(3, 64, 0xFF);   // 右半屏
    // while(1);
    
    // 初始化其他模块（延后以避免干扰LCD）
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

        switch(g_system_state) {
            case STATE_TIME_SET:
                if(key == KEY_VAL_1) {
                    if(g_edit_pos == 0 || g_edit_pos == 1) {
                        g_edit_pos = 2;
                        Display_HomePage();
                    } else if(g_edit_pos >= 5) {
                        DateTime_SetTime(g_temp_hour, g_temp_minute, g_temp_second);
                        g_system_state = STATE_HOME;
                        g_edit_pos = 0;
                        Display_HomePage();
                    } else {
                        g_edit_pos++;
                        Display_HomePage();
                    }
                } else {
                    Handle_TimeSet_Keys(key);
                }
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
        if(g_time_changed) {
            g_time_changed = 0;
            if(g_system_state == STATE_TIME_SET) {
                TempTime_Tick();
                Display_HomePage();
                Buzzer_Check();
            } else if(g_system_state == STATE_HOME || g_system_state == STATE_MENU || g_system_state == STATE_DATE_SET) {
                Display_HomePage();
                Buzzer_Check();
            } else {
                Buzzer_Check();
            }
        }

        if(g_system_state == STATE_STOPWATCH) {
            // 秒表页面刷新
            if(g_refresh_counter >= 2) {   // 20ms刷新一次，提高刷新速率
                g_refresh_counter = 0;
                Display_StopwatchPage();
            }
        }
    }
}
