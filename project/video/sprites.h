#pragma once
#include <stdint.h>
#include <lcom/lcf.h>

#define TRANSPARENT 0xFFFFFE

typedef struct {
  uint32_t *colors;
  uint16_t  width;
  uint16_t  height;
} sprite_t;

sprite_t *sprite_load(xpm_map_t xpm);
void      sprite_draw(sprite_t *sp, uint16_t x, uint16_t y);
void      sprite_destroy(sprite_t *sp);

void cursor_draw(uint16_t x, uint16_t y);
