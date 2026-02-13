#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

#include <dangling/io/path.h>
#include <dangling/core/err.h>
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

uint32_t ldg_io_path_unlink(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_unlink(path) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_mode_set(const char *path, uint32_t mode)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_chmod(path, (int)mode) < 0)) { return ldg_io_path_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_exists_is(const char *path, uint8_t *exists)
{
    DWORD attrs = 0;

    if (LDG_UNLIKELY(!path || !exists)) { return LDG_ERR_FUNC_ARG_NULL; }

    *exists = 0;

    attrs = GetFileAttributesA(path);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        *exists = 1;
        return LDG_ERR_AOK;
    }

    if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND) { return LDG_ERR_AOK; }

    return LDG_ERR_IO_RD;
}

uint32_t ldg_io_symlink_create(const char *target, const char *link_path)
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

uint32_t ldg_io_symlink_rd(const char *link_path, char *buff, uint64_t buff_size, uint64_t *len)
{
    DWORD result = 0;
    HANDLE h = 0x0;

    if (LDG_UNLIKELY(!link_path || !buff || !len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *len = 0;

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    h = CreateFileA(link_path, GENERIC_READ, FILE_SHARE_READ, 0x0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0x0);
    if (LDG_UNLIKELY(h == INVALID_HANDLE_VALUE)) { return LDG_ERR_IO_NOT_FOUND; }

    result = GetFinalPathNameByHandleA(h, buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE), FILE_NAME_NORMALIZED);
    CloseHandle(h);

    if (LDG_UNLIKELY(result == 0 || result >= buff_size)) { return LDG_ERR_IO_RD; }

    buff[result] = LDG_STR_TERM;
    *len = (uint64_t)result;

    return LDG_ERR_AOK;
}

// path helpers

#define LDG_IO_PATH_MAX_WIN 4096

static uint8_t path_sep_is(char c)
{
    return (c == '/' || c == '\\') ? 1 : 0;
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

    while (base_len > 1 && path_sep_is(base[base_len - 1]))
    {
        if (base_len == 3 && base[1] == ':') { break; }

        base_len--;
    }

    while (name_len > 0 && path_sep_is(name[0])) { name++; name_len--; }

    if (LDG_UNLIKELY(base_len + 1 + name_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(memcpy(buff, base, (size_t)base_len) != buff)) { return LDG_ERR_MEM_BAD; }

    pos = base_len;

    if (name_len > 0)
    {
        buff[pos] = '\\';
        pos++;

        if (LDG_UNLIKELY(memcpy(buff + pos, name, (size_t)name_len) != buff + pos)) { return LDG_ERR_MEM_BAD; }

        pos += name_len;
    }

    buff[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_basename_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t i = 0;
    uint64_t last_sep = UINT64_MAX;
    const char *start = 0x0;
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    while (len > 1 && path_sep_is(path[len - 1]))
    {
        if (len == 3 && path[1] == ':') { break; }

        len--;
    }

    for (i = 0; i < len; i++) { if (path_sep_is(path[i])) { last_sep = i; } }

    if (last_sep == UINT64_MAX) { start = path; name_len = len; }
    else { start = path + last_sep + 1; name_len = len - last_sep - 1; }

    if (LDG_UNLIKELY(name_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(memcpy(buff, start, (size_t)name_len) != buff)) { return LDG_ERR_MEM_BAD; }

    buff[name_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_dirname_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t i = 0;
    uint64_t last_sep = UINT64_MAX;
    uint64_t dir_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    while (len > 1 && path_sep_is(path[len - 1]))
    {
        if (len == 3 && path[1] == ':') { break; }

        len--;
    }

    for (i = 0; i < len; i++) { if (path_sep_is(path[i])) { last_sep = i; } }

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

    if (LDG_UNLIKELY(memcpy(buff, path, (size_t)dir_len) != buff)) { return LDG_ERR_MEM_BAD; }

    buff[dir_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_ext_get(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t i = 0;
    uint64_t last_sep = UINT64_MAX;
    uint64_t dot_pos = UINT64_MAX;
    uint64_t base_start = 0;
    uint64_t ext_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_NOT_FOUND; }

    for (i = 0; i < len; i++) { if (path_sep_is(path[i])) { last_sep = i; } }

    base_start = (last_sep == UINT64_MAX) ? 0 : last_sep + 1;

    for (i = len; i > base_start; i--) { if (path[i - 1] == '.') { dot_pos = i - 1; break; } }

    if (dot_pos == UINT64_MAX || dot_pos == base_start) { return LDG_ERR_NOT_FOUND; }

    ext_len = len - dot_pos;
    if (LDG_UNLIKELY(ext_len + LDG_STR_TERM_SIZE > buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(memcpy(buff, path + dot_pos, (size_t)ext_len) != buff)) { return LDG_ERR_MEM_BAD; }

    buff[ext_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint8_t ldg_io_path_absolute_is(const char *path)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(!path)) { return 0; }

    len = strlen(path);

    if (len >= 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':' && path_sep_is(path[2])) { return LDG_TRUTH_TRUE; }

    if (len >= 2 && path_sep_is(path[0]) && path_sep_is(path[1])) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_io_path_normalize(const char *path, char *buff, uint64_t buff_size)
{
    uint64_t len = 0;
    uint64_t i = 0;
    uint64_t pos = 0;
    uint64_t root_len = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    len = strlen(path);
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    for (i = 0; i < len; i++)
    {
        if (LDG_UNLIKELY(pos >= buff_size - LDG_STR_TERM_SIZE)) { return LDG_ERR_MEM_STR_TRUNC; }

        if (path_sep_is(path[i]))
        {
            if (pos > 0 && buff[pos - 1] == '\\') { continue; }

            buff[pos] = '\\';
        }
        else { buff[pos] = path[i]; }

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

uint32_t ldg_io_path_home_get(char *buff, uint64_t buff_size)
{
    DWORD result = 0;
    const char *fallback = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    result = GetEnvironmentVariableA("USERPROFILE", buff, (DWORD)(buff_size - LDG_STR_TERM_SIZE));
    if (result > 0 && result < buff_size)
    {
        buff[result] = LDG_STR_TERM;
        return LDG_ERR_AOK;
    }

    fallback = getenv("USERPROFILE");
    if (fallback) { return ldg_strrbrcpy(buff, fallback, buff_size); }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_io_path_tmp_get(char *buff, uint64_t buff_size)
{
    DWORD result = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    result = GetTempPathA((DWORD)buff_size, buff);
    if (LDG_UNLIKELY(result == 0 || result >= buff_size)) { return LDG_ERR_IO_RD; }

    while (result > 1 && (buff[result - 1] == '\\' || buff[result - 1] == '/'))
    {
        result--;
        buff[result] = LDG_STR_TERM;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_path_resolve(const char *path, char *buff, uint64_t buff_size)
{
    DWORD result = 0;

    if (LDG_UNLIKELY(!path || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    result = GetFullPathNameA(path, (DWORD)buff_size, buff, 0x0);
    if (LDG_UNLIKELY(result == 0)) { return LDG_ERR_IO_RD; }

    if (LDG_UNLIKELY(result >= buff_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    buff[result] = LDG_STR_TERM;

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

    if (path[1] != LDG_STR_TERM && !path_sep_is(path[1])) { return ldg_io_path_normalize(path, buff, buff_size); }

    ret = ldg_mem_pool_create(0, (uint64_t)LDG_IO_PATH_MAX_WIN, &scratch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(scratch, (uint64_t)LDG_IO_PATH_MAX_WIN, (void **)&home);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_io_path_home_get(home, LDG_IO_PATH_MAX_WIN);
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
