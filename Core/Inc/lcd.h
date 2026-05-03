#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "fsmc.h"
#include <stdint.h>
#include <stdbool.h>

/* LCD 尺寸 */
#define LCD_WIDTH    320
#define LCD_HEIGHT   240

/* 常用颜色定义 (RGB565) */
#define COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define COLOR_RED        0xF800
#define COLOR_GREEN      0x07E0
#define COLOR_BLUE       0x001F
#define COLOR_YELLOW     0xFFE0
#define COLOR_GRAY       0x8410

/* LCD 命令定义 (假设是 ILI9341 或兼容芯片) */
#define LCD_CMD_RESET    0x01
#define LCD_CMD_SLEEP_IN 0x10
#define LCD_CMD_SLEEP_OUT 0x11
#define LCD_CMD_MODE     0x3A
#define LCD_CMD_WINDOW   0x2A
#define LCD_CMD_MEMORY   0x2C

/* 函数声明 */
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_SetPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_FillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void LCD_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t size);
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size);
void LCD_ShowNumber(uint16_t x, uint16_t y, int32_t num, uint16_t color, uint16_t bg_color, uint8_t size);
void LCD_ShowFloat(uint16_t x, uint16_t y, float num, uint8_t decimals, uint16_t color, uint16_t bg_color, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */
