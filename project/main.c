#include <lcom/lcf.h>
#include <lcom/timer.h>
#include "../pedro/lab3/kbc.h"
#include "../pedro/lab4/mouse.h"
#include "../pedro/lab5/video.h"
#include "devices/timer.h"
#include "devices/mouse.h"
#include "handlers/handlers.h"
#include "game/game.h"
#include "video/sprites.h"

extern int foo();

static uint8_t timer_bit, kbd_bit, mouse_bit;

/*SUBSCRIBE DEVICES*/
static int devices_init() {
  if (video_map_vram(0x115) != 0) return 1;
  if (video_set_mode(0x115) != 0) return 1;

  if (timer_subscribe_int(&timer_bit) != 0) return 1;
  if (kbc_subscribe_int(&kbd_bit) != 0) {
    timer_unsubscribe_int(); return 1;
  }
  if (mouse_subscribe_int(&mouse_bit) != 0) {
    kbc_unsubscribe_int(); timer_unsubscribe_int(); return 1;
  }
  if (mouse_enable_data_reporting() != 0) {
    mouse_unsubscribe_int(); kbc_unsubscribe_int(); timer_unsubscribe_int(); return 1;
  }

  mouse_state_init(get_mouse_state(), 400, 300);
  return 0;
}

/* UNSUBSCRIBE DEVICES*/
static void devices_cleanup() {
  mouse_disable_data_reporting();
  mouse_unsubscribe_int();
  kbc_unsubscribe_int();
  timer_unsubscribe_int();
  vg_exit();
}



int(proj_main_loop)(int argc, char *argv[]) {
  foo();

  /*subscribe devices*/
  if (devices_init() != 0) return 1;

  /*initialize screen with RGB color*/
  video_clear_screen(0x1a1a2e);

  /*draw initial screen entities*/
  game_init();

  
  int r, ipc_status;
  message msg;

  while (!game_is_over()) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) continue;
    if (!is_ipc_notify(ipc_status)) continue;
    if (_ENDPOINT_P(msg.m_source) != HARDWARE) continue;

    uint32_t irqs = msg.m_notify.interrupts;

    if (irqs & BIT(mouse_bit)) handle_mouse();
    if (irqs & BIT(kbd_bit))   handle_keyboard();

    if (irqs & BIT(timer_bit)) {
      handle_timer();
      video_clear_screen(0x1a1a2e);
      game_draw();
      cursor_draw(get_mouse_state()->x, get_mouse_state()->y);
    }
  }

  devices_cleanup();
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
