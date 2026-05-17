#include "menu.h"
#include "../video/font.h"
#include "../devices/video.h"

static const char *options[] = { "PLAY", "INSTRUCTIONS", "EXIT" };
#define NUM_OPTIONS 3

void menu_draw_main(int selected) {
  draw_string("BATTLESHIP", 220, 80, COLOR_TITLE, 4);

  for (int i = 0; i < NUM_OPTIONS; i++) {
    uint16_t oy  = OPT_Y_START + i * OPT_GAP;
    uint32_t col = (i == selected) ? COLOR_SELECTED : COLOR_UNSELECTED;
    vg_draw_rectangle(OPT_X, oy, OPT_W, OPT_H, col);
    draw_string(options[i], OPT_X + 10, oy + 15, COLOR_TEXT, 2);
  }
}

void menu_draw_pause(int selected) {
  draw_string("PAUSED", 310, 180, COLOR_TITLE, 4);
  uint32_t col0 = (selected == 0) ? COLOR_SELECTED : COLOR_UNSELECTED;
  uint32_t col1 = (selected == 1) ? COLOR_SELECTED : COLOR_UNSELECTED;
  vg_draw_rectangle(OPT_X, 300, OPT_W, OPT_H, col0);
  draw_string("RESUME", OPT_X + 10, 315, COLOR_TEXT, 2);
  vg_draw_rectangle(OPT_X, 370, OPT_W, OPT_H, col1);
  draw_string("QUIT", OPT_X + 10, 385, COLOR_TEXT, 2);
}

void menu_draw_game_over(int winner) {
  draw_string("GAME OVER", 230, 150, COLOR_TITLE, 4);
  if (winner == 1)
    draw_string("PLAYER 1 WINS", 170, 300, COLOR_SELECTED, 3);
  else
    draw_string("PLAYER 2 WINS", 170, 300, COLOR_SELECTED, 3);
  draw_string("PRESS ENTER TO PLAY AGAIN", 80, 420, COLOR_TEXT, 2);
  draw_string("PRESS ESC TO EXIT", 160, 470, COLOR_TEXT, 2);
}

void menu_draw_handover(int player) {
  draw_string("PASS SCREEN TO", 170, 200, COLOR_TITLE, 3);
  if (player == 1)
    draw_string("PLAYER 1", 230, 290, COLOR_SELECTED, 4);
  else
    draw_string("PLAYER 2", 230, 290, COLOR_SELECTED, 4);
  draw_string("PRESS ENTER WHEN READY", 110, 430, COLOR_TEXT, 2);
}

void menu_draw_instructions(void) {
  draw_string("HOW TO PLAY", 200, 60, COLOR_TITLE, 3);
  draw_string("1. PLACE YOUR 5 SHIPS", 80, 160, COLOR_TEXT, 2);
  draw_string("2. TAKE TURNS ATTACKING", 80, 210, COLOR_TEXT, 2);
  draw_string("3. SINK ALL ENEMY SHIPS", 80, 260, COLOR_TEXT, 2);
  draw_string("R - ROTATE SHIP", 80, 340, COLOR_TEXT, 2);
  draw_string("CLICK - PLACE OR ATTACK", 80, 390, COLOR_TEXT, 2);
  draw_string("ESC - PAUSE", 80, 440, COLOR_TEXT, 2);
  draw_string("PRESS ESC TO GO BACK", 120, 520, COLOR_SELECTED, 2);
}

int menu_mouse_hover(int x, int y) {
  for (int i = 0; i < NUM_OPTIONS; i++) {
    uint16_t oy = OPT_Y_START + i * OPT_GAP;
    if (x >= OPT_X && x <= OPT_X + OPT_W &&
        y >= oy    && y <= oy  + OPT_H)
      return i;
  }
  return -1;
}
