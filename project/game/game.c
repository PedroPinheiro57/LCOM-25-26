#include "game.h"
#include "menu.h"
#include <lcom/lcf.h>
#include "../video/font.h"
#include "../devices/keyboard.h"

static game_t g;
static bool   over = false;

void game_init(void) {
  g = (game_t) {
    .tag          = STATE_MAIN_MENU,
    .prev         = STATE_MAIN_MENU,
    .data.menu    = { .selected = 0 }
  };
  font_init();
}

static void transition(game_state_t next) {
  g.prev = g.tag;
  g.tag  = next;
}

void game_handle_keyboard(uint8_t scancode) {
  uint8_t code = key_get_code(scancode);
  bool    make = key_is_make(scancode);

  switch (g.tag) {

    case STATE_MAIN_MENU:
      if (make && code == KEY_UP)
        g.data.menu.selected = (g.data.menu.selected + 2) % 3;
      if (make && code == KEY_DOWN)
        g.data.menu.selected = (g.data.menu.selected + 1) % 3;
      if (!make && code == KEY_ESC)
        over = true;
      if (make && code == KEY_ENTER) {
        switch (g.data.menu.selected) {
          case 0: transition(STATE_PLACE_SHIPS_P1); break;
          case 1: /* instructions — later */ break;
          case 2: over = true; break;
        }
      }
      break;

    case STATE_PLACE_SHIPS_P1:
    case STATE_PLACE_SHIPS_P2:
      if (make && code == KEY_R)
        g.data.place.orient ^= 1;   /* toggle orientation */
      if (!make && code == KEY_ESC)
        transition(STATE_PAUSED);
      break;

    case STATE_TURN_P1:
    case STATE_TURN_P2:
      if (!make && code == KEY_ESC)
        transition(STATE_PAUSED);
      break;

    case STATE_PAUSED:
      if (make && code == KEY_UP)
        g.data.pause.selected = (g.data.pause.selected + 1) % 2;
      if (make && code == KEY_DOWN)
        g.data.pause.selected = (g.data.pause.selected + 1) % 2;
      if (make && code == KEY_ENTER) {
        if (g.data.pause.selected == 0) transition(g.prev); /* resume */
        if (g.data.pause.selected == 1) over = true;        /* quit */
      }
      break;

    default:
      break;
  }
}

void game_handle_mouse(mouse_state_t *ms) {
  switch (g.tag) {

    case STATE_MAIN_MENU: {
      int hover = menu_mouse_hover(ms->x, ms->y);
      if (hover >= 0) {
        g.data.menu.selected = hover;
        if (ms->clicked) {
          switch (hover) {
            case 0: transition(STATE_PLACE_SHIPS_P1); break;
            case 1: /* instructions */ break;
            case 2: over = true; break;
          }
        }
      }
      break;
    }

    default:
      break;
  }
}

void game_handle_timer(void) {}

void game_draw(void) {
  switch (g.tag) {
    case STATE_MAIN_MENU:
      menu_draw_main(g.data.menu.selected);
      break;
    case STATE_PAUSED:
      menu_draw_pause(g.data.pause.selected);
      break;
    case STATE_GAME_OVER:
      menu_draw_game_over(g.data.game_over.winner);
      break;
    default:
      break;
  }
}

bool game_is_over(void) { return over; }
