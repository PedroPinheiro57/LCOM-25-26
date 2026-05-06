#pragma once
#include <stdint.h>
#include <stdbool.h>

/* scancodes (makecodes) */
#define KEY_ESC    0x01
#define KEY_ENTER  0x1C
#define KEY_UP     0x48
#define KEY_DOWN   0x50
#define KEY_LEFT   0x4B
#define KEY_RIGHT  0x4D
#define KEY_R      0x13
#define KEY_SPACE  0x39

/* KBC status register as a bitfield union */
typedef union {
  uint8_t val;
  struct kbc_status_fields {
    uint8_t obf     : 1;
    uint8_t ibf     : 1;
    uint8_t sys     : 1;
    uint8_t cmd     : 1;
    uint8_t _unused : 1;
    uint8_t aux     : 1;
    uint8_t timeout : 1;
    uint8_t parity  : 1;
  } fields;
} kbc_status_t;

bool    key_is_make(uint8_t scancode);
uint8_t key_get_code(uint8_t scancode);
