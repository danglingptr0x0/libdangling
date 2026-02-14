#ifndef LDG_FMT_FMT_H
#define LDG_FMT_FMT_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_fmt_cfg_get(const char **out);
LDG_EXPORT uint32_t ldg_fmt_cfg_path_get(const char **out);

#endif
