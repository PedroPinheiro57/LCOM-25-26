#include <lcom/lcf.h>
#include <lcom/timer.h>
#include "../pedro/lab3/kbc.h"
#include "../pedro/lab4/mouse.h"
#include "../pedro/lab5/video.h"
#include "devices/timer.h"
#include "devices/keyboard.h"
#include "devices/mouse.h"
#include "devices/rtc.h"
#include "video/sprites.h" 

extern int foo(void);

int(proj_main_loop)(int argc, char *argv[]) {
  foo();

  /* init video */
  if (video_map_vram(0x115) != 0) return 1;
  if (video_set_mode(0x115) != 0) return 1;

  video_clear_screen(0x1a1a2e);   /* dark navy background */

  /* subscribe devices */
  uint8_t timer_bit, kbd_bit, mouse_bit;
  if (timer_subscribe_int(&timer_bit) != 0) { vg_exit(); return 1; }
  if (kbc_subscribe_int(&kbd_bit) != 0) { timer_unsubscribe_int(); vg_exit(); return 1; }
  if (mouse_subscribe_int(&mouse_bit) != 0) { kbc_unsubscribe_int(); timer_unsubscribe_int(); vg_exit(); return 1; }
  if (mouse_enable_data_reporting() != 0) { mouse_unsubscribe_int(); kbc_unsubscribe_int(); timer_unsubscribe_int(); vg_exit(); return 1; }

  mouse_state_t ms;
  mouse_state_init(&ms, 400, 300);

  uint8_t mouse_buf[3];
  uint8_t mouse_idx = 0;

  bool done = false;
  int r, ipc_status;
  message msg;

  uint16_t prev_x = 400;
  uint16_t prev_y = 300;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) continue;
    if (!is_ipc_notify(ipc_status)) continue;
    if (_ENDPOINT_P(msg.m_source) != HARDWARE) continue;

    uint32_t irqs = msg.m_notify.interrupts;

    if (irqs & BIT(timer_bit)) {
      timer_int_handler();
    }

    if (irqs & BIT(kbd_bit)) {
      kbc_ih();
      if (!kbc_has_error()) {
        uint8_t sc = kbc_get_scancode_byte();
        if (key_get_code(sc) == KEY_ESC && !key_is_make(sc))
          done = true;
      }
    }

    if (irqs & BIT(mouse_bit)) {
      mouse_ih();
      if (!mouse_has_error()) {
        uint8_t byte = mouse_get_byte();
        if (mouse_idx == 0 && !(byte & BIT(3))) { /* no sync */ }
        else {
          mouse_buf[mouse_idx++] = byte;
          if (mouse_idx == 3) {
            mouse_state_update(&ms, mouse_buf, 800, 600);
            cursor_erase(prev_x, prev_y);
            cursor_draw(ms.x, ms.y);
            prev_x = ms.x;
            prev_y = ms.y;
            mouse_idx = 0;
          }
        }
      }
    }
  }

  mouse_disable_data_reporting();
  mouse_unsubscribe_int();
  kbc_unsubscribe_int();
  timer_unsubscribe_int();
  vg_exit();
  return 0;
}

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/grupo_2leic01_5/project/trace.txt");
  lcf_log_output("/home/lcom/labs/grupo_2leic01_5/project/output.txt");
  if (lcf_start(argc, argv))
    return 1;
  lcf_cleanup();
  return 0;
}
