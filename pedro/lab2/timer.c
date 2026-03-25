#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"


// TEST: lcom_run lab2 "time <timer no.> <frequency> -t 0"
// lcom_run lab2 "time 0 60 -t 0"
int (timer_set_frequency)(uint8_t timer, uint32_t freq) {
  // Prevent division by zero and invalid frequencies
  // max number represented with 16 bits: 65535
  // TIMER_FREQ / 65535 = 18.2. ROUND UP
  if (freq > TIMER_FREQ || freq < 19) return 1; 

  uint8_t st;
  // 1. Read the current timer configuration
  if (timer_get_conf(timer, &st) != 0) return 1;

  // 2. Build the new Control Word
  // Keep the 4 least significant bits intact (Operating mode and BCD)
  st = st & 0x0F; 

  // Add the LSB followed by MSB initialization mode
  st |= TIMER_LSB_MSB;

  // Add the correct timer selection and determine the output port
  int port;
  switch (timer) {
    case 0:
      st |= TIMER_SEL0;
      port = TIMER_0;
      break;
    case 1:
      st |= TIMER_SEL1;
      port = TIMER_1;
      break;
    case 2:
      st |= TIMER_SEL2;
      port = TIMER_2;
      break;
    default:
      return 1; // Invalid timer
  }

  // Write the configured Control Word to the Control Register
  if (sys_outb(TIMER_CTRL, st) != 0) return 1;

  // 3. Write the "divider" initial value to the selected timer's port
  uint16_t divider = TIMER_FREQ / freq;
  uint8_t lsb, msb;
  
  // Extract LSB and MSB
  if (util_get_LSB(divider, &lsb) != 0) return 1;
  if (util_get_MSB(divider, &msb) != 0) return 1;

  // Write the LSB first, then the MSB
  if (sys_outb(port, lsb) != 0) return 1;
  if (sys_outb(port, msb) != 0) return 1;

  return 0;
}



int hook_id = 0; // Value passed to the kernel to identify the interrupt
int timer_counter = 0; // Tracks the number of timer interrupts

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

void (timer_int_handler)() {
  timer_counter++;
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
