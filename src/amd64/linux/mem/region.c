#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <dangling/mem/region.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>

#define LDG_MEM_REGION_MAPS_FMT "/proc/%u32/maps"
#define LDG_MEM_REGION_MAPS_PATH_CAP 32
#define LDG_MEM_REGION_BLK_SIZE 4096
#define LDG_MEM_REGION_LINE_CAP 128
#define LDG_MEM_REGION_PERM_SHARE_IDX 3
#define LDG_MEM_REGION_PERM_SHARE 's'
#define LDG_MEM_REGION_FILE_LEAD '/'
#define LDG_MEM_REGION_QUERY_CAP 1
#define LDG_MEM_REGION_LINE_END '\n'
#define LDG_MEM_REGION_RANGE_SEP '-'
#define LDG_MEM_REGION_FIELD_SEP ' '
#define LDG_MEM_REGION_SEP_LEN 1
#define LDG_MEM_REGION_PERM_CUNT 4
#define LDG_MEM_REGION_PERM_RD_IDX 0
#define LDG_MEM_REGION_PERM_WR_IDX 1
#define LDG_MEM_REGION_PERM_EXEC_IDX 2
#define LDG_MEM_REGION_PERM_RD 'r'
#define LDG_MEM_REGION_PERM_WR 'w'
#define LDG_MEM_REGION_PERM_EXEC 'x'

typedef struct ldg_mem_region_walk
{
    ldg_mem_region_t *out;
    uint64_t addr;
    uint64_t cap;
    uint64_t cunt;
    uint64_t prev_end;
    uint32_t pid;
    uint8_t single;
    uint8_t done;
    uint8_t pudding[2];
} ldg_mem_region_walk_t;

static uint64_t mem_region_hex_take(const char *line, uint64_t len, uint64_t *cursor)
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

static void mem_region_line_take(ldg_mem_region_walk_t *walk, const char *line, uint64_t len)
{
    ldg_mem_region_t *slot = 0x0;
    uint64_t cursor = 0;
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t file_idx = 0;

    if (walk->done == LDG_TRUTH_TRUE) { return; }

    start = mem_region_hex_take(line, len, &cursor);
    if (cursor >= len || line[cursor] != LDG_MEM_REGION_RANGE_SEP) { return; }

    cursor += LDG_MEM_REGION_SEP_LEN;
    end = mem_region_hex_take(line, len, &cursor);
    if (cursor >= len || line[cursor] != LDG_MEM_REGION_FIELD_SEP || end <= start) { return; }

    cursor += LDG_MEM_REGION_SEP_LEN;
    if (cursor + LDG_MEM_REGION_PERM_CUNT > len) { return; }

    if (walk->addr >= end)
    {
        walk->prev_end = end;
        return;
    }

    slot = &walk->out[walk->cunt];

    if (walk->single == LDG_TRUTH_TRUE && walk->addr < start)
    {
        slot->base = walk->prev_end;
        slot->size = start - walk->prev_end;
        slot->alloc_base = walk->prev_end;
        walk->cunt++;
        walk->done = LDG_TRUTH_TRUE;

        return;
    }

    slot->base = start;
    slot->size = end - start;
    slot->alloc_base = start;
    slot->committed = LDG_TRUTH_TRUE;
    slot->readable = (line[cursor + LDG_MEM_REGION_PERM_RD_IDX] == LDG_MEM_REGION_PERM_RD) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    slot->writable = (line[cursor + LDG_MEM_REGION_PERM_WR_IDX] == LDG_MEM_REGION_PERM_WR) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    slot->executable = (line[cursor + LDG_MEM_REGION_PERM_EXEC_IDX] == LDG_MEM_REGION_PERM_EXEC) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    slot->kind = LDG_MEM_REGION_KIND_PRIVATE;

    if (ldg_str_char_find(line, LDG_MEM_REGION_FILE_LEAD, len, &file_idx) == LDG_ERR_AOK) { slot->kind = (line[cursor + LDG_MEM_REGION_PERM_SHARE_IDX] == LDG_MEM_REGION_PERM_SHARE) ? LDG_MEM_REGION_KIND_MAPPED : LDG_MEM_REGION_KIND_IMG; }

    walk->cunt++;

    if (walk->single == LDG_TRUTH_TRUE || walk->cunt >= walk->cap) { walk->done = LDG_TRUTH_TRUE; }
}

static uint32_t mem_region_walk(ldg_mem_region_walk_t *walk)
{
    char blk[LDG_MEM_REGION_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    char line[LDG_MEM_REGION_LINE_CAP] = LDG_ARR_ZERO_INIT;
    char path[LDG_MEM_REGION_MAPS_PATH_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t path_len = 0;
    uint64_t line_len = 0;
    uint64_t idx = 0;
    int64_t got = 0;
    int32_t fd = 0;
    uint32_t ret = LDG_ERR_AOK;

    ret = ldg_str_fmt(path, LDG_MEM_REGION_MAPS_PATH_CAP, &path_len, LDG_MEM_REGION_MAPS_FMT, walk->pid);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fd = (int32_t)open(path, O_RDONLY | O_CLOEXEC);
    if (LDG_UNLIKELY(fd < 0))
    {
        if (errno == EACCES || errno == EPERM) { return LDG_ERR_DENIED; }

        return (errno == ENOENT) ? LDG_ERR_NOT_FOUND : LDG_ERR_IO_OPEN;
    }

    while (walk->done != LDG_TRUTH_TRUE)
    {
        got = (int64_t)read(fd, blk, LDG_MEM_REGION_BLK_SIZE);
        if (got < 0 && errno == EINTR) { continue; }

        if (LDG_UNLIKELY(got < 0)) { ret = LDG_ERR_IO_RD; }

        if (got <= 0) { break; }

        for (idx = 0; idx < (uint64_t)got; idx++)
        {
            if (blk[idx] != LDG_MEM_REGION_LINE_END)
            {
                if (line_len < LDG_MEM_REGION_LINE_CAP) { line[line_len++] = blk[idx]; }

                continue;
            }

            mem_region_line_take(walk, line, line_len);
            line_len = 0;
        }
    }

    if (LDG_UNLIKELY(close(fd) != 0) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t ldg_mem_region_get(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out)
{
    ldg_mem_region_walk_t walk = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_mem_region_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    walk.pid = proc->pid;
    walk.out = out;
    walk.addr = addr;
    walk.cap = LDG_MEM_REGION_QUERY_CAP;
    walk.single = LDG_TRUTH_TRUE;

    ret = mem_region_walk(&walk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (walk.cunt == 0) { return LDG_ERR_EOF; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_region_list(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out, uint64_t cap, uint64_t *out_cunt)
{
    ldg_mem_region_walk_t walk = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, cap * sizeof(ldg_mem_region_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    walk.pid = proc->pid;
    walk.out = out;
    walk.addr = addr;
    walk.cap = cap;

    ret = mem_region_walk(&walk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = walk.cunt;

    return LDG_ERR_AOK;
}
