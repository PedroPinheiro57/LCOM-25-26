#include "bitwise.h"
#include <stdarg.h> 

#define MSK_END -1

uint8_t clear(uint8_t msk, int pos) { 
  return msk &= ~(1<<pos); 
}

uint8_t set(uint8_t msk, int pos) {
  return msk |= (1<<pos);
}

bool is_set(uint8_t msk, int pos) {
  return msk & (1<<pos);
}

uint8_t lsb(uint16_t wide_msk) {
  return (uint8_t) (wide_msk & 0xFF);
}

uint8_t msb(uint16_t wide_msk) {
  return (uint8_t) (wide_msk >>= 8);
}

uint8_t mask(int pos, ...) {
  	uint8_t result = 0;
    va_list args;
    va_start(args, pos);

    while (pos != MSK_END) {
        if (pos >= 0 && pos < 8) {
            result |= (1 << pos);  
        }
        pos = va_arg(args, int); 
    }

    va_end(args);
    return result;
}
