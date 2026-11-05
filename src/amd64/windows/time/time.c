#include <dangling/time/time.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>

#include <time.h>
#include <windows.h>

#define LDG_TIME_CYCLE_HI_SHIFT 32
#define LDG_TIME_WAITABLE_HIGH_RES 0x00000002
#define LDG_TIME_CEIL_BIAS 1
#define LDG_TIME_NS_PER_FILETIME_TICK 100
#define LDG_TIME_TIMER_PERIOD_ONCE 0

uint64_t ldg_time_cycle_get(void)
{
    uint32_t lo = 0;
    uint32_t hi = 0;

    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));

    return ((uint64_t)hi << LDG_TIME_CYCLE_HI_SHIFT) | (uint64_t)lo;
}

uint32_t ldg_time_ns_delay(uint64_t duration)
{
    LARGE_INTEGER due = LDG_STRUCT_ZERO_INIT;
    HANDLE timer = 0x0;
    uint64_t millis = 0;
    uint64_t ticks = 0;
    DWORD waited = 0;

    if (duration == 0) { return LDG_ERR_AOK; }

    timer = CreateWaitableTimerExW(0x0, 0x0, LDG_TIME_WAITABLE_HIGH_RES, TIMER_ALL_ACCESS);

    if (!timer)
    {
        millis = (duration + LDG_NS_PER_MS - LDG_TIME_CEIL_BIAS) / LDG_NS_PER_MS;

        if (LDG_UNLIKELY(millis >= (uint64_t)INFINITE)) { millis = (uint64_t)INFINITE - LDG_TIME_CEIL_BIAS; }

        Sleep((DWORD)millis);

        return LDG_ERR_AOK;
    }

    ticks = (duration + LDG_TIME_NS_PER_FILETIME_TICK - LDG_TIME_CEIL_BIAS) / LDG_TIME_NS_PER_FILETIME_TICK;

    if (LDG_UNLIKELY(ticks > (uint64_t)INT64_MAX)) { ticks = (uint64_t)INT64_MAX; }

    due.QuadPart = -(int64_t)ticks;

    if (LDG_UNLIKELY(!SetWaitableTimer(timer, &due, LDG_TIME_TIMER_PERIOD_ONCE, 0x0, 0x0, FALSE)))
    {
        CloseHandle(timer);
        return LDG_ERR_INVALID;
    }

    waited = WaitForSingleObject(timer, INFINITE);

    CloseHandle(timer);

    if (LDG_UNLIKELY(waited != WAIT_OBJECT_0)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

uint64_t ldg_time_epoch_ms_get(void)
{
    struct timespec ts = { 0 };

    if (LDG_UNLIKELY(clock_gettime(CLOCK_REALTIME, &ts) != 0)) { return 0; }

    return ((uint64_t)ts.tv_sec * LDG_MS_PER_SEC + (uint64_t)ts.tv_nsec / LDG_NS_PER_MS);
}

uint64_t ldg_time_epoch_ns_get(void)
{
    struct timespec ts = { 0 };

    if (LDG_UNLIKELY(clock_gettime(CLOCK_REALTIME, &ts) != 0)) { return 0; }

    return ((uint64_t)ts.tv_sec * LDG_NS_PER_SEC + (uint64_t)ts.tv_nsec);
}

uint32_t ldg_time_monotonic_get(uint64_t *out)
{
    LARGE_INTEGER freq = LDG_STRUCT_ZERO_INIT;
    LARGE_INTEGER ctr = LDG_STRUCT_ZERO_INIT;
    uint64_t rate = 0;
    uint64_t tick = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    if (LDG_UNLIKELY(!QueryPerformanceCounter(&ctr) || ctr.QuadPart < 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    rate = (uint64_t)freq.QuadPart;
    tick = (uint64_t)ctr.QuadPart;

    *out = ((tick / rate) * LDG_MS_PER_SEC) + (((tick % rate) * LDG_MS_PER_SEC) / rate);

    return LDG_ERR_AOK;
}

uint32_t ldg_time_monotonic_ns_get(uint64_t *out)
{
    LARGE_INTEGER freq = LDG_STRUCT_ZERO_INIT;
    LARGE_INTEGER ctr = LDG_STRUCT_ZERO_INIT;
    uint64_t rate = 0;
    uint64_t tick = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    if (LDG_UNLIKELY(!QueryPerformanceCounter(&ctr) || ctr.QuadPart < 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    rate = (uint64_t)freq.QuadPart;
    tick = (uint64_t)ctr.QuadPart;

    *out = ((tick / rate) * LDG_NS_PER_SEC) + (((tick % rate) * LDG_NS_PER_SEC) / rate);

    return LDG_ERR_AOK;
}
