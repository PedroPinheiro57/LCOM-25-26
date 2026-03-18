#include <lcom/lcf.h>

#include <stdint.h>

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
  if (lsb == NULL) return 1;
  *lsb = (uint8_t)(val & 0xFF); 
  return 0;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  if (msb == NULL) return 1;
  *msb = (uint8_t)(val >> 8);
  return 0;
}

int (util_sys_inb)(int port, uint8_t *value) {
  // Temporary 32-bit storage 
	// sys_inb requires a uint32_t pointer. Using temp ensures that
	// if sys_inb fails, the original *value is not overwritten with garbage.
	uint32_t temp; 

  if (sys_inb(port, &temp) != 0) {
    return 1; 
  }

  *value = (uint8_t) temp; // truncate
  return 0; 
}
