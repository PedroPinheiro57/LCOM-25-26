#include <lcom/lcf.h>
#include <minix/sysutil.h>
#include "mouse.h"
#include "kbc.h"

/* hook_id is modified by sys_irqsetpolicy - must keep its value for unsubscribe */
static int hook_id_mouse = 12;

/* Data written by mouse_ih(), read by the test functions */
static uint8_t mouse_byte  = 0;
static bool    mouse_error = false;

/* ------------------------------------------------------------------
 * mouse_subscribe_int
 * Subscribes to IRQ 12 (mouse) with IRQ_REENABLE | IRQ_EXCLUSIVE.
 * IRQ_EXCLUSIVE stops Minix's default mouse driver from consuming
 * bytes before our handler can read them.
 * ------------------------------------------------------------------ */
int mouse_subscribe_int(uint8_t *bit_no) {
  if (bit_no == NULL) return 1;
  *bit_no = hook_id_mouse;
  if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id_mouse) != 0)
    return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * mouse_unsubscribe_int
 * Removes the IRQ 12 policy. Must be called before the program exits
 * so Minix's mouse driver gets control back.
 * ------------------------------------------------------------------ */
int mouse_unsubscribe_int(void) {
  if (sys_irqrmpolicy(&hook_id_mouse) != 0) return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * mouse_ih  (Interrupt Handler)
 * Called every time IRQ 12 fires (one byte arrived from mouse).
 *
 * Steps:
 *   1. Read status register — check OBF and error bits.
 *   2. Always drain the output buffer even on error, otherwise the
 *      KBC will never raise the next interrupt.
 *   3. If parity or timeout error flags are set, mark error and discard.
 *   4. Otherwise store the byte for the caller.
 * ------------------------------------------------------------------ */
void (mouse_ih)(void) {
  uint8_t st;
  mouse_error = false;

  if (util_sys_inb(KBC_ST_REG, &st) != 0) { mouse_error = true; return; }
  if (!(st & KBC_OBF))                     { mouse_error = true; return; }

  uint8_t data;
  if (util_sys_inb(KBC_OUT_BUF, &data) != 0) { mouse_error = true; return; }

  if (st & (KBC_PARITY | KBC_TIMEOUT)) { mouse_error = true; return; }

  mouse_byte = data;
}

uint8_t mouse_get_byte(void)  { return mouse_byte;  }
bool    mouse_has_error(void) { return mouse_error; }

/* ------------------------------------------------------------------
 * mouse_send_cmd
 * Sends one command byte to the mouse via the KBC forwarding mechanism:
 *   1. Write KBC_WRITE_MOUSE (0xD4) to the KBC command register (0x64).
 *   2. Write the command byte to the KBC input buffer (0x60).
 *   3. Read the mouse's ACK byte from the output buffer (0x60).
 *      0xFA = success, 0xFE = resend entire command, 0xFC = fatal error.
 *
 * On NACK we retry the whole sequence from step 1, as the spec requires:
 * "If an argument byte elicits an 0xFE response, the host should
 *  retransmit the entire command, not just the argument byte."
 * ------------------------------------------------------------------ */
int mouse_send_cmd(uint8_t cmd) {
  for (int attempt = 0; attempt < KBC_MAX_TRIES; attempt++) {
    if (kbc_issue_cmd(KBC_WRITE_MOUSE) != 0) return 1;
    if (kbc_write_arg(cmd)             != 0) return 1;

    uint8_t ack;
    if (kbc_read_byte(&ack) != 0) return 1;

    if (ack == MOUSE_ACK)   return 0;   /* success */
    if (ack == MOUSE_ERROR) return 1;   /* unrecoverable */
    /* ack == MOUSE_NACK → retry the whole command */
  }
  return 1; /* too many retries */
}

/* ------------------------------------------------------------------
 * mouse_disable_data_reporting
 * Sends command 0xF5 to the mouse, stopping it from sending packets.
 * Must be called before issuing any other mouse command (spec says:
 * "disable the device before sending any other command").
 * Enable data reporting uses the LCF-provided mouse_enable_data_reporting().
 * ------------------------------------------------------------------ */
int mouse_disable_data_reporting(void) {
  return mouse_send_cmd(MOUSE_DISABLE_DR);
}
