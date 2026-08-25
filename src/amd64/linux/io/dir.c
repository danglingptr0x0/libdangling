#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dangling/io/dir.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

static uint32_t ldg_io_dir_errno_translate(void)
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
        case ENOTDIR:
            return LDG_ERR_IO_NOT_DIR;
        case ENOTEMPTY:
            return LDG_ERR_BUSY;
        default:
            return LDG_ERR_IO_RD;
    }
}

uint32_t ldg_io_dir_create(const char *path, uint32_t access)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mkdir(path, (mode_t)access) < 0)) { return ldg_io_dir_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_destroy(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rmdir(path) < 0)) { return ldg_io_dir_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_open(const char *path, ldg_io_dir_t **out)
{
    ldg_io_dir_t *dir = 0x0;
    DIR *native_dir = 0x0;
    uint64_t path_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    path_len = (uint64_t)strnlen(path, LDG_IO_PATH_MAX);
    if (LDG_UNLIKELY(path_len >= LDG_IO_PATH_MAX - 1)) { return LDG_ERR_OVERFLOW; }

    native_dir = opendir(path);
    if (LDG_UNLIKELY(!native_dir)) { return ldg_io_dir_errno_translate(); }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_dir), (void **)&dir);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        closedir(native_dir);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(memset(dir, 0, sizeof(struct ldg_io_dir)) != dir))
    {
        closedir(native_dir);
        ldg_mem_dealloc(dir);
        return LDG_ERR_MEM_BAD;
    }

    dir->handle = (void *)native_dir;

    ret = ldg_mem_alloc((uint64_t)LDG_IO_DIR_FULL_PATH_MAX, (void **)&dir->full_path);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        closedir(native_dir);
        ldg_mem_dealloc(dir);
        return LDG_ERR_ALLOC_NULL;
    }

    ret = ldg_str_rbrcpy(dir->path, path, LDG_IO_PATH_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        closedir(native_dir);
        ldg_mem_dealloc(dir->full_path);
        ldg_mem_dealloc(dir);
        return ret;
    }

    dir->path_len = path_len;

    if (dir->path_len > 0 && dir->path[dir->path_len - 1] != '/')
    {
        dir->path[dir->path_len] = '/';
        dir->path_len++;
        dir->path[dir->path_len] = LDG_STR_TERM;
    }

    *out = dir;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_rd(ldg_io_dir_t *dir, ldg_io_entry_t *out)
{
    DIR *native_dir = 0x0;
    struct dirent *native_entry = 0x0;
    struct stat sb = LDG_STRUCT_ZERO_INIT;
    uint64_t name_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dir || !out || !dir->full_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(out, LDG_STR_TERM, sizeof(ldg_io_entry_t)) != out)) { return LDG_ERR_MEM_BAD; }

    native_dir = (DIR *)dir->handle;
    errno = 0;
    native_entry = readdir(native_dir);

    if (!native_entry)
    {
        if (errno != 0) { return ldg_io_dir_errno_translate(); }

        return LDG_ERR_EOF;
    }

    ret = ldg_str_rbrcpy(out->name, native_entry->d_name, LDG_IO_ENTRY_NAME_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP)) { return ret; }

    name_len = (uint64_t)strnlen(native_entry->d_name, LDG_IO_ENTRY_NAME_MAX);
    if (dir->path_len + name_len < LDG_IO_DIR_FULL_PATH_MAX)
    {
        if (LDG_UNLIKELY(ldg_mem_secure_cpy(dir->full_path, dir->path, (uint64_t)dir->path_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        if (LDG_UNLIKELY(ldg_mem_secure_cpy(dir->full_path + dir->path_len, native_entry->d_name, (uint64_t)name_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        dir->full_path[dir->path_len + name_len] = LDG_STR_TERM;

        if (lstat(dir->full_path, &sb) == 0) { out->is_dir = S_ISDIR(sb.st_mode) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_close(ldg_io_dir_t *dir)
{
    DIR *native_dir = 0x0;

    if (LDG_UNLIKELY(!dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    native_dir = (DIR *)dir->handle;

    if (dir->full_path) { ldg_mem_dealloc(dir->full_path); }

    if (LDG_UNLIKELY(closedir(native_dir) < 0))
    {
        ldg_mem_dealloc(dir);
        return LDG_ERR_IO_CLOSE;
    }

    ldg_mem_dealloc(dir);

    return LDG_ERR_AOK;
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
