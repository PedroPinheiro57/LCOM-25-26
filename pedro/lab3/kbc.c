#include <lcom/lcf.h>
#include <minix/sysutil.h>
#include "kbc.h"

/* hook_id is modified by sys_irqsetpolicy - must keep its value for unsubscribe */
static int hook_id_kbd = 1;

/* Data written by kbc_ih(), read by the test functions */
static uint8_t scancode_byte = 0;
static bool    ih_error      = false;

/* ------------------------------------------------------------------
 * kbc_subscribe_int
 * Subscribes to IRQ 1 (keyboard) with IRQ_REENABLE | IRQ_EXCLUSIVE.
 * IRQ_EXCLUSIVE is critical: it stops Minix's own keyboard driver
 * from receiving the interrupt and draining the output buffer before
 * our code can read it.
 * ------------------------------------------------------------------ */
int kbc_subscribe_int(uint8_t *bit_no) {
  if (bit_no == NULL) return 1;
  *bit_no = hook_id_kbd;
  if (sys_irqsetpolicy(KBD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id_kbd) != 0)
    return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * kbc_unsubscribe_int
 * Removes the IRQ policy. Must be called before the program exits
 * so Minix's keyboard driver gets control back.
 * ------------------------------------------------------------------ */
int kbc_unsubscribe_int(void) {
  if (sys_irqrmpolicy(&hook_id_kbd) != 0) return 1;
  return 0;
}

/* ------------------------------------------------------------------
 * kbc_ih  (Interrupt Handler)
 * Called every time IRQ 1 fires (one byte arrived from keyboard).
 *
 * Steps:
 *   1. Read status register - check OBF and error bits.
 *   2. Always read the output buffer (even on error) to drain it,
 *      otherwise the KBC will never send the next byte.
 *   3. If any error flag is set, mark ih_error = true and discard.
 *   4. If data is from the mouse (AUX bit), discard it too.
 *   5. Otherwise store the byte in scancode_byte for the caller.
 * ------------------------------------------------------------------ */
void (kbc_ih)() {
  uint8_t st;
  ih_error = false;

  // Read status register
  if (util_sys_inb(KBC_ST_REG, &st) != 0) { ih_error = true; return; }
  if (!(st & KBC_OBF))                     { ih_error = true; return; }

  uint8_t data;
  if (util_sys_inb(KBC_OUT_BUF, &data) != 0) { ih_error = true; return; }

  if (st & (KBC_PARITY | KBC_TIMEOUT)) { ih_error = true; return; }
  if (st & KBC_AUX)                    { ih_error = true; return; }

  scancode_byte = data;
}

uint8_t kbc_get_scancode_byte(void) { return scancode_byte; }
bool    kbc_has_error(void)         { return ih_error; }

/* ------------------------------------------------------------------
 * kbc_read_byte  (used by polling)
 * Spins reading the status register until OBF=1 (data available),
 * then reads and validates the byte.
 * Retries up to KBC_MAX_TRIES times with a delay between each try.
 * ------------------------------------------------------------------ */
#ifdef LAB3
static uint32_t sysinb_count = 0;
void kbc_reset_sysinb_count(void) { sysinb_count = 0; }
uint32_t kbc_get_sysinb_count(void) { return sysinb_count; }
#endif

int kbc_read_byte(uint8_t *byte) {
  uint8_t st, data;
  for (int i = 0; i < KBC_MAX_TRIES; i++) {
#ifdef LAB3
    sysinb_count++;   /* counting the status register read */
#endif
    if (util_sys_inb(KBC_ST_REG, &st) != 0) return 1;
    if (st & KBC_OBF) {
#ifdef LAB3
      sysinb_count++;   /* counting the data register read */
#endif
      if (util_sys_inb(KBC_OUT_BUF, &data) != 0) return 1;
      if (st & (KBC_PARITY | KBC_TIMEOUT))         return 1;
      if (st & KBC_AUX)                            return 1;
      *byte = data;
      return 0;
    }
    tickdelay(micros_to_ticks(KBC_DELAY_US));
  }
  return 1;
}

/* ------------------------------------------------------------------
 * kbc_issue_cmd
 * Waits until the KBC input buffer is empty (IBF=0), then writes
 * a command to the command register (0x64).
 * We must wait because writing while IBF=1 would corrupt the command.
 * ------------------------------------------------------------------ */
int kbc_issue_cmd(uint8_t cmd) {
  uint8_t st;
  for (int i = 0; i < KBC_MAX_TRIES; i++) {
    if (util_sys_inb(KBC_ST_REG, &st) != 0) return 1;
    if (!(st & KBC_IBF)) {
      if (sys_outb(KBC_CMD_REG, cmd) != 0) return 1;
      return 0;
    }
    tickdelay(micros_to_ticks(KBC_DELAY_US));
  }
  return 1;
}

/* ------------------------------------------------------------------
 * kbc_write_arg
 * Waits until IBF=0, then writes an argument to the input buffer
 * (0x60). Used after kbc_issue_cmd() for commands that take an arg.
 * ------------------------------------------------------------------ */
int kbc_write_arg(uint8_t arg) {
  uint8_t st;
  for (int i = 0; i < KBC_MAX_TRIES; i++) {
    if (util_sys_inb(KBC_ST_REG, &st) != 0) return 1;
    if (!(st & KBC_IBF)) {
      if (sys_outb(KBC_IN_BUF, arg) != 0) return 1;
      return 0;
    }
    tickdelay(micros_to_ticks(KBC_DELAY_US));
  }
  return 1;
}

/* ------------------------------------------------------------------
 * kbc_enable_int
 * Re-enables keyboard interrupts by updating the KBC Command Byte.
 * Required at the end of kbd_test_poll() because lcf_start() disables
 * KBC interrupts before calling it. Without this the Minix terminal
 * is frozen after our program exits.
 *
 * Steps: read current Command Byte → set INT bit → write it back.
 * ------------------------------------------------------------------ */
int kbc_enable_int(void) {
  if (kbc_issue_cmd(KBC_READ_CMD) != 0) return 1;
  uint8_t cmd_byte;
  if (kbc_read_byte(&cmd_byte) != 0) return 1;
  cmd_byte |= KBC_INT_KBD;
  if (kbc_issue_cmd(KBC_WRITE_CMD) != 0) return 1;
  if (kbc_write_arg(cmd_byte) != 0) return 1;
  return 0;
}




/* --------------------------------------- */
/* PROJECT */
/* --------------------------------------- */

bool key_is_make(uint8_t sc) {
  return !(sc & 0x80);
}

uint8_t key_get_code(uint8_t sc) {
  return sc & 0x7F;
}
