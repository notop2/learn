#include "lcd.h"
#include <string.h>
#include <math.h>

/* LCD 访问宏定义 */
#define LCD_CMD_ADDR   (*((volatile uint16_t *)0x60000000))
#define LCD_DATA_ADDR  (*((volatile uint16_t *)0x60020000))

/* 8x16 ASCII 字模数据 */
static const uint8_t ascii8x16[][16] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x20 */
    {0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00}, /* 0x21 */
    {0x00,0x10,0x0C,0x06,0x10,0x0C,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x22 */
    {0x40,0xC0,0x78,0x40,0xC0,0x78,0x40,0x00,0x04,0x3F,0x04,0x04,0x3F,0x04,0x04,0x00}, /* 0x23 */
    {0x00,0x00,0x70,0x08,0x30,0x00,0x00,0x00,0x18,0x04,0x04,0x04,0x18,0x04,0x04,0x00}, /* 0x24 */
};

/* 基础字符数组 (简化版) */
static const uint8_t font_8x16[128][16] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x20 */
    {0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00}, /* 0x21 */
    {0x00,0x10,0x0C,0x06,0x10,0x0C,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x22 */
    {0x40,0xC0,0x78,0x40,0xC0,0x78,0x40,0x00,0x04,0x3F,0x04,0x04,0x3F,0x04,0x04,0x00}, /* 0x23 */
    {0x00,0x00,0x70,0x08,0x30,0x00,0x00,0x00,0x18,0x04,0x04,0x04,0x18,0x04,0x04,0x00}, /* 0x24 */
    {0x00,0x60,0x90,0x04,0x18,0x60,0x80,0x00,0x00,0x0C,0x12,0x12,0x0C,0x00,0x00,0x00}, /* 0x25 */
    {0x00,0x00,0xC0,0x38,0xE0,0xC0,0x00,0x00,0x07,0x1C,0x03,0x30,0x0F,0x00,0x00,0x00}, /* 0x26 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x27 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x28 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x29 */
};

/* 写入命令 */
static inline void LCD_WriteCmd(uint16_t cmd) {
    LCD_CMD_ADDR = cmd;
}

/* 写入数据 */
static inline void LCD_WriteData(uint16_t data) {
    LCD_DATA_ADDR = data;
}

/* 设置显示区域 */
static void LCD_SetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint16_t x2 = x + width - 1;
    uint16_t y2 = y + height - 1;
    
    LCD_WriteCmd(0x2A);
    LCD_WriteData(x >> 8);
    LCD_WriteData(x & 0xFF);
    LCD_WriteData(x2 >> 8);
    LCD_WriteData(x2 & 0xFF);
    
    LCD_WriteCmd(0x2B);
    LCD_WriteData(y >> 8);
    LCD_WriteData(y & 0xFF);
    LCD_WriteData(y2 >> 8);
    LCD_WriteData(y2 & 0xFF);
    
    LCD_WriteCmd(0x2C);
}

/* LCD 初始化 */
void LCD_Init(void) {
    /* 简单的初始化序列 (假设是 ILI9341) */
    HAL_Delay(50);
    
    LCD_WriteCmd(0xCB);
    LCD_WriteData(0x39);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x00);
    LCD_WriteData(0x34);
    LCD_WriteData(0x02);
    
    LCD_WriteCmd(0xCF);
    LCD_WriteData(0x00);
    LCD_WriteData(0xC1);
    LCD_WriteData(0x30);
    
    LCD_WriteCmd(0xE8);
    LCD_WriteData(0x85);
    LCD_WriteData(0x00);
    LCD_WriteData(0x78);
    
    LCD_WriteCmd(0xEA);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    
    LCD_WriteCmd(0xED);
    LCD_WriteData(0x64);
    LCD_WriteData(0x03);
    LCD_WriteData(0x12);
    LCD_WriteData(0x81);
    
    LCD_WriteCmd(0xF7);
    LCD_WriteData(0x20);
    
    LCD_WriteCmd(0xC0);
    LCD_WriteData(0x23);
    
    LCD_WriteCmd(0xC1);
    LCD_WriteData(0x10);
    
    LCD_WriteCmd(0xC5);
    LCD_WriteData(0x3e);
    LCD_WriteData(0x28);
    
    LCD_WriteCmd(0xC7);
    LCD_WriteData(0x86);
    
    LCD_WriteCmd(0x36);
    LCD_WriteData(0x48);
    
    LCD_WriteCmd(0x3A);
    LCD_WriteData(0x55);
    
    LCD_WriteCmd(0xB1);
    LCD_WriteData(0x00);
    LCD_WriteData(0x18);
    
    LCD_WriteCmd(0xB6);
    LCD_WriteData(0x08);
    LCD_WriteData(0x82);
    LCD_WriteData(0x27);
    
    LCD_WriteCmd(0xF2);
    LCD_WriteData(0x00);
    
    LCD_WriteCmd(0x26);
    LCD_WriteData(0x01);
    
    LCD_WriteCmd(0xE0);
    LCD_WriteData(0x0F);
    LCD_WriteData(0x31);
    LCD_WriteData(0x2B);
    LCD_WriteData(0x0C);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x08);
    LCD_WriteData(0x4E);
    LCD_WriteData(0xF1);
    LCD_WriteData(0x37);
    LCD_WriteData(0x07);
    LCD_WriteData(0x10);
    LCD_WriteData(0x03);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x09);
    LCD_WriteData(0x00);
    
    LCD_WriteCmd(0xE1);
    LCD_WriteData(0x00);
    LCD_WriteData(0x0E);
    LCD_WriteData(0x14);
    LCD_WriteData(0x03);
    LCD_WriteData(0x11);
    LCD_WriteData(0x07);
    LCD_WriteData(0x31);
    LCD_WriteData(0xC1);
    LCD_WriteData(0x48);
    LCD_WriteData(0x08);
    LCD_WriteData(0x0F);
    LCD_WriteData(0x0C);
    LCD_WriteData(0x31);
    LCD_WriteData(0x36);
    LCD_WriteData(0x0F);
    
    LCD_WriteCmd(0x11);
    HAL_Delay(120);
    
    LCD_WriteCmd(0x29);
}

/* 清屏 */
void LCD_Clear(uint16_t color) {
    LCD_SetWindow(0, 0, LCD_WIDTH, LCD_HEIGHT);
    
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        LCD_WriteData(color);
    }
}

/* 画点 */
void LCD_SetPoint(uint16_t x, uint16_t y, uint16_t color) {
    LCD_SetWindow(x, y, 1, 1);
    LCD_WriteData(color);
}

/* 画线 */
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    int dx, dy, sx, sy, err, e2;
    
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    sx = x1 < x2 ? 1 : -1;
    sy = y1 < y2 ? 1 : -1;
    err = (dx > dy ? dx : -dy) / 2;
    
    while (1) {
        LCD_SetPoint(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) break;
        
        e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 < dy) { err += dx; y1 += sy; }
    }
}

/* 画矩形 */
void LCD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    LCD_DrawLine(x, y, x + width - 1, y, color);
    LCD_DrawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
    LCD_DrawLine(x, y, x, y + height - 1, color);
    LCD_DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

/* 填充矩形 */
void LCD_FillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    LCD_SetWindow(x, y, width, height);
    
    for (uint32_t i = 0; i < width * height; i++) {
        LCD_WriteData(color);
    }
}

/* 画字符 (简化版) */
void LCD_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t size) {
    uint8_t temp, t1;
    uint16_t y0 = y;
    
    if (ch > 127) ch = '?';
    ch -= ' ';
    
    for (uint8_t t = 0; t < 16; t++) {
        temp = ascii8x16[ch % 5][t];
        for (uint8_t t2 = 0; t2 < 8; t2++) {
            if (temp & 0x80) {
                LCD_SetPoint(x, y, color);
            } else {
                LCD_SetPoint(x, y, bg_color);
            }
            temp <<= 1;
            y++;
        }
        y = y0;
        x++;
    }
}

/* 显示字符串 */
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size) {
    while (*str) {
        LCD_DrawChar(x, y, *str, color, bg_color, size);
        x += 8;
        if (x > LCD_WIDTH - 8) {
            x = 0;
            y += 16;
        }
        str++;
    }
}

/* 显示数字 */
void LCD_ShowNumber(uint16_t x, uint16_t y, int32_t num, uint16_t color, uint16_t bg_color, uint8_t size) {
    char buf[16];
    int len = 0;
    
    if (num < 0) {
        LCD_DrawChar(x, y, '-', color, bg_color, size);
        x += 8;
        num = -num;
    }
    
    if (num == 0) {
        buf[len++] = '0';
    } else {
        while (num > 0) {
            buf[len++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    for (int i = len - 1; i >= 0; i--) {
        LCD_DrawChar(x, y, buf[i], color, bg_color, size);
        x += 8;
    }
}

/* 显示浮点数 */
void LCD_ShowFloat(uint16_t x, uint16_t y, float num, uint8_t decimals, uint16_t color, uint16_t bg_color, uint8_t size) {
    int32_t integer = (int32_t)num;
    int32_t frac = (int32_t)((num - integer) * pow(10, decimals));
    
    if (frac < 0) frac = -frac;
    
    LCD_ShowNumber(x, y, integer, color, bg_color, size);
    
    while ((integer /= 10) || integer == 0) {
        x += 8;
        if (integer == 0) break;
    }
    
    LCD_DrawChar(x, y, '.', color, bg_color, size);
    x += 8;
    
    for (int i = decimals - 1; i >= 0; i--) {
        int div = 1;
        for (int j = 0; j < i; j++) div *= 10;
        LCD_DrawChar(x, y, '0' + (frac / div) % 10, color, bg_color, size);
        x += 8;
    }
}
