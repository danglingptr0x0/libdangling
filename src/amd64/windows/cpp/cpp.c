#include <stdint.h>

#include <dangling/cpp/cpp.h>
#include <dangling/core/err.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>

#define LDG_CPP_WIN_HIDDEN_CUNT 1
#define LDG_CPP_WIN_POS_MAX (LDG_CPP_ARG_MAX + LDG_CPP_WIN_HIDDEN_CUNT)
#define LDG_CPP_WIN_REG_POS_CUNT 3
#define LDG_CPP_WIN_FP_NONE 0x0
#define LDG_CPP_WIN_FP_FIRST 0x1
#define LDG_CPP_WIN_FP_SECOND 0x2
#define LDG_CPP_WIN_FP_THIRD 0x4
#define LDG_CPP_MSVC_SLOT_CUNT 1

#define LDG_CPP_WIN_STACK_TYPE_LIST uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t
#define LDG_CPP_WIN_STACK_LIST(word) word[3], word[4], word[5], word[6], word[7], word[8], word[9], word[10], word[11], word[12], word[13], word[14], word[15], word[16]
#define LDG_CPP_WIN_FUNC_TYPEDEF(ret_type, type_cb, first, second, third) typedef ret_type (*type_cb)(void *obj, first, second, third, LDG_CPP_WIN_STACK_TYPE_LIST)

typedef void (*cpp_func_t)(void);

LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_none_t, uint64_t, uint64_t, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_first_t, double, uint64_t, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_second_t, uint64_t, double, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_first_second_t, double, double, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_third_t, uint64_t, uint64_t, double);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_first_third_t, double, uint64_t, double);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_second_third_t, uint64_t, double, double);
LDG_CPP_WIN_FUNC_TYPEDEF(uint64_t, cpp_win_uint64_fp_all_t, double, double, double);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_none_t, uint64_t, uint64_t, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_first_t, double, uint64_t, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_second_t, uint64_t, double, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_first_second_t, double, double, uint64_t);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_third_t, uint64_t, uint64_t, double);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_first_third_t, double, uint64_t, double);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_second_third_t, uint64_t, double, double);
LDG_CPP_WIN_FUNC_TYPEDEF(double, cpp_win_fp64_fp_all_t, double, double, double);

typedef union cpp_bits
{
    uint64_t val;
    double fp64;
    float fp32;
} cpp_bits_t;

static uint32_t cpp_arg_bits_get(const ldg_cpp_arg_t *arg, uint64_t *out, uint8_t *out_fp)
{
    cpp_bits_t bits = LDG_STRUCT_ZERO_INIT;
    const uint8_t *src = 0x0;
    uint32_t idx = 0;

    *out = 0;
    *out_fp = LDG_TRUTH_FALSE;

    if (arg->kind == LDG_CPP_ARG_KIND_INT)
    {
        *out = arg->val;
        return LDG_ERR_AOK;
    }

    if (arg->kind == LDG_CPP_ARG_KIND_PTR)
    {
        *out = (uint64_t)(uintptr_t)arg->ptr;
        return LDG_ERR_AOK;
    }

    if (arg->kind == LDG_CPP_ARG_KIND_FP32)
    {
        bits.fp32 = arg->fp32;
        *out = bits.val;
        *out_fp = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    if (arg->kind == LDG_CPP_ARG_KIND_FP64)
    {
        bits.fp64 = arg->fp64;
        *out = bits.val;
        *out_fp = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(arg->kind != LDG_CPP_ARG_KIND_STRUCT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!arg->ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(arg->size) || arg->size > LDG_CPP_STRUCT_SIZE_MAX)) { return LDG_ERR_UNSUPPORTED; }

    src = (const uint8_t *)arg->ptr;

    for (idx = 0; idx < arg->size; idx++) { *out |= (uint64_t)src[idx] << (idx * LDG_BYTE_BITS); }

    return LDG_ERR_AOK;
}

static uint32_t cpp_entry_get(void *obj, uint32_t slot, cpp_func_t *out)
{
    const cpp_func_t *table = 0x0;

    *out = 0x0;

    table = *(const cpp_func_t *const *)obj;
    if (LDG_UNLIKELY(!table)) { return LDG_ERR_INVALID; }

    *out = table[slot];
    if (LDG_UNLIKELY(!*out)) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint64_t cpp_win_uint64_dispatch(cpp_func_t entry, void *obj, uint32_t mask, const uint64_t *val, const double *fp64)
{
    switch (mask)
    {
        case LDG_CPP_WIN_FP_FIRST:
            return ((cpp_win_uint64_fp_first_t)entry)(obj, fp64[0], val[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_SECOND:
            return ((cpp_win_uint64_fp_second_t)entry)(obj, val[0], fp64[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_FIRST | LDG_CPP_WIN_FP_SECOND:
            return ((cpp_win_uint64_fp_first_second_t)entry)(obj, fp64[0], fp64[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_uint64_fp_third_t)entry)(obj, val[0], val[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_FIRST | LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_uint64_fp_first_third_t)entry)(obj, fp64[0], val[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_SECOND | LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_uint64_fp_second_third_t)entry)(obj, val[0], fp64[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_FIRST | LDG_CPP_WIN_FP_SECOND | LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_uint64_fp_all_t)entry)(obj, fp64[0], fp64[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        default:
            return ((cpp_win_uint64_fp_none_t)entry)(obj, val[0], val[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
    }
}

static double cpp_win_fp64_dispatch(cpp_func_t entry, void *obj, uint32_t mask, const uint64_t *val, const double *fp64)
{
    switch (mask)
    {
        case LDG_CPP_WIN_FP_FIRST:
            return ((cpp_win_fp64_fp_first_t)entry)(obj, fp64[0], val[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_SECOND:
            return ((cpp_win_fp64_fp_second_t)entry)(obj, val[0], fp64[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_FIRST | LDG_CPP_WIN_FP_SECOND:
            return ((cpp_win_fp64_fp_first_second_t)entry)(obj, fp64[0], fp64[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_fp64_fp_third_t)entry)(obj, val[0], val[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_FIRST | LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_fp64_fp_first_third_t)entry)(obj, fp64[0], val[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_SECOND | LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_fp64_fp_second_third_t)entry)(obj, val[0], fp64[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        case LDG_CPP_WIN_FP_FIRST | LDG_CPP_WIN_FP_SECOND | LDG_CPP_WIN_FP_THIRD:
            return ((cpp_win_fp64_fp_all_t)entry)(obj, fp64[0], fp64[1], fp64[2], LDG_CPP_WIN_STACK_LIST(val));
        default:
            return ((cpp_win_fp64_fp_none_t)entry)(obj, val[0], val[1], val[2], LDG_CPP_WIN_STACK_LIST(val));
    }
}

static uint32_t cpp_invoke(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, void *hidden, uint8_t ret_fp, uint64_t *out)
{
    uint64_t val[LDG_CPP_WIN_POS_MAX] = LDG_ARR_ZERO_INIT;
    double fp64[LDG_CPP_WIN_REG_POS_CUNT] = LDG_ARR_ZERO_INIT;
    cpp_bits_t bits = LDG_STRUCT_ZERO_INIT;
    cpp_func_t entry = 0x0;
    uint64_t word = 0;
    uint32_t mask = LDG_CPP_WIN_FP_NONE;
    uint32_t pos = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t is_fp = LDG_TRUTH_FALSE;

    *out = 0;

    if (LDG_UNLIKELY(!obj)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!arg && arg_cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arg_cunt > LDG_CPP_ARG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (hidden)
    {
        val[pos] = (uint64_t)(uintptr_t)hidden;
        pos++;
    }

    for (idx = 0; idx < arg_cunt; idx++)
    {
        ret = cpp_arg_bits_get(&arg[idx], &word, &is_fp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        val[pos] = word;

        if (is_fp == LDG_TRUTH_TRUE && pos < LDG_CPP_WIN_REG_POS_CUNT)
        {
            bits.val = word;
            fp64[pos] = bits.fp64;
            mask |= (uint32_t)LDG_CPP_WIN_FP_FIRST << pos;
        }

        pos++;
    }

    ret = cpp_entry_get(obj, slot, &entry);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ret_fp == LDG_TRUTH_TRUE)
    {
        bits.fp64 = cpp_win_fp64_dispatch(entry, obj, mask, val, fp64);
        *out = bits.val;
        return LDG_ERR_AOK;
    }

    *out = cpp_win_uint64_dispatch(entry, obj, mask, val, fp64);

    return LDG_ERR_AOK;
}

static uint8_t cpp_group_same_is(const ldg_cpp_class_desc_t *desc, uint32_t lhs, uint32_t rhs)
{
    if (lhs == desc->dtor_idx || rhs == desc->dtor_idx)
    {
        if (lhs == rhs) { return LDG_TRUTH_TRUE; }

        return LDG_TRUTH_FALSE;
    }

    if (desc->name_id[lhs] == desc->name_id[rhs]) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t cpp_group_lead_is(const ldg_cpp_class_desc_t *desc, uint32_t method_idx)
{
    uint32_t idx = 0;

    for (idx = 0; idx < method_idx; idx++) { if (cpp_group_same_is(desc, idx, method_idx) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_cpp_slot_get(const ldg_cpp_class_desc_t *desc, uint32_t method_idx, uint32_t *out)
{
    uint32_t slot = 0;
    uint32_t lead = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(!desc->name_id && desc->method_cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(method_idx >= desc->method_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(desc->dtor_idx != LDG_CPP_DTOR_NONE && desc->dtor_idx >= desc->method_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (lead = 0; lead < desc->method_cunt; lead++)
    {
        if (cpp_group_lead_is(desc, lead) != LDG_TRUTH_TRUE) { continue; }

        if (cpp_group_same_is(desc, lead, method_idx) == LDG_TRUTH_TRUE)
        {
            for (idx = method_idx + LDG_CPP_MSVC_SLOT_CUNT; idx < desc->method_cunt; idx++) { if (cpp_group_same_is(desc, idx, method_idx) == LDG_TRUTH_TRUE) { slot += LDG_CPP_MSVC_SLOT_CUNT; } }

            *out = slot;

            return LDG_ERR_AOK;
        }

        for (idx = lead; idx < desc->method_cunt; idx++) { if (cpp_group_same_is(desc, idx, lead) == LDG_TRUTH_TRUE) { slot += LDG_CPP_MSVC_SLOT_CUNT; } }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_cpp_void_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt)
{
    uint64_t raw = 0;

    return cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_FALSE, &raw);
}

uint32_t ldg_cpp_uint8_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint8_t *out)
{
    uint64_t raw = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_FALSE, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint8_t)raw;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_uint16_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint16_t *out)
{
    uint64_t raw = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_FALSE, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint16_t)raw;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_uint32_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint32_t *out)
{
    uint64_t raw = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_FALSE, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint32_t)raw;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_uint64_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint64_t *out)
{
    uint64_t raw = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_FALSE, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = raw;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_fp32_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, float *out)
{
    cpp_bits_t bits = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_TRUE, &bits.val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = bits.fp32;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_fp64_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, double *out)
{
    cpp_bits_t bits = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, 0x0, LDG_TRUTH_TRUE, &bits.val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = bits.fp64;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_struct_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, void *out, uint32_t out_size)
{
    uint8_t *dst = 0x0;
    uint64_t raw = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(out_size) || out_size > LDG_CPP_STRUCT_SIZE_MAX)) { return LDG_ERR_UNSUPPORTED; }

    dst = (uint8_t *)out;

    for (idx = 0; idx < out_size; idx++) { dst[idx] = 0; }

    return cpp_invoke(obj, slot, arg, arg_cunt, out, LDG_TRUTH_FALSE, &raw);
}
