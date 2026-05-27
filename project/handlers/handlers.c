#include "handlers.h"
#include "../devices/mouse.h"
#include <lcom/lcf.h>
#include "../../pedro/lab3/kbc.h"
#include "../../pedro/lab4/mouse.h"
#include "../devices/keyboard.h"
#include "../devices/mouse.h"
#include "../../pedro/lab5/video.h"
#include "../video/sprites.h"
#include "../game/game.h"

void handle_timer(void) {
  timer_int_handler();
  game_handle_timer();
}

/* ------------------------------------------------------------------
 * Scancode assembler
 * Arrow keys and other extended keys send two bytes: 0xE0 followed
 * by the key byte. kbc_ih() fires once per byte, so handle_keyboard()
 * is called twice. We buffer the 0xE0 prefix and only dispatch to
 * game_handle_keyboard() once we have a complete scancode.
 *
 * For 1-byte scancodes (most keys): dispatch immediately.
 * For 2-byte scancodes: swallow the 0xE0 byte, wait for the next
 * byte, then dispatch that byte directly — the key constants
 * (KEY_UP=0x48 etc.) match the second byte, so no adjustment needed.
 * ------------------------------------------------------------------ */
static bool expecting_second_byte = false;

void handle_keyboard(void) {
  kbc_ih();
  if (kbc_has_error()) return;

  uint8_t sc = kbc_get_scancode_byte();

  if (sc == TWOBYTE_PREFIX) {
    expecting_second_byte = true;
    return;   /* swallow prefix, wait for the real key byte */
  }

  /* second byte of a 2-byte scancode, or a plain 1-byte scancode */
  expecting_second_byte = false;
  game_handle_keyboard(sc);
}

void handle_mouse(void) {
  mouse_ih();
  if (mouse_has_error()) return;

  uint8_t byte = mouse_get_byte();
  uint8_t idx  = get_mouse_idx();

  if (idx == 0 && !(byte & MOUSE_SYNC_BIT)) return;

  get_mouse_buf()[idx++] = byte;
  set_mouse_idx(idx);

  if (idx == 3) {
    mouse_state_update(get_mouse_state(), get_mouse_buf(),
                       video_get_hres(), video_get_vres());
    set_mouse_idx(0);
  }
}
