#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  /* To be implemented by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
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

  // Read-Back Command (i8254):
  // |7 6|5|4|3|2|1|0|
  // |1 1|C|S|T2|T1|T0|X|
  // 11 → Read-Back command
  // C (bit 5) → 0 = latch count, 1 = don't latch count
  // S (bit 4) → 0 = latch status, 1 = don't latch status
  // T2,T1,T0 → select timer(s) (1 = selected)
  // bit 0 → unused (don't care)


  // Status byte (i8254):
  // |7   |6    |5 4     |3 2 1   |0   |
  // |OUT |NULL |  RW    | MODE   |BCD |
  // OUT  → current output of the timer (1 or 0)
  // NULL → 1 if count not yet loaded, 0 if valid
  // RW   → access mode (LSB, MSB, or LSB followed by MSB)
  // MODE → operating mode (0–5)
  // BCD  → 0 = binary, 1 = BCD


  uint8_t rb_command = TIMER_RB_CMD;  

  // Bit 5: 1 means DO NOT latch count.
  rb_command |= BIT(5); 

  // Bit 4: 0 means LATCH status. (It's already 0)

  // Select the timer 
  switch(timer) {
      case 0: rb_command |= BIT(1); break; // Timer 0
      case 1: rb_command |= BIT(2); break; // Timer 1
      case 2: rb_command |= BIT(3); break; // Timer 2
      default: return 1; // invalid timer
  }

  // write Read-Back command to timer control port
  if(sys_outb(TIMER_CTRL, rb_command) != 0) return 1;

  // read from correct timer port
  int port = (timer == 0) ? TIMER_0 : (timer == 1) ? TIMER_1 : TIMER_2;
  uint8_t temp;
  
  if(util_sys_inb(port, &temp) != 0) return 1;

  *st = temp; // store status byte
  return 0;
}

int (timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {
  union timer_status_field_val value;

  switch(field) {

      // status byte
      case tsf_all:
          value.byte = st;
          break;

      // initialization mode
      case tsf_initial: {
          uint8_t access = (st & (TIMER_LSB | TIMER_MSB));
          switch(access) {
              case TIMER_LSB: value.in_mode = LSB_only; break;
              case TIMER_MSB: value.in_mode = MSB_only; break;
              case TIMER_LSB_MSB: value.in_mode = MSB_after_LSB; break;
              default: value.in_mode = INVAL_val; break;
          }
          break;
      }

      // counting mode
      case tsf_mode:
          value.count_mode = (st >> 1) & 0x07; // bits 3,2,1
          if (value.count_mode == 6) value.count_mode = 2; // mode 2
          else if (value.count_mode == 7) value.count_mode = 3; // mode 3
          break;

      // counting base
      case tsf_base:
          value.bcd = st & 0x01; // bit 0
          break;
  }

  return timer_print_config(timer, field, value);
}
