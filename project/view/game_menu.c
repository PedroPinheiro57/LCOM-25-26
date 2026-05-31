#include "game_menu.h"
#include "../view/font.h"
#include "../../pedro/lab5/video.h"


static const char *options[] = { "PLAY", "INSTRUCTIONS", "EXIT" };
#define NUM_OPTIONS 3

void menu_draw_main(int selected) {
  /* "BATTLESHIP" = 10 chars * 8px * scale4 = 320px  → x = (800-320)/2 = 240 */
  draw_string("BATTLESHIP", 240, 80, COLOR_TITLE, 4);

  for (int i = 0; i < NUM_OPTIONS; i++) {
    uint16_t oy  = OPT_Y_START + i * OPT_GAP;
    uint32_t col = (i == selected) ? COLOR_SELECTED : COLOR_UNSELECTED;
    vg_draw_rectangle_project(OPT_X, oy, OPT_W, OPT_H, col);
    draw_string(options[i], OPT_X + 10, oy + 15, COLOR_TEXT, 2);
  }
}

void menu_draw_pause(int selected) {
  /* "PAUSED" = 6 * 8 * scale4 = 192px  → x = (800-192)/2 = 304 */
  draw_string("PAUSED", 304, 180, COLOR_TITLE, 4);

  uint32_t col0 = (selected == 0) ? COLOR_SELECTED : COLOR_UNSELECTED;
  uint32_t col1 = (selected == 1) ? COLOR_SELECTED : COLOR_UNSELECTED;
  vg_draw_rectangle_project(OPT_X, 300, OPT_W, OPT_H, col0);
  draw_string("RESUME", OPT_X + 10, 315, COLOR_TEXT, 2);
  vg_draw_rectangle_project(OPT_X, 370, OPT_W, OPT_H, col1);
  draw_string("QUIT", OPT_X + 10, 385, COLOR_TEXT, 2);
}

void menu_draw_game_over(int winner) {
  /* "GAME OVER" = 9 * 8 * scale4 = 288px  → x = (800-288)/2 = 256 */
  draw_string("GAME OVER", 256, 150, COLOR_TITLE, 4);

  /* "PLAYER X WINS" = 13 * 8 * scale3 = 312px  → x = (800-312)/2 = 244 */
  if (winner == 1)
    draw_string("PLAYER 1 WINS", 244, 280, COLOR_SELECTED, 3);
  else
    draw_string("PLAYER 2 WINS", 244, 280, COLOR_SELECTED, 3);

  /* "PRESS ENTER TO PLAY AGAIN" = 25 * 8 * scale2 = 400px  → x = (800-400)/2 = 200 */
  draw_string("PRESS ENTER TO PLAY AGAIN", 200, 390, COLOR_TEXT, 2);
  /* "PRESS ESC TO EXIT" = 17 * 8 * scale2 = 272px  → x = (800-272)/2 = 264 */
  draw_string("PRESS ESC TO EXIT", 264, 440, COLOR_TEXT, 2);
}

void menu_draw_handover(int player) {
  /* "PASS SCREEN TO" = 14 * 8 * scale3 = 336px  → x = (800-336)/2 = 232 */
  draw_string("PASS SCREEN TO", 232, 200, COLOR_TITLE, 3);

  /* "PLAYER X" = 8 * 8 * scale4 = 256px  → x = (800-256)/2 = 272 */
  if (player == 1)
    draw_string("PLAYER 1", 272, 290, COLOR_SELECTED, 4);
  else
    draw_string("PLAYER 2", 272, 290, COLOR_SELECTED, 4);

  /* "PRESS ENTER WHEN READY" = 22 * 8 * scale2 = 352px  → x = (800-352)/2 = 224 */
  draw_string("PRESS ENTER WHEN READY", 224, 430, COLOR_TEXT, 2);
}

void menu_draw_instructions(void) {
  /* "HOW TO PLAY" = 11 * 8 * scale3 = 264px  → x = (800-264)/2 = 268 */
  draw_string("HOW TO PLAY", 268, 60, COLOR_TITLE, 3);

  draw_string("1. PLACE YOUR 5 SHIPS", 80, 160, COLOR_TEXT, 2);
  draw_string("2. TAKE TURNS ATTACKING", 80, 210, COLOR_TEXT, 2);
  draw_string("3. SINK ALL ENEMY SHIPS", 80, 260, COLOR_TEXT, 2);
  draw_string("R - ROTATE SHIP", 80, 340, COLOR_TEXT, 2);
  draw_string("CLICK - PLACE OR ATTACK", 80, 390, COLOR_TEXT, 2);
  draw_string("ESC - PAUSE", 80, 440, COLOR_TEXT, 2);

  /* "PRESS ESC TO GO BACK" = 20 * 8 * scale2 = 320px  → x = (800-320)/2 = 240 */
  draw_string("PRESS ESC TO GO BACK", 240, 520, COLOR_SELECTED, 2);
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
