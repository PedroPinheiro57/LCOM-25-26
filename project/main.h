#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "devices/mouse.h"

mouse_state_t *get_mouse_state(void);
uint8_t       *get_mouse_buf(void);
uint8_t        get_mouse_idx(void);
bool           get_mouse_packet_ready(void);
uint16_t       get_prev_x(void);
uint16_t       get_prev_y(void);

void set_mouse_idx(uint8_t val);
void set_mouse_packet_ready(bool val);
void set_prev_x(uint16_t val);
void set_prev_y(uint16_t val);
