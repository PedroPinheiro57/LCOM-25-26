#include "sprites.h"
#include "../devices/video.h"
#include <stdlib.h>

#define CURSOR_SIZE  10
#define CURSOR_COLOR 0xFFFFFF
#define CURSOR_BG    0x1a1a2e

sprite_t *sprite_load(xpm_map_t xpm) {
  sprite_t *sp = malloc(sizeof(sprite_t));
  if (sp == NULL) return NULL;

  xpm_image_t img;
  sp->colors = (uint32_t *) xpm_load(xpm, XPM_8_8_8_8, &img);
  if (sp->colors == NULL) { free(sp); return NULL; }

  sp->width  = img.width;
  sp->height = img.height;
  return sp;
}

void sprite_draw(sprite_t *sp, uint16_t x, uint16_t y) {
  if (sp == NULL) return;
  for (uint16_t row = 0; row < sp->height; row++) {
    for (uint16_t col = 0; col < sp->width; col++) {
      uint32_t color = sp->colors[row * sp->width + col];
      if (color == TRANSPARENT) continue;
      vg_draw_pixel(x + col, y + row, color);
    }
  }
}

void sprite_destroy(sprite_t *sp) {
  if (sp == NULL) return;
  if (sp->colors) free(sp->colors);
  free(sp);
}

void cursor_draw(uint16_t x, uint16_t y) {
  vg_draw_rectangle(x, y, CURSOR_SIZE, CURSOR_SIZE, CURSOR_COLOR);
}
