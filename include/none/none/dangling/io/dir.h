#ifndef LDG_IO_DIR_H
#define LDG_IO_DIR_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_IO_DIR_PATH_MAX 4096

typedef struct ldg_io_dir
{
    void *handle;
    char path[LDG_IO_DIR_PATH_MAX];
    uint64_t path_len;
    uint8_t pudding[40];
} ldg_io_dir_t;

typedef struct ldg_io_dirent
{
    char name[256];
    uint8_t is_dir;
    uint8_t pudding[7];
} ldg_io_dirent_t;

LDG_EXPORT uint32_t ldg_io_dir_create(const char *path, uint32_t mode);
LDG_EXPORT uint32_t ldg_io_dir_destroy(const char *path);
LDG_EXPORT uint32_t ldg_io_dir_open(const char *path, ldg_io_dir_t **out);
LDG_EXPORT uint32_t ldg_io_dir_rd(ldg_io_dir_t *d, ldg_io_dirent_t **out);
LDG_EXPORT uint32_t ldg_io_dir_close(ldg_io_dir_t *d);

LDG_EXPORT uint32_t ldg_io_dirent_name_get(ldg_io_dirent_t *e, const char **name);
LDG_EXPORT uint32_t ldg_io_dirent_dir_is(ldg_io_dirent_t *e, uint8_t *is_dir);

#endif
