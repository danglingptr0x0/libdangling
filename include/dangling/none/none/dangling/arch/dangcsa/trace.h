#ifndef LDG_ARCH_DANGCSA_TRACE_H
#define LDG_ARCH_DANGCSA_TRACE_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/arch/dangcsa/dlu.h>
#include <dangling/math/stat.h>

#define LDG_DANGCSA_TRACE_INSTR_TOK_CUNT 8
#define LDG_DANGCSA_TRACE_RESP_TOK_CUNT 11
#define LDG_DANGCSA_TRACE_TOK_CUNT 19
#define LDG_DANGCSA_TRACE_TOK_MASK 256
#define LDG_DANGCSA_TRACE_TOK_PAD 257
#define LDG_DANGCSA_TRACE_VOCAB_CUNT 258
#define LDG_DANGCSA_TRACE_EXC_FAULT 7

LDG_EXPORT uint32_t ldg_dangcsa_trace_instr_draw(const ldg_dangcsa_dlu_t *dlu, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, ldg_qword_t *out);
LDG_EXPORT uint32_t ldg_dangcsa_trace_instr_encode(ldg_qword_t raw, uint32_t *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_dangcsa_trace_resp_encode(const ldg_dangcsa_dlu_t *dlu, uint32_t *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_dangcsa_trace_sample_fill(ldg_dangcsa_dlu_t *dlu, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, uint32_t instr_cunt, double *tok_in, double *tgt, double *mask, uint32_t cap, uint32_t *out_cunt);

#endif
