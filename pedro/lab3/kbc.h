#pragma once
#include <stdint.h>
#include <stdbool.h>

/* KBC ports */
#define KBC_OUT_BUF   0x60
#define KBC_IN_BUF    0x60
#define KBC_ST_REG    0x64
#define KBC_CMD_REG   0x64

/* Status register bits */
#define KBC_OBF       BIT(0)   /* Output buffer full - data ready to read */
#define KBC_IBF       BIT(1)   /* Input buffer full  - don't write yet    */
#define KBC_AUX       BIT(5)   /* Mouse data (not keyboard)               */
#define KBC_TIMEOUT   BIT(6)   /* Timeout error - discard byte            */
#define KBC_PARITY    BIT(7)   /* Parity error  - discard byte            */

/* KBC commands (written to 0x64) */
#define KBC_READ_CMD  0x20     /* Read current Command Byte  */
#define KBC_WRITE_CMD 0x60     /* Write new Command Byte     */

/* Command Byte bits */
#define KBC_INT_KBD   BIT(0)   /* Enable keyboard interrupts */

/* Scancode constants */
#define ESC_BREAKCODE  0x81    /* Breakcode of ESC key - termination condition */
#define TWOBYTE_PREFIX 0xE0    /* First byte of a 2-byte scancode             */

/* Keyboard IRQ line */
#define KBD_IRQ        1

/* Retry/delay config for polling loops */
#define KBC_DELAY_US   20000
#define KBC_MAX_TRIES  10

/* Subscribe/unsubscribe KBC interrupts */
int(kbc_subscribe_int)(uint8_t *bit_no);
int(kbc_unsubscribe_int)(void);

/* Interrupt handler - reads one byte from KBC output buffer */
void (kbc_ih)(void);

/* Poll KBC output buffer until a valid byte arrives */
int kbc_read_byte(uint8_t *byte);

/* Issue a command to the KBC command register (0x64) */
int kbc_issue_cmd(uint8_t cmd);

/* Write an argument to the KBC input buffer (0x60) */
int kbc_write_arg(uint8_t arg);

/* Re-enable keyboard interrupts via KBC Command Byte (used after polling) */
int kbc_enable_int(void);

/* Getters for data set by kbc_ih() */
uint8_t kbc_get_scancode_byte(void);
bool    kbc_has_error(void);

/* sys_inb call counter (only active when LAB3 is defined) */
void kbc_reset_sysinb_count(void);
uint32_t kbc_get_sysinb_count(void);


/* --------------------------------------- */
/* PROJECT */
/* --------------------------------------- */

/* scancodes (makecodes) */
#define KEY_ESC    0x01
#define KEY_ENTER  0x1C
#define KEY_UP     0x48
#define KEY_DOWN   0x50
#define KEY_LEFT   0x4B
#define KEY_RIGHT  0x4D
#define KEY_R      0x13
#define KEY_SPACE  0x39

bool    key_is_make(uint8_t scancode);
uint8_t key_get_code(uint8_t scancode);
