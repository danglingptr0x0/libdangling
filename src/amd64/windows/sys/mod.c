#include <stdint.h>
#include <windows.h>
#include <tlhelp32.h>

#include <dangling/sys/mod.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#define LDG_SYS_MOD_QUERY_CAP 1
#define LDG_SYS_MOD_ADDR_UNUSED 0

typedef enum ldg_sys_mod_walk_kind
{
    LDG_SYS_MOD_WALK_LIST = 0,
    LDG_SYS_MOD_WALK_NAME,
    LDG_SYS_MOD_WALK_ADDR,
    LDG_SYS_MOD_WALK_KIND_CUNT
} ldg_sys_mod_walk_kind_t;

static uint32_t sys_mod_entry_fill(const MODULEENTRY32 *entry, ldg_sys_mod_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_sys_mod_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->base = (uint64_t)(uintptr_t)entry->modBaseAddr;
    out->size = (uint64_t)entry->modBaseSize;

    ret = ldg_str_rbrcpy(out->name, entry->szModule, LDG_SYS_MOD_NAME_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_str_rbrcpy(out->path, entry->szExePath, LDG_SYS_MOD_PATH_MAX);
}

static uint8_t sys_mod_hit_is(uint32_t kind, const char *key, uint64_t addr, const ldg_sys_mod_t *mod)
{
    uint32_t order = 0;

    if (kind == LDG_SYS_MOD_WALK_LIST) { return LDG_TRUTH_TRUE; }

    if (kind == LDG_SYS_MOD_WALK_ADDR) { return (addr >= mod->base && addr - mod->base < mod->size) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return (ldg_str_case_cmp(mod->name, key, LDG_SYS_MOD_NAME_MAX, &order) == LDG_ERR_AOK && order == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sys_mod_walk(const ldg_sys_proc_t *proc, uint32_t kind, const char *key, uint64_t addr, ldg_sys_mod_t *out, uint64_t cap, uint64_t *cunt)
{
    MODULEENTRY32 entry = LDG_STRUCT_ZERO_INIT;
    HANDLE snap = INVALID_HANDLE_VALUE;
    uint32_t ret = LDG_ERR_AOK;

    *cunt = 0;

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, (DWORD)proc->pid);
    if (LDG_UNLIKELY(snap == INVALID_HANDLE_VALUE)) { return LDG_ERR_DENIED; }

    entry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(snap, &entry))
    {
        do
        {
            if (*cunt >= cap) { break; }

            if (sys_mod_entry_fill(&entry, &out[*cunt]) != LDG_ERR_AOK) { continue; }

            if (sys_mod_hit_is(kind, key, addr, &out[*cunt]) != LDG_TRUTH_TRUE) { continue; }

            (*cunt)++;
        }
        while (Module32Next(snap, &entry));
    }

    if (*cunt < cap && LDG_UNLIKELY(ldg_mem_secure_zero(&out[*cunt], sizeof(ldg_sys_mod_t)) != LDG_ERR_AOK)) { ret = LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(!CloseHandle(snap)) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static uint32_t sys_mod_query(const ldg_sys_proc_t *proc, uint32_t kind, const char *key, uint64_t addr, ldg_sys_mod_t *out)
{
    uint64_t cunt = 0;
    uint32_t ret = 0;

    ret = sys_mod_walk(proc, kind, key, addr, out, LDG_SYS_MOD_QUERY_CAP, &cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (cunt == 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_mod_list(const ldg_sys_proc_t *proc, ldg_sys_mod_t *out, uint64_t cap, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!proc || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return sys_mod_walk(proc, LDG_SYS_MOD_WALK_LIST, 0x0, LDG_SYS_MOD_ADDR_UNUSED, out, cap, out_cunt);
}

uint32_t ldg_sys_mod_find(const ldg_sys_proc_t *proc, const char *name, ldg_sys_mod_t *out)
{
    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name) { return sys_mod_query(proc, LDG_SYS_MOD_WALK_NAME, name, LDG_SYS_MOD_ADDR_UNUSED, out); }

    return sys_mod_query(proc, LDG_SYS_MOD_WALK_LIST, 0x0, LDG_SYS_MOD_ADDR_UNUSED, out);
}

uint32_t ldg_sys_mod_addr_find(const ldg_sys_proc_t *proc, uint64_t addr, ldg_sys_mod_t *out)
{
    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return sys_mod_query(proc, LDG_SYS_MOD_WALK_ADDR, 0x0, addr, out);
}

uint32_t ldg_sys_mod_sym_get(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod, const char *name, uint64_t *out_addr)
{
    FARPROC sym = 0x0;

    if (LDG_UNLIKELY(!proc || !mod || !name || !out_addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_addr = 0;

    if (LDG_UNLIKELY(proc->pid != (uint32_t)GetCurrentProcessId())) { return LDG_ERR_UNSUPPORTED; }

    sym = GetProcAddress((HMODULE)(uintptr_t)mod->base, name);
    if (!sym) { return LDG_ERR_NOT_FOUND; }

    return ldg_mem_cpy(out_addr, &sym, sizeof(FARPROC));
}

uint32_t ldg_sys_mod_pin(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod)
{
    HMODULE pinned = 0x0;

    if (LDG_UNLIKELY(!proc || !mod)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(proc->pid != (uint32_t)GetCurrentProcessId())) { return LDG_ERR_UNSUPPORTED; }

    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCSTR)(uintptr_t)mod->base, &pinned)) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}
