#include <stdint.h>

#include <dangling/thread/yield.h>
#include <dangling/sys/handover.h>
#include <dangling/time/time.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_THREAD_YIELD_SYM "dangling_ctx_yield"

typedef uint32_t (*ldg_thread_yield_cb_t)(void *kernel);

uint32_t ldg_thread_yield(uint64_t ns)
{
    static ldg_thread_yield_cb_t cached = 0x0;
    static void *kernel = 0x0;
    uint64_t start = 0;
    uint64_t now = 0;
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (!cached)
    {
        ret = ldg_sys_kernel_get(&kernel);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sys_sym_get(LDG_THREAD_YIELD_SYM, &addr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cached = (ldg_thread_yield_cb_t)(uintptr_t)addr;
    }

    ret = cached(kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_EMPTY)) { return ret; }

    if (ns == 0) { return LDG_ERR_AOK; }

    start = ldg_time_cycle_get();
    if (LDG_UNLIKELY(start == UINT64_MAX)) { return LDG_ERR_NOT_INIT; }

    now = start;

    while ((now - start) * LDG_NS_PER_MS < ns)
    {
        ret = cached(kernel);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_EMPTY)) { return ret; }

        now = ldg_time_cycle_get();
        if (LDG_UNLIKELY(now == UINT64_MAX)) { return LDG_ERR_NOT_INIT; }
    }

    return LDG_ERR_AOK;
}
