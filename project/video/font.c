#include "font.h"
#include "../../pedro/lab5/video.h"
#include "../assets/pixmaps.h"
#include <lcom/lcf.h>
#include "sprites.h"

#define FONT_CHAR_W  8
#define FONT_CHAR_H  12
#define FONT_COLS    16
#define FONT_FIRST   32

static sprite_t *font_sprite = NULL;

void font_init(void) {
  font_sprite = sprite_load((xpm_map_t) font_xpm);
}

void draw_char(char c, uint16_t x, uint16_t y, uint32_t color, uint8_t scale) {
  if (font_sprite == NULL) return;
  if (c < 32 || c > 127) return;

  int idx = c - FONT_FIRST;
  uint16_t src_x = (idx % FONT_COLS) * FONT_CHAR_W;
  uint16_t src_y = (idx / FONT_COLS) * FONT_CHAR_H;

  for (uint16_t row = 0; row < FONT_CHAR_H; row++) {
    for (uint16_t col = 0; col < FONT_CHAR_W; col++) {
      uint32_t pixel = font_sprite->colors[(src_y + row) * font_sprite->width + (src_x + col)];
      
      /* skip transparent and black, draw everything else */
      if (pixel == TRANSPARENT) continue;
      if ((pixel & 0x00FFFFFF) == 0) continue;  /* skip black */
      
      vg_draw_rectangle(x + col * scale, y + row * scale, scale, scale, color);
    }
  }
}

void draw_string(const char *s, uint16_t x, uint16_t y, uint32_t color, uint8_t scale) {
  uint16_t cx = x;
  while (*s) {
    draw_char(*s, cx, y, color, scale);
    cx += FONT_CHAR_W * scale;
    s++;
  }
}

bool font_is_loaded(void) { return font_sprite != NULL; }
