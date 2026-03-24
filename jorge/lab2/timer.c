#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  if (timer > 2 || freq < 19 || freq > TIMER_FREQ) return 1;

  uint8_t st;
  if (timer_get_conf(timer, &st) != 0) return 1;

  uint8_t control_word = (st & 0x0F);
  
  control_word |= (timer << 6) | TIMER_LSB_MSB;

  uint16_t divisor = (uint16_t) (TIMER_FREQ / freq);
  uint8_t lsb, msb;
  util_get_LSB(divisor, &lsb);
  util_get_MSB(divisor, &msb);

  if (sys_outb(TIMER_CTRL, control_word) != 0) return 1;

  if (sys_outb(TIMER_0 + timer, lsb) != 0) return 1;
  if (sys_outb(TIMER_0 + timer, msb) != 0) return 1;

  return 0;
}

int (timer_subscribe_int)(uint8_t *bit_no) {
    /* To be implemented by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}

int (timer_unsubscribe_int)() {
  /* To be implemented by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}

void (timer_int_handler)() {
  /* To be implemented by the students */
  printf("%s is not yet implemented!\n", __func__);
}

int (timer_get_conf)(uint8_t timer, uint8_t *st) {
  if (st == NULL || timer > 2) return 1;

  uint8_t rb_cmd = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);

  if (sys_outb(TIMER_CTRL, rb_cmd) != 0) return 1;
  
  return util_sys_inb(TIMER_0 + timer, st);
}

int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {
  union timer_status_field_val val;

  switch (field) {
    case tsf_all:
      val.byte = st;
      break;
    case tsf_initial:
      val.in_mode = (st >> 4) & 0x03;
      break;
    case tsf_mode:
      val.count_mode = (st >> 1) & 0x07;
      if (val.count_mode == 6) val.count_mode = 2; 
      if (val.count_mode == 7) val.count_mode = 3;
      break;
    case tsf_base:
      val.bcd = st & TIMER_BCD;
      break;
    default: return 1;
  }

  return timer_print_config(timer, field, val);
}
