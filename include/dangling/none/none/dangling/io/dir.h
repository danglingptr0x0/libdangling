#ifndef LDG_IO_DIR_H
#define LDG_IO_DIR_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/io/path.h>

#define LDG_IO_ENTRY_NAME_MAX 256
#define LDG_IO_DIR_FULL_PATH_MAX (LDG_IO_PATH_MAX + LDG_IO_ENTRY_NAME_MAX)

typedef struct ldg_io_dir
{
    void *handle;
    char *full_path;
    char path[LDG_IO_PATH_MAX];
    uint64_t path_len;
    uint8_t pudding[32];
} ldg_io_dir_t;

typedef struct ldg_io_entry
{
    char name[LDG_IO_ENTRY_NAME_MAX];
    uint8_t is_dir;
    uint8_t pudding[7];
} ldg_io_entry_t;

LDG_EXPORT uint32_t ldg_io_dir_create(const char *path, uint32_t access);
LDG_EXPORT uint32_t ldg_io_dir_destroy(const char *path);
LDG_EXPORT uint32_t ldg_io_dir_open(const char *path, ldg_io_dir_t **out);
LDG_EXPORT uint32_t ldg_io_dir_rd(ldg_io_dir_t *dir, ldg_io_entry_t *out);
LDG_EXPORT uint32_t ldg_io_dir_close(ldg_io_dir_t *dir);

LDG_EXPORT uint32_t ldg_io_entry_name_get(ldg_io_entry_t *entry, const char **name);
LDG_EXPORT uint8_t ldg_io_entry_dir_is(ldg_io_entry_t *entry);

#endif
