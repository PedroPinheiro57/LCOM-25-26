#include <lcom/lcf.h>

#include <stdint.h>

int(util_get_LSB)(uint16_t val, uint8_t *lsb) {
  /* To be implemented by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
}

int(util_get_MSB)(uint16_t val, uint8_t *msb) {
  /* To be implemented by the students */
  printf("%s is not yet implemented!\n", __func__);

  return 1;
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
