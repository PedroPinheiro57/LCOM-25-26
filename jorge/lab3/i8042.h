#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_

#include <lcom/lcf.h>

#define KBD_IRQ         1       // Keyboard interrupt line (IRQ)

#define KBC_ST_REG      0x64    // KBC Status Register address
#define KBC_OUT_BUF     0x60    // KBC Output Buffer address

// --- Specific Codes ---
#define ESC_BREAKCODE   0x81    // ESC key breakcode
#define TWO_BYTE_PREFIX 0xE0    // 2-byte scancode prefix

#define OBF             BIT(0)  // Output Buffer Full (Data available for reading)
#define TIMEOUT_ERR     BIT(6)  // Timeout Error
#define PARITY_ERR      BIT(7)  // Parity Error

#endif /* _LCOM_I8042_H_ */
