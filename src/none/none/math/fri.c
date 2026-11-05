#include <stdint.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>
#include <dangling/math/fri.h>

#define LDG_MATH_FRI_BUFF_LANE_CUNT 3
#define LDG_MATH_FRI_PREV_OFF 1
#define LDG_MATH_FRI_LANE_PAIR 2

static uint32_t fri_log_get(uint64_t cunt)
{
    uint32_t log_cunt = 0;
    uint64_t walk = 0;

    walk = cunt;

    while (walk > LDG_MATH_FRI_PREV_OFF)
    {
        walk /= LDG_MATH_FRI_FOLD_RADIX;
        log_cunt++;
    }

    return log_cunt;
}

static uint32_t fri_denom_invert(uint64_t cunt, ldg_math_field_elem_t shift, ldg_math_field_elem_t *buff, ldg_math_field_elem_t **out_inverted)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t half = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t root = 0;
    ldg_math_field_elem_t curr = 0;
    ldg_math_field_elem_t *denom = 0x0;
    ldg_math_field_elem_t *mid = 0x0;
    ldg_math_field_elem_t *inverted = 0x0;

    ret = ldg_math_field_root_get(fri_log_get(cunt), &root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    half = cunt / LDG_MATH_FRI_FOLD_RADIX;
    denom = buff;
    mid = buff + half;
    inverted = mid + half;

    curr = ldg_math_field_mul((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, ldg_math_field_reduce(shift));

    for (idx = 0; idx < half; idx++)
    {
        denom[idx] = curr;
        curr = ldg_math_field_mul(curr, root);
    }

    ret = ldg_math_field_batch_invert(denom, half, mid, inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_inverted = inverted;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_fold_buff_cunt_get(uint64_t cunt, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(cunt < LDG_MATH_FRI_FOLD_RADIX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(cunt))) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = (cunt / LDG_MATH_FRI_FOLD_RADIX) * LDG_MATH_FRI_BUFF_LANE_CUNT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_fold(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t shift, ldg_math_field_elem_t alpha, ldg_math_field_elem_t *out, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t half = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t radix_inverse = 0;
    ldg_math_field_elem_t sum = 0;
    ldg_math_field_elem_t diff = 0;
    ldg_math_field_elem_t *inverted = 0x0;

    if (LDG_UNLIKELY(!src || !out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_fri_fold_buff_cunt_get(cunt, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_field_invert_get((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, &radix_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fri_denom_invert(cunt, shift, buff, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    half = cunt / LDG_MATH_FRI_FOLD_RADIX;

    for (idx = 0; idx < half; idx++)
    {
        sum = ldg_math_field_add(src[idx], src[idx + half]);
        diff = ldg_math_field_sub(src[idx], src[idx + half]);
        out[idx] = ldg_math_field_add(ldg_math_field_mul(sum, radix_inverse), ldg_math_field_mul(ldg_math_field_reduce(alpha), ldg_math_field_mul(diff, inverted[idx])));
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_fold_pair_get(ldg_math_field_elem_t lhs_val, ldg_math_field_elem_t rhs_val, ldg_math_field_elem_t point, ldg_math_field_elem_t alpha, ldg_math_field_elem_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    ldg_math_field_elem_t radix_inverse = 0;
    ldg_math_field_elem_t denom_inverse = 0;
    ldg_math_field_elem_t sum = 0;
    ldg_math_field_elem_t diff = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(ldg_math_field_reduce(point) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_field_invert_get((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, &radix_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_invert_get(ldg_math_field_mul((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, ldg_math_field_reduce(point)), &denom_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sum = ldg_math_field_add(lhs_val, rhs_val);
    diff = ldg_math_field_sub(lhs_val, rhs_val);
    *out = ldg_math_field_add(ldg_math_field_mul(sum, radix_inverse), ldg_math_field_mul(ldg_math_field_reduce(alpha), ldg_math_field_mul(diff, denom_inverse)));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_shift_next_get(ldg_math_field_elem_t shift, ldg_math_field_elem_t *out)
{
    ldg_math_field_elem_t reduced = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    reduced = ldg_math_field_reduce(shift);

    if (LDG_UNLIKELY(reduced == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = ldg_math_field_mul(reduced, reduced);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_layer_cunt_get(uint32_t log_domain, uint32_t log_remainder, uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_domain > LDG_MATH_NTT_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(log_remainder > log_domain)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = log_domain - log_remainder;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_remainder_ck(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t shift, uint64_t degree_bound, const ldg_math_field_elem_t *twiddle_inverse, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!src || !twiddle_inverse || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(cunt))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(degree_bound > cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++) { buff[idx] = src[idx]; }

    ret = ldg_math_ntt_coset_interpolate(buff, fri_log_get(cunt), shift, twiddle_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = degree_bound; idx < cunt; idx++) { if (buff[idx] != LDG_MATH_FIELD_ADD_IDENT) { return LDG_ERR_INVALID; } }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_ext_fold(const ldg_math_ext_t *src, uint64_t cunt, ldg_math_field_elem_t shift, ldg_math_ext_t alpha, ldg_math_ext_t *out, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t half = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t radix_inverse = 0;
    ldg_math_ext_t sum = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t diff = LDG_STRUCT_ZERO_INIT;
    ldg_math_field_elem_t *inverted = 0x0;

    if (LDG_UNLIKELY(!src || !out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_fri_fold_buff_cunt_get(cunt, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_field_invert_get((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, &radix_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fri_denom_invert(cunt, shift, buff, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    half = cunt / LDG_MATH_FRI_FOLD_RADIX;

    for (idx = 0; idx < half; idx++)
    {
        sum = ldg_math_ext_add(src[idx], src[idx + half]);
        diff = ldg_math_ext_sub(src[idx], src[idx + half]);
        out[idx] = ldg_math_ext_add(ldg_math_ext_scale(sum, radix_inverse), ldg_math_ext_mul(alpha, ldg_math_ext_scale(diff, inverted[idx])));
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_ext_fold_pair_get(ldg_math_ext_t lhs_val, ldg_math_ext_t rhs_val, ldg_math_field_elem_t point, ldg_math_ext_t alpha, ldg_math_ext_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    ldg_math_field_elem_t radix_inverse = 0;
    ldg_math_field_elem_t denom_inverse = 0;
    ldg_math_ext_t sum = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t diff = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);

    if (LDG_UNLIKELY(ldg_math_field_reduce(point) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_field_invert_get((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, &radix_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_invert_get(ldg_math_field_mul((ldg_math_field_elem_t)LDG_MATH_FRI_FOLD_RADIX, ldg_math_field_reduce(point)), &denom_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sum = ldg_math_ext_add(lhs_val, rhs_val);
    diff = ldg_math_ext_sub(lhs_val, rhs_val);
    *out = ldg_math_ext_add(ldg_math_ext_scale(sum, radix_inverse), ldg_math_ext_mul(alpha, ldg_math_ext_scale(diff, denom_inverse)));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_fri_ext_remainder_ck(const ldg_math_ext_t *src, uint64_t cunt, ldg_math_field_elem_t shift, uint64_t degree_bound, const ldg_math_field_elem_t *twiddle_inverse, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;
    ldg_math_field_elem_t *lo = 0x0;
    ldg_math_field_elem_t *hi = 0x0;

    if (LDG_UNLIKELY(!src || !twiddle_inverse || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(cunt))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < (cunt * LDG_MATH_FRI_LANE_PAIR))) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(degree_bound > cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    lo = buff;
    hi = buff + cunt;

    for (idx = 0; idx < cunt; idx++)
    {
        lo[idx] = src[idx].lo;
        hi[idx] = src[idx].hi;
    }

    ret = ldg_math_ntt_coset_interpolate(lo, fri_log_get(cunt), shift, twiddle_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_coset_interpolate(hi, fri_log_get(cunt), shift, twiddle_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = degree_bound; idx < cunt; idx++) { if (lo[idx] != LDG_MATH_FIELD_ADD_IDENT || hi[idx] != LDG_MATH_FIELD_ADD_IDENT) { return LDG_ERR_INVALID; } }

    return LDG_ERR_AOK;
}
