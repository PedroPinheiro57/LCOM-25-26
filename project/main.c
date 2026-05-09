#include <lcom/lcf.h>
#include <lcom/timer.h>
#include "../pedro/lab3/kbc.h"
#include "../pedro/lab4/mouse.h"
#include "../pedro/lab5/video.h"
#include "devices/timer.h"
#include "devices/mouse.h"
#include "handlers/handlers.h"
#include "game/game.h"
#include "main.h"
#include "video/sprites.h"

extern int foo(void);


/* DEFAULT VALUES FOR MOUSE*/
static mouse_state_t ms;
static uint8_t  mouse_buf[3];
static uint8_t  mouse_idx         = 0;
static bool     mouse_packet_ready = false;
static uint16_t prev_x            = 400;
static uint16_t prev_y            = 300;

static uint8_t timer_bit, kbd_bit, mouse_bit;


/* getters */
mouse_state_t *get_mouse_state(void)       { return &ms; }
uint8_t       *get_mouse_buf(void)         { return mouse_buf; }
uint8_t        get_mouse_idx(void)         { return mouse_idx; }
bool           get_mouse_packet_ready(void){ return mouse_packet_ready; }
uint16_t       get_prev_x(void)            { return prev_x; }
uint16_t       get_prev_y(void)            { return prev_y; }

/* setters */
void set_mouse_idx(uint8_t val)          { mouse_idx = val; }
void set_mouse_packet_ready(bool val)    { mouse_packet_ready = val; }
void set_prev_x(uint16_t val)            { prev_x = val; }
void set_prev_y(uint16_t val)            { prev_y = val; }


/* SUBSCRIBE DEVICES*/
static int devices_init(void) {
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

  mouse_state_init(&ms, 400, 300);
  return 0;
}

/* UNSUBSCRIBE DEVICES*/
static void devices_cleanup(void) {
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

    if (irqs & BIT(timer_bit)) {
      handle_timer();
      
      /* only redraw every 2 ticks to reduce flicker */
      if (timer_get_counter() % 2 == 0) {
        video_clear_screen(0x1a1a2e);
        game_draw();
        cursor_draw(get_mouse_state()->x, get_mouse_state()->y);
      }
    }
    if (irqs & BIT(kbd_bit))   handle_keyboard();
    if (irqs & BIT(mouse_bit)) handle_mouse();

    game_draw();
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
