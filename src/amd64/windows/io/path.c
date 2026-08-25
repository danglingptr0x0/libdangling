#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

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
        default:
            return LDG_ERR_IO_WR;
    }
}

uint32_t ldg_io_path_rename(const char *old_path, const char *new_path)
{
    if (LDG_UNLIKELY(!old_path || !new_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!MoveFileExA(old_path, new_path, MOVEFILE_REPLACE_EXISTING))) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_destroy(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_unlink(path) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_access_set(const char *path, uint32_t access)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_chmod(path, (int)access) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_path_exists_is(const char *path)
{
    DWORD attr = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    attr = GetFileAttributesA(path);
    if (attr != INVALID_FILE_ATTRIBUTES) { return LDG_TRUTH_TRUE; }

    if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_MAYBE;
}

uint32_t ldg_io_link_create(const char *target, const char *link_path)
{
    DWORD flags = 0;
    DWORD attrs = 0;

    if (LDG_UNLIKELY(!target || !link_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    attrs = GetFileAttributesA(target);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) { flags = SYMBOLIC_LINK_FLAG_DIRECTORY; }

    flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

    if (LDG_UNLIKELY(!CreateSymbolicLinkA(link_path, target, flags))) { return LDG_ERR_IO_PERM; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_link_rd(const char *link_path, char *buff, uint64_t buff_size, uint64_t *len)
{
    DWORD res = 0;
    HANDLE h = 0x0;

    if (LDG_UNLIKELY(!link_path || !buff || !len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *len = 0;

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    h = CreateFileA(link_path, GENERIC_READ, FILE_SHARE_READ, 0x0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0x0);
    if (LDG_UNLIKELY(h == INVALID_HANDLE_VALUE)) { return LDG_ERR_IO_NOT_FOUND; }

    res = GetFinalPathNameByHandleA(h, buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE), FILE_NAME_NORMALIZED);
    CloseHandle(h);

    if (LDG_UNLIKELY(res == 0 || res >= buff_size)) { return LDG_ERR_IO_RD; }

    buff[res] = LDG_STR_TERM;
    *len = (uint64_t)res;

    return LDG_ERR_AOK;
}

static uint8_t path_sep_is(char character)
{
    if (character == '/' || character == '\\') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_io_path_sep_get(void)
{
    return '\\';
}

uint32_t ldg_io_path_join(const char *base, const char *name, char *buff, uint64_t buff_size)
{
    uint64_t base_len = 0;
    uint64_t name_len = 0;
    uint64_t pos = 0;

    if (LDG_UNLIKELY(!base || !name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    base_len = strlen(base);
    name_len = strlen(name);

    while (base_len > 1 && path_sep_is(base[base_len - 1]) == LDG_TRUTH_TRUE)
    {
        if (base_len == 3 && base[1] == ':') { break; }

        base_len--;
    }

    while (name_len > 0 && path_sep_is(name[0]) == LDG_TRUTH_TRUE) { name++; name_len--; }

    if (LDG_UNLIKELY(base_len + 1 + name_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, base, (size_t)base_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    pos = base_len;

    if (name_len > 0)
    {
        buff[pos] = '\\';
        pos++;

        if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff + pos, name, (size_t)name_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

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

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    while (len > 1 && path_sep_is(path[len - 1]) == LDG_TRUTH_TRUE)
    {
        if (len == 3 && path[1] == ':') { break; }

        len--;
    }

    for (idx = 0; idx < len; idx++) { if (path_sep_is(path[idx]) == LDG_TRUTH_TRUE) { last_sep = idx; } }

    if (last_sep == UINT64_MAX) { start = path; name_len = len; }
    else { start = path + last_sep + 1; name_len = len - last_sep - 1; }

    if (LDG_UNLIKELY(name_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, start, (size_t)name_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

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

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    while (len > 1 && path_sep_is(path[len - 1]) == LDG_TRUTH_TRUE)
    {
        if (len == 3 && path[1] == ':') { break; }

        len--;
    }

    for (idx = 0; idx < len; idx++) { if (path_sep_is(path[idx]) == LDG_TRUTH_TRUE) { last_sep = idx; } }

    if (last_sep == UINT64_MAX)
    {
        buff[0] = '.';
        buff[1] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    if (last_sep == 2 && path[1] == ':') { dir_len = 3; }
    else if (last_sep == 0) { dir_len = 1; }
    else { dir_len = last_sep; }

    if (LDG_UNLIKELY(dir_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, path, (size_t)dir_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

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

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_NOT_FOUND; }

    for (idx = 0; idx < len; idx++) { if (path_sep_is(path[idx]) == LDG_TRUTH_TRUE) { last_sep = idx; } }

    base_start = (last_sep == UINT64_MAX) ? 0 : last_sep + 1;

    for (idx = len; idx > base_start; idx--) { if (path[idx - 1] == '.') { dot_pos = idx - 1; break; } }

    if (dot_pos == UINT64_MAX || dot_pos == base_start) { return LDG_ERR_NOT_FOUND; }

    ext_len = len - dot_pos;
    if (LDG_UNLIKELY(ext_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(buff, path + dot_pos, (size_t)ext_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    buff[ext_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint8_t ldg_io_path_absolute_is(const char *path)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    len = strlen(path);

    if (len >= 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':' && path_sep_is(path[2]) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (len >= 2 && path_sep_is(path[0]) == LDG_TRUTH_TRUE && path_sep_is(path[1]) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_io_path_normalize(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t idx = 0;
    uint64_t pos = 0;
    uint64_t root_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    for (idx = 0; idx < len; idx++)
    {
        if (LDG_UNLIKELY(pos >= buff_size - LDG_STR_TERM_SIZE)) { return LDG_ERR_MEM_STR_TRUNC; }

        if (path_sep_is(path[idx]) == LDG_TRUTH_TRUE)
        {
            if (pos > 0 && buff[pos - 1] == '\\') { continue; }

            buff[pos] = '\\';
        }
        else { buff[pos] = path[idx]; }

        pos++;
    }

    if (pos >= 3 && buff[1] == ':' && buff[2] == '\\') { root_len = 3; }
    else if (pos >= 2 && buff[0] == '\\' && buff[1] == '\\') { root_len = 2; }
    else { root_len = 0; }

    while (pos > root_len && buff[pos - 1] == '\\') { pos--; }

    if (pos == 0) { buff[0] = '\\'; pos = 1; }

    buff[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_user_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;
    const char *fallback = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetEnvironmentVariableA("USERPROFILE", buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE));
    if (res > 0 && res < buff_size)
    {
        buff[res] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    fallback = getenv("USERPROFILE");
    if (fallback) { return ldg_str_rbrcpy(buff, fallback, buff_size); }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_io_path_work_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetTempPathA((DWORD)buff_size, buff);
    if (LDG_UNLIKELY(res == 0 || res >= buff_size)) { return LDG_ERR_IO_RD; }

    while (res > 1 && (buff[res - 1] == '\\' || buff[res - 1] == '/'))
    {
        res--;
        buff[res] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_prog_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetModuleFileNameA(0x0, buff, (DWORD)buff_size);
    if (LDG_UNLIKELY(res == 0)) { return LDG_ERR_IO_RD; }

    if (LDG_UNLIKELY((uint64_t)res >= buff_size)) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_conf_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;
    const char *fallback = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetEnvironmentVariableA("APPDATA", buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE));
    if (res > 0 && res < buff_size)
    {
        buff[res] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    fallback = getenv("APPDATA");
    if (fallback) { return ldg_str_rbrcpy(buff, fallback, buff_size); }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_io_path_data_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;
    const char *fallback = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetEnvironmentVariableA("LOCALAPPDATA", buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE));
    if (res > 0 && res < buff_size)
    {
        buff[res] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    fallback = getenv("LOCALAPPDATA");
    if (fallback) { return ldg_str_rbrcpy(buff, fallback, buff_size); }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_io_path_cache_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;
    const char *fallback = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetEnvironmentVariableA("LOCALAPPDATA", buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE));
    if (res > 0 && res < buff_size)
    {
        buff[res] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    fallback = getenv("LOCALAPPDATA");
    if (fallback) { return ldg_str_rbrcpy(buff, fallback, buff_size); }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_io_path_state_get(char *buff, uint64_t buff_size)
{
    DWORD res = 0;
    const char *fallback = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (uint64_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetEnvironmentVariableA("LOCALAPPDATA", buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE));
    if (res > 0 && res < buff_size)
    {
        buff[res] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    fallback = getenv("LOCALAPPDATA");
    if (fallback) { return ldg_str_rbrcpy(buff, fallback, buff_size); }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_io_path_resolve(const char *path, char *buff, uint64_t buff_size)
{
    DWORD res = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    res = GetFullPathNameA(path, (DWORD)buff_size, buff, 0x0);
    if (LDG_UNLIKELY(res == 0)) { return LDG_ERR_IO_RD; }

    if (LDG_UNLIKELY(res >= buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    buff[res] = LDG_STR_TERM;

    return LDG_ERR_AOK;
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

    if (path[1] != LDG_STR_TERM && path_sep_is(path[1]) != LDG_TRUTH_TRUE) { return ldg_io_path_normalize(path, buff, buff_size); }

    ret = ldg_mem_pool_create(0, (uint64_t)LDG_IO_PATH_MAX, &scratch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)LDG_IO_PATH_MAX, (void **)&home);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_io_path_user_get(home, LDG_IO_PATH_MAX);
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
