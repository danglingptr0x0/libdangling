#include <stdint.h>
#include <dlfcn.h>

#include <dangling/sys/lib.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_sys_lib_open(const char *path, void **out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(path[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (LDG_UNLIKELY(!*out)) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_lib_sym_get(void *lib, const char *name, ldg_sys_lib_func_t *out)
{
    void *addr = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!lib || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    addr = dlsym(lib, name);
    if (LDG_UNLIKELY(!addr)) { return LDG_ERR_NOT_FOUND; }

    *out = (ldg_sys_lib_func_t)(uintptr_t)addr;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_lib_close(void *lib)
{
    if (LDG_UNLIKELY(!lib)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dlclose(lib) != 0)) { return LDG_ERR_IO_CLOSE; }

    return LDG_ERR_AOK;
}
