#include <stdint.h>

#include <dangling/cpp/cpp.h>
#include <dangling/core/err.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>

#define LDG_CPP_SYSV_FP_REG_MAX 8
#define LDG_CPP_ITANIUM_DTOR_SLOT_CUNT 2
#define LDG_CPP_ITANIUM_METHOD_SLOT_CUNT 1

#define LDG_CPP_SYSV_VAL_TYPE_LIST uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t
#define LDG_CPP_SYSV_FP_TYPE_LIST double, double, double, double, double, double, double, double
#define LDG_CPP_SYSV_VAL_LIST(word) word[0], word[1], word[2], word[3], word[4], word[5], word[6], word[7], word[8], word[9], word[10], word[11], word[12], word[13], word[14], word[15]
#define LDG_CPP_SYSV_FP_LIST(fp) fp[0], fp[1], fp[2], fp[3], fp[4], fp[5], fp[6], fp[7]

typedef void (*cpp_func_t)(void);
typedef uint64_t (*cpp_sysv_uint64_t)(void *obj, LDG_CPP_SYSV_VAL_TYPE_LIST, LDG_CPP_SYSV_FP_TYPE_LIST);
typedef double (*cpp_sysv_fp64_t)(void *obj, LDG_CPP_SYSV_VAL_TYPE_LIST, LDG_CPP_SYSV_FP_TYPE_LIST);

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

static uint32_t cpp_invoke(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint8_t ret_fp, uint64_t *out)
{
    uint64_t val[LDG_CPP_ARG_MAX] = LDG_ARR_ZERO_INIT;
    double fp64[LDG_CPP_SYSV_FP_REG_MAX] = LDG_ARR_ZERO_INIT;
    cpp_bits_t bits = LDG_STRUCT_ZERO_INIT;
    cpp_func_t entry = 0x0;
    cpp_sysv_fp64_t fp_fn = 0x0;
    cpp_sysv_uint64_t uint_fn = 0x0;
    uint64_t word = 0;
    uint32_t val_cunt = 0;
    uint32_t fp_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t is_fp = LDG_TRUTH_FALSE;

    *out = 0;

    if (LDG_UNLIKELY(!obj)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!arg && arg_cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arg_cunt > LDG_CPP_ARG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < arg_cunt; idx++)
    {
        ret = cpp_arg_bits_get(&arg[idx], &word, &is_fp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (is_fp == LDG_TRUTH_TRUE)
        {
            if (LDG_UNLIKELY(fp_cunt >= LDG_CPP_SYSV_FP_REG_MAX)) { return LDG_ERR_UNSUPPORTED; }

            bits.val = word;
            fp64[fp_cunt] = bits.fp64;
            fp_cunt++;
        }
        else
        {
            val[val_cunt] = word;
            val_cunt++;
        }
    }

    ret = cpp_entry_get(obj, slot, &entry);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ret_fp == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(&fp_fn, &entry, sizeof(fp_fn));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        bits.fp64 = fp_fn(obj, LDG_CPP_SYSV_VAL_LIST(val), LDG_CPP_SYSV_FP_LIST(fp64));
        *out = bits.val;
        return LDG_ERR_AOK;
    }

    ret = ldg_mem_cpy(&uint_fn, &entry, sizeof(uint_fn));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = uint_fn(obj, LDG_CPP_SYSV_VAL_LIST(val), LDG_CPP_SYSV_FP_LIST(fp64));

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_slot_get(const ldg_cpp_class_desc_t *desc, uint32_t method_idx, uint32_t *out)
{
    uint32_t slot = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(!desc->name_id && desc->method_cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(method_idx >= desc->method_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(desc->dtor_idx != LDG_CPP_DTOR_NONE && desc->dtor_idx >= desc->method_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < method_idx; idx++) { if (idx == desc->dtor_idx) { slot += LDG_CPP_ITANIUM_DTOR_SLOT_CUNT; }
        else { slot += LDG_CPP_ITANIUM_METHOD_SLOT_CUNT; } }

    *out = slot;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_void_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt)
{
    uint64_t raw = 0;

    return cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_FALSE, &raw);
}

uint32_t ldg_cpp_uint8_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint8_t *out)
{
    uint64_t raw = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_FALSE, &raw);
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

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_FALSE, &raw);
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

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_FALSE, &raw);
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

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_FALSE, &raw);
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

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_TRUE, &bits.val);
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

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_TRUE, &bits.val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = bits.fp64;

    return LDG_ERR_AOK;
}

uint32_t ldg_cpp_struct_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, void *out, uint32_t out_size)
{
    uint8_t *dst = 0x0;
    uint64_t raw = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(out_size) || out_size > LDG_CPP_STRUCT_SIZE_MAX)) { return LDG_ERR_UNSUPPORTED; }

    dst = (uint8_t *)out;

    for (idx = 0; idx < out_size; idx++) { dst[idx] = 0; }

    ret = cpp_invoke(obj, slot, arg, arg_cunt, LDG_TRUTH_FALSE, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < out_size; idx++) { dst[idx] = (uint8_t)(raw >> (idx * LDG_BYTE_BITS)); }

    return LDG_ERR_AOK;
}
