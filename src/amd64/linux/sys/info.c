#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dangling/sys/info.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

uint32_t ldg_sys_hostname_get(char *buff, uint64_t buff_size)
{
    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(gethostname(buff, (uint64_t)buff_size - LDG_STR_TERM_SIZE) < 0)) { return LDG_ERR_IO_RD; }

    buff[buff_size - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_cpu_cunt_get(uint32_t *cunt)
{
    int64_t result = 0;

    if (LDG_UNLIKELY(!cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cunt = 0;

    result = (int64_t)sysconf(_SC_NPROCESSORS_ONLN);
    if (LDG_UNLIKELY(result <= 0)) { return LDG_ERR_UNSUPPORTED; }

    *cunt = (uint32_t)result;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_page_size_get(uint64_t *size)
{
    int64_t result = 0;

    if (LDG_UNLIKELY(!size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *size = 0;

    result = (int64_t)sysconf(_SC_PAGESIZE);
    if (LDG_UNLIKELY(result <= 0)) { return LDG_ERR_UNSUPPORTED; }

    *size = (uint64_t)result;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_env_get(const char *name, char *buff, uint64_t buff_size)
{
    const char *val = 0x0;

    if (LDG_UNLIKELY(!name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv(name);
    if (!val) { return LDG_ERR_NOT_FOUND; }

    return ldg_strrbrcpy(buff, val, (uint64_t)buff_size);
}

uint32_t ldg_sys_pid_get(uint64_t *pid)
{
    if (LDG_UNLIKELY(!pid)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pid = (uint64_t)getpid();

    return LDG_ERR_AOK;
}
