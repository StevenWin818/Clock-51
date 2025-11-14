/* Board-level configuration. Edit these according to your hardware wiring. */
#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

/*
 * BUZZER_ACTIVE_LOW:
 *  - 0 : 蜂鸣器为高电平有效（BUZZER_ON = 1）
 *  - 1 : 蜂鸣器为低电平有效（BUZZER_ON = 0）
 *
 * 默认值设为 0 以兼容仓库中早期提交的行为；如果你的板子蜂鸣器为
 * 低电平有效，请将其改为 1 并重新编译。
 */
#define BUZZER_ACTIVE_LOW 0

#endif
