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
void sprite_draw_rotated(sprite_t *sp, uint16_t x, uint16_t y, bool rotate);

typedef enum {
  CURSOR_NORMAL,
  CURSOR_HOVER,
} cursor_mode_t;

void cursor_init(void);
void cursor_set_mode(cursor_mode_t mode);
void cursor_draw(uint16_t x, uint16_t y);

typedef struct {
    uint8_t   no_pixmaps;
    uint8_t   cur_pixmap;
    sprite_t *pixmaps[8];
    uint16_t  x;
    uint16_t  y;
} animated_sprite_t;

animated_sprite_t* anim_sprite_create(uint16_t x, uint16_t y, uint8_t no_pixmaps);
void anim_sprite_update(animated_sprite_t *anim);
void anim_sprite_draw(animated_sprite_t *anim);
void anim_sprite_destroy(animated_sprite_t *anim);
