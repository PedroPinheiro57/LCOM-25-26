#include "rtc.h"
#include <minix/syslib.h>
#define TODO return -1

#define RTC_ADDR_REG 0x70
#define RTC_DATA_REG 0x71
#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define RTC_REG_DAY 0x07
#define RTC_REG_MONTH 0x08
#define RTC_REG_YEAR 0x09
#define RTC_UIP_MSK (1 << 7)
#define RTC_DM_MSK (1 << 2)

static int bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

int rtc_read_date(rtc_date *date) {

    uint32_t reg_a, reg_b, day, month, year;

    // 1. Wait for UIP to be cleared
    while (reg_a & RTC_UIP_MSK) {
		if (sys_outb(RTC_ADDR_REG, RTC_REG_A) != 0) return 1;
        if (sys_inb(RTC_DATA_REG, &reg_a) != 0) return 1;
		tickdelay(micros_to_ticks(200));
	}
    
    // 2. Read the Date Registers
    // Read Day
    sys_outb(RTC_ADDR_REG, RTC_REG_DAY);
    sys_inb(RTC_DATA_REG, &day);
    
    // Read Month
    sys_outb(RTC_ADDR_REG, RTC_REG_MONTH);
    sys_inb(RTC_DATA_REG, &month);
    
    // Read Year
    sys_outb(RTC_ADDR_REG, RTC_REG_YEAR);
    sys_inb(RTC_DATA_REG, &year);

    // 3. Check Data Mode (Binary vs BCD) in Register B
    sys_outb(RTC_ADDR_REG, RTC_REG_B);
    sys_inb(RTC_DATA_REG, &reg_b);

    // If DM bit is 0, the data is in BCD and needs conversion
    if (!(reg_b & RTC_DM_MSK)) {
        date->day = (uint8_t)bcd_to_bin((uint8_t)day);
        date->month = (uint8_t)bcd_to_bin((uint8_t)month);
        date->year = (uint8_t)bcd_to_bin((uint8_t)year);
    } else {
        date->day = (uint8_t)day;
        date->month = (uint8_t)month;
        date->year = (uint8_t)year;
    }

    return 0;
  }