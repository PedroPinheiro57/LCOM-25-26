#pragma once
#include <stdint.h>
#include <lcom/lcf.h>

int video_set_mode(uint16_t mode);
int video_map_vram(uint16_t mode);
int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color);


// PROJECT

int      video_init(uint16_t mode);
int      video_swap_buffers(void);

int      vg_draw_pixel_project(uint16_t x, uint16_t y, uint32_t color);

/*
 * vg_draw_pixel_fast — same as vg_draw_pixel but declared here so
 * font.c / sprites.c can call it without going through the full
 * vg_draw_rectangle stack.  Bounds-checked, no return value.
 */
void     vg_draw_pixel_fast(uint16_t x, uint16_t y, uint32_t color);

int      (vg_draw_hline_project)(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
int      (vg_draw_rectangle_project)(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void     video_clear_screen(void);
uint16_t video_get_hres(void);
uint16_t video_get_vres(void);
