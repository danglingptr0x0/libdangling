#include <stdint.h>

#include <dangling/io/file.h>
#include <dangling/sys/handover.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_FS_SLOT_MAX 1024
#define LDG_IO_FS_ACCESS_MAX 0xFFFU
#define LDG_IO_FS_OPEN_NONE 0x00
#define LDG_IO_FS_OPEN_CREATE 0x04

#define LDG_IO_FS_SYM_OPEN "dangling_fs_root_file_open"
#define LDG_IO_FS_SYM_RD "dangling_fs_root_file_rd"
#define LDG_IO_FS_SYM_WR "dangling_fs_root_file_wr"
#define LDG_IO_FS_SYM_SEEK "dangling_fs_root_file_seek"
#define LDG_IO_FS_SYM_CLOSE "dangling_fs_root_file_close"
#define LDG_IO_FS_SYM_INFO_GET "dangling_fs_root_file_info_get"
#define LDG_IO_FS_SYM_WIDTH_SET "dangling_fs_root_file_width_set"
#define LDG_IO_FS_SYM_MODIFIED_GET "dangling_fs_root_file_modified_get"
#define LDG_IO_FS_WIDTH_NONE 0

#define LDG_IO_FS_TYPE_FILE 0
#define LDG_IO_FS_TYPE_DIR 1

typedef uint32_t (*ldg_io_fs_open_cb_t)(void *kernel, const char *path, uint32_t flags, uint32_t *out_fd);
typedef uint32_t (*ldg_io_fs_rd_cb_t)(void *kernel, uint32_t fd, void *buff, uint32_t len, uint32_t *out);
typedef uint32_t (*ldg_io_fs_wr_cb_t)(void *kernel, uint32_t fd, const void *buff, uint32_t len, uint32_t *out);
typedef uint32_t (*ldg_io_fs_seek_cb_t)(void *kernel, uint32_t fd, uint32_t pos);
typedef uint32_t (*ldg_io_fs_close_cb_t)(void *kernel, uint32_t fd);
typedef uint32_t (*ldg_io_fs_info_get_cb_t)(void *kernel, const char *path, uint32_t *out_width, uint32_t *out_type);
typedef uint32_t (*ldg_io_fs_width_set_cb_t)(void *kernel, uint32_t fd, uint32_t width);
typedef uint32_t (*ldg_io_fs_modified_get_cb_t)(void *kernel, const char *path, uint32_t *out_ms, uint8_t *out_known);

typedef struct ldg_io_fs_slot
{
    ldg_io_file_t file;
    uint64_t pos;
    uint64_t width;
    uint32_t type;
    uint8_t is_open;
    uint8_t pudding[3];
} ldg_io_fs_slot_t;

typedef struct ldg_io_fs_state
{
    void *kernel;
    ldg_io_fs_open_cb_t open;
    ldg_io_fs_rd_cb_t rd;
    ldg_io_fs_wr_cb_t wr;
    ldg_io_fs_seek_cb_t seek;
    ldg_io_fs_close_cb_t close;
    ldg_io_fs_info_get_cb_t info_get;
    ldg_io_fs_width_set_cb_t width_set;
    ldg_io_fs_modified_get_cb_t modified_get;
    ldg_io_fs_slot_t slot[LDG_IO_FS_SLOT_MAX];
    uint8_t is_bound;
    uint8_t width_set_tried;
    uint8_t modified_get_tried;
    uint8_t pudding[5];
} ldg_io_fs_state_t;

static ldg_io_fs_state_t* fs_state_get(void)
{
    static ldg_io_fs_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint32_t fs_err_translate(uint32_t kern)
{
    if (kern == LDG_ERR_EMPTY) { return LDG_ERR_EOF; }

    if (kern == LDG_ERR_NOT_FOUND) { return LDG_ERR_IO_NOT_FOUND; }

    return kern;
}

static uint32_t fs_bind_ensure(ldg_io_fs_state_t *state)
{
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (state->is_bound == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_sys_kernel_get(&state->kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sys_sym_get(LDG_IO_FS_SYM_OPEN, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->open = (ldg_io_fs_open_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_FS_SYM_RD, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->rd = (ldg_io_fs_rd_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_FS_SYM_WR, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->wr = (ldg_io_fs_wr_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_FS_SYM_SEEK, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->seek = (ldg_io_fs_seek_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_FS_SYM_CLOSE, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->close = (ldg_io_fs_close_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_FS_SYM_INFO_GET, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->info_get = (ldg_io_fs_info_get_cb_t)(uintptr_t)addr;

    state->is_bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t fs_width_bind_ensure(ldg_io_fs_state_t *state)
{
    uint64_t addr = 0;

    if (LDG_UNLIKELY(!state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (state->width_set) { return LDG_ERR_AOK; }

    if (state->width_set_tried == LDG_TRUTH_TRUE) { return LDG_ERR_UNSUPPORTED; }

    state->width_set_tried = LDG_TRUTH_TRUE;

    if (ldg_sys_sym_get(LDG_IO_FS_SYM_WIDTH_SET, &addr) != LDG_ERR_AOK) { return LDG_ERR_UNSUPPORTED; }

    state->width_set = (ldg_io_fs_width_set_cb_t)(uintptr_t)addr;

    return LDG_ERR_AOK;
}

static uint32_t fs_modified_bind_ensure(ldg_io_fs_state_t *state)
{
    uint64_t addr = 0;

    if (state->modified_get) { return LDG_ERR_AOK; }

    if (state->modified_get_tried == LDG_TRUTH_TRUE) { return LDG_ERR_UNSUPPORTED; }

    state->modified_get_tried = LDG_TRUTH_TRUE;

    if (ldg_sys_sym_get(LDG_IO_FS_SYM_MODIFIED_GET, &addr) != LDG_ERR_AOK) { return LDG_ERR_UNSUPPORTED; }

    state->modified_get = (ldg_io_fs_modified_get_cb_t)(uintptr_t)addr;

    return LDG_ERR_AOK;
}

static uint32_t fs_slot_find(ldg_io_fs_state_t *state, const ldg_io_file_t *handle, ldg_io_fs_slot_t **out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!handle)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < LDG_IO_FS_SLOT_MAX; idx++)
    {
        if (&state->slot[idx].file != handle) { continue; }

        if (LDG_UNLIKELY(state->slot[idx].is_open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

        *out = &state->slot[idx];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t fs_slot_claim(ldg_io_fs_state_t *state, ldg_io_fs_slot_t **out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (idx = 0; idx < LDG_IO_FS_SLOT_MAX; idx++)
    {
        if (state->slot[idx].is_open == LDG_TRUTH_TRUE) { continue; }

        *out = &state->slot[idx];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FULL;
}

uint32_t ldg_io_file_open(const char *path, uint32_t flags, uint32_t access, ldg_io_file_t **out)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t fs_flags = LDG_IO_FS_OPEN_NONE;
    uint32_t width = 0;
    uint32_t type = 0;
    uint32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(access > LDG_IO_FS_ACCESS_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((flags & LDG_IO_EXCLUSIVE) != 0)) { return LDG_ERR_UNSUPPORTED; }

    state = fs_state_get();

    ret = fs_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if ((flags & LDG_IO_TRUNCATE) != 0)
    {
        ret = fs_width_bind_ensure(state);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = fs_slot_claim(state, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if ((flags & LDG_IO_CREATE) != 0) { fs_flags |= LDG_IO_FS_OPEN_CREATE; }

    ret = fs_err_translate(state->open(state->kernel, path, fs_flags, &fd));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fs_err_translate(state->info_get(state->kernel, path, &width, &type));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        state->close(state->kernel, fd);
        return ret;
    }

    slot->file.handle = (int32_t)fd;
    slot->pos = 0;
    slot->width = (uint64_t)width;
    slot->type = type;
    slot->is_open = LDG_TRUTH_TRUE;

    if ((flags & LDG_IO_TRUNCATE) != 0)
    {
        ret = fs_err_translate(state->width_set(state->kernel, fd, LDG_IO_FS_WIDTH_NONE));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            state->close(state->kernel, fd);
            slot->is_open = LDG_TRUTH_FALSE;
            return ret;
        }

        width = LDG_IO_FS_WIDTH_NONE;
        slot->width = (uint64_t)width;
    }

    if ((flags & LDG_IO_APPEND) != 0)
    {
        ret = fs_err_translate(state->seek(state->kernel, fd, width));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            state->close(state->kernel, fd);
            slot->is_open = LDG_TRUTH_FALSE;
            return ret;
        }

        slot->pos = (uint64_t)width;
    }

    *out = &slot->file;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_close(ldg_io_file_t *handle)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fs_err_translate(state->close(state->kernel, (uint32_t)slot->file.handle));

    slot->is_open = LDG_TRUTH_FALSE;
    slot->pos = 0;
    slot->width = 0;
    slot->file.handle = 0;

    return ret;
}

uint32_t ldg_io_file_rd(ldg_io_file_t *handle, void *buff, uint64_t size, uint64_t *bytes_rd)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint8_t *cursor = 0x0;
    uint64_t done = 0;
    uint64_t remaining = 0;
    uint32_t chunk = 0;
    uint32_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bytes_rd)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_rd = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (size == 0) { return LDG_ERR_AOK; }

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cursor = (uint8_t *)buff;

    while (done < size)
    {
        remaining = size - done;
        chunk = (remaining > (uint64_t)UINT32_MAX) ? UINT32_MAX : (uint32_t)remaining;
        got = 0;

        ret = fs_err_translate(state->rd(state->kernel, (uint32_t)slot->file.handle, cursor + done, chunk, &got));

        if (ret == LDG_ERR_EOF) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(got > chunk)) { return LDG_ERR_MEM_CORRUPTION; }

        if (got == 0) { break; }

        done += (uint64_t)got;
        slot->pos += (uint64_t)got;
    }

    if (LDG_UNLIKELY(done == 0 && ret == LDG_ERR_EOF)) { return LDG_ERR_EOF; }

    *bytes_rd = done;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_wr(ldg_io_file_t *handle, const void *buff, uint64_t size, uint64_t *bytes_wr)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    const uint8_t *cursor = 0x0;
    uint64_t done = 0;
    uint64_t remaining = 0;
    uint32_t chunk = 0;
    uint32_t put = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bytes_wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_wr = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (size == 0) { return LDG_ERR_AOK; }

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cursor = (const uint8_t *)buff;

    while (done < size)
    {
        remaining = size - done;
        chunk = (remaining > (uint64_t)UINT32_MAX) ? UINT32_MAX : (uint32_t)remaining;
        put = 0;

        ret = fs_err_translate(state->wr(state->kernel, (uint32_t)slot->file.handle, cursor + done, chunk, &put));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (done == 0) { return ret; }

            break;
        }

        if (LDG_UNLIKELY(put > chunk)) { return LDG_ERR_MEM_CORRUPTION; }

        if (put == 0) { break; }

        done += (uint64_t)put;
        slot->pos += (uint64_t)put;

        if (slot->pos > slot->width) { slot->width = slot->pos; }
    }

    *bytes_wr = done;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_seek(ldg_io_file_t *handle, uint64_t offset, uint32_t whence)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint64_t tgt = 0;
    uint32_t ret = 0;

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (whence == LDG_IO_SEEK_START) { tgt = offset; }
    else if (whence == LDG_IO_SEEK_CURRENT) { tgt = slot->pos + offset; }
    else if (whence == LDG_IO_SEEK_END) { tgt = slot->width + offset; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(tgt > (uint64_t)UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    ret = fs_err_translate(state->seek(state->kernel, (uint32_t)slot->file.handle, (uint32_t)tgt));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    slot->pos = tgt;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_pos_get(ldg_io_file_t *handle, uint64_t *pos)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pos = 0;

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *pos = slot->pos;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_info_get(const char *path, ldg_io_info_t *out)
{
    ldg_io_fs_state_t *state = 0x0;
    uint32_t width = 0;
    uint32_t type = 0;
    uint32_t modified_ms = 0;
    uint32_t ret = 0;
    uint8_t modified_known = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(ldg_io_info_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    state = fs_state_get();

    ret = fs_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fs_err_translate(state->info_get(state->kernel, path, &width, &type));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->size = (uint64_t)width;

    if (type == LDG_IO_FS_TYPE_DIR) { out->is_dir = LDG_TRUTH_TRUE; }
    else { out->is_file = LDG_TRUTH_TRUE; }

    if (fs_modified_bind_ensure(state) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    ret = fs_err_translate(state->modified_get(state->kernel, path, &modified_ms, &modified_known));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (modified_known == LDG_TRUTH_TRUE) { out->modified_ns = (uint64_t)modified_ms * LDG_NS_PER_MS; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_info_get(ldg_io_file_t *handle, ldg_io_info_t *out)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(ldg_io_info_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->size = slot->width;

    if (slot->type == LDG_IO_FS_TYPE_DIR) { out->is_dir = LDG_TRUTH_TRUE; }
    else { out->is_file = LDG_TRUTH_TRUE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_commit(ldg_io_file_t *handle)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_truncate(ldg_io_file_t *handle, uint64_t size)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(size > (uint64_t)UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fs_width_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fs_err_translate(state->width_set(state->kernel, (uint32_t)slot->file.handle, (uint32_t)size));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    slot->width = size;

    if (slot->pos > slot->width) { slot->pos = slot->width; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_lock(ldg_io_file_t *handle)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_file_unlock(ldg_io_file_t *handle)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    state = fs_state_get();

    ret = fs_slot_find(state, handle, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_pipe_create(ldg_io_file_t **rd, ldg_io_file_t **wr)
{
    if (LDG_UNLIKELY(!rd || !wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *rd = 0x0;
    *wr = 0x0;

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_file_dup(ldg_io_file_t *src, ldg_io_file_t **out)
{
    ldg_io_fs_state_t *state = 0x0;
    ldg_io_fs_slot_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    state = fs_state_get();

    ret = fs_slot_find(state, src, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_UNSUPPORTED;
}
