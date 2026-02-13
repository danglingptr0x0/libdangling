#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <direct.h>
#include <windows.h>

#include <dangling/io/dir.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#define LDG_IO_DIR_PATH_MAX 4096

struct ldg_io_dir
{
    void *handle;
    char path[LDG_IO_DIR_PATH_MAX];
    uint64_t path_len;
    uint8_t pudding[40];
};

struct ldg_io_dirent
{
    char name[256];
    uint8_t is_dir;
    uint8_t pudding[7];
};

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

uint32_t ldg_io_dir_create(const char *path, uint32_t mode)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (mode == 0) { mode = 0755; }

    if (LDG_UNLIKELY(_mkdir(path) < 0)) { return ldg_io_dir_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_destroy(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_rmdir(path) < 0)) { return ldg_io_dir_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_open(const char *path, ldg_io_dir_t **out)
{
    ldg_io_dir_t *d = 0x0;
    DIR *dir = 0x0;
    uint64_t path_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    path_len = (uint64_t)strnlen(path, LDG_IO_DIR_PATH_MAX);
    if (LDG_UNLIKELY(path_len >= LDG_IO_DIR_PATH_MAX - 1)) { return LDG_ERR_OVERFLOW; }

    dir = opendir(path);
    if (LDG_UNLIKELY(!dir)) { return ldg_io_dir_errno_translate(); }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_dir), (void **)&d);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        closedir(dir);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(memset(d, 0, sizeof(struct ldg_io_dir)) != d))
    {
        closedir(dir);
        ldg_mem_dealloc(d);
        return LDG_ERR_MEM_BAD;
    }

    d->handle = (void *)dir;

    ret = ldg_strrbrcpy(d->path, path, LDG_IO_DIR_PATH_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        closedir(dir);
        ldg_mem_dealloc(d);
        return ret;
    }

    d->path_len = path_len;

    if (d->path_len > 0 && d->path[d->path_len - 1] != '/' && d->path[d->path_len - 1] != '\\')
    {
        d->path[d->path_len] = '/';
        d->path_len++;
        d->path[d->path_len] = LDG_STR_TERM;
    }

    *out = d;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_rd(ldg_io_dir_t *d, ldg_io_dirent_t **out)
{
    DIR *dir = 0x0;
    struct dirent *entry = 0x0;
    ldg_io_dirent_t *e = 0x0;
    ldg_mem_pool_t *scratch = 0x0;
    char *full_path = 0x0;
    uint64_t full_path_size = LDG_IO_DIR_PATH_MAX + 256;
    uint64_t name_len = 0;
    uint32_t ret = 0;
    DWORD attrs = 0;

    if (LDG_UNLIKELY(!d || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    dir = (DIR *)d->handle;
    errno = 0;
    entry = readdir(dir);

    if (!entry)
    {
        if (errno != 0) { return ldg_io_dir_errno_translate(); }

        return LDG_ERR_EOF;
    }

    ret = ldg_mem_pool_create(0, full_path_size, &scratch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(scratch, full_path_size, (void **)&full_path);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return ret; }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_dirent), (void **)&e);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(e, 0, sizeof(struct ldg_io_dirent)) != e))
    {
        ldg_mem_dealloc(e);
        ldg_mem_pool_destroy(&scratch);
        return LDG_ERR_MEM_BAD;
    }

    ret = ldg_strrbrcpy(e->name, entry->d_name, 256);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        ldg_mem_dealloc(e);
        ldg_mem_pool_destroy(&scratch);
        return ret;
    }

    name_len = (uint64_t)strnlen(entry->d_name, 256);
    if (d->path_len + name_len < full_path_size)
    {
        if (LDG_UNLIKELY(memcpy(full_path, d->path, (uint64_t)d->path_len) != full_path))
        {
            ldg_mem_dealloc(e);
            ldg_mem_pool_destroy(&scratch);
            return LDG_ERR_MEM_BAD;
        }

        if (LDG_UNLIKELY(memcpy(full_path + d->path_len, entry->d_name, (uint64_t)name_len) != full_path + d->path_len))
        {
            ldg_mem_dealloc(e);
            ldg_mem_pool_destroy(&scratch);
            return LDG_ERR_MEM_BAD;
        }

        full_path[d->path_len + name_len] = LDG_STR_TERM;

        attrs = GetFileAttributesA(full_path);
        if (attrs != INVALID_FILE_ATTRIBUTES) { e->is_dir = (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0; }
    }

    *out = e;
    ldg_mem_pool_destroy(&scratch);

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dir_close(ldg_io_dir_t *d)
{
    DIR *dir = 0x0;

    if (LDG_UNLIKELY(!d)) { return LDG_ERR_FUNC_ARG_NULL; }

    dir = (DIR *)d->handle;

    if (LDG_UNLIKELY(closedir(dir) < 0))
    {
        ldg_mem_dealloc(d);
        return LDG_ERR_IO_CLOSE;
    }

    ldg_mem_dealloc(d);

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dirent_name_get(ldg_io_dirent_t *e, const char **name)
{
    if (LDG_UNLIKELY(!e || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    *name = e->name;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_dirent_dir_is(ldg_io_dirent_t *e, uint8_t *is_dir)
{
    if (LDG_UNLIKELY(!e || !is_dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    *is_dir = e->is_dir;

    return LDG_ERR_AOK;
}
