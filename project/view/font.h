#pragma once
#include <stdint.h>
#include <lcom/lcf.h>

#define FONT_CHAR_W  8
#define FONT_CHAR_H  12
#define FONT_COLS    16
#define FONT_FIRST   32

void     font_init(void);
bool     font_is_loaded(void);
void     draw_char(char c, uint16_t x, uint16_t y, uint32_t color, uint8_t scale);
void     draw_string(const char *s, uint16_t x, uint16_t y, uint32_t color, uint8_t scale);
