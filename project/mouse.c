#include "devices/mouse.h"

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

  ms->clicked  = (!prev_lb && ms->lb);   /* rising edge */
  ms->released = (prev_lb && !ms->lb);   /* falling edge */

  int16_t dx = buf[1];
  int16_t dy = buf[2];

  if (b1.fields.x_sign) dx |= 0xFF00;
  if (b1.fields.y_sign) dy |= 0xFF00;

  ms->x += dx;
  ms->y -= dy;   /* PS/2 Y is inverted relative to screen */

  /* clamp to screen boundaries */
  if (ms->x < 0)            ms->x = 0;
  if (ms->y < 0)            ms->y = 0;
  if (ms->x >= (int16_t) hres) ms->x = hres - 1;
  if (ms->y >= (int16_t) vres) ms->y = vres - 1;

  ms->moved = (dx != 0 || dy != 0);
}
