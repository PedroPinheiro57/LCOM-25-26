#include "menu.h"
#include "../video/font.h"
#include "../../pedro/lab5/video.h"


static const char *options[] = { "PLAY", "INSTRUCTIONS", "EXIT" };
#define NUM_OPTIONS 3

void menu_draw_main(int selected) {
  /* title */
  draw_string("BATTLESHIP", 220, 80, COLOR_TITLE, 4);

  /* options */
  for (int i = 0; i < NUM_OPTIONS; i++) {
    uint16_t oy = OPT_Y_START + i * OPT_GAP;
    uint32_t col = (i == selected) ? COLOR_SELECTED : COLOR_UNSELECTED;
    vg_draw_rectangle(OPT_X, oy, OPT_W, OPT_H, col);
    draw_string(options[i], OPT_X + 10, oy + 15, COLOR_TEXT, 2);
  }
}

int menu_mouse_hover(int x, int y) {
  for (int i = 0; i < NUM_OPTIONS; i++) {
    uint16_t oy = OPT_Y_START + i * OPT_GAP;
    if (x >= OPT_X && x <= OPT_X + OPT_W &&
        y >= oy    && y <= oy + OPT_H)
      return i;
  }
  return -1;
}
