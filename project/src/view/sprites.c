#include "sprites.h"
#include "../../pedro/lab5/video.h"
#include <stdlib.h>
#include "../assets/mouse_cursor.xpm"
#include "../assets/hand_cursor.xpm"
#include "font.h"
#include "../model/board.h"
#include "renderer.h"

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
  uint32_t trans_color = xpm_transparency_color(XPM_8_8_8_8); 

  for (uint16_t row = 0; row < sp->height; row++) {
    for (uint16_t col = 0; col < sp->width; col++) {
      uint32_t color = sp->colors[row * sp->width + col];
      
      if (color == TRANSPARENT || color == trans_color) continue;
      
      vg_draw_pixel_project(x + col, y + row, color);
    }
  }
}

void sprite_draw_rotated(sprite_t *sp, uint16_t x, uint16_t y, bool rotate) {
  if (sp == NULL) return;
  uint32_t trans_color = xpm_transparency_color(XPM_8_8_8_8);

  for (uint16_t row = 0; row < sp->height; row++) {
    for (uint16_t col = 0; col < sp->width; col++) {
      uint32_t color = sp->colors[row * sp->width + col];
      if (color == TRANSPARENT || color == trans_color) continue;
      
      if (!rotate) {
          vg_draw_pixel_project(x + col, y + row, color);
      } else {
          vg_draw_pixel_project(x + (sp->height - 1 - row), y + col, color);
      }
    }
  }
}

static sprite_t    *cursor_normal = NULL;
static sprite_t    *cursor_hover  = NULL;
static cursor_mode_t cursor_mode  = CURSOR_NORMAL;

void cursor_set_mode(cursor_mode_t mode) { cursor_mode = mode; }

void cursor_init(void) {
  cursor_normal = sprite_load((xpm_map_t) cursor_hover_xpm);   /* arrow = normal */
  cursor_hover  = sprite_load((xpm_map_t) cursor_normal_xpm);  /* hand  = hover  */
}

void cursor_draw(uint16_t x, uint16_t y) {
  sprite_t *cur = (cursor_mode == CURSOR_HOVER) ? cursor_hover : cursor_normal;
  if (cur) sprite_draw(cur, x, y);
}

animated_sprite_t* anim_sprite_create(uint16_t x, uint16_t y, uint8_t no_pixmaps) {
    animated_sprite_t *anim = (animated_sprite_t*) malloc(sizeof(animated_sprite_t));
    if (anim == NULL) return NULL;
    
    anim->x = x;
    anim->y = y;
    anim->no_pixmaps = no_pixmaps;
    anim->cur_pixmap = 0; 

    for(int i = 0; i < 8; i++) {
        anim->pixmaps[i] = NULL;
    }
    
    return anim;
}

void anim_sprite_update(animated_sprite_t *anim) {
    if (anim == NULL || anim->no_pixmaps == 0) return;
    
    anim->cur_pixmap++;
    
    if (anim->cur_pixmap >= anim->no_pixmaps) {
        anim->cur_pixmap = 0;
    }
}

void anim_sprite_draw(animated_sprite_t *anim) {
    if (anim == NULL) return;
    
    sprite_t *current_frame = anim->pixmaps[anim->cur_pixmap];
    
    if (current_frame != NULL) {
        sprite_draw(current_frame, anim->x, anim->y);
    }
}

void sprite_destroy(sprite_t *sp) {
  if (sp == NULL) return;
  if (sp->colors) free(sp->colors);
  free(sp);
}

void anim_sprite_destroy(animated_sprite_t *anim) {
    if (anim == NULL) return;
    
    for(int i = 0; i < anim->no_pixmaps; i++) {
        if (anim->pixmaps[i] != NULL) {
            sprite_destroy(anim->pixmaps[i]);
        }
    }
    free(anim);
}

void destroy_game_sprites(void) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        sprite_destroy(get_ship_sprite(i));
        sprite_destroy(get_ship_dead_sprite(i));
    }
    
    anim_sprite_destroy(get_anim_flame());
    anim_sprite_destroy(get_anim_explosion());

    sprite_destroy(font_get_sprite());
    sprite_destroy(cursor_normal);
    sprite_destroy(cursor_hover);

    sprite_destroy(get_miss_sprite());
}

void sprite_draw_mini(sprite_t *sp, uint16_t x, uint16_t y, bool rotate) {
  if (sp == NULL) return;
  uint32_t trans_color = xpm_transparency_color(XPM_8_8_8_8);

  for (uint16_t row = 0; row < sp->height; row += 2) {
    for (uint16_t col = 0; col < sp->width; col += 2) {
      uint32_t color = sp->colors[row * sp->width + col];
      
      if (color == TRANSPARENT || color == trans_color) continue;
      
      uint16_t px, py;
      if (!rotate) {
          px = x + col / 2;
          py = y + row / 2;
      } else {
          px = x + (sp->height - 1 - row) / 2;
          py = y + col / 2;
      }
      
      vg_draw_pixel_project(px, py, color);
    }
  }
}
