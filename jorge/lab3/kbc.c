#include <lcom/lcf.h>
#include "kbc.h"
#include "i8042.h"
#include <minix/sysutil.h>

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

int kbc_read_data_poll(uint8_t *data) {
    uint8_t status;
    int tries = 10;

    while (tries > 0) {
        if (util_sys_inb(KBC_ST_REG, &status) != 0) return 1;

        if (status & OBF) {
            if (util_sys_inb(KBC_OUT_BUF, data) != 0) return 1;

            if ((status & (PARITY_ERR | TIMEOUT_ERR)) == 0) {
                return 0; 
            } else {
                return 1; 
            }
        }
        tickdelay(micros_to_ticks(DELAY_US));
        tries--;
    }
    return 1;
}

int kbc_write_command(uint8_t port, uint8_t cmd) {
    uint8_t status;
    int tries = 10;

    while (tries > 0) {
        if (util_sys_inb(KBC_ST_REG, &status) != 0) return 1;

        if ((status & IBF) == 0) { 
            return sys_outb(port, cmd); 
        }
        tickdelay(micros_to_ticks(DELAY_US));
        tries--;
    }
    return 1; 
}

int kbc_restore_keyboard_interrupts() {
    uint8_t command_byte;

    if (kbc_write_command(KBC_CMD_REG, KBC_READ_CMD) != 0) return 1;

    if (kbc_read_data_poll(&command_byte) != 0) return 1;

    command_byte = command_byte | INT_KBD;

    if (kbc_write_command(KBC_CMD_REG, KBC_WRITE_CMD) != 0) return 1;

    if (kbc_write_command(KBC_IN_BUF, command_byte) != 0) return 1;

    return 0;
}
