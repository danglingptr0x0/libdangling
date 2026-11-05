#include <stdint.h>
#include <windows.h>

#include <dangling/sys/cpuset.h>
#include <dangling/sys/lib.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_CPUSET_BUFF_CAP (LDG_SYS_CPUSET_MAX * (uint32_t)sizeof(SYSTEM_CPU_SET_INFORMATION))
#define LDG_SYS_CPUSET_FLAG_NONE 0
#define LDG_SYS_CPUSET_KERNEL_LIB "kernel32.dll"
#define LDG_SYS_CPUSET_THREAD_ENTRY "SetThreadSelectedCpuSets"
#define LDG_SYS_CPUSET_PROC_ENTRY "SetProcessDefaultCpuSets"
#define LDG_SYS_CPUSET_LIST_ENTRY "GetSystemCpuSetInformation"

typedef BOOL (WINAPI *ldg_sys_cpuset_set_t)(HANDLE tgt, const ULONG *id, ULONG cunt);
typedef BOOL (WINAPI *ldg_sys_cpuset_list_t)(SYSTEM_CPU_SET_INFORMATION *info, ULONG cap, ULONG *out_len, HANDLE proc, ULONG flags);

static uint32_t cpuset_entry_get(const char *entry_name, void **lib, ldg_sys_lib_func_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!lib || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *lib = 0x0;
    *out = 0x0;

    ret = ldg_sys_lib_open(LDG_SYS_CPUSET_KERNEL_LIB, lib);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sys_lib_sym_get(*lib, entry_name, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_sys_lib_close(*lib) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("sys: cpuset kernel lib close failed"); }

        *lib = 0x0;

        return ret;
    }

    return LDG_ERR_AOK;
}

static uint32_t cpuset_set_run(const char *entry_name, HANDLE tgt, const uint32_t *id, uint32_t cunt)
{
    void *lib = 0x0;
    ldg_sys_lib_func_t func = 0x0;
    ldg_sys_cpuset_set_t entry = 0x0;
    BOOL done = FALSE;
    uint32_t ret = 0;

    ret = cpuset_entry_get(entry_name, &lib, &func);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    entry = (ldg_sys_cpuset_set_t)func;

    done = entry(tgt, (const ULONG *)id, (ULONG)cunt);

    ret = ldg_sys_lib_close(lib);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!done)) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_cpuset_list(ldg_sys_cpuset_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint8_t buff[LDG_SYS_CPUSET_BUFF_CAP] = LDG_ARR_ZERO_INIT;
    SYSTEM_CPU_SET_INFORMATION *rec = 0x0;
    void *lib = 0x0;
    ldg_sys_lib_func_t func = 0x0;
    ldg_sys_cpuset_list_t entry = 0x0;
    BOOL done = FALSE;
    DWORD fault = 0;
    ULONG need = 0;
    ULONG pos = 0;
    uint64_t found = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = cpuset_entry_get(LDG_SYS_CPUSET_LIST_ENTRY, &lib, &func);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    entry = (ldg_sys_cpuset_list_t)func;

    done = entry((SYSTEM_CPU_SET_INFORMATION *)(void *)buff, (ULONG)sizeof(buff), &need, GetCurrentProcess(), LDG_SYS_CPUSET_FLAG_NONE);
    fault = GetLastError();

    ret = ldg_sys_lib_close(lib);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (!done)
    {
        if (fault == ERROR_INSUFFICIENT_BUFFER) { return LDG_ERR_FULL; }

        return LDG_ERR_UNSUPPORTED;
    }

    if (LDG_UNLIKELY(need > (ULONG)sizeof(buff))) { return LDG_ERR_FULL; }

    while (pos < need && found < cap)
    {
        rec = (SYSTEM_CPU_SET_INFORMATION *)(void *)(buff + pos);

        if (LDG_UNLIKELY(rec->Size == 0)) { break; }

        if (rec->Type == CpuSetInformation)
        {
            out[found].id = (uint32_t)rec->CpuSet.Id;
            out[found].group = (uint16_t)rec->CpuSet.Group;
            out[found].logical_proc_idx = (uint8_t)rec->CpuSet.LogicalProcessorIndex;
            out[found].core_idx = (uint8_t)rec->CpuSet.CoreIndex;
            out[found].cache_idx = (uint8_t)rec->CpuSet.LastLevelCacheIndex;
            out[found].numa_idx = (uint8_t)rec->CpuSet.NumaNodeIndex;
            out[found].efficiency_class = (uint8_t)rec->CpuSet.EfficiencyClass;
            found++;
        }

        pos += rec->Size;
    }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_cpuset_proc_default_set(const uint32_t *id, uint32_t cunt)
{
    if (LDG_UNLIKELY(!id && cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > LDG_SYS_CPUSET_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return cpuset_set_run(LDG_SYS_CPUSET_PROC_ENTRY, GetCurrentProcess(), id, cunt);
}

uint32_t ldg_sys_cpuset_thread_self_set(const uint32_t *id, uint32_t cunt)
{
    if (LDG_UNLIKELY(!id && cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > LDG_SYS_CPUSET_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return cpuset_set_run(LDG_SYS_CPUSET_THREAD_ENTRY, GetCurrentThread(), id, cunt);
}
