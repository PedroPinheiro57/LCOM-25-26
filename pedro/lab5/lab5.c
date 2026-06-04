// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>

#include "video.h"
#include "../lab4/kbc.h"

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


// 1.
// lcom_run lab5 "init <mode> <delay>"
// lcom_run lab5 "init 0x105 9"

// lcom_run lab5 "init <mode> <delay> -t 1"
// lcom_run lab5 "init 0x105 9 -t 1"

// 2.
// lcom_run lab5 "rectangle 0x105 100 100 200 150 0x0A"
// lcom_run lab5 "rectangle 0x105 100 100 200 150 0x0A -t 1"

// 3.
// lcom_run lab5 "xpm 0 100 100"
// lcom_run lab5 "xpm 1 100 100"
// lcom_run lab5 "xpm 4 100 100"

// lcom_run lab5 "xpm 0 100 100 -t 1"
// lcom_run lab5 "xpm 1 100 100 -t 1"
// lcom_run lab5 "xpm 4 100 100 -t 1"


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

  /*Draw the rectangle*/
  if (vg_draw_rectangle(x, y, width, height, color) != 0) {
    vg_exit();
    return 1;
  }

  // Keyboard interrupt - Wait for ESC 
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
  uint16_t mode = 0x105;

  /* Step 1: map VRAM before switching mode */
  if (video_map_vram(mode) != 0) return 1;

  /* Step 2: switch to graphics mode */
  if (video_set_mode(mode) != 0) return 1;

  /* Step 3: load XPM into a pixmap */
  xpm_image_t img;
  uint8_t *pixmap = xpm_load(xpm, XPM_INDEXED, &img);
  if (pixmap == NULL) {
    vg_exit();
    return 1;
  }

  /* Step 4: draw each pixel from the pixmap to VRAM */
  for (uint16_t row = 0; row < img.height; row++) {
    for (uint16_t col = 0; col < img.width; col++) {
      uint8_t color = pixmap[row * img.width + col];
      if (vg_draw_pixel(x + col, y + row, color) != 0) {
        vg_exit();
        return 1;
      }
    }
  }

  /* Step 5: wait for ESC breakcode */
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
