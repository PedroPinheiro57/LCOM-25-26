#pragma once
#include <stdint.h>
#include <stdbool.h>

/* first byte of PS/2 packet as a bitfield union */
typedef union {
  uint8_t val;
  struct {
    uint8_t lb     : 1;   /* left button */
    uint8_t rb     : 1;   /* right button */
    uint8_t mb     : 1;   /* middle button */
    uint8_t sync   : 1;   /* always 1 */
    uint8_t x_sign : 1;   /* X sign bit */
    uint8_t y_sign : 1;   /* Y sign bit */
    uint8_t x_ovf  : 1;   /* X overflow */
    uint8_t y_ovf  : 1;   /* Y overflow */
  } fields;
} mouse_byte1_t;

typedef struct {
  int16_t x, y;      /* current cursor position */
  bool    lb;        /* left button held */
  bool    rb;        /* right button held */
  bool    mb;        /* middle button held */
  bool    clicked;   /* left button just pressed this packet */
  bool    released;  /* left button just released this packet */
  bool    moved;     /* cursor moved this packet */
} mouse_state_t;

void mouse_state_init(mouse_state_t *ms, int16_t start_x, int16_t start_y);
void mouse_state_update(mouse_state_t *ms, uint8_t buf[3], uint16_t hres, uint16_t vres);

/* mouse state accessors */
mouse_state_t *get_mouse_state(void);
uint8_t       *get_mouse_buf(void);
uint8_t        get_mouse_idx(void);
bool           get_mouse_packet_ready(void);

void           set_mouse_idx(uint8_t val);
void           set_mouse_packet_ready(bool val);
