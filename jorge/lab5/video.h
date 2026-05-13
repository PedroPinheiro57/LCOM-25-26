#pragma once
#include <stdint.h>
#include <lcom/lcf.h>

int video_map_vram(uint16_t mode);
int video_set_mode(uint16_t mode);
int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color);
