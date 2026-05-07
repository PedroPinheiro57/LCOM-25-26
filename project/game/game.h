#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../devices/mouse.h"

void game_init(void);
void game_handle_timer(void);
void game_handle_keyboard(uint8_t scancode);
void game_handle_mouse(mouse_state_t *ms);
void game_draw(void);
bool game_is_over(void);
