#include <stdint.h>
#include <windows.h>

#include <dangling/thread/yield.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_thread_yield(uint64_t ns)
{
    uint64_t ms = 0;

    if (ns == 0) { return LDG_ERR_AOK; }

    ms = ns / LDG_NS_PER_MS;
    if (ms == 0) { ms = 1; }

    Sleep((uint32_t)ms);

    return LDG_ERR_AOK;
}
