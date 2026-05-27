#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../devices/mouse.h"
#include "../devices/rtc.h"

typedef enum {
  STATE_MAIN_MENU,
  STATE_INSTRUCTIONS,
  STATE_PLACE_SHIPS_P1,
  STATE_HANDOVER_P2,
  STATE_PLACE_SHIPS_P2,
  STATE_HANDOVER_P1,
  STATE_TURN_P1,
  STATE_TURN_P2,
  STATE_PAUSED,
  STATE_GAME_OVER,
  STATE_EXIT
} game_state_t;

#include "board.h"

typedef struct {
  game_state_t tag;
  game_state_t prev;
  board_t      p1_board;
  board_t      p2_board;

  /* RTC clock — updated every second by game_handle_timer */
  rtc_time_t   rtc;
  uint32_t     tick_count;   /* ticks since last RTC read */

  union {
    struct { int selected; } menu;
    struct {
      int player;
      int ship_idx;
      int orient;
      int cursor_col;
      int cursor_row;
    } place;
    struct {
      int  player;
      int  cursor_col;
      int  cursor_row;
      bool last_hit;
    } turn;
    struct { int selected; } pause;
    struct { int winner;   } game_over;
  } data;
} game_t;

void game_init(void);
void game_handle_timer(void);
void game_handle_keyboard(uint8_t scancode);
void game_handle_mouse(mouse_state_t *ms);
void game_draw(void);
void game_erase_cursor(void);
void game_save_cursor(int16_t x, int16_t y);
bool game_is_over(void);
