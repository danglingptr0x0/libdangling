#include <time.h>

#include <dangling/time/time.h>
#include <dangling/core/err.h>

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
