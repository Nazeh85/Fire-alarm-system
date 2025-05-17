#ifndef LCD1602_H
#define LCD1602_H

#include <stdint.h>

void lcd1602_init(uint8_t addr);
void lcd1602_clear(uint8_t addr);
void lcd1602_gotoxy(uint8_t addr, uint8_t col, uint8_t row);
void lcd1602_write_string(uint8_t addr, const char *str);

#endif // LCD1602_H
