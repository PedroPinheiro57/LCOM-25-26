#include "font.h"
#include "../../pedro/lab5/video.h"
#include <lcom/lcf.h>
#include "sprites.h"
#include "../assets/font_assets.xpm"

#define FONT_CHAR_W  8
#define FONT_CHAR_H  12
#define FONT_COLS    16
#define FONT_FIRST   32

static sprite_t *font_sprite = NULL;

void font_init(void) {
  font_sprite = sprite_load((xpm_map_t) font_xpm);
}

/*
 * draw_char — fast path
 *
 * Old code called vg_draw_rectangle(scale, scale) for every opaque font
 * pixel, which meant vg_draw_hline → vg_draw_pixel in nested loops with
 * a full offset recomputation on every call.
 *
 * New code iterates the font glyph once.  For each opaque source pixel it
 * writes `scale` horizontal runs of `scale` pixels directly via
 * vg_draw_pixel_fast, which is a single bounds check + memcpy per pixel.
 * This eliminates all the intermediate function-call overhead.
 */
void draw_char(char c, uint16_t x, uint16_t y, uint32_t color, uint8_t scale) {
  if (font_sprite == NULL) return;
  if (c < 32 || c > 127) return;

  int      idx   = c - FONT_FIRST;
  uint16_t src_x = (idx % FONT_COLS) * FONT_CHAR_W;
  uint16_t src_y = (idx / FONT_COLS) * FONT_CHAR_H;

  for (uint16_t row = 0; row < FONT_CHAR_H; row++) {
    const uint32_t *src_row =
      &font_sprite->colors[(src_y + row) * font_sprite->width + src_x];

    uint16_t dst_y_base = y + row * scale;

    for (uint16_t col = 0; col < FONT_CHAR_W; col++) {
      uint32_t pixel = src_row[col];

      /* skip transparent and pure-black pixels */
      if (pixel == TRANSPARENT)        continue;
      if ((pixel & 0x00FFFFFF) == 0)   continue;

      uint16_t dst_x_base = x + col * scale;

      /* write scale×scale block of `color` pixels */
      for (uint8_t sy = 0; sy < scale; sy++) {
        uint16_t dy = dst_y_base + sy;
        for (uint8_t sx = 0; sx < scale; sx++) {
          vg_draw_pixel_fast(dst_x_base + sx, dy, color);
        }
      }
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
