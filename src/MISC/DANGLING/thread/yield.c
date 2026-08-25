#include <stdint.h>

#include <dangling/thread/yield.h>
#include <dangling/sys/handover.h>
#include <dangling/time/time.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_THREAD_YIELD_SYM "ctx_yield"
#define LDG_THREAD_YIELD_STALL_MAX 1024

typedef void (*ldg_thread_yield_cb_t)(void);

uint32_t ldg_thread_yield(uint64_t ns)
{
    static ldg_thread_yield_cb_t cached = 0x0;
    uint64_t start = 0;
    uint64_t prev = 0;
    uint64_t now = 0;
    uint64_t stall = 0;
    uint32_t addr = 0;
    uint32_t ret = 0;

    if (!cached)
    {
        ret = ldg_sys_sym_get(LDG_THREAD_YIELD_SYM, &addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cached = (ldg_thread_yield_cb_t)(uintptr_t)addr;
    }

    cached();

    if (ns == 0) { return LDG_ERR_AOK; }

    start = ldg_time_epoch_ns_get();
    if (LDG_UNLIKELY(start == UINT64_MAX)) { return LDG_ERR_AOK; }

    prev = start;
    now = start;

    while ((now - start) < ns)
    {
        cached();

        now = ldg_time_epoch_ns_get();
        if (LDG_UNLIKELY(now == UINT64_MAX)) { return LDG_ERR_AOK; }

        if (now != prev)
        {
            prev = now;
            stall = 0;

            continue;
        }

        stall++;
        if (LDG_UNLIKELY(stall >= LDG_THREAD_YIELD_STALL_MAX)) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}
