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
    if (access == LDG_IO_RDONLY) { pf = O_RDONLY; }
    else if (access == LDG_IO_WRONLY) { pf = O_WRONLY; }
    else if (access == LDG_IO_RDWR) { pf = O_RDWR; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_flags & LDG_IO_CREATE) { pf |= O_CREAT; }

    if (ldg_flags & LDG_IO_TRUNC) { pf |= O_TRUNC; }

    if (ldg_flags & LDG_IO_APPEND) { pf |= O_APPEND; }

    if (ldg_flags & LDG_IO_EXCL) { pf |= O_EXCL; }

    pf |= _O_BINARY;

    *posix_flags = pf;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_open(const char *path, uint32_t flags, uint32_t mode, ldg_io_file_t **out)
{
    ldg_io_file_t *f = 0x0;
    int32_t posix_flags = 0;
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_io_flags_translate(flags, &posix_flags);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_file), (void **)&f);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(f, 0, sizeof(struct ldg_io_file)) != f))
    {
        ldg_mem_dealloc(f);
        return LDG_ERR_MEM_BAD;
    }

    fd = (int32_t)_open(path, (int)posix_flags, (int)mode);
    if (LDG_UNLIKELY(fd < 0))
    {
        ret = ldg_io_errno_translate();
        ldg_mem_dealloc(f);
        return ret;
    }

    f->fd = fd;
    *out = f;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_close(ldg_io_file_t *f)
{
    int32_t ret = 0;

    if (LDG_UNLIKELY(!f)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = (int32_t)_close((int)f->fd);
    ldg_mem_dealloc(f);

    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_IO_CLOSE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_rd(ldg_io_file_t *f, void *buff, uint64_t size, uint64_t *bytes_rd)
{
    int32_t result = 0;

    if (LDG_UNLIKELY(!f || !buff || !bytes_rd)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_rd = 0;

    result = (int32_t)_read((int)f->fd, buff, (uint32_t)size);
    if (LDG_UNLIKELY(result < 0)) { return ldg_io_errno_translate(); }

    *bytes_rd = (uint64_t)result;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_wr(ldg_io_file_t *f, const void *buff, uint64_t size, uint64_t *bytes_wr)
{
    int32_t result = 0;

    if (LDG_UNLIKELY(!f || !buff || !bytes_wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_wr = 0;

    result = (int32_t)_write((int)f->fd, buff, (uint32_t)size);
    if (LDG_UNLIKELY(result < 0)) { return ldg_io_errno_translate(); }

    *bytes_wr = (uint64_t)result;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_seek(ldg_io_file_t *f, uint64_t offset, uint32_t whence)
{
    int32_t posix_whence = 0;
    int64_t result = 0;

    if (LDG_UNLIKELY(!f)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(whence > LDG_IO_SEEK_END)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (whence == LDG_IO_SEEK_SET) { posix_whence = SEEK_SET; }
    else if (whence == LDG_IO_SEEK_CUR) { posix_whence = SEEK_CUR; }
    else { posix_whence = SEEK_END; }

    result = (int64_t)_lseeki64((int)f->fd, (int64_t)offset, (int)posix_whence);
    if (LDG_UNLIKELY(result < 0)) { return LDG_ERR_IO_SEEK; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_pos_get(ldg_io_file_t *f, uint64_t *pos)
{
    int64_t result = 0;

    if (LDG_UNLIKELY(!f || !pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pos = 0;

    result = (int64_t)_lseeki64((int)f->fd, 0, SEEK_CUR);
    if (LDG_UNLIKELY(result < 0)) { return LDG_ERR_IO_SEEK; }

    *pos = (uint64_t)result;

    return LDG_ERR_AOK;
}

static void ldg_io_stat_populate(ldg_io_stat_t *st, const struct _stat64 *sb, const char *path)
{
    DWORD attrs = 0;

    st->size = (uint64_t)sb->st_size;
    st->mtime_ns = (uint64_t)sb->st_mtime * LDG_NS_PER_SEC;
    st->mode = (uint32_t)sb->st_mode;
    st->is_dir = (sb->st_mode & _S_IFDIR) ? 1 : 0;
    st->is_file = (sb->st_mode & _S_IFREG) ? 1 : 0;
    st->is_link = 0;

    if (path)
    {
        attrs = GetFileAttributesA(path);
        if (attrs != INVALID_FILE_ATTRIBUTES) { st->is_link = (attrs & FILE_ATTRIBUTE_REPARSE_POINT) ? 1 : 0; }
    }
}

uint32_t ldg_io_file_stat(const char *path, ldg_io_stat_t *st)
{
    struct _stat64 sb = { 0 };

    if (LDG_UNLIKELY(!path || !st)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(st, 0, sizeof(ldg_io_stat_t)) != st)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(_stat64(path, &sb) < 0)) { return ldg_io_errno_translate(); }

    ldg_io_stat_populate(st, &sb, path);

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_fstat(ldg_io_file_t *f, ldg_io_stat_t *st)
{
    struct _stat64 sb = { 0 };

    if (LDG_UNLIKELY(!f || !st)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(st, 0, sizeof(ldg_io_stat_t)) != st)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(_fstat64((int)f->fd, &sb) < 0)) { return ldg_io_errno_translate(); }

    ldg_io_stat_populate(st, &sb, 0x0);

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_sync(ldg_io_file_t *f)
{
    if (LDG_UNLIKELY(!f)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_commit((int)f->fd) < 0)) { return LDG_ERR_IO_FLUSH; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_truncate(ldg_io_file_t *f, uint64_t size)
{
    if (LDG_UNLIKELY(!f)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(_chsize_s((int)f->fd, (int64_t)size) != 0)) { return ldg_io_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_lock(ldg_io_file_t *f)
{
    HANDLE h = 0x0;
    OVERLAPPED ov = { 0 };

    if (LDG_UNLIKELY(!f)) { return LDG_ERR_FUNC_ARG_NULL; }

    h = (HANDLE)_get_osfhandle((int)f->fd);
    if (LDG_UNLIKELY(h == INVALID_HANDLE_VALUE)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(!LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, LDG_IO_LOCK_ALL, LDG_IO_LOCK_ALL, &ov))) { return LDG_ERR_IO_PERM; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_unlock(ldg_io_file_t *f)
{
    HANDLE h = 0x0;
    OVERLAPPED ov = { 0 };

    if (LDG_UNLIKELY(!f)) { return LDG_ERR_FUNC_ARG_NULL; }

    h = (HANDLE)_get_osfhandle((int)f->fd);
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

    rd_file->fd = pipefd[0];
    wr_file->fd = pipefd[1];
    *rd = rd_file;
    *wr = wr_file;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_file_dup(ldg_io_file_t *src, ldg_io_file_t **out)
{
    ldg_io_file_t *f = 0x0;
    int32_t new_fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    new_fd = (int32_t)_dup((int)src->fd);
    if (LDG_UNLIKELY(new_fd < 0)) { return ldg_io_errno_translate(); }

    ret = ldg_mem_alloc((uint64_t)sizeof(struct ldg_io_file), (void **)&f);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        _close((int)new_fd);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(memset(f, 0, sizeof(struct ldg_io_file)) != f))
    {
        _close((int)new_fd);
        ldg_mem_dealloc(f);
        return LDG_ERR_MEM_BAD;
    }

    f->fd = new_fd;
    *out = f;

    return LDG_ERR_AOK;
}
