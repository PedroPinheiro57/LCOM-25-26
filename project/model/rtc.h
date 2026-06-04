/**
 * @file rtc.h
 * @brief Real-Time Clock (RTC/CMOS) access for reading the system time and date.
 *
 * Communicates with the PC RTC through the standard CMOS port pair at
 * 0x70/0x71, waiting for the Update-In-Progress flag to clear before
 * reading so the values are always consistent.
 */

#pragma once
#include <stdint.h>

/** @brief CMOS address/command port. */
#define RTC_ADDR  0x70
/** @brief CMOS data port. */
#define RTC_DATA  0x71

/** @name CMOS register indices */
/** @{ */
#define RTC_REG_A  0x0A /**< Status register A (UIP flag, oscillator divider). */
#define RTC_REG_B  0x0B /**< Status register B (data format flags).            */
#define RTC_SEC    0x00 /**< Current seconds. */
#define RTC_MIN    0x02 /**< Current minutes. */
#define RTC_HOUR   0x04 /**< Current hours.   */
#define RTC_DAY    0x07 /**< Day of month.     */
#define RTC_MONTH  0x08 /**< Month.            */
#define RTC_YEAR   0x09 /**< Year (2-digit).   */
/** @} */

/** @brief Bit 7 of register A – Update In Progress.  Poll until clear before reading. */
#define RTC_UIP  BIT(7)
/** @brief Bit 2 of register B – Data Mode: 1 = binary, 0 = BCD. */
#define RTC_DM   BIT(2)

/**
 * @brief Wall-clock time read from the RTC.
 */
typedef struct {
    uint8_t hours;   /**< Hours   (0–23). */
    uint8_t minutes; /**< Minutes (0–59). */
    uint8_t seconds; /**< Seconds (0–59). */
} rtc_time_t;

/**
 * @brief Calendar date read from the RTC.
 */
typedef struct {
    uint8_t day;   /**< Day of the month (1–31). */
    uint8_t month; /**< Month (1–12).            */
    uint8_t year;  /**< Year, last two digits.   */
} rtc_date_t;

/**
 * @brief Reads the current time from the RTC.
 *
 * Waits for the UIP flag to clear and converts BCD to binary if needed.
 *
 * @param t Output struct to fill.
 * @return 0 on success, non-zero on error.
 */
int rtc_read_time(rtc_time_t *t);

/**
 * @brief Reads the current date from the RTC.
 *
 * Waits for the UIP flag to clear and converts BCD to binary if needed.
 *
 * @param d Output struct to fill.
 * @return 0 on success, non-zero on error.
 */
int rtc_read_date(rtc_date_t *d);
