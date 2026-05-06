#include <lcom/lcf.h>
#include "../pedro/lab5/video.h"
#include "../pedro/lab3/kbc.h"
#include <lcom/timer.h>
#include "devices/timer.h"
#include "devices/keyboard.h"


extern int foo(void);
int(proj_main_loop)(int argc, char *argv[]) {
  foo();
  uint8_t kbd_bit, timer_bit;

  if (timer_subscribe_int(&timer_bit) != 0) return 1;
  if (kbc_subscribe_int(&kbd_bit) != 0) {
    timer_unsubscribe_int();
    return 1;
  }

  bool done = false;
  int r, ipc_status;
  message msg;

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
        if (key_is_make(sc))
          printf("key: 0x%02X\n", key_get_code(sc));
        if (key_get_code(sc) == KEY_ESC && !key_is_make(sc))
          done = true;
      }
    }
  }

  kbc_unsubscribe_int();
  timer_unsubscribe_int();
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
