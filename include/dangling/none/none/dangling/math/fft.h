#ifndef LDG_MATH_FFT_H
#define LDG_MATH_FFT_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/complex.h>

#define LDG_MATH_FFT_FACTOR_MAX 64
#define LDG_MATH_FFT_FACTOR_CUTOFF 31

typedef struct ldg_math_fft_ctx
{
    ldg_math_complex_t *twiddle;
    ldg_math_complex_t *twiddle_engine;
    ldg_math_complex_t *twiddle_real;
    ldg_math_complex_t *chirp;
    ldg_math_complex_t *chirp_fft;
    ldg_math_complex_t *scratch_a;
    ldg_math_complex_t *scratch_b;
    struct ldg_math_fft_ctx *half;
    uint64_t len;
    uint64_t engine_len;
    uint32_t factor_cunt;
    uint32_t factor_engine_cunt;
    uint8_t factor[LDG_MATH_FFT_FACTOR_MAX];
    uint8_t factor_engine[LDG_MATH_FFT_FACTOR_MAX];
    uint8_t is_bluestein;
    uint8_t pudding[7];
} ldg_math_fft_ctx_t;

LDG_EXPORT uint32_t ldg_math_fft_ctx_create(uint64_t len, ldg_math_fft_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_fft_ctx_destroy(ldg_math_fft_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_fft_forward_compute(ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, ldg_math_complex_t *dst);
LDG_EXPORT uint32_t ldg_math_fft_inverse_compute(ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, ldg_math_complex_t *dst);
LDG_EXPORT uint32_t ldg_math_fft_real_forward_compute(ldg_math_fft_ctx_t *ctx, const double *src, ldg_math_complex_t *dst);
LDG_EXPORT uint32_t ldg_math_fft_real_inverse_compute(ldg_math_fft_ctx_t *ctx, const ldg_math_complex_t *src, double *dst);

#endif
