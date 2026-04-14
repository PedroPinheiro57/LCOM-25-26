#include <lcom/lcf.h>
#include <lcom/lab3.h>
#include <stdbool.h>
#include <stdint.h>
#include "kbc.h"



// lcom_run lab3 "scan -t <test no.>"       # test no. 0 to 5
// lcom_run lab3 "poll -t <test no.>"       # test no. 0 to 5
// lcom_run lab3 "timed <n> -t <test no.>"  # test no. 0 to 10



/* ------------------------------------------------------------------
 * Timer functions from lab2's libtimer.a
 * We declare them here directly - no .h file needed, the linker
 * will find them in libtimer.a.
 * ------------------------------------------------------------------ */
extern int timer_counter;
void (timer_int_handler)(void);
int  (timer_subscribe_int)(uint8_t *bit_no);
int  (timer_unsubscribe_int)(void);


int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");
  lcf_log_output("/home/lcom/labs/lab3/output.txt");
  if (lcf_start(argc, argv))
    return 1;
  lcf_cleanup();
  return 0;
}

/* ------------------------------------------------------------------
 * process_byte  (shared scancode assembly logic)
 *
 * The KBC sends scancodes one byte at a time. A complete scancode is:
 *   - 1 byte  for most keys
 *   - 2 bytes for special keys (first byte is always 0xE0)
 *
 * We buffer bytes in sc_buf until the scancode is complete, then:
 *   - Determine make (key press) vs break (key release) from MSB of
 *     the last byte: 0 = make, 1 = break.
 *   - Print it with kbd_print_scancode().
 *   - Return true if this was the ESC breakcode (0x81) → caller exits.
 * ------------------------------------------------------------------ */
static uint8_t sc_buf[2];
static uint8_t sc_idx = 0;

static bool process_byte(uint8_t byte) {
  sc_buf[sc_idx++] = byte;

  /* Complete when we have 2 bytes, OR we have 1 byte that is NOT 0xE0 */
  bool complete = (sc_idx == 2) || (sc_idx == 1 && byte != TWOBYTE_PREFIX);

  if (complete) {
    /* Make/break is the MSB of the last byte of the scancode */
    bool is_make = !(sc_buf[sc_idx - 1] & BIT(7));

    kbd_print_scancode(is_make, sc_idx, sc_buf);

    bool esc = (sc_idx == 1 && byte == ESC_BREAKCODE);
    sc_idx = 0; /* reset buffer for next scancode */
    return esc;
  }

  return false; /* incomplete scancode, wait for next byte */
}

/* ------------------------------------------------------------------
 * kbd_test_scan  — interrupt-driven scancode reading
 *
 * 1. Subscribe KBC interrupts on IRQ 1 with IRQ_EXCLUSIVE so Minix's
 *    own keyboard driver doesn't consume our bytes.
 * 2. On each KBC interrupt: kbc_ih() reads one byte from the output
 *    buffer. process_byte() assembles and prints complete scancodes.
 * 3. Exit when ESC breakcode detected. Unsubscribe before returning.
 * ------------------------------------------------------------------ */
int foo();
int(kbd_test_scan)() {
  foo();
  uint8_t kbd_bit;
  if (kbc_subscribe_int(&kbd_bit) != 0) return 1;

  sc_idx = 0;
  bool done = false;
  int r, ipc_status;
  message msg;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status) &&
        _ENDPOINT_P(msg.m_source) == HARDWARE &&
        (msg.m_notify.interrupts & BIT(kbd_bit))) {

      kbc_ih();

      if (!kbc_has_error())
        done = process_byte(kbc_get_scancode_byte());
    }
  }

  if (kbc_unsubscribe_int() != 0) return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * kbd_test_poll  — polling-based scancode reading (no interrupts)
 *
 * lcf_start() disables KBC interrupts before calling this function
 * so Minix's keyboard driver won't interfere.
 *
 * Instead of sleeping in driver_receive(), we actively spin calling
 * kbc_read_byte() which polls the status register until OBF=1.
 *
 * At the end we MUST call kbc_enable_int() to restore keyboard
 * interrupts, otherwise the Minix terminal freezes after we exit.
 *
 * Why more sys_inb() calls than kbd_test_scan()?
 *   Polling reads the status register on every loop iteration even
 *   when no key is pressed. Interrupts only read status + data once
 *   per byte that actually arrives. So polling makes far more calls.
 * ------------------------------------------------------------------ */
int(kbd_test_poll)() {
  sc_idx = 0;
  bool done = false;

#ifdef LAB3
  kbc_reset_sysinb_count();
#endif

  while (!done) {
    uint8_t byte;
    if (kbc_read_byte(&byte) != 0) continue;
    done = process_byte(byte);
  }

#ifdef LAB3
  kbd_print_no_sysinb(kbc_get_sysinb_count());
#endif

  if (kbc_enable_int() != 0) return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * kbd_test_timed_scan  — keyboard interrupts + Timer 0 interrupts
 *
 * Same as kbd_test_scan but with a second exit condition:
 *   if no keyboard byte arrives for n full seconds → exit.
 *
 * Timer logic:
 *   Timer 0 runs at 60 Hz by default (we do NOT change its config).
 *   idle_ticks counts ticks since the last keyboard byte.
 *   When idle_ticks >= n * 60, n seconds have passed with no input.
 *   Any keyboard byte resets idle_ticks to 0.
 *
 * Both the timer and keyboard IRQ bits are checked inside the same
 * driver_receive() loop - this is the standard LCOM way to handle
 * multiple devices at the same time.
 * ------------------------------------------------------------------ */
int(kbd_test_timed_scan)(uint8_t n) {
  uint8_t kbd_bit, timer_bit;

  if (kbc_subscribe_int(&kbd_bit) != 0) return 1;
  if (timer_subscribe_int(&timer_bit) != 0) {
    kbc_unsubscribe_int(); /* clean up already-subscribed IRQ */
    return 1;
  }

  sc_idx = 0;
  bool done     = false;
  int idle_ticks = 0;
  int r, ipc_status;
  message msg;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }
    if (!is_ipc_notify(ipc_status)) continue;
    if (_ENDPOINT_P(msg.m_source) != HARDWARE) continue;

    uint32_t irqs = msg.m_notify.interrupts;

    /* --- Timer 0 interrupt --- */
    if (irqs & BIT(timer_bit)) {
      timer_int_handler();
      idle_ticks++;
      if (idle_ticks >= n * 60)
        done = true;
    }

    /* --- Keyboard interrupt --- */
    if (irqs & BIT(kbd_bit)) {
      kbc_ih();
      if (!kbc_has_error()) {
        idle_ticks = 0; /* reset idle counter on any keyboard activity */
        done = process_byte(kbc_get_scancode_byte());
      }
    }
  }

  kbc_unsubscribe_int();
  timer_unsubscribe_int();
  return 0;
}
