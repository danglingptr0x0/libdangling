#ifndef LDG_IO_FILE_H
#define LDG_IO_FILE_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef struct ldg_io_file ldg_io_file_t;

#define LDG_IO_RDONLY  0x01
#define LDG_IO_WRONLY  0x02
#define LDG_IO_RDWR    0x03
#define LDG_IO_CREATE  0x04
#define LDG_IO_TRUNC   0x08
#define LDG_IO_APPEND  0x10
#define LDG_IO_EXCL    0x20

#define LDG_IO_SEEK_SET 0
#define LDG_IO_SEEK_CUR 1
#define LDG_IO_SEEK_END 2

typedef struct ldg_io_stat
{
    uint64_t size;
    uint64_t mtime_ns;
    uint32_t mode;
    uint8_t is_dir;
    uint8_t is_file;
    uint8_t is_link;
    uint8_t pudding[41];
} LDG_ALIGNED ldg_io_stat_t;

// lifecycle
LDG_EXPORT uint32_t ldg_io_file_open(const char *path, uint32_t flags, uint32_t mode, ldg_io_file_t **out);
LDG_EXPORT uint32_t ldg_io_file_close(ldg_io_file_t *f);

// read/write
LDG_EXPORT uint32_t ldg_io_file_rd(ldg_io_file_t *f, void *buff, uint64_t size, uint64_t *bytes_rd);
LDG_EXPORT uint32_t ldg_io_file_wr(ldg_io_file_t *f, const void *buff, uint64_t size, uint64_t *bytes_wr);

// positioning
LDG_EXPORT uint32_t ldg_io_file_seek(ldg_io_file_t *f, uint64_t offset, uint32_t whence);
LDG_EXPORT uint32_t ldg_io_file_pos_get(ldg_io_file_t *f, uint64_t *pos);

// metadata
LDG_EXPORT uint32_t ldg_io_file_stat(const char *path, ldg_io_stat_t *st);
LDG_EXPORT uint32_t ldg_io_file_fstat(ldg_io_file_t *f, ldg_io_stat_t *st);

// sync/lock/truncate
LDG_EXPORT uint32_t ldg_io_file_sync(ldg_io_file_t *f);
LDG_EXPORT uint32_t ldg_io_file_truncate(ldg_io_file_t *f, uint64_t size);
LDG_EXPORT uint32_t ldg_io_file_lock(ldg_io_file_t *f);
LDG_EXPORT uint32_t ldg_io_file_unlock(ldg_io_file_t *f);

// pipe/dup
LDG_EXPORT uint32_t ldg_io_pipe_create(ldg_io_file_t **rd, ldg_io_file_t **wr);
LDG_EXPORT uint32_t ldg_io_file_dup(ldg_io_file_t *src, ldg_io_file_t **out);

#endif
