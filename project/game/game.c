#include "game.h"
#include "menu.h"
#include <lcom/lcf.h>
#include "../video/font.h"
#include "../video/sprites.h"
#include "../devices/keyboard.h"
#include "../devices/video.h"

#define CURSOR_SIZE 10

static game_t  g;
static bool    over    = false;
static bool    dirty   = true;
static int16_t prev_cx = 0;
static int16_t prev_cy = 0;

void game_init(void) {
  g = (game_t) {
    .tag          = STATE_MAIN_MENU,
    .prev         = STATE_MAIN_MENU,
    .data.menu    = { .selected = 0 }
  };
  font_init();
  cursor_init();   
  dirty   = true;
  prev_cx = 400;
  prev_cy = 300;
}

static void transition(game_state_t next) {
  g.prev = g.tag;
  g.tag  = next;
  dirty  = true;
}

void game_handle_keyboard(uint8_t scancode) {
  uint8_t code = key_get_code(scancode);
  bool    make = key_is_make(scancode);

  switch (g.tag) {

    case STATE_MAIN_MENU:
      if (make && code == KEY_UP) {
        g.data.menu.selected = (g.data.menu.selected + 2) % 3;
        dirty = true;
      }
      if (make && code == KEY_DOWN) {
        g.data.menu.selected = (g.data.menu.selected + 1) % 3;
        dirty = true;
      }
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
      if (make && code == KEY_R) {
        g.data.place.orient ^= 1;
        dirty = true;
      }
      if (!make && code == KEY_ESC)
        transition(STATE_PAUSED);
      break;

    case STATE_TURN_P1:
    case STATE_TURN_P2:
      if (!make && code == KEY_ESC)
        transition(STATE_PAUSED);
      break;

    case STATE_PAUSED:
      if (make && code == KEY_UP) {
        g.data.pause.selected = (g.data.pause.selected + 1) % 2;
        dirty = true;
      }
      if (make && code == KEY_DOWN) {
        g.data.pause.selected = (g.data.pause.selected + 1) % 2;
        dirty = true;
      }
      if (make && code == KEY_ENTER) {
        if (g.data.pause.selected == 0) transition(g.prev);
        if (g.data.pause.selected == 1) over = true;
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
      cursor_set_mode(hover >= 0 ? CURSOR_NORMAL : CURSOR_HOVER);
      if (hover >= 0 && hover != g.data.menu.selected) {
        g.data.menu.selected = hover;
        dirty = true;
      }
      if (ms->clicked && hover >= 0) {
        switch (hover) {
          case 0: transition(STATE_PLACE_SHIPS_P1); break;
          case 1: /* instructions */ break;
          case 2: over = true; break;
        }
      }
      break;
    }

    default:
      cursor_set_mode(CURSOR_NORMAL);
      break;
  }

  if (ms->moved) dirty = true;
}

void game_handle_timer(void) {}

/*
 * game_erase_cursor
 * Paints a black rectangle over the previous cursor position.
 * Call this at the START of the timer tick, before game_draw(),
 * so the ghost is erased regardless of whether a full redraw follows.
 */
void game_erase_cursor(void) {
  vg_draw_rectangle(prev_cx, prev_cy, CURSOR_SIZE + 1, CURSOR_SIZE + 1, 0x000000);
}

/*
 * game_save_cursor
 * Records where the cursor was drawn this frame so the next frame
 * can erase it.  Call this AFTER cursor_draw().
 */
void game_save_cursor(int16_t x, int16_t y) {
  prev_cx = x;
  prev_cy = y;
}

/*
 * game_draw
 * Full clear + redraw only when dirty.
 * Does NOT touch the cursor — caller owns that.
 */
void game_draw(void) {
  video_clear_screen(0x000000);

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
