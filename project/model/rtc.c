#include <lcom/lcf.h>
#include "rtc.h"


static uint8_t bcd_to_bin(uint8_t bcd) {
  return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static int rtc_read_reg(uint8_t reg, uint8_t *val) {
  if (sys_outb(RTC_ADDR, reg) != 0) return 1;
  if (util_sys_inb(RTC_DATA, val)  != 0) return 1;
  return 0;
}

static void wait_uip(void) {
  uint8_t reg_a;
  do {
    sys_outb(RTC_ADDR, RTC_REG_A);
    util_sys_inb(RTC_DATA, &reg_a);
  } while (reg_a & RTC_UIP);
}

static bool is_bcd(void) {
  uint8_t reg_b;
  sys_outb(RTC_ADDR, RTC_REG_B);
  util_sys_inb(RTC_DATA, &reg_b);
  return !(reg_b & RTC_DM);   /* DM=0 means BCD */
}

int rtc_read_time(rtc_time_t *t) {
  wait_uip();
  uint8_t h, m, s;
  if (rtc_read_reg(RTC_HOUR, &h) != 0) return 1;
  if (rtc_read_reg(RTC_MIN,  &m) != 0) return 1;
  if (rtc_read_reg(RTC_SEC,  &s) != 0) return 1;

  if (is_bcd()) {
    t->hours   = bcd_to_bin(h);
    t->minutes = bcd_to_bin(m);
    t->seconds = bcd_to_bin(s);
  } else {
    t->hours   = h;
    t->minutes = m;
    t->seconds = s;
  }
  return 0;
}

int rtc_read_date(rtc_date_t *d) {
  wait_uip();
  uint8_t day, month, year;
  if (rtc_read_reg(RTC_DAY,   &day)   != 0) return 1;
  if (rtc_read_reg(RTC_MONTH, &month) != 0) return 1;
  if (rtc_read_reg(RTC_YEAR,  &year)  != 0) return 1;

  if (is_bcd()) {
    d->day   = bcd_to_bin(day);
    d->month = bcd_to_bin(month);
    d->year  = bcd_to_bin(year);
  } else {
    d->day   = day;
    d->month = month;
    d->year  = year;
  }
  return 0;
}
