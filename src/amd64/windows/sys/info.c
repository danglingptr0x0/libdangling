#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <dangling/sys/info.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

uint32_t ldg_sys_hostname_get(char *buff, uint64_t buff_size)
{
    DWORD size = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    size = (DWORD)(buff_size - LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(!GetComputerNameExA(ComputerNameDnsHostname, buff, &size))) { return LDG_ERR_IO_RD; }

    buff[buff_size - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_cpu_cunt_get(uint32_t *cunt)
{
    SYSTEM_INFO si = { 0 };

    if (LDG_UNLIKELY(!cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cunt = 0;

    GetSystemInfo(&si);
    if (LDG_UNLIKELY(si.dwNumberOfProcessors == 0)) { return LDG_ERR_UNSUPPORTED; }

    *cunt = (uint32_t)si.dwNumberOfProcessors;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_page_size_get(uint64_t *size)
{
    SYSTEM_INFO si = { 0 };

    if (LDG_UNLIKELY(!size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *size = 0;

    GetSystemInfo(&si);
    if (LDG_UNLIKELY(si.dwPageSize == 0)) { return LDG_ERR_UNSUPPORTED; }

    *size = (uint64_t)si.dwPageSize;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_env_get(const char *name, char *buff, uint64_t buff_size)
{
    const char *val = 0x0;

    if (LDG_UNLIKELY(!name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv(name);
    if (!val) { return LDG_ERR_NOT_FOUND; }

    return ldg_strrbrcpy(buff, val, (uint64_t)buff_size);
}

uint32_t ldg_sys_pid_get(uint64_t *pid)
{
    if (LDG_UNLIKELY(!pid)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pid = (uint64_t)GetCurrentProcessId();

    return LDG_ERR_AOK;
}
