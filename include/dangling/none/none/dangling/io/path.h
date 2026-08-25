#ifndef LDG_IO_PATH_H
#define LDG_IO_PATH_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_IO_PATH_MAX 4096

LDG_EXPORT uint32_t ldg_io_path_rename(const char *old_path, const char *new_path);
LDG_EXPORT uint32_t ldg_io_path_destroy(const char *path);
LDG_EXPORT uint32_t ldg_io_path_access_set(const char *path, uint32_t access);
LDG_EXPORT uint8_t ldg_io_path_exists_is(const char *path);

LDG_EXPORT uint32_t ldg_io_link_create(const char *tgt, const char *link_path);
LDG_EXPORT uint32_t ldg_io_link_rd(const char *link_path, char *buff, uint64_t buff_size, uint64_t *len);

LDG_EXPORT uint8_t  ldg_io_path_sep_get(void);
LDG_EXPORT uint32_t ldg_io_path_join(const char *base, const char *name, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_leaf_get(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_parent_get(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_ext_get(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint8_t  ldg_io_path_absolute_is(const char *path);
LDG_EXPORT uint32_t ldg_io_path_normalize(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_user_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_work_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_prog_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_conf_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_data_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_cache_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_state_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_resolve(const char *path, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_io_path_expand(const char *path, char *buff, uint64_t buff_size);

#endif
