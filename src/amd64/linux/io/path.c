#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dangling/io/path.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

static uint32_t ldg_io_path_errno_translate(void)
{
    switch (errno)
    {
        case ENOENT:
            return LDG_ERR_IO_NOT_FOUND;
        case EACCES:
        case EPERM:
            return LDG_ERR_IO_PERM;
        case EEXIST:
            return LDG_ERR_EXISTS;
        case EISDIR:
            return LDG_ERR_IO_IS_DIR;
        case ENOTDIR:
            return LDG_ERR_IO_NOT_DIR;
        default:
            return LDG_ERR_IO_WR;
    }
}

uint32_t ldg_io_path_rename(const char *old_path, const char *new_path)
{
    if (LDG_UNLIKELY(!old_path || !new_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rename(old_path, new_path) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_destroy(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(unlink(path) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_access_set(const char *path, uint32_t access)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(chmod(path, (mode_t)access) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_path_exists_is(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (access(path, F_OK) == 0) { return LDG_TRUTH_TRUE; }

    if (errno == ENOENT) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_MAYBE;
}

uint32_t ldg_io_link_create(const char *target, const char *link_path)
{
    if (LDG_UNLIKELY(!target || !link_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(symlink(target, link_path) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_link_rd(const char *link_path, char *buff, uint64_t buff_size, uint64_t *len)
{
    int64_t res = 0;

    if (LDG_UNLIKELY(!link_path || !buff || !len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *len = 0;

    res = (int64_t)readlink(link_path, buff, (uint64_t)buff_size - LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(res < 0)) { return ldg_io_path_errno_translate(); }

    buff[res] = LDG_STR_TERM;
    *len = (uint64_t)res;

    return LDG_ERR_AOK;
}

uint8_t ldg_io_path_sep_get(void)
{
    return '/';
}

uint32_t ldg_io_path_join(const char *base, const char *name, char *buff, uint64_t buff_size)
{
    uint64_t base_len = 0;
    uint64_t name_len = 0;
    uint64_t pos = 0;

    if (LDG_UNLIKELY(!base || !name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    base_len = strlen(base);
    name_len = strlen(name);

    while (base_len > 1 && base[base_len - 1] == '/') { base_len--; }

    while (name_len > 0 && name[0] == '/') { name++; name_len--; }

    if (LDG_UNLIKELY(base_len + 1 + name_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, base, (uint64_t)base_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    pos = base_len;

    if (name_len > 0)
    {
        buff[pos] = '/';
        pos++;

        if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff + pos, name, (uint64_t)name_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        pos += name_len;
    }

    buff[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_leaf_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t idx = 0;
    uint64_t last_sep = UINT64_MAX;
    const char *start = 0x0;
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    while (len > 1 && path[len - 1] == '/') { len--; }

    for (idx = 0; idx < len; idx++) { if (path[idx] == '/') { last_sep = idx; } }

    if (last_sep == UINT64_MAX) { start = path; name_len = len; }
    else { start = path + last_sep + 1; name_len = len - last_sep - 1; }

    if (LDG_UNLIKELY(name_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, start, (uint64_t)name_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    buff[name_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_parent_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t idx = 0;
    uint64_t last_sep = UINT64_MAX;
    uint64_t dir_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    while (len > 1 && path[len - 1] == '/') { len--; }

    for (idx = 0; idx < len; idx++) { if (path[idx] == '/') { last_sep = idx; } }

    if (last_sep == UINT64_MAX)
    {
        buff[0] = '.';
        buff[1] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    if (last_sep == 0) { dir_len = 1; }
    else { dir_len = last_sep; }

    if (LDG_UNLIKELY(dir_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, path, (uint64_t)dir_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    buff[dir_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_ext_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t idx = 0;
    uint64_t last_sep = UINT64_MAX;
    uint64_t dot_pos = UINT64_MAX;
    uint64_t base_start = 0;
    uint64_t ext_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_NOT_FOUND; }

    for (idx = 0; idx < len; idx++) { if (path[idx] == '/') { last_sep = idx; } }

    base_start = (last_sep == UINT64_MAX) ? 0 : last_sep + 1;

    for (idx = len; idx > base_start; idx--) { if (path[idx - 1] == '.') { dot_pos = idx - 1; break; } }

    if (dot_pos == UINT64_MAX || dot_pos == base_start) { return LDG_ERR_NOT_FOUND; }

    ext_len = len - dot_pos;
    if (LDG_UNLIKELY(ext_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, path + dot_pos, (uint64_t)ext_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    buff[ext_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint8_t ldg_io_path_absolute_is(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (path[0] == '/') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_io_path_normalize(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t idx = 0;
    uint64_t pos = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    for (idx = 0; idx < len; idx++)
    {
        if (LDG_UNLIKELY(pos >= buff_size - LDG_STR_TERM_SIZE)) { return LDG_ERR_MEM_STR_TRUNC; }

        if (path[idx] == '/' && pos > 0 && buff[pos - 1] == '/') { continue; }

        buff[pos] = path[idx];
        pos++;
    }

    while (pos > 1 && buff[pos - 1] == '/') { pos--; }

    buff[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_user_get(char *buff, uint64_t buff_size)
{
    const char *home = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    home = getenv("HOME");
    if (LDG_UNLIKELY(!home)) { return LDG_ERR_NOT_FOUND; }

    return ldg_str_rbrcpy(buff, home, buff_size);
}

uint32_t ldg_io_path_work_get(char *buff, uint64_t buff_size)
{
    const char *tmp = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    tmp = getenv("TMPDIR");
    if (tmp) { return ldg_str_rbrcpy(buff, tmp, buff_size); }

    return ldg_str_rbrcpy(buff, "/tmp", buff_size);
}

uint32_t ldg_io_path_prog_get(char *buff, uint64_t buff_size)
{
    int64_t res = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = (int64_t)readlink("/proc/self/exe", buff, (uint64_t)buff_size - LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(res < 0)) { return ldg_io_path_errno_translate(); }

    if (LDG_UNLIKELY((uint64_t)res >= (uint64_t)buff_size - LDG_STR_TERM_SIZE)) { return LDG_ERR_STR_TRUNC; }

    buff[res] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_conf_get(char *buff, uint64_t buff_size)
{
    const char *val = 0x0;
    uint64_t len = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv("XDG_CONFIG_HOME");
    if (val && val[0] == '/') { return ldg_str_rbrcpy(buff, val, buff_size); }

    err = ldg_io_path_user_get(buff, buff_size);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    len = strlen(buff);
    if (LDG_UNLIKELY(len + strlen("/.config") + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    return ldg_str_rbrcpy(buff + len, "/.config", buff_size - len);
}

uint32_t ldg_io_path_data_get(char *buff, uint64_t buff_size)
{
    const char *val = 0x0;
    uint64_t len = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv("XDG_DATA_HOME");
    if (val && val[0] == '/') { return ldg_str_rbrcpy(buff, val, buff_size); }

    err = ldg_io_path_user_get(buff, buff_size);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    len = strlen(buff);
    if (LDG_UNLIKELY(len + strlen("/.local/share") + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    return ldg_str_rbrcpy(buff + len, "/.local/share", buff_size - len);
}

uint32_t ldg_io_path_cache_get(char *buff, uint64_t buff_size)
{
    const char *val = 0x0;
    uint64_t len = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv("XDG_CACHE_HOME");
    if (val && val[0] == '/') { return ldg_str_rbrcpy(buff, val, buff_size); }

    err = ldg_io_path_user_get(buff, buff_size);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    len = strlen(buff);
    if (LDG_UNLIKELY(len + strlen("/.cache") + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    return ldg_str_rbrcpy(buff + len, "/.cache", buff_size - len);
}

uint32_t ldg_io_path_state_get(char *buff, uint64_t buff_size)
{
    const char *val = 0x0;
    uint64_t len = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv("XDG_STATE_HOME");
    if (val && val[0] == '/') { return ldg_str_rbrcpy(buff, val, buff_size); }

    err = ldg_io_path_user_get(buff, buff_size);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    len = strlen(buff);
    if (LDG_UNLIKELY(len + strlen("/.local/state") + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    return ldg_str_rbrcpy(buff + len, "/.local/state", buff_size - len);
}

uint32_t ldg_io_path_resolve(const char *path, char *buff, uint64_t buff_size)
{
    ldg_mem_pool_t *scratch = 0x0;
    char *resolved = 0x0;
    char *component = 0x0;
    char *link_target = 0x0;
    char *tmp = 0x0;
    uint64_t rpos = 0;
    uint64_t ppos = 0;
    uint64_t plen = 0;
    uint64_t clen = 0;
    uint64_t llen = 0;
    int64_t link_res = 0;
    uint32_t symlink_depth = 0;
    uint32_t ret = 0;
    struct stat st = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    ret = ldg_mem_pool_create(0, (uint64_t)PATH_MAX * 4, &scratch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)PATH_MAX, (void **)&resolved);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)PATH_MAX, (void **)&component);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)PATH_MAX, (void **)&link_target);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)PATH_MAX, (void **)&tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    plen = (uint64_t)strnlen(path, PATH_MAX);
    if (LDG_UNLIKELY(plen == 0)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_STR_EMPTY; }

    if (path[0] != '/')
    {
        if (LDG_UNLIKELY(!getcwd(resolved, PATH_MAX))) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_IO_RD; }

        rpos = (uint64_t)strnlen(resolved, PATH_MAX);
    }
    else
    {
        resolved[0] = '/';
        rpos = 1;
    }

    while (ppos < plen)
    {
        while (ppos < plen && path[ppos] == '/') { ppos++; }

        if (ppos >= plen) { break; }

        clen = 0;
        while (ppos < plen && path[ppos] != '/' && clen < PATH_MAX - 1)
        {
            component[clen] = path[ppos];
            clen++;
            ppos++;
        }
        component[clen] = LDG_STR_TERM;

        if (clen == 1 && component[0] == '.') { continue; }

        if (clen == 2 && component[0] == '.' && component[1] == '.')
        {
            if (rpos > 1)
            {
                rpos--;
                while (rpos > 1 && resolved[rpos - 1] != '/') { rpos--; }
            }

            resolved[rpos] = LDG_STR_TERM;
            continue;
        }

        if (rpos > 1)
        {
            if (LDG_UNLIKELY(rpos >= PATH_MAX - 1)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_OVERFLOW; }

            resolved[rpos] = '/';
            rpos++;
        }

        if (LDG_UNLIKELY(rpos + clen >= PATH_MAX)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_OVERFLOW; }

        if (LDG_UNLIKELY(ldg_mem_secure_cpy(resolved + rpos, component, (uint64_t)clen) != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

        rpos += clen;
        resolved[rpos] = LDG_STR_TERM;

        if (LDG_UNLIKELY(memset(&st, 0, sizeof(st)) != &st)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

        if (LDG_UNLIKELY(lstat(resolved, &st) < 0)) { ldg_mem_pool_destroy(&scratch); return ldg_io_path_errno_translate(); }

        if (S_ISLNK(st.st_mode))
        {
            symlink_depth++;
            if (LDG_UNLIKELY(symlink_depth > 40)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_IO_RD; }

            link_res = (int64_t)readlink(resolved, link_target, PATH_MAX - 1);
            if (LDG_UNLIKELY(link_res < 0)) { ldg_mem_pool_destroy(&scratch); return ldg_io_path_errno_translate(); }

            link_target[link_res] = LDG_STR_TERM;
            llen = (uint64_t)link_res;

            if (LDG_UNLIKELY(memset(tmp, 0, PATH_MAX) != tmp)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

            if (LDG_UNLIKELY(ldg_mem_secure_cpy(tmp, link_target, llen) != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

            if (ppos < plen)
            {
                if (LDG_UNLIKELY(llen + 1 + (plen - ppos) >= PATH_MAX)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_OVERFLOW; }

                tmp[llen] = '/';
                if (LDG_UNLIKELY(ldg_mem_secure_cpy(tmp + llen + 1, path + ppos, plen - ppos) != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

                tmp[llen + 1 + (plen - ppos)] = LDG_STR_TERM;
            }

            path = tmp;
            plen = (uint64_t)strnlen(tmp, PATH_MAX);
            ppos = 0;

            if (link_target[0] == '/')
            {
                resolved[0] = '/';
                rpos = 1;
                resolved[1] = LDG_STR_TERM;
            }
            else
            {
                if (rpos > 1)
                {
                    rpos--;
                    while (rpos > 1 && resolved[rpos - 1] != '/') { rpos--; }
                }

                resolved[rpos] = LDG_STR_TERM;
            }
        }
    }

    if (rpos > 1 && resolved[rpos - 1] == '/') { rpos--; }

    resolved[rpos] = LDG_STR_TERM;

    ret = ldg_str_rbrcpy(buff, resolved, buff_size);
    ldg_mem_pool_destroy(&scratch);

    return ret;
}

uint32_t ldg_io_path_expand(const char *path, char *buff, uint64_t buff_size)
{
    ldg_mem_pool_t *scratch = 0x0;
    char *home = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    if (path[0] != '~') { return ldg_io_path_normalize(path, buff, buff_size); }

    if (path[1] != LDG_STR_TERM && path[1] != '/') { return ldg_io_path_normalize(path, buff, buff_size); }

    ret = ldg_mem_pool_create(0, (uint64_t)PATH_MAX, &scratch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)PATH_MAX, (void **)&home);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_io_path_user_get(home, PATH_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    if (path[1] == LDG_STR_TERM)
    {
        ret = ldg_io_path_normalize(home, buff, buff_size);
        ldg_mem_pool_destroy(&scratch);
        return ret;
    }

    ret = ldg_io_path_join(home, path + 2, buff, buff_size);
    ldg_mem_pool_destroy(&scratch);

    return ret;
}
