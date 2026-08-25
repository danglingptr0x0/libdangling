#include <stdint.h>

#include <dangling/io/dir.h>
#include <dangling/sys/handover.h>
#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_DIR_SLOT_MAX 12
#define LDG_IO_DIR_NAME_MAX 64

#define LDG_IO_DIR_SYM_OPEN "fs_dir_open"
#define LDG_IO_DIR_SYM_RD "fs_dir_rd"
#define LDG_IO_DIR_SYM_CLOSE "fs_close"

#define LDG_IO_DIR_KERN_AOK 0
#define LDG_IO_DIR_KERN_ARG_NULL 1
#define LDG_IO_DIR_KERN_ARG_INVALID 2
#define LDG_IO_DIR_KERN_FULL 5
#define LDG_IO_DIR_KERN_EMPTY 7
#define LDG_IO_DIR_KERN_NOT_FOUND 403
#define LDG_IO_DIR_KERN_NOT_OPEN 407
#define LDG_IO_DIR_KERN_NOT_FILE 409

#define LDG_IO_DIR_TYPE_FILE 0
#define LDG_IO_DIR_TYPE_DIR 1

typedef uint32_t (*ldg_io_dir_open_cb_t)(const char *path, uint32_t *out_fd);
typedef uint32_t (*ldg_io_dir_rd_cb_t)(uint32_t fd, char *name, uint32_t *name_len, uint8_t *out_type);
typedef uint32_t (*ldg_io_dir_close_cb_t)(uint32_t fd);

typedef struct ldg_io_dir_slot
{
    ldg_io_dir_t dir;
    uint32_t fd;
    uint8_t is_open;
    uint8_t pudding[3];
} ldg_io_dir_slot_t;

typedef struct ldg_io_dir_state
{
    ldg_io_dir_open_cb_t open;
    ldg_io_dir_rd_cb_t rd;
    ldg_io_dir_close_cb_t close;
    ldg_io_dir_slot_t slot[LDG_IO_DIR_SLOT_MAX];
    uint8_t is_bound;
    uint8_t pudding[7];
} ldg_io_dir_state_t;

static ldg_io_dir_state_t* dir_state_get(void)
{
    static ldg_io_dir_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint32_t dir_err_translate(uint32_t kern)
{
    if (kern == LDG_IO_DIR_KERN_AOK) { return LDG_ERR_AOK; }

    if (kern == LDG_IO_DIR_KERN_ARG_NULL) { return LDG_ERR_FUNC_ARG_NULL; }

    if (kern == LDG_IO_DIR_KERN_ARG_INVALID) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (kern == LDG_IO_DIR_KERN_FULL) { return LDG_ERR_FULL; }

    if (kern == LDG_IO_DIR_KERN_EMPTY) { return LDG_ERR_EOF; }

    if (kern == LDG_IO_DIR_KERN_NOT_FOUND) { return LDG_ERR_IO_NOT_FOUND; }

    if (kern == LDG_IO_DIR_KERN_NOT_OPEN) { return LDG_ERR_CLOSED; }

    if (kern == LDG_IO_DIR_KERN_NOT_FILE) { return LDG_ERR_IO_NOT_DIR; }

    return LDG_ERR_INVALID;
}

static uint32_t dir_bind_ensure(ldg_io_dir_state_t *state)
{
    uint32_t addr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (state->is_bound == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_sys_sym_get(LDG_IO_DIR_SYM_OPEN, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->open = (ldg_io_dir_open_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_DIR_SYM_RD, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->rd = (ldg_io_dir_rd_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_DIR_SYM_CLOSE, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->close = (ldg_io_dir_close_cb_t)(uintptr_t)addr;

    state->is_bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t dir_slot_find(ldg_io_dir_state_t *state, const ldg_io_dir_t *dir, ldg_io_dir_slot_t **out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < LDG_IO_DIR_SLOT_MAX; idx++)
    {
        if (&state->slot[idx].dir != dir) { continue; }

        if (LDG_UNLIKELY(state->slot[idx].is_open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

        *out = &state->slot[idx];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t dir_slot_claim(ldg_io_dir_state_t *state, ldg_io_dir_slot_t **out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (idx = 0; idx < LDG_IO_DIR_SLOT_MAX; idx++)
    {
        if (state->slot[idx].is_open == LDG_TRUTH_TRUE) { continue; }

        *out = &state->slot[idx];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FULL;
}

uint32_t ldg_io_dir_create(const char *path, uint32_t access)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(access == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_dir_destroy(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_dir_open(const char *path, ldg_io_dir_t **out)
{
    ldg_io_dir_state_t *state = 0x0;
    ldg_io_dir_slot_t *slot = 0x0;
    uint64_t path_len = 0;
    uint32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    state = dir_state_get();

    ret = dir_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = dir_slot_claim(state, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = dir_err_translate(state->open(path, &fd));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_rbrcpy(slot->dir.path, path, LDG_IO_PATH_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        state->close(fd);
        return ret;
    }

    ret = ldg_str_len_get(slot->dir.path, LDG_IO_PATH_MAX, &path_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        state->close(fd);
        return ret;
    }

    slot->fd = fd;
    slot->dir.handle = (void *)(uintptr_t)fd;
    slot->dir.full_path = slot->dir.path;
    slot->dir.path_len = path_len;
    slot->is_open = LDG_TRUTH_TRUE;

    *out = &slot->dir;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_rd(ldg_io_dir_t *dir, ldg_io_entry_t *out)
{
    ldg_io_dir_state_t *state = 0x0;
    ldg_io_dir_slot_t *slot = 0x0;
    uint32_t name_len = 0;
    uint32_t ret = 0;
    uint8_t type = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->name[0] = LDG_STR_TERM;
    out->is_dir = LDG_TRUTH_FALSE;

    state = dir_state_get();

    ret = dir_slot_find(state, dir, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    name_len = LDG_IO_DIR_NAME_MAX;

    ret = dir_err_translate(state->rd(slot->fd, out->name, &name_len, &type));
    if (ret != LDG_ERR_AOK) { return ret; }

    if (LDG_UNLIKELY(name_len > LDG_IO_DIR_NAME_MAX)) { return LDG_ERR_MEM_CORRUPTION; }

    out->name[name_len] = LDG_STR_TERM;

    if (type == LDG_IO_DIR_TYPE_DIR) { out->is_dir = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_close(ldg_io_dir_t *dir)
{
    ldg_io_dir_state_t *state = 0x0;
    ldg_io_dir_slot_t *slot = 0x0;
    uint32_t ret = 0;

    state = dir_state_get();

    ret = dir_slot_find(state, dir, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = dir_err_translate(state->close(slot->fd));

    slot->is_open = LDG_TRUTH_FALSE;
    slot->fd = 0;
    slot->dir.handle = 0x0;
    slot->dir.full_path = 0x0;
    slot->dir.path_len = 0;
    slot->dir.path[0] = LDG_STR_TERM;

    return ret;
}

uint32_t ldg_io_entry_name_get(ldg_io_entry_t *entry, const char **name)
{
    if (LDG_UNLIKELY(!entry || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    *name = entry->name;

    return LDG_ERR_AOK;
}

uint8_t ldg_io_entry_dir_is(ldg_io_entry_t *entry)
{
    if (LDG_UNLIKELY(!entry)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (entry->is_dir == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
