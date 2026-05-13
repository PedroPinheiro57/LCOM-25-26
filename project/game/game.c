#include "game.h"
#include "menu.h"
#include <lcom/lcf.h>
#include "../video/sprites.h"
#include "../video/font.h"
#include "../devices/keyboard.h"

static game_t g;
static bool over = false;

void game_init(void) {
  g.state         = STATE_MAIN_MENU;
  g.menu_selected = 0;
  font_init();
}

void game_handle_keyboard(uint8_t scancode) {
  uint8_t code = key_get_code(scancode);
  bool    make = key_is_make(scancode);

  if (g.state == STATE_MAIN_MENU) {
    if (make && code == KEY_UP)
      g.menu_selected = (g.menu_selected + 2) % 3;  /* wrap up */
    if (make && code == KEY_DOWN)
      g.menu_selected = (g.menu_selected + 1) % 3;  /* wrap down */
    if (!make && code == KEY_ESC)
      over = true;
    if (make && code == KEY_ENTER) {
      if (g.menu_selected == 0) g.state = STATE_PLACE_SHIPS_P1;
      if (g.menu_selected == 1) g.state = STATE_MAIN_MENU; /* instructions later */
      if (g.menu_selected == 2) over = true;
    }
  }
}

void game_handle_mouse(mouse_state_t *ms) {
  if (g.state == STATE_MAIN_MENU) {
    int hover = menu_mouse_hover(ms->x, ms->y);
    if (hover >= 0) {
      g.menu_selected = hover;
      if (ms->clicked) {
        if (hover == 0) g.state = STATE_PLACE_SHIPS_P1;
        if (hover == 1) g.state = STATE_MAIN_MENU;
        if (hover == 2) over = true;
      }
    }
  }
}

void game_handle_timer(void) {}

void game_draw(void) {
  if (g.state == STATE_MAIN_MENU)
    menu_draw_main(g.menu_selected);
}

bool game_is_over(void) { return over; }
