#include <stdint.h>

#include <dangling/time/time.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>

#define LDG_TIME_RTC_BASE 0x90050000U

#define LDG_TIME_RTC_STATUS_OFF 0x00U
#define LDG_TIME_RTC_LATCH_OFF 0x04U
#define LDG_TIME_RTC_EPOCH_LO_OFF 0x08U
#define LDG_TIME_RTC_EPOCH_HI_OFF 0x0CU
#define LDG_TIME_RTC_TICK_LO_OFF 0x10U
#define LDG_TIME_RTC_TICK_HI_OFF 0x14U
#define LDG_TIME_RTC_FREQ_OFF 0x18U

#define LDG_TIME_RTC_STATUS_PRESENT_BIT 0
#define LDG_TIME_RTC_STATUS_EPOCH_SET_BIT 1

#define LDG_TIME_RTC_LATCH_TRIGGER 0x1U
#define LDG_TIME_RTC_HI_SHIFT 32

static uint32_t rtc_reg_get(uint32_t off)
{
    return *(volatile uint32_t *)(uintptr_t)(LDG_TIME_RTC_BASE + off);
}

static void rtc_latch_take(void)
{
    *(volatile uint32_t *)(uintptr_t)(LDG_TIME_RTC_BASE + LDG_TIME_RTC_LATCH_OFF) = LDG_TIME_RTC_LATCH_TRIGGER;
}

static uint64_t rtc_pair_get(uint32_t lo_off, uint32_t hi_off)
{
    uint64_t lo = 0;
    uint64_t hi = 0;

    lo = (uint64_t)rtc_reg_get(lo_off);
    hi = (uint64_t)rtc_reg_get(hi_off);

    return lo | (hi << LDG_TIME_RTC_HI_SHIFT);
}

static uint32_t rtc_epoch_sec_get(uint64_t *out)
{
    uint32_t status = 0;

    *out = 0;

    status = rtc_reg_get(LDG_TIME_RTC_STATUS_OFF);

    if (LDG_UNLIKELY((status & LDG_BIT(LDG_TIME_RTC_STATUS_PRESENT_BIT)) == 0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY((status & LDG_BIT(LDG_TIME_RTC_STATUS_EPOCH_SET_BIT)) == 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    rtc_latch_take();

    *out = rtc_pair_get(LDG_TIME_RTC_EPOCH_LO_OFF, LDG_TIME_RTC_EPOCH_HI_OFF);

    return LDG_ERR_AOK;
}

uint64_t ldg_time_epoch_ms_get(void)
{
    uint64_t sec = 0;

    if (LDG_UNLIKELY(rtc_epoch_sec_get(&sec) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(sec > UINT64_MAX / LDG_MS_PER_SEC)) { return UINT64_MAX; }

    return sec * LDG_MS_PER_SEC;
}

uint64_t ldg_time_epoch_ns_get(void)
{
    uint64_t sec = 0;

    if (LDG_UNLIKELY(rtc_epoch_sec_get(&sec) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(sec > UINT64_MAX / LDG_NS_PER_SEC)) { return UINT64_MAX; }

    return sec * LDG_NS_PER_SEC;
}

uint64_t ldg_time_cycle_get(void)
{
    uint64_t tick = 0;
    uint32_t status = 0;

    status = rtc_reg_get(LDG_TIME_RTC_STATUS_OFF);

    if (LDG_UNLIKELY((status & LDG_BIT(LDG_TIME_RTC_STATUS_PRESENT_BIT)) == 0)) { return tick; }

    rtc_latch_take();

    tick = rtc_pair_get(LDG_TIME_RTC_TICK_LO_OFF, LDG_TIME_RTC_TICK_HI_OFF);

    return tick;
}

uint32_t ldg_time_ns_delay(uint64_t duration)
{
    uint64_t start = 0;
    uint64_t tgt = 0;
    uint32_t freq = 0;

    if (duration == 0) { return LDG_ERR_AOK; }

    freq = rtc_reg_get(LDG_TIME_RTC_FREQ_OFF);

    if (LDG_UNLIKELY(freq == 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    start = ldg_time_cycle_get();

    tgt = (duration / LDG_NS_PER_SEC) * freq;
    tgt += ((duration % LDG_NS_PER_SEC) * freq) / LDG_NS_PER_SEC;

    while ((ldg_time_cycle_get() - start) < tgt) {}

    return LDG_ERR_AOK;
}

uint32_t ldg_time_monotonic_get(double *out)
{
    uint64_t tick = 0;
    uint32_t status = 0;
    uint32_t freq = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    status = rtc_reg_get(LDG_TIME_RTC_STATUS_OFF);

    if (LDG_UNLIKELY((status & LDG_BIT(LDG_TIME_RTC_STATUS_PRESENT_BIT)) == 0)) { return LDG_ERR_NOT_INIT; }

    rtc_latch_take();

    tick = rtc_pair_get(LDG_TIME_RTC_TICK_LO_OFF, LDG_TIME_RTC_TICK_HI_OFF);
    freq = rtc_reg_get(LDG_TIME_RTC_FREQ_OFF);

    if (LDG_UNLIKELY(freq == 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    *out = (double)tick / (double)freq;

    return LDG_ERR_AOK;
}
