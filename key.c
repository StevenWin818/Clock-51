#include "key.h"

// 全局按键变量
KeyInfo data g_key_info = {KEY_VAL_NONE, KEY_NONE, 0};

// 静态变量用于消抖和状态跟踪
static unsigned char last_key_val = KEY_VAL_NONE;
static unsigned char debounce_count = 0;
static bit key_pressed_flag = 0;
static bit key_reported = 0;  // 标记按键是否已被读取
static bit key_released = 0;  // 标记按键已释放

// 按键初始化
void Key_Init(void) {
    // P3口高4位可能被其他功能占用，只设置低4位为输入
    // 写1到对应位以设置为输入模式（51单片机准双向口特性）
    P3 |= 0x0F;  // P3.0-3设为高（输入模式）
}

// 读取当前按下的键值
static unsigned char Key_ReadValue(void) {
    if(KEY1 == 0) return KEY_VAL_1;
    if(KEY2 == 0) return KEY_VAL_2;
    if(KEY3 == 0) return KEY_VAL_3;
    if(KEY4 == 0) return KEY_VAL_4;
    return KEY_VAL_NONE;
}

// 按键扫描 (在定时器中断中每10ms调用一次)
void Key_Scan(void) {
    unsigned char current_key = Key_ReadValue();
    
    // 消抖处理
    if(current_key == last_key_val) {
        if(debounce_count < 3) {
            debounce_count++;
        } else {
            // 稳定按键状态
            if(current_key != KEY_VAL_NONE) {
                // 有键按下
                if(!key_pressed_flag) {
                    // 第一次检测到按键
                    key_pressed_flag = 1;
                    key_reported = 0;  // 重置上报标志
                    g_key_info.key_val = current_key;
                    g_key_info.key_state = KEY_PRESS;
                    g_key_info.press_ticks = 0;
                } else {
                    // 持续按下
                    g_key_info.press_ticks++;
                    if(g_key_info.press_ticks >= KEY_LONG_PRESS_TICKS) {
                        // 长按
                        if(g_key_info.key_state != KEY_LONG) {
                            g_key_info.key_state = KEY_LONG;
                        }
                    }
                }
            } else {
                // 无键按下
                if(key_pressed_flag) {
                    // 按键释放
                    key_pressed_flag = 0;
                    key_released = 1;  // 设置释放标志
                    // 不立即清除key_val和key_state，让GetPressed读取
                }
            }
        }
    } else {
        // 按键值变化，重新开始消抖
        last_key_val = current_key;
        debounce_count = 0;
    }
}

// 获取按键值并清除状态 (返回值: KEY_VAL_NONE, KEY_VAL_1~4)
// 返回高4位表示是否长按，低4位表示键值
unsigned char Key_GetPressed(void) {
    unsigned char result = 0;
    
    // 检测长按（优先）
    if(g_key_info.key_state == KEY_LONG && !key_reported) {
        result = g_key_info.key_val | 0x10;  // 高4位标记长按
        key_reported = 1;
    }
    // 检测短按释放
    else if(g_key_info.key_state == KEY_PRESS && key_released && !key_reported) {
        result = g_key_info.key_val;
        key_reported = 1;
    }
    
    // 清除已上报的按键
    if(key_reported && key_released) {
        g_key_info.key_val = KEY_VAL_NONE;
        g_key_info.key_state = KEY_NONE;
        g_key_info.press_ticks = 0;
        key_released = 0;
        key_reported = 0;
    }
    
    return result;
}
