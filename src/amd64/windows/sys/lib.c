#include <stdint.h>
#include <windows.h>

#include <dangling/sys/lib.h>
#include <dangling/io/path.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_LIB_PATH_CAP 1024
#define LDG_SYS_LIB_MB_NULL_TERM (-1)
#define LDG_SYS_LIB_LD_DEFAULT 0

uint32_t ldg_sys_lib_open(const char *path, void **out)
{
    WCHAR wide[LDG_SYS_LIB_PATH_CAP] = LDG_ARR_ZERO_INIT;
    HMODULE lib = 0x0;
    DWORD flags = LDG_SYS_LIB_LD_DEFAULT;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(path[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, LDG_SYS_LIB_MB_NULL_TERM, wide, LDG_SYS_LIB_PATH_CAP) <= 0)
    {
        if (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) { return LDG_ERR_STR_INVALID; }

        return LDG_ERR_STR_TRUNC;
    }

    if (ldg_io_path_absolute_is(path) == LDG_TRUTH_TRUE) { flags = LOAD_WITH_ALTERED_SEARCH_PATH; }

    lib = LoadLibraryExW(wide, 0x0, flags);
    if (LDG_UNLIKELY(!lib)) { return LDG_ERR_IO_OPEN; }

    *out = (void *)lib;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_lib_sym_get(void *lib, const char *name, ldg_sys_lib_func_t *out)
{
    FARPROC addr = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!lib || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    addr = GetProcAddress((HMODULE)lib, name);
    if (LDG_UNLIKELY(!addr)) { return LDG_ERR_NOT_FOUND; }

    *out = (ldg_sys_lib_func_t)addr;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_lib_close(void *lib)
{
    if (LDG_UNLIKELY(!lib)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!FreeLibrary((HMODULE)lib))) { return LDG_ERR_IO_CLOSE; }

    return LDG_ERR_AOK;
}
