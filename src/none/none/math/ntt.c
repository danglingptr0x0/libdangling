#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>

#define LDG_MATH_NTT_RADIX 2
#define LDG_MATH_NTT_STEP_ADVANCE 1
#define LDG_MATH_NTT_UNIT_DOMAIN 1
#define LDG_MATH_NTT_PREV_OFF 1

static uint64_t ntt_idx_reverse(uint64_t idx, uint32_t log_cunt)
{
    uint64_t rev = 0;
    uint32_t bit = 0;

    for (bit = 0; bit < log_cunt; bit++)
    {
        rev <<= LDG_MATH_NTT_STEP_ADVANCE;
        rev |= (idx >> bit) & LDG_MATH_FIELD_LSB_MASK;
    }

    return rev;
}

static void ntt_butterfly_run(ldg_math_field_elem_t *data, uint64_t cunt, const ldg_math_field_elem_t *twiddle)
{
    uint64_t stage_len = 0;
    uint64_t half = 0;
    uint64_t step = 0;
    uint64_t blk_start = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t lhs = 0;
    ldg_math_field_elem_t rhs = 0;
    ldg_math_field_elem_t tw = 0;

    for (stage_len = LDG_MATH_NTT_RADIX; stage_len <= cunt; stage_len <<= LDG_MATH_NTT_STEP_ADVANCE)
    {
        half = stage_len / LDG_MATH_NTT_RADIX;
        step = cunt / stage_len;

        for (blk_start = 0; blk_start < cunt; blk_start += stage_len) { for (idx = 0; idx < half; idx++)
            {
                tw = twiddle[idx * step];
                lhs = data[blk_start + idx];
                rhs = ldg_math_field_mul(data[blk_start + idx + half], tw);
                data[blk_start + idx] = ldg_math_field_add(lhs, rhs);
                data[blk_start + idx + half] = ldg_math_field_sub(lhs, rhs);
            }
        }
    }
}

static void ntt_coset_scale(ldg_math_field_elem_t *data, uint64_t cunt, ldg_math_field_elem_t shift)
{
    uint64_t idx = 0;
    ldg_math_field_elem_t curr = LDG_MATH_FIELD_MUL_IDENT;

    for (idx = 0; idx < cunt; idx++)
    {
        data[idx] = ldg_math_field_mul(data[idx], curr);
        curr = ldg_math_field_mul(curr, shift);
    }
}

uint32_t ldg_math_ntt_domain_cunt_get(uint32_t log_cunt, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_cunt > LDG_MATH_NTT_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = (uint64_t)LDG_MATH_NTT_UNIT_DOMAIN << log_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_twiddle_cunt_get(uint32_t log_cunt, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_cunt > LDG_MATH_NTT_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (log_cunt == 0) { return LDG_ERR_AOK; }

    *out = (uint64_t)LDG_MATH_NTT_UNIT_DOMAIN << (log_cunt - LDG_MATH_NTT_PREV_OFF);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_twiddle_fill(uint32_t log_cunt, uint8_t inverse_is, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t root = 0;
    ldg_math_field_elem_t curr = LDG_MATH_FIELD_MUL_IDENT;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_ntt_twiddle_cunt_get(log_cunt, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_field_root_get(log_cunt, &root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_root_order_ck(root, log_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (inverse_is == LDG_TRUTH_TRUE)
    {
        ret = ldg_math_field_invert_get(root, &root);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_field_root_order_ck(root, log_cunt);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < need; idx++)
    {
        buff[idx] = curr;
        curr = ldg_math_field_mul(curr, root);
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_bit_reverse(ldg_math_field_elem_t *data, uint32_t log_cunt)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    uint64_t rev = 0;
    ldg_math_field_elem_t swap = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_ntt_domain_cunt_get(log_cunt, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        rev = ntt_idx_reverse(idx, log_cunt);

        if (rev > idx)
        {
            swap = data[idx];
            data[idx] = data[rev];
            data[rev] = swap;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_evaluate(ldg_math_field_elem_t *data, uint32_t log_cunt, const ldg_math_field_elem_t *twiddle)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t cunt = 0;

    if (LDG_UNLIKELY(!data || !twiddle)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_ntt_domain_cunt_get(log_cunt, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_bit_reverse(data, log_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ntt_butterfly_run(data, cunt, twiddle);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_interpolate(ldg_math_field_elem_t *data, uint32_t log_cunt, const ldg_math_field_elem_t *twiddle)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t scale = 0;

    ret = ldg_math_ntt_evaluate(data, log_cunt, twiddle);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(log_cunt, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_invert_get((ldg_math_field_elem_t)cunt, &scale);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++) { data[idx] = ldg_math_field_mul(data[idx], scale); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_coset_evaluate(ldg_math_field_elem_t *data, uint32_t log_cunt, ldg_math_field_elem_t shift, const ldg_math_field_elem_t *twiddle)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t cunt = 0;

    if (LDG_UNLIKELY(!data || !twiddle)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_ntt_domain_cunt_get(log_cunt, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ntt_coset_scale(data, cunt, ldg_math_field_reduce(shift));

    return ldg_math_ntt_evaluate(data, log_cunt, twiddle);
}

uint32_t ldg_math_ntt_coset_interpolate(ldg_math_field_elem_t *data, uint32_t log_cunt, ldg_math_field_elem_t shift, const ldg_math_field_elem_t *twiddle)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t cunt = 0;
    ldg_math_field_elem_t shift_inverse = 0;

    if (LDG_UNLIKELY(!data || !twiddle)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_field_invert_get(shift, &shift_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_interpolate(data, log_cunt, twiddle);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(log_cunt, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ntt_coset_scale(data, cunt, shift_inverse);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ntt_extend(const ldg_math_field_elem_t *src, uint32_t log_src, ldg_math_field_elem_t *dst, uint32_t log_dst, ldg_math_field_elem_t shift, const ldg_math_field_elem_t *twiddle)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t src_cunt = 0;
    uint64_t dst_cunt = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!src || !dst || !twiddle)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(log_src > log_dst)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_ntt_domain_cunt_get(log_src, &src_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(log_dst, &dst_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < src_cunt; idx++) { dst[idx] = ldg_math_field_reduce(src[idx]); }

    for (idx = src_cunt; idx < dst_cunt; idx++) { dst[idx] = LDG_MATH_FIELD_ADD_IDENT; }

    return ldg_math_ntt_coset_evaluate(dst, log_dst, shift, twiddle);
}
