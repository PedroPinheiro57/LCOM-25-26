// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>

#include "video.h"
#include "kbc.h"

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need it]
    lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab5/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
        return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}


int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for (uint16_t row = 0; row < height; row++) {
    if (vg_draw_hline(x, y + row, width, color) != 0) return 1;
  }
  return 0;
}

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (uint16_t i = 0; i < len; i++) {
    if (vg_draw_pixel(x + i, y, color) != 0) return 1;
  }
  return 0;
}


// 1.
// lcom_run lab5 "init <mode> <delay>"
// lcom_run lab5 "init 0x105 9"

// lcom_run lab5 "init <mode> <delay> -t 1"
// lcom_run lab5 "init 0x105 9 -t 1"

// 2.
// lcom_run lab5 "rectangle 0x105 100 100 200 150 0x0A"
// lcom_run lab5 "rectangle 0x105 100 100 200 150 0x0A -t 1"

int(video_test_init)(uint16_t mode, uint8_t delay) {

  if (video_set_mode(mode) != 0) return 1;

  tickdelay(micros_to_ticks((uint32_t)delay * 1000000u));

  if (vg_exit() != 0) return 1;

  return 0;
}


int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y,
                           uint16_t width, uint16_t height, uint32_t color) {
  if (video_map_vram(mode) != 0) return 1;
  if (video_set_mode(mode) != 0) return 1;

  /* Use LCF's vg_draw_rectangle directly — it's a macro that also
     handles test interception */
  if (vg_draw_rectangle(x, y, width, height, color) != 0) {
    vg_exit();
    return 1;
  }

  uint8_t kbd_bit;
  if (kbc_subscribe_int(&kbd_bit) != 0) {
    vg_exit();
    return 1;
  }

  int r, ipc_status;
  message msg;
  bool done = false;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }
    if (!is_ipc_notify(ipc_status))                continue;
    if (_ENDPOINT_P(msg.m_source) != HARDWARE)     continue;
    if (!(msg.m_notify.interrupts & BIT(kbd_bit))) continue;

    kbc_ih();
    if (kbc_has_error()) continue;

    if (kbc_get_scancode_byte() == ESC_BREAKCODE)
      done = true;
  }

  kbc_unsubscribe_int();
  if (vg_exit() != 0) return 1;
  return 0;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {

  return 0;

}
