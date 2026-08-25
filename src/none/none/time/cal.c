#include <stdint.h>

#include <dangling/time/cal.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TIME_CAL_ERA_YEARS 400
#define LDG_TIME_CAL_ERA_DAYS 146097
#define LDG_TIME_CAL_ERA_LAST_DAY 146096
#define LDG_TIME_CAL_CENTURY_DAYS 36524
#define LDG_TIME_CAL_QUAD_DAYS 1460
#define LDG_TIME_CAL_YEAR_DAYS 365
#define LDG_TIME_CAL_LEAP_STEP 4
#define LDG_TIME_CAL_CENTURY_STEP 100
#define LDG_TIME_CAL_EPOCH_SHIFT 719468
#define LDG_TIME_CAL_MONTH_NUMER 153
#define LDG_TIME_CAL_MONTH_BIAS 2
#define LDG_TIME_CAL_MONTH_DENOM 5
#define LDG_TIME_CAL_MARCH_PIVOT 2
#define LDG_TIME_CAL_MARCH_BACK 3
#define LDG_TIME_CAL_MARCH_FORWARD 9
#define LDG_TIME_CAL_MONTH_ROLL 10
#define LDG_TIME_CAL_STEP 1
#define LDG_TIME_CAL_EPOCH_WEEKDAY LDG_TIME_WEEKDAY_THURSDAY

#define LDG_TIME_CAL_FEBRUARY 2
#define LDG_TIME_CAL_SHORT_MONTH_DAYS 30
#define LDG_TIME_CAL_LONG_MONTH_DAYS 31
#define LDG_TIME_CAL_FEBRUARY_DAYS 28
#define LDG_TIME_CAL_FEBRUARY_LEAP_DAYS 29
#define LDG_TIME_CAL_LONG_MONTH_MASK 0x15AAU
#define LDG_TIME_CAL_DIGIT_ZERO '0'
#define LDG_TIME_CAL_DATE_SEP '-'
#define LDG_TIME_CAL_TIME_SEP ':'
#define LDG_TIME_CAL_FIELD_SEP ' '
#define LDG_TIME_CAL_YEAR_WIDTH 4
#define LDG_TIME_CAL_PAIR_WIDTH 2
#define LDG_TIME_CAL_TEXT_LEN 19

uint8_t ldg_time_leap_year_is(uint32_t year)
{
    if ((year % LDG_TIME_CAL_LEAP_STEP) != 0) { return LDG_TRUTH_FALSE; }

    if ((year % LDG_TIME_CAL_CENTURY_STEP) != 0) { return LDG_TRUTH_TRUE; }

    if ((year % LDG_TIME_CAL_ERA_YEARS) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_time_month_day_cunt_get(uint32_t year, uint8_t month, uint8_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(month < LDG_TIME_MONTH_JANUARY || month > LDG_TIME_MONTH_DECEMBER)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (month == LDG_TIME_CAL_FEBRUARY)
    {
        *out = (ldg_time_leap_year_is(year) == LDG_TRUTH_TRUE) ? LDG_TIME_CAL_FEBRUARY_LEAP_DAYS : LDG_TIME_CAL_FEBRUARY_DAYS;

        return LDG_ERR_AOK;
    }

    if ((LDG_TIME_CAL_LONG_MONTH_MASK >> month) & LDG_TIME_CAL_STEP) { *out = LDG_TIME_CAL_LONG_MONTH_DAYS; }
    else { *out = LDG_TIME_CAL_SHORT_MONTH_DAYS; }

    return LDG_ERR_AOK;
}

static uint16_t cal_year_day_get(uint32_t year, uint8_t month, uint8_t day)
{
    uint16_t total = 0;
    uint8_t idx = 0;
    uint8_t span = 0;

    for (idx = LDG_TIME_MONTH_JANUARY; idx < month; idx++)
    {
        if (ldg_time_month_day_cunt_get(year, idx, &span) != LDG_ERR_AOK) { return total; }

        total = (uint16_t)(total + span);
    }

    return (uint16_t)(total + day - LDG_TIME_CAL_STEP);
}

uint32_t ldg_time_cal_get(uint64_t epoch_sec, ldg_time_cal_t *out)
{
    int64_t days = 0;
    int64_t era = 0;
    int64_t day_of_era = 0;
    int64_t year_of_era = 0;
    int64_t year = 0;
    int64_t day_of_year = 0;
    int64_t month_slot = 0;
    int64_t month = 0;
    int64_t day = 0;
    uint64_t rest = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->year = 0;
    out->year_day = 0;
    out->month = 0;
    out->day = 0;
    out->hour = 0;
    out->minute = 0;
    out->second = 0;
    out->weekday = 0;

    days = (int64_t)(epoch_sec / LDG_TIME_SECS_PER_DAY);
    rest = epoch_sec % LDG_TIME_SECS_PER_DAY;

    out->hour = (uint8_t)(rest / LDG_SECS_PER_HOUR);
    out->minute = (uint8_t)((rest % LDG_SECS_PER_HOUR) / LDG_SECS_PER_MIN);
    out->second = (uint8_t)(rest % LDG_SECS_PER_MIN);

    out->weekday = (uint8_t)((days + LDG_TIME_CAL_EPOCH_WEEKDAY) % LDG_TIME_DAYS_PER_WEEK);

    days += LDG_TIME_CAL_EPOCH_SHIFT;

    era = days / LDG_TIME_CAL_ERA_DAYS;
    day_of_era = days - (era * LDG_TIME_CAL_ERA_DAYS);

    year_of_era = (day_of_era - (day_of_era / LDG_TIME_CAL_QUAD_DAYS) + (day_of_era / LDG_TIME_CAL_CENTURY_DAYS) - (day_of_era / LDG_TIME_CAL_ERA_LAST_DAY)) / LDG_TIME_CAL_YEAR_DAYS;

    year = year_of_era + (era * LDG_TIME_CAL_ERA_YEARS);

    day_of_year = day_of_era - ((LDG_TIME_CAL_YEAR_DAYS * year_of_era) + (year_of_era / LDG_TIME_CAL_LEAP_STEP) - (year_of_era / LDG_TIME_CAL_CENTURY_STEP));

    month_slot = ((LDG_TIME_CAL_MONTH_DENOM * day_of_year) + LDG_TIME_CAL_MONTH_BIAS) / LDG_TIME_CAL_MONTH_NUMER;

    day = day_of_year - (((LDG_TIME_CAL_MONTH_NUMER * month_slot) + LDG_TIME_CAL_MONTH_BIAS) / LDG_TIME_CAL_MONTH_DENOM) + LDG_TIME_CAL_STEP;

    month = month_slot + ((month_slot < LDG_TIME_CAL_MONTH_ROLL) ? LDG_TIME_CAL_MARCH_BACK : -LDG_TIME_CAL_MARCH_FORWARD);

    if (month <= LDG_TIME_CAL_MARCH_PIVOT) { year += LDG_TIME_CAL_STEP; }

    out->year = (uint32_t)year;
    out->month = (uint8_t)month;
    out->day = (uint8_t)day;

    out->year_day = cal_year_day_get(out->year, out->month, out->day);

    return LDG_ERR_AOK;
}

static uint64_t cal_digits_wr(char *dst, uint64_t pos, uint32_t val, uint32_t width)
{
    uint32_t idx = 0;

    for (idx = width; idx > 0; idx--)
    {
        dst[pos + idx - LDG_TIME_CAL_STEP] = (char)(LDG_TIME_CAL_DIGIT_ZERO + (val % LDG_BASE_DEC));
        val /= LDG_BASE_DEC;
    }

    return pos + width;
}

uint32_t ldg_time_cal_fmt(const ldg_time_cal_t *cal, char *dst, uint64_t cap, uint64_t *out_len)
{
    uint64_t pos = 0;

    if (LDG_UNLIKELY(!out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(!cal || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < (LDG_TIME_CAL_TEXT_LEN + LDG_STR_TERM_SIZE))) { return LDG_ERR_STR_TRUNC; }

    pos = cal_digits_wr(dst, pos, cal->year, LDG_TIME_CAL_YEAR_WIDTH);

    dst[pos] = LDG_TIME_CAL_DATE_SEP;
    pos++;

    pos = cal_digits_wr(dst, pos, (uint32_t)cal->month, LDG_TIME_CAL_PAIR_WIDTH);

    dst[pos] = LDG_TIME_CAL_DATE_SEP;
    pos++;

    pos = cal_digits_wr(dst, pos, (uint32_t)cal->day, LDG_TIME_CAL_PAIR_WIDTH);

    dst[pos] = LDG_TIME_CAL_FIELD_SEP;
    pos++;

    pos = cal_digits_wr(dst, pos, (uint32_t)cal->hour, LDG_TIME_CAL_PAIR_WIDTH);

    dst[pos] = LDG_TIME_CAL_TIME_SEP;
    pos++;

    pos = cal_digits_wr(dst, pos, (uint32_t)cal->minute, LDG_TIME_CAL_PAIR_WIDTH);

    dst[pos] = LDG_TIME_CAL_TIME_SEP;
    pos++;

    pos = cal_digits_wr(dst, pos, (uint32_t)cal->second, LDG_TIME_CAL_PAIR_WIDTH);

    dst[pos] = LDG_STR_TERM;

    *out_len = pos;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_epoch_sec_get(const ldg_time_cal_t *cal, uint64_t *out)
{
    int64_t year = 0;
    int64_t era = 0;
    int64_t year_of_era = 0;
    int64_t day_of_year = 0;
    int64_t day_of_era = 0;
    int64_t days = 0;
    uint8_t month_days = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!cal)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cal->month < LDG_TIME_MONTH_JANUARY || cal->month > LDG_TIME_MONTH_DECEMBER)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_time_month_day_cunt_get(cal->year, cal->month, &month_days);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cal->day < LDG_TIME_CAL_STEP || cal->day > month_days)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cal->hour >= LDG_TIME_HOURS_PER_DAY)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cal->minute >= LDG_TIME_MINS_PER_HOUR)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cal->second >= LDG_SECS_PER_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cal->year < LDG_TIME_EPOCH_YEAR)) { return LDG_ERR_RANGE; }

    year = (int64_t)cal->year;

    if (cal->month <= LDG_TIME_CAL_MARCH_PIVOT) { year -= LDG_TIME_CAL_STEP; }

    era = year / LDG_TIME_CAL_ERA_YEARS;
    year_of_era = year - (era * LDG_TIME_CAL_ERA_YEARS);

    day_of_year = (((LDG_TIME_CAL_MONTH_NUMER * ((int64_t)cal->month + ((cal->month > LDG_TIME_CAL_MARCH_PIVOT) ? -LDG_TIME_CAL_MARCH_BACK : LDG_TIME_CAL_MARCH_FORWARD))) + LDG_TIME_CAL_MONTH_BIAS) / LDG_TIME_CAL_MONTH_DENOM) + (int64_t)cal->day - LDG_TIME_CAL_STEP;

    day_of_era = (year_of_era * LDG_TIME_CAL_YEAR_DAYS) + (year_of_era / LDG_TIME_CAL_LEAP_STEP) - (year_of_era / LDG_TIME_CAL_CENTURY_STEP) + day_of_year;

    days = (era * LDG_TIME_CAL_ERA_DAYS) + day_of_era - LDG_TIME_CAL_EPOCH_SHIFT;

    *out = ((uint64_t)days * LDG_TIME_SECS_PER_DAY) + ((uint64_t)cal->hour * LDG_SECS_PER_HOUR) + ((uint64_t)cal->minute * LDG_SECS_PER_MIN) + (uint64_t)cal->second;

    return LDG_ERR_AOK;
}
