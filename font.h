#ifndef __FONT_H__
#define __FONT_H__

// 8x16字体 - 数字0-9和冒号、横杠、斜杠
extern unsigned char code Font_8x16[][16];
// 扩展：用于部分大写字母的 8x16 字模
extern unsigned char code Font_8x16_ext[][16];

// 16x16字体 - 数字0-9和冒号、横杠、斜杠
extern unsigned char code Font_16x16[][32];

// 24x32字体 - 大数字0-9、冒号、点
extern unsigned char code Font_24x32[][96]; /* 注意：Display_Char_24x32 渲染函数已移除 */

// 字符索引函数
unsigned char GetCharIndex(char c);

#endif
