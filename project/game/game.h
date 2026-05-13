#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../devices/mouse.h"

typedef enum {
  STATE_MAIN_MENU,
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

typedef struct {
  game_state_t state;
  game_state_t prev_state;   /* for pause/resume */
  int          menu_selected; /* 0=Play, 1=Instructions, 2=Exit */
} game_t;

void game_init(void);
void game_handle_timer(void);
void game_handle_keyboard(uint8_t scancode);
void game_handle_mouse(mouse_state_t *ms);
void game_draw(void);
bool game_is_over(void);
