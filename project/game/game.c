#include "game.h"
#include <lcom/lcf.h>
#include "../video/sprites.h"
#include "../video/font.h"
#include "../devices/keyboard.h"

void game_init(void) {
  font_init();
}

void game_handle_timer(void) {}
static bool over = false;

void game_handle_keyboard(uint8_t scancode) {
  if (key_get_code(scancode) == KEY_ESC && !key_is_make(scancode))
    over = true;
}

bool game_is_over(void) { return over; }
void game_handle_mouse(mouse_state_t *ms) { (void)ms; }


void game_draw(void) {
  printf("font loaded: %d\n", font_is_loaded());
  draw_string("BATTLESHIP", 200, 50, 0xFFFFFF, 3);
}
