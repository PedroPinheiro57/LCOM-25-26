#pragma once
#include <stdint.h>
#include <stdbool.h>

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
