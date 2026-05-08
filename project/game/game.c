#include "game.h"
#include <lcom/lcf.h>
#include "../video/sprites.h"

static xpm_row_t const test_sprite[] = {
  "8 8 3 1",
  ". c #FFFFFE",
  "R c #FF0000",
  "W c #FFFFFF",
  "RRRRRRRR",
  "RWWWWWWR",
  "RWRRRWWR",
  "RWWWWWWR",
  "RWRRRWWR",
  "RWWWWWWR",
  "RWWWWWWR",
  "RRRRRRRR"
};

static sprite_t *test_sp = NULL;

void game_init(void) {
  test_sp = sprite_load((xpm_map_t) test_sprite);
}

void game_handle_timer(void) {}
void game_handle_keyboard(uint8_t scancode) { (void)scancode; }
void game_handle_mouse(mouse_state_t *ms) { (void)ms; }
bool game_is_over(void) { return false; }

void game_draw(void) {
  if (test_sp) sprite_draw(test_sp, 100, 100);
}
