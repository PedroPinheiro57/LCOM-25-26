#include "keyboard.h"

bool key_is_make(uint8_t sc) {
  return !(sc & 0x80);
}

uint8_t key_get_code(uint8_t sc) {
  return sc & 0x7F;
}
