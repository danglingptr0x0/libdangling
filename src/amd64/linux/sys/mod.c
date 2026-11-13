#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

#include <dangling/sys/mod.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/core/types.h>
#include <dangling/mem/vmem.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>

#define LDG_SYS_MOD_MAPS_FMT "/proc/%u32/maps"
#define LDG_SYS_MOD_MAPS_PATH_CAP 32
#define LDG_SYS_MOD_BLK_SIZE 4096
#define LDG_SYS_MOD_LINE_CAP (LDG_SYS_MOD_PATH_MAX + 128)
#define LDG_SYS_MOD_LINE_END '\n'
#define LDG_SYS_MOD_RANGE_SEP '-'
#define LDG_SYS_MOD_FIELD_SEP ' '
#define LDG_SYS_MOD_PATH_LEAD '/'
#define LDG_SYS_MOD_SEP_LEN 1
#define LDG_SYS_MOD_PERM_FIELD_LEN 5
#define LDG_SYS_MOD_ELF_MAGIC "\177ELF"
#define LDG_SYS_MOD_ELF_MAGIC_LEN 4
#define LDG_SYS_MOD_QUERY_CAP 1
#define LDG_SYS_MOD_ADDR_UNUSED 0

typedef enum ldg_sys_mod_walk_kind
{
    LDG_SYS_MOD_WALK_LIST = 0,
    LDG_SYS_MOD_WALK_NAME,
    LDG_SYS_MOD_WALK_PATH,
    LDG_SYS_MOD_WALK_ADDR,
    LDG_SYS_MOD_WALK_KIND_CUNT
} ldg_sys_mod_walk_kind_t;

typedef struct ldg_sys_mod_walk
{
    ldg_sys_mod_t pending;
    const ldg_sys_proc_t *proc;
    ldg_sys_mod_t *out;
    const char *key;
    uint64_t addr;
    uint64_t cap;
    uint64_t cunt;
    uint32_t kind;
    uint8_t pending_live;
    uint8_t done;
    uint8_t pudding[2];
} ldg_sys_mod_walk_t;

static uint64_t sys_mod_hex_take(const char *line, uint64_t len, uint64_t *cursor)
{
    uint64_t val = 0;
    byte_t nibble = 0;

    while (*cursor < len && ldg_hex_to_nipple(line[*cursor], &nibble) == LDG_ERR_AOK)
    {
        val = (val << LDG_NIBBLE_BITS) | (uint64_t)nibble;
        (*cursor)++;
    }

    return val;
}

static uint8_t sys_mod_pending_hit_is(const ldg_sys_mod_walk_t *walk)
{
    uint32_t order = 0;

    if (walk->kind == LDG_SYS_MOD_WALK_LIST) { return LDG_TRUTH_TRUE; }

    if (walk->kind == LDG_SYS_MOD_WALK_ADDR) { return (walk->addr >= walk->pending.base && walk->addr - walk->pending.base < walk->pending.size) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (walk->kind == LDG_SYS_MOD_WALK_NAME) { return (ldg_str_cmp(walk->pending.name, walk->key, LDG_SYS_MOD_NAME_MAX, &order) == LDG_ERR_AOK && order == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    return (ldg_str_cmp(walk->pending.path, walk->key, LDG_SYS_MOD_PATH_MAX, &order) == LDG_ERR_AOK && order == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void sys_mod_pending_emit(ldg_sys_mod_walk_t *walk)
{
    if (walk->pending_live != LDG_TRUTH_TRUE || walk->done == LDG_TRUTH_TRUE) { return; }

    walk->pending_live = LDG_TRUTH_FALSE;

    if (sys_mod_pending_hit_is(walk) != LDG_TRUTH_TRUE) { return; }

    if (LDG_UNLIKELY(ldg_mem_cpy(&walk->out[walk->cunt], &walk->pending, sizeof(ldg_sys_mod_t)) != LDG_ERR_AOK)) { return; }

    walk->cunt++;

    if (walk->kind != LDG_SYS_MOD_WALK_LIST || walk->cunt >= walk->cap) { walk->done = LDG_TRUTH_TRUE; }
}

static uint8_t sys_mod_elf_is(const ldg_sys_proc_t *proc, uint64_t base)
{
    byte_t magic[LDG_SYS_MOD_ELF_MAGIC_LEN] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0;
    uint32_t order = 0;

    if (ldg_mem_vmem_rd(proc, base, magic, LDG_SYS_MOD_ELF_MAGIC_LEN, &got) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    if (ldg_mem_cmp(magic, LDG_SYS_MOD_ELF_MAGIC, LDG_SYS_MOD_ELF_MAGIC_LEN, &order) != LDG_ERR_AOK || order != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static void sys_mod_line_take(ldg_sys_mod_walk_t *walk, const char *line, uint64_t len)
{
    const char *path = 0x0;
    uint64_t cursor = 0;
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t file_pos = 0;
    uint64_t path_idx = 0;
    uint64_t leaf_idx = 0;
    uint32_t order = 0;

    if (walk->done == LDG_TRUTH_TRUE) { return; }

    start = sys_mod_hex_take(line, len, &cursor);
    if (cursor >= len || line[cursor] != LDG_SYS_MOD_RANGE_SEP) { return; }

    cursor += LDG_SYS_MOD_SEP_LEN;
    end = sys_mod_hex_take(line, len, &cursor);
    if (cursor >= len || line[cursor] != LDG_SYS_MOD_FIELD_SEP || end <= start) { return; }

    cursor += LDG_SYS_MOD_SEP_LEN + LDG_SYS_MOD_PERM_FIELD_LEN;
    if (cursor >= len) { return; }

    file_pos = sys_mod_hex_take(line, len, &cursor);

    if (ldg_str_char_find(line, LDG_SYS_MOD_PATH_LEAD, len, &path_idx) != LDG_ERR_AOK) { return; }

    path = line + path_idx;

    if (walk->pending_live == LDG_TRUTH_TRUE && ldg_str_cmp(walk->pending.path, path, LDG_SYS_MOD_PATH_MAX, &order) == LDG_ERR_AOK && order == 0)
    {
        walk->pending.size = end - walk->pending.base;
        return;
    }

    sys_mod_pending_emit(walk);
    if (walk->done == LDG_TRUTH_TRUE || file_pos != 0) { return; }

    if (sys_mod_elf_is(walk->proc, start) != LDG_TRUTH_TRUE) { return; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(&walk->pending, sizeof(ldg_sys_mod_t)) != LDG_ERR_AOK)) { return; }

    if (ldg_str_rbrcpy(walk->pending.path, path, LDG_SYS_MOD_PATH_MAX) != LDG_ERR_AOK) { return; }

    if (ldg_str_char_last_find(path, LDG_SYS_MOD_PATH_LEAD, LDG_SYS_MOD_PATH_MAX, &leaf_idx) != LDG_ERR_AOK) { return; }

    if (ldg_str_rbrcpy(walk->pending.name, path + leaf_idx + LDG_SYS_MOD_SEP_LEN, LDG_SYS_MOD_NAME_MAX) != LDG_ERR_AOK) { return; }

    walk->pending.base = start;
    walk->pending.size = end - start;
    walk->pending_live = LDG_TRUTH_TRUE;
}

static uint32_t sys_mod_walk(ldg_sys_mod_walk_t *walk)
{
    char blk[LDG_SYS_MOD_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    char line[LDG_SYS_MOD_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char maps[LDG_SYS_MOD_MAPS_PATH_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t maps_len = 0;
    uint64_t line_len = 0;
    uint64_t idx = 0;
    int64_t got = 0;
    int32_t fd = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(walk->proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_str_fmt(maps, LDG_SYS_MOD_MAPS_PATH_CAP, &maps_len, LDG_SYS_MOD_MAPS_FMT, walk->proc->pid);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fd = (int32_t)open(maps, O_RDONLY | O_CLOEXEC);
    if (LDG_UNLIKELY(fd < 0))
    {
        if (errno == EACCES || errno == EPERM) { return LDG_ERR_DENIED; }

        return (errno == ENOENT) ? LDG_ERR_NOT_FOUND : LDG_ERR_IO_OPEN;
    }

    while (walk->done != LDG_TRUTH_TRUE)
    {
        got = (int64_t)read(fd, blk, LDG_SYS_MOD_BLK_SIZE);
        if (got < 0 && errno == EINTR) { continue; }

        if (LDG_UNLIKELY(got < 0)) { ret = LDG_ERR_IO_RD; }

        if (got <= 0) { break; }

        for (idx = 0; idx < (uint64_t)got; idx++)
        {
            if (blk[idx] != LDG_SYS_MOD_LINE_END)
            {
                if (line_len + LDG_STR_TERM_SIZE < LDG_SYS_MOD_LINE_CAP) { line[line_len++] = blk[idx]; }

                continue;
            }

            line[line_len] = LDG_STR_TERM;
            sys_mod_line_take(walk, line, line_len);
            line_len = 0;
        }
    }

    sys_mod_pending_emit(walk);

    if (LDG_UNLIKELY(close(fd) != 0) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static uint32_t sys_mod_query(const ldg_sys_proc_t *proc, uint32_t kind, const char *key, uint64_t addr, ldg_sys_mod_t *out)
{
    ldg_sys_mod_walk_t walk = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_sys_mod_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    walk.proc = proc;
    walk.out = out;
    walk.key = key;
    walk.addr = addr;
    walk.cap = LDG_SYS_MOD_QUERY_CAP;
    walk.kind = kind;

    ret = sys_mod_walk(&walk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (walk.cunt == 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint8_t sys_mod_prog_is(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod)
{
    char prog[LDG_SYS_MOD_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t order = 0;

    if (ldg_sys_proc_img_path_get(proc, prog, LDG_SYS_MOD_PATH_MAX) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    if (ldg_str_cmp(prog, mod->path, LDG_SYS_MOD_PATH_MAX, &order) != LDG_ERR_AOK || order != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_sys_mod_list(const ldg_sys_proc_t *proc, ldg_sys_mod_t *out, uint64_t cap, uint64_t *out_cunt)
{
    ldg_sys_mod_walk_t walk = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    walk.proc = proc;
    walk.out = out;
    walk.cap = cap;
    walk.kind = LDG_SYS_MOD_WALK_LIST;

    ret = sys_mod_walk(&walk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = walk.cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_mod_find(const ldg_sys_proc_t *proc, const char *name, ldg_sys_mod_t *out)
{
    char prog[LDG_SYS_MOD_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (name) { return sys_mod_query(proc, LDG_SYS_MOD_WALK_NAME, name, LDG_SYS_MOD_ADDR_UNUSED, out); }

    ret = ldg_sys_proc_img_path_get(proc, prog, LDG_SYS_MOD_PATH_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return sys_mod_query(proc, LDG_SYS_MOD_WALK_PATH, prog, LDG_SYS_MOD_ADDR_UNUSED, out);
}

uint32_t ldg_sys_mod_addr_find(const ldg_sys_proc_t *proc, uint64_t addr, ldg_sys_mod_t *out)
{
    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return sys_mod_query(proc, LDG_SYS_MOD_WALK_ADDR, 0x0, addr, out);
}

uint32_t ldg_sys_mod_sym_get(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod, const char *name, uint64_t *out_addr)
{
    void *lib = 0x0;
    void *sym = 0x0;

    if (LDG_UNLIKELY(!proc || !mod || !name || !out_addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_addr = 0;

    if (LDG_UNLIKELY(proc->pid != (uint32_t)getpid())) { return LDG_ERR_UNSUPPORTED; }

    lib = dlopen((sys_mod_prog_is(proc, mod) == LDG_TRUTH_TRUE) ? 0x0 : mod->path, RTLD_NOLOAD | RTLD_NOW);
    if (!lib) { return LDG_ERR_NOT_FOUND; }

    sym = dlsym(lib, name);

    if (LDG_UNLIKELY(dlclose(lib) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (!sym) { return LDG_ERR_NOT_FOUND; }

    *out_addr = (uint64_t)(uintptr_t)sym;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_mod_pin(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod)
{
    void *lib = 0x0;

    if (LDG_UNLIKELY(!proc || !mod)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(proc->pid != (uint32_t)getpid())) { return LDG_ERR_UNSUPPORTED; }

    if (sys_mod_prog_is(proc, mod) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    lib = dlopen(mod->path, RTLD_NOLOAD | RTLD_NODELETE | RTLD_NOW);
    if (!lib) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(dlclose(lib) != 0)) { return LDG_ERR_IO_CLOSE; }

    return LDG_ERR_AOK;
}
