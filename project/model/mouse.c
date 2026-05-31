#include "mouse.h"

#define MOUSE_SENSITIVITY 1

void mouse_state_init(mouse_state_t *ms, int16_t start_x, int16_t start_y) {
  ms->x        = start_x;
  ms->y        = start_y;
  ms->lb       = false;
  ms->rb       = false;
  ms->mb       = false;
  ms->clicked  = false;
  ms->released = false;
  ms->moved    = false;
}

void mouse_state_update(mouse_state_t *ms, uint8_t buf[3], uint16_t hres, uint16_t vres) {
  mouse_byte1_t b1 = { .val = buf[0] };

  bool prev_lb = ms->lb;
  ms->lb = b1.fields.lb;
  ms->rb = b1.fields.rb;
  ms->mb = b1.fields.mb;

  ms->clicked  = (!prev_lb && ms->lb);
  ms->released = (prev_lb && !ms->lb);

  if (b1.fields.x_ovf || b1.fields.y_ovf) {
    ms->moved = false;
    goto clamp;
  }

  int16_t dx = buf[1];
  int16_t dy = buf[2];

  if (b1.fields.x_sign) dx |= 0xFF00;
  if (b1.fields.y_sign) dy |= 0xFF00;

  ms->x    += dx;
  ms->y    -= dy;
  ms->moved = (dx != 0 || dy != 0);

clamp:
  if (ms->x < 0)               ms->x = 0;
  if (ms->y < 0)               ms->y = 0;
  if (ms->x >= (int16_t)hres)  ms->x = hres - 1;
  if (ms->y >= (int16_t)vres)  ms->y = vres - 1;
}

static mouse_state_t ms;
static uint8_t  mouse_buf[3];
static uint8_t  mouse_idx          = 0;
static bool     mouse_packet_ready = false;

mouse_state_t *get_mouse_state(void)        { return &ms; }
uint8_t       *get_mouse_buf(void)          { return mouse_buf; }
uint8_t        get_mouse_idx(void)          { return mouse_idx; }
bool           get_mouse_packet_ready(void) { return mouse_packet_ready; }
void           set_mouse_idx(uint8_t val)       { mouse_idx = val; }
void           set_mouse_packet_ready(bool val) { mouse_packet_ready = val; }
