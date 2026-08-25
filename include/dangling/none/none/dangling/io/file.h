#ifndef LDG_IO_FILE_H
#define LDG_IO_FILE_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef struct ldg_io_file
{
    int32_t handle;
    uint8_t pudding[60];
} ldg_io_file_t;

#define LDG_IO_RD_ONLY 0x01
#define LDG_IO_WR_ONLY 0x02
#define LDG_IO_RD_WR 0x03
#define LDG_IO_CREATE 0x04
#define LDG_IO_TRUNCATE 0x08
#define LDG_IO_APPEND 0x10
#define LDG_IO_EXCLUSIVE 0x20

#define LDG_IO_SEEK_START 0
#define LDG_IO_SEEK_CURRENT 1
#define LDG_IO_SEEK_END 2

typedef struct ldg_io_info
{
    uint64_t size;
    uint64_t modified_ns;
    uint32_t access;
    uint8_t is_dir;
    uint8_t is_file;
    uint8_t is_link;
    uint8_t pudding[41];
} LDG_ALIGNED ldg_io_info_t;

LDG_EXPORT uint32_t ldg_io_file_open(const char *path, uint32_t flags, uint32_t access, ldg_io_file_t **out);
LDG_EXPORT uint32_t ldg_io_file_close(ldg_io_file_t *handle);

LDG_EXPORT uint32_t ldg_io_file_rd(ldg_io_file_t *handle, void *buff, uint64_t size, uint64_t *bytes_rd);
LDG_EXPORT uint32_t ldg_io_file_wr(ldg_io_file_t *handle, const void *buff, uint64_t size, uint64_t *bytes_wr);

LDG_EXPORT uint32_t ldg_io_file_seek(ldg_io_file_t *handle, uint64_t offset, uint32_t whence);
LDG_EXPORT uint32_t ldg_io_file_pos_get(ldg_io_file_t *handle, uint64_t *pos);

LDG_EXPORT uint32_t ldg_io_path_info_get(const char *path, ldg_io_info_t *out);
LDG_EXPORT uint32_t ldg_io_file_info_get(ldg_io_file_t *handle, ldg_io_info_t *out);

LDG_EXPORT uint32_t ldg_io_file_commit(ldg_io_file_t *handle);
LDG_EXPORT uint32_t ldg_io_file_truncate(ldg_io_file_t *handle, uint64_t size);
LDG_EXPORT uint32_t ldg_io_file_lock(ldg_io_file_t *handle);
LDG_EXPORT uint32_t ldg_io_file_unlock(ldg_io_file_t *handle);

LDG_EXPORT uint32_t ldg_io_pipe_create(ldg_io_file_t **rd, ldg_io_file_t **wr);
LDG_EXPORT uint32_t ldg_io_file_dup(ldg_io_file_t *src, ldg_io_file_t **out);

#endif
