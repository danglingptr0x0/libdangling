#ifndef LDG_CORE_ARITH_H
#define LDG_CORE_ARITH_H

#if !defined(__GNUC__) && !defined(__clang__)
#error "arith.h requires GCC or Clang __builtin_*_overflow support"
#endif

#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static inline uint32_t ldg_arith_32_add(uint32_t a, uint32_t b, uint32_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(__builtin_add_overflow(a, b, ret))) { *ret = 0; return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_32_sub(uint32_t a, uint32_t b, uint32_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(__builtin_sub_overflow(a, b, ret))) { *ret = 0; return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_32_mul(uint32_t a, uint32_t b, uint32_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(__builtin_mul_overflow(a, b, ret))) { *ret = 0; return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_32_div(uint32_t a, uint32_t b, uint32_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(b == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *ret = a / b;
    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_64_add(uint64_t a, uint64_t b, uint64_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(__builtin_add_overflow(a, b, ret))) { *ret = 0; return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_64_sub(uint64_t a, uint64_t b, uint64_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(__builtin_sub_overflow(a, b, ret))) { *ret = 0; return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_64_mul(uint64_t a, uint64_t b, uint64_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(__builtin_mul_overflow(a, b, ret))) { *ret = 0; return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_arith_64_div(uint64_t a, uint64_t b, uint64_t *ret)
{
    if (LDG_UNLIKELY(!ret)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ret = 0;
    if (LDG_UNLIKELY(b == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *ret = a / b;
    return LDG_ERR_AOK;
}

#endif
