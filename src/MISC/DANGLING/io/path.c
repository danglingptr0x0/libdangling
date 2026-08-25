#include <stdint.h>

#include <dangling/io/path.h>
#include <dangling/io/file.h>
#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_PATH_SEP '<'
#define LDG_IO_PATH_EXT_SEP '.'
#define LDG_IO_PATH_SEP_SKIP 1
#define LDG_IO_PATH_LAST_OFF 1
#define LDG_IO_PATH_TERM_OFF 1

#define LDG_IO_PATH_ROOT_USER "<user"
#define LDG_IO_PATH_ROOT_WORK "<work"
#define LDG_IO_PATH_ROOT_PROG "<prog"
#define LDG_IO_PATH_ROOT_CONF "<conf"
#define LDG_IO_PATH_ROOT_DATA "<data"
#define LDG_IO_PATH_ROOT_CACHE "<cache"
#define LDG_IO_PATH_ROOT_STATE "<state"

static uint32_t path_root_get(const char *root, char *buff, uint64_t buff_size)
{
    if (LDG_UNLIKELY(!root || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_str_rbrcpy(buff, root, buff_size);
}

uint8_t ldg_io_path_sep_get(void)
{
    return (uint8_t)LDG_IO_PATH_SEP;
}

uint8_t ldg_io_path_absolute_is(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (path[0] == LDG_IO_PATH_SEP) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_io_path_exists_is(const char *path)
{
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ldg_io_path_info_get(path, &info) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_io_path_join(const char *base, const char *name, char *buff, uint64_t buff_size)
{
    uint64_t base_len = 0;
    uint64_t cursor = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!base || !name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_len_get(base, buff_size, &base_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_rbrcpy(buff, base, buff_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cursor = base_len;

    if (base_len > 0 && buff[base_len - LDG_IO_PATH_LAST_OFF] != LDG_IO_PATH_SEP)
    {
        if (LDG_UNLIKELY(cursor + LDG_IO_PATH_TERM_OFF >= buff_size)) { return LDG_ERR_STR_TRUNC; }

        buff[cursor] = LDG_IO_PATH_SEP;
        cursor++;
        buff[cursor] = LDG_STR_TERM;
    }

    return ldg_str_concatenate(buff, name, buff_size);
}

uint32_t ldg_io_path_leaf_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_char_last_find(path, LDG_IO_PATH_SEP, LDG_IO_PATH_MAX, &idx);
    if (ret != LDG_ERR_AOK) { return ldg_str_rbrcpy(buff, path, buff_size); }

    return ldg_str_rbrcpy(buff, path + idx + LDG_IO_PATH_SEP_SKIP, buff_size);
}

uint32_t ldg_io_path_parent_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t idx = 0;
    uint64_t keep = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[0] = LDG_STR_TERM;

    ret = ldg_str_char_last_find(path, LDG_IO_PATH_SEP, LDG_IO_PATH_MAX, &idx);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    keep = idx;
    if (keep == 0) { keep = LDG_IO_PATH_SEP_SKIP; }

    if (LDG_UNLIKELY(keep + LDG_IO_PATH_TERM_OFF > buff_size)) { return LDG_ERR_STR_TRUNC; }

    for (idx = 0; idx < keep; idx++) { buff[idx] = path[idx]; }

    buff[keep] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_ext_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t dot_idx = 0;
    uint64_t sep_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[0] = LDG_STR_TERM;

    ret = ldg_str_char_last_find(path, LDG_IO_PATH_EXT_SEP, LDG_IO_PATH_MAX, &dot_idx);
    if (ret != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (ldg_str_char_last_find(path, LDG_IO_PATH_SEP, LDG_IO_PATH_MAX, &sep_idx) == LDG_ERR_AOK) { if (dot_idx < sep_idx) { return LDG_ERR_AOK; } }

    return ldg_str_rbrcpy(buff, path + dot_idx + LDG_IO_PATH_SEP_SKIP, buff_size);
}

uint32_t ldg_io_path_normalize(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t src_idx = 0;
    uint64_t dst_idx = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_len_get(path, LDG_IO_PATH_MAX, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (src_idx = 0; src_idx < len; src_idx++)
    {
        if (path[src_idx] == LDG_IO_PATH_SEP && dst_idx > 0 && buff[dst_idx - LDG_IO_PATH_LAST_OFF] == LDG_IO_PATH_SEP) { continue; }

        if (LDG_UNLIKELY(dst_idx + LDG_IO_PATH_TERM_OFF >= buff_size)) { return LDG_ERR_STR_TRUNC; }

        buff[dst_idx] = path[src_idx];
        dst_idx++;
    }

    if (dst_idx > LDG_IO_PATH_SEP_SKIP && buff[dst_idx - LDG_IO_PATH_LAST_OFF] == LDG_IO_PATH_SEP) { dst_idx--; }

    buff[dst_idx] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_resolve(const char *path, char *buff, uint64_t buff_size)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_io_path_absolute_is(path) == LDG_TRUTH_TRUE) { return ldg_io_path_normalize(path, buff, buff_size); }

    if (LDG_UNLIKELY(buff_size < LDG_IO_PATH_SEP_SKIP + LDG_IO_PATH_TERM_OFF)) { return LDG_ERR_STR_TRUNC; }

    buff[0] = LDG_IO_PATH_SEP;
    buff[LDG_IO_PATH_SEP_SKIP] = LDG_STR_TERM;

    ret = ldg_str_concatenate(buff, path, buff_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_expand(const char *path, char *buff, uint64_t buff_size)
{
    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_str_rbrcpy(buff, path, buff_size);
}

uint32_t ldg_io_path_user_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_USER, buff, buff_size);
}

uint32_t ldg_io_path_work_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_WORK, buff, buff_size);
}

uint32_t ldg_io_path_prog_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_PROG, buff, buff_size);
}

uint32_t ldg_io_path_conf_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_CONF, buff, buff_size);
}

uint32_t ldg_io_path_data_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_DATA, buff, buff_size);
}

uint32_t ldg_io_path_cache_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_CACHE, buff, buff_size);
}

uint32_t ldg_io_path_state_get(char *buff, uint64_t buff_size)
{
    return path_root_get(LDG_IO_PATH_ROOT_STATE, buff, buff_size);
}

uint32_t ldg_io_path_rename(const char *old_path, const char *new_path)
{
    if (LDG_UNLIKELY(!old_path || !new_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_path_destroy(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_path_access_set(const char *path, uint32_t access)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(access == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_link_create(const char *tgt, const char *link_path)
{
    if (LDG_UNLIKELY(!tgt || !link_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_io_link_rd(const char *link_path, char *buff, uint64_t buff_size, uint64_t *len)
{
    if (LDG_UNLIKELY(!link_path || !buff || !len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *len = 0;

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}
