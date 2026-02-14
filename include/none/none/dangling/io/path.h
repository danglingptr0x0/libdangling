#ifndef LDG_IO_PATH_H
#define LDG_IO_PATH_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_io_path_rename(const char *old_path, const char *new_path);
LDG_EXPORT uint32_t ldg_io_path_unlink(const char *path);
LDG_EXPORT uint32_t ldg_io_path_mode_set(const char *path, uint32_t mode);
LDG_EXPORT uint32_t ldg_io_path_exists_is(const char *path, uint8_t *exists);

LDG_EXPORT uint32_t ldg_io_symlink_create(const char *target, const char *link_path);
LDG_EXPORT uint32_t ldg_io_symlink_rd(const char *link_path, char *buff, uint64_t buff_size, uint64_t *len);

LDG_EXPORT uint8_t  ldg_io_path_sep_get(void);
LDG_EXPORT uint32_t ldg_io_path_join(const char *base, const char *name, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_basename_get(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_dirname_get(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_ext_get(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint8_t  ldg_io_path_absolute_is(const char *path);
LDG_EXPORT uint32_t ldg_io_path_normalize(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_home_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_tmp_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_resolve(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_expand(const char *path, char *buff, uint64_t buff_size);

#endif
