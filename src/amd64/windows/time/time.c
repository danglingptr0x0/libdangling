#include <dangling/time/time.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>

#include <time.h>
#include <windows.h>

#define LDG_TIME_CYCLE_HI_SHIFT 32

uint64_t ldg_time_cycle_get(void)
{
    uint32_t lo = 0;
    uint32_t hi = 0;

    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));

    return ((uint64_t)hi << LDG_TIME_CYCLE_HI_SHIFT) | (uint64_t)lo;
}

uint32_t ldg_time_ns_delay(uint64_t duration)
{
    uint64_t millis = 0;

    if (duration == 0) { return LDG_ERR_AOK; }

    millis = duration / LDG_NS_PER_MS;

    if (millis == 0) { millis = LDG_STR_TERM_SIZE; }

    Sleep((DWORD)millis);

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

uint32_t ldg_time_monotonic_get(double *out)
{
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER ctr = { 0 };

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!QueryPerformanceFrequency(&freq))) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    if (LDG_UNLIKELY(!QueryPerformanceCounter(&ctr))) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    *out = (double)ctr.QuadPart / (double)freq.QuadPart;

    return LDG_ERR_AOK;
}
