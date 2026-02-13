#include <dangling/time/time.h>
#include <dangling/core/err.h>

#include <time.h>
#include <windows.h>

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

double ldg_time_monotonic_get(void)
{
    LARGE_INTEGER freq = { 0 };
    LARGE_INTEGER ctr = { 0 };

    if (LDG_UNLIKELY(!QueryPerformanceFrequency(&freq))) { return 0.0; }

    if (LDG_UNLIKELY(!QueryPerformanceCounter(&ctr))) { return 0.0; }

    return (double)ctr.QuadPart / (double)freq.QuadPart;
}
