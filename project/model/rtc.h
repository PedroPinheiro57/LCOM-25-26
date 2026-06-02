#pragma once
#include <stdint.h>

#define RTC_ADDR  0x70
#define RTC_DATA  0x71

#define RTC_REG_A   0x0A
#define RTC_REG_B   0x0B
#define RTC_SEC     0x00
#define RTC_MIN     0x02
#define RTC_HOUR    0x04
#define RTC_DAY     0x07
#define RTC_MONTH   0x08
#define RTC_YEAR    0x09

#define RTC_UIP  BIT(7)   /* update in progress */
#define RTC_DM   BIT(2)   /* data mode: 1=binary, 0=BCD */


typedef struct {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
} rtc_time_t;

typedef struct {
  uint8_t day;
  uint8_t month;
  uint8_t year;
} rtc_date_t;

int rtc_read_time(rtc_time_t *t);
int rtc_read_date(rtc_date_t *d);
