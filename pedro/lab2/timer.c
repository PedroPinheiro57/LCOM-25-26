#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"


int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  // 1. Boundary Check: The 8254 divider is 16 bits. 
  // Max divider is 65535. Min freq = TIMER_FREQ / 65535 ≈ 19 Hz.
  if (freq < 19 || freq > TIMER_FREQ) return 1;

  // 2. Get current configuration to preserve Mode and BCD
  uint8_t st;
  if (timer_get_conf(timer, &st) != 0) return 1;

  // 3. Build the Control Word
  // We MUST mask the status: keep only the 4 LSBs (Operating Mode and BCD)
  // Bits 3, 2, 1 are Mode; Bit 0 is BCD.
  uint8_t control_word = (st & 0x0F);

  // 4. Set Access Mode (Bits 5, 4) to LSB followed by MSB (0x30)
  control_word |= TIMER_LSB_MSB;

  // 5. Set Timer Selection (Bits 7, 6) and identify the data port
  int port;
  switch (timer) {
    case 0:
      control_word |= TIMER_SEL0;
      port = TIMER_0;
      break;
    case 1:
      control_word |= TIMER_SEL1;
      port = TIMER_1;
      break;
    case 2:
      control_word |= TIMER_SEL2;
      port = TIMER_2;
      break;
    default:
      return 1;
  }

  // 6. Write the Control Word to the Control Register (0x43)
  if (sys_outb(TIMER_CTRL, control_word) != 0) return 1;

  // 7. Calculate and write the divider
  uint16_t divider = (uint16_t) (TIMER_FREQ / freq);
  uint8_t lsb, msb;
  
  util_get_LSB(divider, &lsb);
  util_get_MSB(divider, &msb);

  // Write LSB first, then MSB to the specific timer port
  if (sys_outb(port, lsb) != 0) return 1;
  if (sys_outb(port, msb) != 0) return 1;

  return 0;
}



int hook_id = 0; // Value passed to the kernel to identify the interrupt
static int timer_counter = 0; // Tracks the number of timer interrupts

uint32_t timer_get_counter() {
  return timer_counter;
}

void timer_reset_counter() {
  timer_counter = 0;
}

void (timer_int_handler)() {
  timer_counter++;
}


int (timer_subscribe_int)(uint8_t *bit_no) {
  if (bit_no == NULL) return 1;
  
  // The kernel will send notifications with the bit corresponding to the initial hook_id
  *bit_no = hook_id; 
  
  // Subscribe to Timer 0 interrupts
  if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id) != 0) {
    return 1;
  }
  
  return 0;
}



int (timer_unsubscribe_int)() {
  // Remove the policy using the modified hook_id
  if (sys_irqrmpolicy(&hook_id) != 0) {
    return 1;
  }
  return 0;
}



int (timer_get_conf)(uint8_t timer, uint8_t *st) {

  // Read-Back Command (i8254):
  // |7 6|5|4|3|2|1|0|
  // |1 1|C|S|T2|T1|T0|X|
  // 11 → Read-Back command
  // C (bit 5) → 0 = latch count, 1 = don't latch count
  // S (bit 4) → 0 = latch status, 1 = don't latch status
  // T2,T1,T0 → select timer(s) (1 = selected)
  // bit 0 → unused 


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
          //remap if needed ????????
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


int foo(){
  return 0;
}
