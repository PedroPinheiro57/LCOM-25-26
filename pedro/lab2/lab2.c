#include <lcom/lcf.h>
#include <lcom/lab2.h>

#include <stdbool.h>
#include <stdint.h>


int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab2/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab2/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

int(timer_test_read_config)(uint8_t timer, enum timer_status_field field) {
  uint8_t conf;

  // read timer configuration
  if (timer_get_conf(timer, &conf) != 0) {
      printf("Error reading timer %d configuration\n", timer);
      return 1;
  }
  
  // display requested field
  if (timer_display_conf(timer, conf, field) != 0) {
      printf("Error displaying timer %d configuration\n", timer);
      return 1;
  }

  return 0;
}

int(timer_test_time_base)(uint8_t timer, uint32_t freq) {

  if (timer_set_frequency(timer, freq) != 0) {
      printf("Failed to set timer %d frequency to %d Hz\n", timer, freq);
      return 1;
  }
  
  return 0;
}

extern int timer_counter; // Import the global counter from timer.c

// TEST: minix$ lcom_run lab2 "int 3 -t 0"
int(timer_test_int)(uint8_t time) {
  uint8_t bit_no;
  
  // 1. Subscribe to Timer 0 interrupts
  if (timer_subscribe_int(&bit_no) != 0) {
    printf("Failed to subscribe to timer interrupts.\n");
    return 1;
  }

  uint32_t irq_set = BIT(bit_no); // Create the bitmask for testing the message
  int ipc_status, r;
  message msg;

  // 2. Interrupt loop
  while (time > 0) { 
    // Get a request message.
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) { 
        printf("driver_receive failed with: %d", r);
        continue;
    }
    
    if (is_ipc_notify(ipc_status)) { /* received notification */
        switch (_ENDPOINT_P(msg.m_source)) {
            case HARDWARE: /* hardware interrupt notification */				
                if (msg.m_notify.interrupts & irq_set) { /* subscribed interrupt */
                    
                    timer_int_handler(); // Increment the counter
                    
                    // Minix default timer frequency is 60Hz (60 ticks per second)
                    if (timer_counter % 60 == 0) { 
                        timer_print_elapsed_time();
                        time--; // Decrement the seconds remaining
                    }
                }
                break;
            default:
                break; /* no other notifications expected: do nothing */	
        }
    } 
  }

  // 3. Unsubscribe from Timer 0 interrupts
  if (timer_unsubscribe_int() != 0) {
    printf("Failed to unsubscribe from timer interrupts.\n");
    return 1;
  }

  return 0;
}
