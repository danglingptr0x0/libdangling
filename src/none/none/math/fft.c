
#include <dangling/math/fft.h>
#include <dangling/math/complex.h>
#include <dangling/math/math.h>
#include <dangling/core/arith.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#define LDG_MATH_FFT_SPAN_TRIVIAL 1
#define LDG_MATH_FFT_EMPTY_PRODUCT 1
#define LDG_MATH_FFT_NYQUIST_BIN 1

static uint32_t fft_alloc_complex(uint64_t cunt, ldg_math_complex_t **out)
{
    void *blk = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(cunt > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    *out = (ldg_math_complex_t *)blk;

    return LDG_ERR_AOK;
}

static void fft_factor_fill(uint64_t len, uint8_t *factor, uint32_t *out_cunt, uint64_t *out_remainder)
{
    uint64_t remainder = len;
    uint64_t divisor = 0;
    uint32_t cunt = 0;

    while (remainder % 2 == 0)
    {
        factor[cunt] = 2;
        cunt++;
        remainder = remainder / 2;
    }

    for (divisor = 3; divisor <= LDG_MATH_FFT_FACTOR_CUTOFF; divisor += 2) { while (remainder % divisor == 0)
        {
            factor[cunt] = (uint8_t)divisor;
            cunt++;
            remainder = remainder / divisor;
        }
    }

    *out_cunt = cunt;
    *out_remainder = remainder;
}

static void fft_twiddle_fill(ldg_math_complex_t *twiddle, uint64_t cunt, uint64_t denom)
{
    uint64_t idx = 0;
    double angle = 0.0;

    for (idx = 0; idx < cunt; idx++)
    {
        angle = -(LDG_MATH_TAU * (double)idx) / (double)denom;
        twiddle[idx].re = ldg_math_cos_get(angle);
        twiddle[idx].im = ldg_math_sin_get(angle);
    }
}

LDG_NOINLINE static void fft_combine(ldg_math_complex_t *dst, uint64_t radix, uint64_t sub_len, uint64_t twiddle_stride, const ldg_math_complex_t *twiddle)
{
    ldg_math_complex_t tmp[LDG_MATH_FFT_FACTOR_CUTOFF] = LDG_ARR_ZERO_INIT;
    uint64_t pos = 0;
    uint64_t leg = 0;
    uint64_t blk = 0;
    uint64_t twiddle_idx = 0;
    double lo_re = 0.0;
    double lo_im = 0.0;
    double hi_re = 0.0;
    double hi_im = 0.0;
    double spin_re = 0.0;
    double spin_im = 0.0;
    double acc_re = 0.0;
    double acc_im = 0.0;

    if (radix == 2)
    {
        for (pos = 0; pos < sub_len; pos++)
        {
            spin_re = twiddle[pos * twiddle_stride].re;
            spin_im = twiddle[pos * twiddle_stride].im;
            lo_re = dst[pos].re;
            lo_im = dst[pos].im;
            hi_re = dst[sub_len + pos].re * spin_re - dst[sub_len + pos].im * spin_im;
            hi_im = dst[sub_len + pos].re * spin_im + dst[sub_len + pos].im * spin_re;
            dst[pos].re = lo_re + hi_re;
            dst[pos].im = lo_im + hi_im;
            dst[sub_len + pos].re = lo_re - hi_re;
            dst[sub_len + pos].im = lo_im - hi_im;
        }

        return;
    }

    for (pos = 0; pos < sub_len; pos++)
    {
        for (leg = 0; leg < radix; leg++)
        {
            twiddle_idx = leg * pos * twiddle_stride;
            spin_re = twiddle[twiddle_idx].re;
            spin_im = twiddle[twiddle_idx].im;
            tmp[leg].re = dst[leg * sub_len + pos].re * spin_re - dst[leg * sub_len + pos].im * spin_im;
            tmp[leg].im = dst[leg * sub_len + pos].re * spin_im + dst[leg * sub_len + pos].im * spin_re;
        }

        for (blk = 0; blk < radix; blk++)
        {
            acc_re = tmp[0].re;
            acc_im = tmp[0].im;
            for (leg = 1; leg < radix; leg++)
            {
                twiddle_idx = ((leg * blk) % radix) * sub_len * twiddle_stride;
                acc_re += tmp[leg].re * twiddle[twiddle_idx].re - tmp[leg].im * twiddle[twiddle_idx].im;
                acc_im += tmp[leg].re * twiddle[twiddle_idx].im + tmp[leg].im * twiddle[twiddle_idx].re;
            }

            dst[blk * sub_len + pos].re = acc_re;
            dst[blk * sub_len + pos].im = acc_im;
        }
    }
}

LDG_NOINLINE static void fft_transform(const ldg_math_complex_t *src, ldg_math_complex_t *dst, uint64_t len, const uint8_t *factor_seq, uint32_t factor_cunt, const ldg_math_complex_t *twiddle)
{
    uint64_t weight[LDG_MATH_FFT_FACTOR_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t dst_idx = 0;
    uint64_t src_idx = 0;
    uint64_t rest = 0;
    uint64_t span = 0;
    uint64_t group_cunt = 0;
    uint64_t group = 0;
    uint64_t radix = 0;
    uint32_t level = 0;

    if (len == LDG_MATH_FFT_SPAN_TRIVIAL || factor_cunt == 0)
    {
        dst[0] = src[0];
        return;
    }

    weight[0] = LDG_MATH_FFT_EMPTY_PRODUCT;
    for (level = 1; level < factor_cunt; level++) { weight[level] = weight[level - 1] * (uint64_t)factor_seq[level - 1]; }

    for (dst_idx = 0; dst_idx < len; dst_idx++)
    {
        rest = dst_idx;
        span = len;
        src_idx = 0;

        for (level = 0; level < factor_cunt; level++)
        {
            radix = (uint64_t)factor_seq[level];
            span = span / radix;
            src_idx += (rest / span) * weight[level];
            rest = rest % span;
        }

        dst[dst_idx] = src[src_idx];
    }

    level = factor_cunt;
    while (level > 0)
    {
        level--;
        group_cunt = weight[level];
        span = len / group_cunt;
        radix = (uint64_t)factor_seq[level];

        for (group = 0; group < group_cunt; group++) { fft_combine(dst + group * span, radix, span / radix, group_cunt, twiddle); }
    }
}

static void fft_run(const ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, ldg_math_complex_t *dst)
{
    const ldg_math_complex_t *engine_src = src;
    uint64_t idx = 0;
    double prod_re = 0.0;
    double prod_im = 0.0;
    double scale_factor = 0.0;

    if (ctx->is_bluestein != LDG_TRUTH_TRUE)
    {
        if (src == dst)
        {
            for (idx = 0; idx < ctx->len; idx++) { ctx->scratch_a[idx] = src[idx]; }

            engine_src = ctx->scratch_a;
        }

        fft_transform(engine_src, dst, ctx->len, ctx->factor, ctx->factor_cunt, ctx->twiddle);
        return;
    }

    for (idx = 0; idx < ctx->len; idx++)
    {
        prod_re = src[idx].re * ctx->chirp[idx].re - src[idx].im * ctx->chirp[idx].im;
        prod_im = src[idx].re * ctx->chirp[idx].im + src[idx].im * ctx->chirp[idx].re;
        ctx->scratch_a[idx].re = prod_re;
        ctx->scratch_a[idx].im = prod_im;
    }

    for (idx = ctx->len; idx < ctx->engine_len; idx++)
    {
        ctx->scratch_a[idx].re = 0.0;
        ctx->scratch_a[idx].im = 0.0;
    }

    fft_transform(ctx->scratch_a, ctx->scratch_b, ctx->engine_len, ctx->factor_engine, ctx->factor_engine_cunt, ctx->twiddle_engine);

    for (idx = 0; idx < ctx->engine_len; idx++)
    {
        prod_re = ctx->scratch_b[idx].re * ctx->chirp_fft[idx].re - ctx->scratch_b[idx].im * ctx->chirp_fft[idx].im;
        prod_im = ctx->scratch_b[idx].re * ctx->chirp_fft[idx].im + ctx->scratch_b[idx].im * ctx->chirp_fft[idx].re;
        ctx->scratch_b[idx].re = prod_re;
        ctx->scratch_b[idx].im = -prod_im;
    }

    fft_transform(ctx->scratch_b, ctx->scratch_a, ctx->engine_len, ctx->factor_engine, ctx->factor_engine_cunt, ctx->twiddle_engine);

    scale_factor = 1.0 / (double)ctx->engine_len;
    for (idx = 0; idx < ctx->len; idx++)
    {
        prod_re = ctx->scratch_a[idx].re * scale_factor;
        prod_im = -(ctx->scratch_a[idx].im) * scale_factor;
        dst[idx].re = prod_re * ctx->chirp[idx].re - prod_im * ctx->chirp[idx].im;
        dst[idx].im = prod_re * ctx->chirp[idx].im + prod_im * ctx->chirp[idx].re;
    }
}

static void fft_inverse_run(const ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, ldg_math_complex_t *dst)
{
    uint64_t idx = 0;
    double re_val = 0.0;
    double im_val = 0.0;
    double scale_factor = 0.0;

    for (idx = 0; idx < ctx->len; idx++)
    {
        re_val = src[idx].re;
        im_val = -src[idx].im;
        dst[idx].re = re_val;
        dst[idx].im = im_val;
    }

    fft_run(ctx, dst, dst);

    scale_factor = 1.0 / (double)ctx->len;
    for (idx = 0; idx < ctx->len; idx++)
    {
        dst[idx].re = dst[idx].re * scale_factor;
        dst[idx].im = -(dst[idx].im) * scale_factor;
    }
}

static void fft_real_even_forward(const ldg_math_fft_ctx_t *ctx, const double *src, ldg_math_complex_t *dst)
{
    uint64_t half_len = ctx->len / 2;
    uint64_t idx = 0;
    double even_re = 0.0;
    double even_im = 0.0;
    double odd_re = 0.0;
    double odd_im = 0.0;
    double lead_re = 0.0;
    double lead_im = 0.0;
    double fold_re = 0.0;
    double fold_im = 0.0;
    double spin_re = 0.0;
    double spin_im = 0.0;

    for (idx = 0; idx < half_len; idx++)
    {
        ctx->scratch_a[idx].re = src[2 * idx];
        ctx->scratch_a[idx].im = src[2 * idx + 1];
    }

    fft_run(ctx->half, ctx->scratch_a, ctx->scratch_b);

    for (idx = 0; idx <= half_len; idx++)
    {
        lead_re = ctx->scratch_b[idx % half_len].re;
        lead_im = ctx->scratch_b[idx % half_len].im;
        fold_re = ctx->scratch_b[(half_len - idx) % half_len].re;
        fold_im = ctx->scratch_b[(half_len - idx) % half_len].im;
        even_re = 0.5 * (lead_re + fold_re);
        even_im = 0.5 * (lead_im - fold_im);
        odd_re = 0.5 * (lead_im + fold_im);
        odd_im = 0.5 * (fold_re - lead_re);
        spin_re = ctx->twiddle_real[idx].re;
        spin_im = ctx->twiddle_real[idx].im;
        dst[idx].re = even_re + spin_re * odd_re - spin_im * odd_im;
        dst[idx].im = even_im + spin_re * odd_im + spin_im * odd_re;
    }
}

static void fft_real_even_inverse(const ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, double *dst)
{
    uint64_t half_len = ctx->len / 2;
    uint64_t idx = 0;
    double even_re = 0.0;
    double even_im = 0.0;
    double diff_re = 0.0;
    double diff_im = 0.0;
    double odd_re = 0.0;
    double odd_im = 0.0;
    double spin_re = 0.0;
    double spin_im = 0.0;

    for (idx = 0; idx < half_len; idx++)
    {
        even_re = 0.5 * (src[idx].re + src[half_len - idx].re);
        even_im = 0.5 * (src[idx].im - src[half_len - idx].im);
        diff_re = 0.5 * (src[idx].re - src[half_len - idx].re);
        diff_im = 0.5 * (src[idx].im + src[half_len - idx].im);
        spin_re = ctx->twiddle_real[idx].re;
        spin_im = ctx->twiddle_real[idx].im;
        odd_re = spin_re * diff_re + spin_im * diff_im;
        odd_im = spin_re * diff_im - spin_im * diff_re;
        ctx->scratch_a[idx].re = even_re - odd_im;
        ctx->scratch_a[idx].im = even_im + odd_re;
    }

    fft_inverse_run(ctx->half, ctx->scratch_a, ctx->scratch_b);

    for (idx = 0; idx < half_len; idx++)
    {
        dst[2 * idx] = ctx->scratch_b[idx].re;
        dst[2 * idx + 1] = ctx->scratch_b[idx].im;
    }
}

static void fft_real_odd_forward(const ldg_math_fft_ctx_t *ctx, const double *src, ldg_math_complex_t *dst)
{
    uint64_t idx = 0;

    for (idx = 0; idx < ctx->len; idx++)
    {
        ctx->scratch_a[idx].re = src[idx];
        ctx->scratch_a[idx].im = 0.0;
    }

    fft_run(ctx, ctx->scratch_a, ctx->scratch_b);

    for (idx = 0; idx <= ctx->len / 2; idx++) { dst[idx] = ctx->scratch_b[idx]; }
}

static void fft_real_odd_inverse(const ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, double *dst)
{
    uint64_t bin_cunt = ctx->len / 2;
    uint64_t idx = 0;

    for (idx = 0; idx <= bin_cunt; idx++) { ctx->scratch_a[idx] = src[idx]; }

    for (idx = bin_cunt + 1; idx < ctx->len; idx++)
    {
        ctx->scratch_a[idx].re = src[ctx->len - idx].re;
        ctx->scratch_a[idx].im = -src[ctx->len - idx].im;
    }

    fft_inverse_run(ctx, ctx->scratch_a, ctx->scratch_b);

    for (idx = 0; idx < ctx->len; idx++) { dst[idx] = ctx->scratch_b[idx].re; }
}

static void fft_ctx_buff_free(ldg_math_fft_ctx_t *ctx)
{
    if (ctx->twiddle) { ldg_mem_dealloc(ctx->twiddle); }

    if (ctx->twiddle_engine) { ldg_mem_dealloc(ctx->twiddle_engine); }

    if (ctx->twiddle_real) { ldg_mem_dealloc(ctx->twiddle_real); }

    if (ctx->chirp) { ldg_mem_dealloc(ctx->chirp); }

    if (ctx->chirp_fft) { ldg_mem_dealloc(ctx->chirp_fft); }

    if (ctx->scratch_a) { ldg_mem_dealloc(ctx->scratch_a); }

    if (ctx->scratch_b) { ldg_mem_dealloc(ctx->scratch_b); }

    ldg_mem_dealloc(ctx);
}

static void fft_ctx_destroy(ldg_math_fft_ctx_t *ctx)
{
    ldg_math_fft_ctx_t *half = 0x0;

    half = ctx->half;
    ctx->half = 0x0;

    if (half) { fft_ctx_buff_free(half); }

    fft_ctx_buff_free(ctx);
}

static uint32_t fft_ctx_build(uint64_t len, uint8_t real_support, ldg_math_fft_ctx_t **out)
{
    ldg_math_fft_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t remainder = 0;
    uint64_t doubled = 0;
    uint64_t tgt = 0;
    uint64_t engine_len = 0;
    uint64_t scratch_len = 0;
    uint64_t square = 0;
    uint64_t idx = 0;
    double angle = 0.0;
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_fft_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_fft_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_fft_ctx_t)) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->len = len;
    fft_factor_fill(len, ctx->factor, &ctx->factor_cunt, &remainder);
    scratch_len = len;

    if (remainder != 1)
    {
        ctx->is_bluestein = LDG_TRUTH_TRUE;
        ret = ldg_arith_uint64_mul(len, 2, &doubled);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            fft_ctx_destroy(ctx);
            return LDG_ERR_OVERFLOW;
        }

        tgt = doubled - 1;
        engine_len = 1;
        while (engine_len < tgt)
        {
            if (LDG_UNLIKELY(engine_len > UINT64_MAX / 2))
            {
                fft_ctx_destroy(ctx);
                return LDG_ERR_OVERFLOW;
            }

            engine_len = engine_len * 2;
        }

        ctx->engine_len = engine_len;
        fft_factor_fill(engine_len, ctx->factor_engine, &ctx->factor_engine_cunt, &remainder);
        scratch_len = engine_len;
    }

    ret = fft_alloc_complex(len, &ctx->twiddle);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        fft_ctx_destroy(ctx);
        return ret;
    }

    fft_twiddle_fill(ctx->twiddle, len, len);

    ret = fft_alloc_complex(scratch_len, &ctx->scratch_a);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        fft_ctx_destroy(ctx);
        return ret;
    }

    ret = fft_alloc_complex(scratch_len, &ctx->scratch_b);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        fft_ctx_destroy(ctx);
        return ret;
    }

    if (ctx->is_bluestein == LDG_TRUTH_TRUE)
    {
        ret = fft_alloc_complex(engine_len, &ctx->twiddle_engine);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            fft_ctx_destroy(ctx);
            return ret;
        }

        fft_twiddle_fill(ctx->twiddle_engine, engine_len, engine_len);

        ret = fft_alloc_complex(len, &ctx->chirp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            fft_ctx_destroy(ctx);
            return ret;
        }

        square = 0;
        for (idx = 0; idx < len; idx++)
        {
            angle = -(LDG_MATH_PI * (double)square) / (double)len;
            ctx->chirp[idx].re = ldg_math_cos_get(angle);
            ctx->chirp[idx].im = ldg_math_sin_get(angle);
            square = (square + 2 * idx + 1) % doubled;
        }

        ret = fft_alloc_complex(engine_len, &ctx->chirp_fft);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            fft_ctx_destroy(ctx);
            return ret;
        }

        for (idx = 0; idx < engine_len; idx++)
        {
            ctx->scratch_a[idx].re = 0.0;
            ctx->scratch_a[idx].im = 0.0;
        }

        ctx->scratch_a[0].re = ctx->chirp[0].re;
        ctx->scratch_a[0].im = -ctx->chirp[0].im;
        for (idx = 1; idx < len; idx++)
        {
            ctx->scratch_a[idx].re = ctx->chirp[idx].re;
            ctx->scratch_a[idx].im = -ctx->chirp[idx].im;
            ctx->scratch_a[engine_len - idx].re = ctx->chirp[idx].re;
            ctx->scratch_a[engine_len - idx].im = -ctx->chirp[idx].im;
        }

        fft_transform(ctx->scratch_a, ctx->chirp_fft, engine_len, ctx->factor_engine, ctx->factor_engine_cunt, ctx->twiddle_engine);
    }

    if (real_support && len % 2 == 0)
    {
        ret = fft_alloc_complex(len / 2 + LDG_MATH_FFT_NYQUIST_BIN, &ctx->twiddle_real);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            fft_ctx_destroy(ctx);
            return ret;
        }

        fft_twiddle_fill(ctx->twiddle_real, len / 2 + LDG_MATH_FFT_NYQUIST_BIN, len);
    }

    *out = ctx;

    return LDG_ERR_AOK;
}

static uint32_t fft_ctx_create(uint64_t len, uint8_t real_support, ldg_math_fft_ctx_t **out)
{
    ldg_math_fft_ctx_t *ctx = 0x0;
    uint32_t ret = 0;

    *out = 0x0;

    ret = fft_ctx_build(len, real_support, &ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (real_support && len % 2 == 0)
    {
        ret = fft_ctx_build(len / 2, LDG_TRUTH_FALSE, &ctx->half);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            fft_ctx_destroy(ctx);
            return ret;
        }
    }

    *out = ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_fft_ctx_create(uint64_t len, ldg_math_fft_ctx_t **out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return fft_ctx_create(len, LDG_TRUTH_TRUE, out);
}

LDG_EXPORT uint32_t ldg_math_fft_ctx_destroy(ldg_math_fft_ctx_t **ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!*ctx)) { return LDG_ERR_AOK; }

    fft_ctx_destroy(*ctx);
    *ctx = 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_fft_forward_compute(ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, ldg_math_complex_t *dst)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    fft_run(ctx, src, dst);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_fft_inverse_compute(ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, ldg_math_complex_t *dst)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    fft_inverse_run(ctx, src, dst);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_fft_real_forward_compute(ldg_math_fft_ctx_t *ctx, const double *src, ldg_math_complex_t *dst)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->len % 2 == 0) { fft_real_even_forward(ctx, src, dst); }
    else { fft_real_odd_forward(ctx, src, dst); }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_fft_real_inverse_compute(ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, double *dst)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->len % 2 == 0) { fft_real_even_inverse(ctx, src, dst); }
    else { fft_real_odd_inverse(ctx, src, dst); }

    return LDG_ERR_AOK;
}
