#include <lcom/lcf.h>
#include "kbc.h"
#include "i8042.h"

static int hook_id = KBD_IRQ;
static uint8_t scancode = 0;   
static bool error_kbc = false;

int kbd_subscribe_int(uint8_t *bit_no) {
  *bit_no = hook_id;
  return sys_irqsetpolicy(KBD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id);
}

int kbd_unsubscribe_int() {
  return sys_irqrmpolicy(&hook_id);
}

void (kbc_ih)() {
  uint8_t status;
  error_kbc = false;

  // 1. Read the Status Register
  if (util_sys_inb(KBC_ST_REG, &status) != 0) {
    error_kbc = true;
    return;
  }

  // 2. Check if there is data in the Output Buffer
  if (status & OBF) {
    // 3. Read the Output Buffer (the scancode)
    if (util_sys_inb(KBC_OUT_BUF, &scancode) != 0) {
      error_kbc = true;
      return;
    }
    
    // 4. Check for Parity or Timeout errors
    if ((status & (PARITY_ERR | TIMEOUT_ERR)) != 0) {
      error_kbc = true;
    }
  } else {
    error_kbc = true;
  }
}

uint8_t kbd_get_scancode() {
  return scancode;
}

bool kbd_has_error() {
  return error_kbc;
}
