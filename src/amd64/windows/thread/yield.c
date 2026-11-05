#include <stdint.h>
#include <windows.h>

#include <dangling/thread/yield.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_THREAD_YIELD_SLICE_MS 0U
#define LDG_THREAD_YIELD_MS_MIN 1U

uint32_t ldg_thread_yield(uint64_t ns)
{
    uint64_t ms = 0;

    if (ns == 0)
    {
        if (SwitchToThread() == 0) { Sleep(LDG_THREAD_YIELD_SLICE_MS); }

        return LDG_ERR_AOK;
    }

    ms = ns / LDG_NS_PER_MS;
    if (ms == 0) { ms = LDG_THREAD_YIELD_MS_MIN; }

    Sleep((uint32_t)ms);

    return LDG_ERR_AOK;
}
