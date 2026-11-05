#include <stdint.h>

#include <dangling/sys/lib.h>
#include <dangling/sys/handover.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_sys_lib_open(const char *path, void **out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_lib_sym_get(void *lib, const char *name, ldg_sys_lib_func_t *out)
{
    void *kernel = 0x0;
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!lib || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_sys_kernel_get(&kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(lib != kernel)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_sys_sym_get(name, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (ldg_sys_lib_func_t)(uintptr_t)addr;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_lib_close(void *lib)
{
    if (LDG_UNLIKELY(!lib)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}
