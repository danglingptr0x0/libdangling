#ifndef LDG_SYS_LIB_H
#define LDG_SYS_LIB_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef void (*ldg_sys_lib_func_t)(void);

LDG_EXPORT uint32_t ldg_sys_lib_open(const char *path, void **out);
LDG_EXPORT uint32_t ldg_sys_lib_sym_get(void *lib, const char *name, ldg_sys_lib_func_t *out);
LDG_EXPORT uint32_t ldg_sys_lib_close(void *lib);

#endif
