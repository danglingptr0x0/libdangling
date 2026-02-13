#ifndef LDG_CORE_MACROS_H
#define LDG_CORE_MACROS_H

#include <stdint.h>

#define LDG_LIKELY(x) __builtin_expect(!!(x), 1)
#define LDG_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define LDG_KIB 1024ULL
#define LDG_MIB (LDG_KIB * LDG_KIB)
#define LDG_GIB (LDG_KIB * LDG_MIB)

#define LDG_MS_PER_SEC 1000ULL
#define LDG_NS_PER_MS 1000000ULL
#define LDG_NS_PER_SEC 1000000000ULL
#define LDG_SECS_PER_MIN 60
#define LDG_SECS_PER_HOUR 3600

#define LDG_BASE_DEC 10
#define LDG_BASE_HEX 16

#define LDG_STR_TERM '\0'
#define LDG_STR_TERM_SIZE 1

#define LDG_IO_PIPE_BUFF_SIZE 4096

#define LDG_TRUTH_TRUE 1
#define LDG_TRUTH_FALSE 0
#define LDG_TRUTH_MAYBE 2

#define LDG_STRUCT_ZERO_INIT { 0 }
#define LDG_ARR_ZERO_INIT { 0 }

#define LDG_AMD64_CACHE_LINE_WIDTH 64
#define LDG_ALIGNED __attribute__((aligned(LDG_AMD64_CACHE_LINE_WIDTH)))
#define LDG_ALIGNED_UP(x) (((uint64_t)(x) + (LDG_AMD64_CACHE_LINE_WIDTH - 1)) & ~(uint64_t)(LDG_AMD64_CACHE_LINE_WIDTH - 1))
#define LDG_ALIGNED_DOWN(x) ((uint64_t)(x) & ~(uint64_t)(LDG_AMD64_CACHE_LINE_WIDTH - 1))

#define LDG_BOOL_ASSERT(cond) do { if (LDG_UNLIKELY(!(cond))) { *(volatile uint8_t *)0x0 = 0; } } while (0)

#ifdef _WIN32
#define LDG_EXPORT __attribute__((dllexport))
#else
#define LDG_EXPORT __attribute__((visibility("default")))
#endif

#endif
