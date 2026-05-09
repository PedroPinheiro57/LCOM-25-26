#include "handlers.h"
#include "../devices/mouse.h"
#include <lcom/lcf.h>
#include "../../pedro/lab3/kbc.h"
#include "../../pedro/lab4/mouse.h"
#include "../../pedro/lab5/video.h"
#include "../devices/keyboard.h"
#include "../devices/mouse.h"
#include "../video/sprites.h"
#include "../game/game.h"

void handle_timer(void) {
  timer_int_handler();

  if (get_mouse_packet_ready()) {
    mouse_state_update(get_mouse_state(), get_mouse_buf(), 800, 600);
    set_mouse_packet_ready(false);
  }

  game_handle_timer();
}

void handle_keyboard(void) {
  kbc_ih();
  if (kbc_has_error()) return;
  uint8_t sc = kbc_get_scancode_byte();
  game_handle_keyboard(sc);
}

void handle_mouse(void) {
  mouse_ih();
  if (mouse_has_error()) return;

  uint8_t byte = mouse_get_byte();
  uint8_t idx  = get_mouse_idx();

  if (idx == 0 && !(byte & BIT(3))) return;

  get_mouse_buf()[idx++] = byte;
  set_mouse_idx(idx);

  if (idx == 3) {
    set_mouse_packet_ready(true);
    set_mouse_idx(0);
  }
}
