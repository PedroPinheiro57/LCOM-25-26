#include "bitwise.h"
#include <stdarg.h>  

#define MSK_END -1

uint8_t clear(uint8_t msk, int pos) {
    uint8_t bitmask = ~(1 << pos); // 1 << 4 = 10000. ~(1<<4) = 01111
    return msk & bitmask; // zero the bit at pos
}

uint8_t set(uint8_t msk, int pos) { 
    uint8_t bitmask = 1 << pos;
    return msk | bitmask; 
}

bool is_set(uint8_t msk, int pos) { 
    return (msk & (1 << pos)) != 0;
}

uint8_t lsb(uint16_t wide_msk) {  
	 return (uint8_t)(wide_msk & 0xFF);
}

uint8_t msb(uint16_t wide_msk) { 
	return (uint8_t)(wide_msk >>= 8);
}

uint8_t mask(int pos, ...) { 
	uint8_t result = 0;
    va_list args;
    va_start(args, pos);

    while (pos != MSK_END) {
        if (pos >= 0 && pos < 8) {
            result |= (1 << pos);  // set the bit at position pos
        }
        pos = va_arg(args, int);  // get next argument
    }

    va_end(args);
    return result;
}
