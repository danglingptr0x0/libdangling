#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

#include <dangling/io/file.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define LDG_IO_LOCK_ALL 0xFFFFFFFF

// errno
static uint32_t ldg_io_errno_translate(void)
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
        case EBADF:
            return LDG_ERR_INVALID;
        case EINTR:
            return LDG_ERR_INTERRUPTED;
        default:
            return LDG_ERR_IO_RD;
    }
}

// flags
static uint32_t ldg_io_flags_translate(uint32_t ldg_flags, int32_t *posix_flags)
{
    uint32_t access = 0;
    int32_t pf = 0;

    if (LDG_UNLIKELY(!posix_flags)) { return LDG_ERR_FUNC_ARG_NULL; }

    access = ldg_flags & 0x03;
    if (access == LDG_IO_RD_ONLY) { pf = O_RDONLY; }
    else if (access == LDG_IO_WR_ONLY) { pf = O_WRONLY; }
    else if (access == LDG_IO_RD_WR) { pf = O_RDWR; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_flags & LDG_IO_CREATE) { pf |= O_CREAT; }

    if (ldg_flags & LDG_IO_TRUNCATE) { pf |= O_TRUNC; }

    if (ldg_flags & LDG_IO_APPEND) { pf |= O_APPEND; }

    if (ldg_flags & LDG_IO_EXCLUSIVE) { pf |= O_EXCL; }

    pf |= _O_BINARY;

    *posix_flags = pf;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_open(const char *path, uint32_t flags, uint32_t access, ldg_io_file_t **out)
{
    ldg_io_file_t *file = 0x0;
    int32_t posix_flags = 0;
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_io_flags_translate(flags, &posix_flags);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_file), (void **)&file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(file, 0, sizeof(struct ldg_io_file)) != file))
    {
        ldg_mem_dealloc(file);
        return LDG_ERR_MEM_BAD;
    }

    fd = (int32_t)_open(path, (int)posix_flags, (int)access);
    if (LDG_UNLIKELY(fd < 0))
    {
        ret = ldg_io_errno_translate();
        ldg_mem_dealloc(file);
        return ret;
    }

    file->handle = fd;
    *out = file;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_close(ldg_io_file_t *file)
{
    int32_t ret = 0;

    if (LDG_UNLIKELY(!file)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = (int32_t)_close((int)file->handle);
    ldg_mem_dealloc(file);

    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_IO_CLOSE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_rd(ldg_io_file_t *file, void *buff, uint64_t size, uint64_t *bytes_rd)
{
    int32_t res = 0;

    if (LDG_UNLIKELY(!file || !buff || !bytes_rd)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_rd = 0;

    res = (int32_t)_read((int)file->handle, buff, (uint32_t)size);
    if (LDG_UNLIKELY(res < 0)) { return ldg_io_errno_translate(); }

    *bytes_rd = (uint64_t)res;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_wr(ldg_io_file_t *file, const void *buff, uint64_t size, uint64_t *bytes_wr)
{
    int32_t res = 0;

    if (LDG_UNLIKELY(!file || !buff || !bytes_wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_wr = 0;

    res = (int32_t)_write((int)file->handle, buff, (uint32_t)size);
    if (LDG_UNLIKELY(res < 0)) { return ldg_io_errno_translate(); }

    *bytes_wr = (uint64_t)res;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_seek(ldg_io_file_t *file, uint64_t offset, uint32_t whence)
{
    int32_t posix_whence = 0;
    int64_t res = 0;

    if (LDG_UNLIKELY(!file)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(whence > LDG_IO_SEEK_END)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (whence == LDG_IO_SEEK_START) { posix_whence = SEEK_SET; }
    else if (whence == LDG_IO_SEEK_CURRENT) { posix_whence = SEEK_CUR; }
    else { posix_whence = SEEK_END; }

    res = (int64_t)_lseeki64((int)file->handle, (int64_t)offset, (int)posix_whence);
    if (LDG_UNLIKELY(res < 0)) { return LDG_ERR_IO_SEEK; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_pos_get(ldg_io_file_t *file, uint64_t *pos)
{
    int64_t res = 0;

    if (LDG_UNLIKELY(!file || !pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pos = 0;

    res = (int64_t)_lseeki64((int)file->handle, 0, SEEK_CUR);
    if (LDG_UNLIKELY(res < 0)) { return LDG_ERR_IO_SEEK; }

    *pos = (uint64_t)res;

    return LDG_ERR_AOK;
}

static void ldg_io_info_populate(ldg_io_info_t *out, const struct _stat64 *sb, const char *path)
{
    DWORD attrs = 0;

    out->size = (uint64_t)sb->st_size;
    out->modified_ns = (uint64_t)sb->st_mtime * LDG_NS_PER_SEC;
    out->access = (uint32_t)sb->st_mode;
    out->is_dir = (sb->st_mode & _S_IFDIR) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->is_file = (sb->st_mode & _S_IFREG) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->is_link = LDG_TRUTH_FALSE;

    if (path)
    {
        attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES) { out->is_link = (attrs & FILE_ATTRIBUTE_REPARSE_POINT) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
    }
}

uint32_t ldg_io_path_info_get(const char *path, ldg_io_info_t *out)
{
    struct _stat64 sb = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!path || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(out, (uint64_t)sizeof(ldg_io_info_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(_stat64(path, &sb) < 0)) { return ldg_io_errno_translate(); }

    ldg_io_info_populate(out, &sb, path);

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_info_get(ldg_io_file_t *handle, ldg_io_info_t *out)
{
    struct _stat64 sb = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!handle || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(out, (uint64_t)sizeof(ldg_io_info_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(_fstat64((int32_t)handle->handle, &sb) < 0)) { return ldg_io_errno_translate(); }

    ldg_io_info_populate(out, &sb, 0x0);

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_commit(ldg_io_file_t *file)
{
    if (LDG_UNLIKELY(!file)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_commit((int)file->handle) < 0)) { return LDG_ERR_IO_FLUSH; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_truncate(ldg_io_file_t *file, uint64_t size)
{
    if (LDG_UNLIKELY(!file)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_chsize_s((int)file->handle, (int64_t)size) != 0)) { return ldg_io_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_lock(ldg_io_file_t *file)
{
    HANDLE h = 0x0;
    OVERLAPPED ov = { 0 };

    if (LDG_UNLIKELY(!file)) { return LDG_ERR_FUNC_ARG_NULL; }

    h = (HANDLE)_get_osfhandle((int)file->handle);
    if (LDG_UNLIKELY(h == INVALID_HANDLE_VALUE)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(!LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, LDG_IO_LOCK_ALL, LDG_IO_LOCK_ALL, &ov))) { return LDG_ERR_IO_PERM; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_unlock(ldg_io_file_t *file)
{
    HANDLE h = 0x0;
    OVERLAPPED ov = { 0 };

    if (LDG_UNLIKELY(!file)) { return LDG_ERR_FUNC_ARG_NULL; }

    h = (HANDLE)_get_osfhandle((int)file->handle);
    if (LDG_UNLIKELY(h == INVALID_HANDLE_VALUE)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(!UnlockFileEx(h, 0, LDG_IO_LOCK_ALL, LDG_IO_LOCK_ALL, &ov))) { return ldg_io_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pipe_create(ldg_io_file_t **rd, ldg_io_file_t **wr)
{
    int32_t pipefd[2] = { 0 };
    ldg_io_file_t *rd_file = 0x0;
    ldg_io_file_t *wr_file = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rd || !wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *rd = 0x0;
    *wr = 0x0;

    if (LDG_UNLIKELY(_pipe((int *)pipefd, LDG_IO_PIPE_BUFF_SIZE, _O_BINARY) < 0)) { return ldg_io_errno_translate(); }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_file), (void **)&rd_file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        _close((int)pipefd[0]);
        _close((int)pipefd[1]);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(memset(rd_file, 0, sizeof(struct ldg_io_file)) != rd_file))
    {
        ldg_mem_dealloc(rd_file);
        _close((int)pipefd[0]);
        _close((int)pipefd[1]);
        return LDG_ERR_MEM_BAD;
    }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_file), (void **)&wr_file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(rd_file);
        _close((int)pipefd[0]);
        _close((int)pipefd[1]);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(memset(wr_file, 0, sizeof(struct ldg_io_file)) != wr_file))
    {
        ldg_mem_dealloc(rd_file);
        ldg_mem_dealloc(wr_file);
        _close((int)pipefd[0]);
        _close((int)pipefd[1]);
        return LDG_ERR_MEM_BAD;
    }

    rd_file->handle = pipefd[0];
    wr_file->handle = pipefd[1];
    *rd = rd_file;
    *wr = wr_file;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_dup(ldg_io_file_t *src, ldg_io_file_t **out)
{
    ldg_io_file_t *file = 0x0;
    int32_t new_fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    new_fd = (int32_t)_dup((int)src->handle);
    if (LDG_UNLIKELY(new_fd < 0)) { return ldg_io_errno_translate(); }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_file), (void **)&file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        _close((int)new_fd);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(memset(file, 0, sizeof(struct ldg_io_file)) != file))
    {
        _close((int)new_fd);
        ldg_mem_dealloc(file);
        return LDG_ERR_MEM_BAD;
    }

    file->handle = new_fd;
    *out = file;

    return LDG_ERR_AOK;
}
