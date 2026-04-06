#include <lcom/lcf.h>
#include <lcom/lab3.h>
#include <stdbool.h>
#include <stdint.h>

#include "kbc.h"
#include "i8042.h"

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/grupo_2leic01_5/jorge/lab2/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/grupo_2leic01_5/jorge/lab2/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}




int(kbd_test_scan)() {
  int ipc_status;
  message msg;
  uint8_t bit_no;
  
  // 1. Subscribe to interrupts
  if (kbd_subscribe_int(&bit_no) != 0) {
    printf("Error subscribing to keyboard interrupts.\n");
    return 1;
  }

  uint32_t irq_set = BIT(bit_no);
  
  uint8_t bytes[2];
  uint8_t size = 0;
  bool make = false;

  // 2. Main loop
  while (kbd_get_scancode() != ESC_BREAKCODE) {
    
    if (driver_receive(ANY, &msg, &ipc_status) != 0) {
      printf("driver_receive failed\n");
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            
            kbc_ih();

            if (kbd_has_error()) continue; 

            uint8_t current_scancode = kbd_get_scancode(); 

            bytes[size] = current_scancode;
            size++;

            if (current_scancode == TWO_BYTE_PREFIX) {
              continue; 
            }

            make = !(current_scancode & BIT(7)); 
            kbd_print_scancode(make, size, bytes);
            
            size = 0; 
          }
          break;
        default:
          break;
      }
    }
  }

  // 3. Unsubscribe from interrupts
  if (kbd_unsubscribe_int() != 0) {
    printf("Error unsubscribing from keyboard interrupts.\n");
    return 1;
  }

  return 0;
}




int(kbd_test_poll)() {
  /* To be completed by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}

int(kbd_test_timed_scan)(uint8_t n) {
  /* To be completed by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}
