#ifndef LDG_LOGGING_H
#define LDG_LOGGING_H

#include <stdint.h>
#include <stdarg.h>
#include <dangling/core/macros.h>

#define LDG_LOGGING_LVL_ERR 0
#define LDG_LOGGING_LVL_WARN 1
#define LDG_LOGGING_LVL_INFO 2
#define LDG_LOGGING_LVL_DEBUG 3
#define LDG_LOGGING_LVL_CUNT 4

#define LDG_LOGGING_LINE_CAP 512
#define LDG_LOGGING_CON ((const char *)(uintptr_t)0x1U)

LDG_EXPORT uint32_t ldg_logging_line_wr(const char *path, uint32_t lvl, const char *fmt, ...);
LDG_EXPORT uint32_t ldg_logging_variable_line_wr(const char *path, uint32_t lvl, const char *fmt, va_list args);

#endif
