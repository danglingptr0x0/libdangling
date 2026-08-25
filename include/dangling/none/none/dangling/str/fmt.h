#ifndef LDG_STR_FMT_H
#define LDG_STR_FMT_H

#include <stdint.h>
#include <stdarg.h>
#include <dangling/core/macros.h>

#define LDG_STR_FMT_ESCAPE '%'
#define LDG_STR_FMT_PRECISION_DEFAULT 6
#define LDG_STR_FMT_DIGIT_CAP 64

LDG_EXPORT uint32_t ldg_str_fmt(char *dst, uint64_t cap, uint64_t *out_len, const char *fmt, ...);
LDG_EXPORT uint32_t ldg_str_variable_fmt(char *dst, uint64_t cap, uint64_t *out_len, const char *fmt, va_list args);

#endif
