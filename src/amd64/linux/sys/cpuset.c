#include <stdint.h>

#include <dangling/sys/cpuset.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_sys_cpuset_list(ldg_sys_cpuset_t *out, uint64_t cap, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_cpuset_proc_default_set(const uint32_t *id, uint32_t cunt)
{
    if (LDG_UNLIKELY(!id && cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > LDG_SYS_CPUSET_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_cpuset_thread_self_set(const uint32_t *id, uint32_t cunt)
{
    if (LDG_UNLIKELY(!id && cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > LDG_SYS_CPUSET_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}
