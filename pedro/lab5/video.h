#pragma once
#include <stdint.h>
#include <lcom/lcf.h>

int video_set_mode(uint16_t mode);
int video_map_vram(uint16_t mode);
int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color);


// PROJECT
void     video_clear_screen(uint32_t color);
uint16_t video_get_hres(void);
uint16_t video_get_vres(void);
