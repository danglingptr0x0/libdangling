#include <stdint.h>
#include <time.h>

#include <dangling/thread/yield.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_thread_yield(uint64_t ns)
{
    uint64_t secs = 0;
    uint64_t elapsed = 0;
    struct timespec req = { 0 };

    if (ns == 0) { return LDG_ERR_AOK; }

    secs = ns / LDG_NS_PER_SEC;
    elapsed = ns % LDG_NS_PER_SEC;
    req.tv_sec = (time_t)secs;
    req.tv_nsec = (int64_t)elapsed;

    if (LDG_UNLIKELY(nanosleep(&req, 0x0) != 0)) { return LDG_ERR_INTERRUPTED; }

    return LDG_ERR_AOK;
}
