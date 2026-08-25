#ifndef LDG_TIME_CAL_H
#define LDG_TIME_CAL_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_TIME_SECS_PER_DAY 86400
#define LDG_TIME_MINS_PER_HOUR 60
#define LDG_TIME_HOURS_PER_DAY 24
#define LDG_TIME_MONTHS_PER_YEAR 12
#define LDG_TIME_DAYS_PER_WEEK 7

#define LDG_TIME_MONTH_JANUARY 1
#define LDG_TIME_MONTH_DECEMBER 12

#define LDG_TIME_WEEKDAY_THURSDAY 4

#define LDG_TIME_EPOCH_YEAR 1970

typedef struct ldg_time_cal
{
    uint32_t year;
    uint16_t year_day;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
} ldg_time_cal_t;

LDG_EXPORT uint8_t ldg_time_leap_year_is(uint32_t year);
LDG_EXPORT uint32_t ldg_time_month_day_cunt_get(uint32_t year, uint8_t month, uint8_t *out);
LDG_EXPORT uint32_t ldg_time_cal_get(uint64_t epoch_sec, ldg_time_cal_t *out);
LDG_EXPORT uint32_t ldg_time_epoch_sec_get(const ldg_time_cal_t *cal, uint64_t *out);
LDG_EXPORT uint32_t ldg_time_cal_fmt(const ldg_time_cal_t *cal, char *dst, uint64_t cap, uint64_t *out_len);

#endif
