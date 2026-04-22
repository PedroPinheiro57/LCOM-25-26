#include <lcom/lcf.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "mouse.h"
#include "kbc.h"


// lcom_run lab4 "packet <N> -t <0-5>"
// lcom_run lab4 "async <T> -t <0-5>"


/* Timer functions from lab2's libtimer.a */
extern int timer_counter;
void (timer_int_handler)(void);
int  (timer_subscribe_int)(uint8_t *bit_no);
int  (timer_unsubscribe_int)(void);

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");
  lcf_log_output("/home/lcom/labs/lab4/output.txt");
  if (lcf_start(argc, argv))
    return 1;
  lcf_cleanup();
  return 0;
}

/* ------------------------------------------------------------------
 * parse_packet
 * Assembles 3 raw bytes into a struct packet, interpreting the PS/2
 * standard packet format:
 *
 *   Byte 1: buttons + sign bits + overflow bits (bit 3 always 1)
 *   Byte 2: X movement (two's complement if X sign bit set)
 *   Byte 3: Y movement (two's complement if Y sign bit set)
 *
 * Two's complement sign extension: if the sign bit is set in byte 1,
 * OR the high byte of the int16_t with 0xFF00 to produce the correct
 * negative value.
 * ------------------------------------------------------------------ */
int foo();
static void parse_packet(uint8_t bytes[3], struct packet *pp) {
  foo();

  pp->bytes[0] = bytes[0];
  pp->bytes[1] = bytes[1];
  pp->bytes[2] = bytes[2];

  pp->lb = bytes[0] & MOUSE_LB;
  pp->rb = bytes[0] & MOUSE_RB;
  pp->mb = bytes[0] & MOUSE_MB;

  pp->delta_x = bytes[1];
  if (bytes[0] & MOUSE_X_SIGN)
    pp->delta_x |= 0xFF00;   /* sign-extend to int16_t */

  pp->delta_y = bytes[2];
  if (bytes[0] & MOUSE_Y_SIGN)
    pp->delta_y |= 0xFF00;

  pp->x_ov = bytes[0] & MOUSE_X_OVF;
  pp->y_ov = bytes[0] & MOUSE_Y_OVF;
}

/* ------------------------------------------------------------------
 * mouse_test_packet  — read and display exactly cnt PS/2 packets
 *
 * 1. Try to disable data reporting first. We ignore failure here
 *    because on a fresh Minix boot data reporting is already off, and
 *    the disable command may NACK in that state. Subscribing IRQ 12
 *    before enabling keeps us safe from missed interrupts.
 * 2. Subscribe mouse interrupts (IRQ 12, exclusive).
 * 3. Enable data reporting so the mouse starts sending packets.
 * 4. On each IRQ 12 interrupt: mouse_ih() reads one byte.
 *    Accumulate 3 bytes into a packet buffer. Synchronise on bit 3
 *    of byte 1 which is always 1: discard any byte at index 0 that
 *    has bit 3 clear, it cannot be a valid first byte.
 * 5. After cnt packets: disable data reporting, unsubscribe, return.
 * ------------------------------------------------------------------ */
int (mouse_test_packet)(uint32_t cnt) {
  /* Best-effort disable before subscribing; ignore failure (may already be off) */
  mouse_disable_data_reporting();

  uint8_t mouse_bit;
  if (mouse_subscribe_int(&mouse_bit) != 0) return 1;

  /* LCF-provided macro — enables data reporting via the real mouse command */
  if (mouse_enable_data_reporting() != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  uint8_t  pkt_buf[3];
  uint8_t  pkt_idx   = 0;
  uint32_t pkt_count = 0;
  int      r, ipc_status;
  message  msg;

  while (pkt_count < cnt) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }
    if (!is_ipc_notify(ipc_status))                  continue;
    if (_ENDPOINT_P(msg.m_source) != HARDWARE)       continue;
    if (!(msg.m_notify.interrupts & BIT(mouse_bit))) continue;

    mouse_ih();
    if (mouse_has_error()) continue;

    uint8_t byte = mouse_get_byte();

    /* Synchronisation: discard bytes at position 0 that lack the sync bit */
    if (pkt_idx == 0 && !(byte & MOUSE_SYNC_BIT)) continue;

    pkt_buf[pkt_idx++] = byte;

    if (pkt_idx == 3) {
      struct packet pp;
      parse_packet(pkt_buf, &pp);
      mouse_print_packet(&pp);
      pkt_idx = 0;
      pkt_count++;
    }
  }

  /* Restore mouse to Minix's default state: data reporting off */
  if (mouse_disable_data_reporting() != 0) {
    mouse_unsubscribe_int();
    return 1;
  }
  if (mouse_unsubscribe_int() != 0) return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * mouse_test_async  — display packets, exit after idle_time seconds
 *
 * Same as mouse_test_packet but uses Timer 0 interrupts to measure
 * inactivity. If no complete packet arrives within idle_time seconds
 * the function exits.
 *
 * Timer logic (mirrors kbd_test_timed_scan from lab3):
 *   We query the actual timer frequency with sys_hz() instead of
 *   hard-coding 60, to be robust against configuration changes.
 *   idle_ticks counts ticks since the last complete packet.
 *   When idle_ticks >= idle_time * freq → exit.
 *
 *   Crucially: we check timer BEFORE mouse in the IRQ dispatch.
 *   This means that when both bits arrive in the same message we
 *   increment idle_ticks first, then immediately reset it to 0 if
 *   a packet completes in the same round. This avoids spurious
 *   early exits caused by batched IRQ delivery.
 *
 * Note: this function implements enable/disable itself via
 * mouse_enable_data_reporting() (LCF macro) and
 * mouse_disable_data_reporting() (our implementation), satisfying
 * the lab requirement that mouse_test_async not rely on the provided
 * mouse_enable_data_reporting helper.
 * ------------------------------------------------------------------ */
int (mouse_test_async)(uint8_t idle_time) {
  /* Best-effort disable before subscribing; ignore failure */
  mouse_disable_data_reporting();

  uint8_t mouse_bit, timer_bit;

  if (mouse_subscribe_int(&mouse_bit) != 0) return 1;

  if (timer_subscribe_int(&timer_bit) != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  if (mouse_enable_data_reporting() != 0) {
    timer_unsubscribe_int();
    mouse_unsubscribe_int();
    return 1;
  }

  /* Get the real timer frequency instead of assuming 60 Hz */
  uint32_t freq = sys_hz();

  uint8_t  pkt_buf[3];
  uint8_t  pkt_idx    = 0;
  uint32_t idle_ticks = 0;
  bool     done       = false;
  int      r, ipc_status;
  message  msg;

  while (!done) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }
    if (!is_ipc_notify(ipc_status))            continue;
    if (_ENDPOINT_P(msg.m_source) != HARDWARE) continue;

    uint32_t irqs = msg.m_notify.interrupts;

    /* --- Timer 0 interrupt (check first so mouse can reset it below) --- */
    if (irqs & BIT(timer_bit)) {
      timer_int_handler();
      idle_ticks++;
      if (idle_ticks >= (uint32_t) idle_time * freq)
        done = true;
    }

    /* --- Mouse interrupt --- */
    if (irqs & BIT(mouse_bit)) {
      mouse_ih();
      if (!mouse_has_error()) {
        uint8_t byte = mouse_get_byte();

        /* Synchronisation: discard bytes at position 0 without sync bit */
        if (pkt_idx == 0 && !(byte & MOUSE_SYNC_BIT)) {
          /* do nothing — keep idle_ticks running */
        } else {
          pkt_buf[pkt_idx++] = byte;

          if (pkt_idx == 3) {
            struct packet pp;
            parse_packet(pkt_buf, &pp);
            mouse_print_packet(&pp);
            pkt_idx    = 0;
            idle_ticks = 0;   /* reset idle counter on every complete packet */
          }
        }
      }
    }
  }

  /* Restore mouse to Minix's default state */
  mouse_disable_data_reporting();
  mouse_unsubscribe_int();
  timer_unsubscribe_int();
  return 0;
}
