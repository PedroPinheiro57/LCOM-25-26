#include "sprites.h"
#include "../../pedro/lab5/video.h"

#define CURSOR_SIZE 10
#define CURSOR_COLOR  0xFFFFFF   /* white */
#define CURSOR_BG     0x1a1a2e  /* same as background — used to erase */

void cursor_draw(uint16_t x, uint16_t y) {
  vg_draw_rectangle(x, y, CURSOR_SIZE, CURSOR_SIZE, CURSOR_COLOR);
}

void cursor_erase(uint16_t x, uint16_t y) {
  vg_draw_rectangle(x, y, CURSOR_SIZE, CURSOR_SIZE, CURSOR_BG);
}
