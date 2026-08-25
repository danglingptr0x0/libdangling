#include <time.h>
#include <errno.h>

#include <dangling/time/time.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

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
    struct timespec span = LDG_STRUCT_ZERO_INIT;
    struct timespec rest = LDG_STRUCT_ZERO_INIT;

    if (duration == 0) { return LDG_ERR_AOK; }

    span.tv_sec = (time_t)(duration / LDG_NS_PER_SEC);
    span.tv_nsec = (int64_t)(duration % LDG_NS_PER_SEC);

    while (nanosleep(&span, &rest) != 0)
    {
        if (errno != EINTR) { return LDG_ERR_INTERRUPTED; }

        span = rest;
    }

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
    struct timespec ts = { 0 };

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    *out = (double)ts.tv_sec + (double)ts.tv_nsec / (double)LDG_NS_PER_SEC;

    return LDG_ERR_AOK;
}
