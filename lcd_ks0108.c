#include "lcd_ks0108.h"

// 延迟函数 - 增加延迟时间
void LCD_Delay(void) {
    unsigned char i;
    for(i = 0; i < 10; i++);  // 增加到10
}

// helper: 选择左右半屏
static void LCD_Select(unsigned char side) {
    if (side == 0) {
        LCD_CS1 = 1;
        LCD_CS2 = 0;
    } else {
        LCD_CS1 = 0;
        LCD_CS2 = 1;
    }
}

static void LCD_Deselect(void) {
    LCD_CS1 = 0;
    LCD_CS2 = 0;
}

// 检查忙状态
void LCD_CheckBusy(unsigned char side) {
    unsigned char status;
    unsigned char timeout = 0;
    
    LCD_DATA = 0xFF;
    LCD_DI = 0;  // 指令
    LCD_RW = 1;  // 读
    LCD_Select(side);
    
    do {
        LCD_E = 1;
        LCD_Delay();
        status = LCD_DATA;
        LCD_E = 0;
        LCD_Delay();
        timeout++;
        if(timeout > 200) break;  // 超时退出
    } while(status & 0x80);  // 检查忙标志
    
    LCD_Deselect();
}

// 写命令
void LCD_WriteCmd(unsigned char cmd, unsigned char side) {
    LCD_CheckBusy(side);
    LCD_DI = 0;  // 指令
    LCD_RW = 0;  // 写
    LCD_Select(side);
    LCD_DATA = cmd;
    LCD_E = 1;
    LCD_Delay();
    LCD_E = 0;
    LCD_Delay();
    LCD_Deselect();
}

// 写数据
void LCD_WriteData(unsigned char dat, unsigned char side) {
    LCD_CheckBusy(side);
    LCD_DI = 1;  // 数据
    LCD_RW = 0;  // 写
    LCD_Select(side);
    LCD_DATA = dat;
    LCD_E = 1;
    LCD_Delay();
    LCD_E = 0;
    LCD_Delay();
    LCD_Deselect();
}


// LCD初始化
void LCD_Init(void) {
    unsigned int i;
    
    // 复位脉冲 - 严格按照KS0108时序
    LCD_RST = 0;
    for(i = 0; i < 5000; i++);  // RST低电平至少1ms
    LCD_RST = 1;
    for(i = 0; i < 10000; i++);  // RST释放后等待LCD稳定(至少10ms)
    
    // 初始化左右两半屏
    LCD_WriteCmd(LCD_CMD_DISPLAY_ON, 0);
    LCD_WriteCmd(LCD_CMD_DISPLAY_ON, 1);
    LCD_WriteCmd(LCD_CMD_START_LINE | 0, 0);
    LCD_WriteCmd(LCD_CMD_START_LINE | 0, 1);
    
    for(i = 0; i < 1000; i++);  // 额外稳定时间
    LCD_Clear();
}

// 设置位置
void LCD_SetPos(unsigned char page, unsigned char col) {
    unsigned char side;
    
    if(col < 64) {
        side = 0;
    } else {
        side = 1;
        col -= 64;
    }
    
    // 确保 page 和 col 在合法范围
    if(page > 7) page = 7;
    if(col > 63) col = 63;
    
    LCD_WriteCmd(LCD_CMD_SET_X | page, side);
    LCD_WriteCmd(LCD_CMD_SET_Y | col, side);
}

// 清屏
void LCD_Clear(void) {
    unsigned char page, col;
    
    for(page = 0; page < LCD_PAGES; page++) {
        for(col = 0; col < LCD_WIDTH; col++) {
            LCD_DrawByte(page, col, 0x00);
        }
    }
}

// 清除指定区域
void LCD_ClearArea(unsigned char page_start, unsigned char page_end, unsigned char col_start, unsigned char col_end) {
    unsigned char page, col;
    
    for(page = page_start; page <= page_end; page++) {
        for(col = col_start; col <= col_end; col++) {
            LCD_DrawByte(page, col, 0x00);
        }
    }
}

// 在指定位置画一个字节
void LCD_DrawByte(unsigned char page, unsigned char col, unsigned char dat) {
    LCD_SetPos(page, col);
    if(col < 64) {
        LCD_WriteData(dat, 0);
    } else {
        LCD_WriteData(dat, 1);
    }
}
