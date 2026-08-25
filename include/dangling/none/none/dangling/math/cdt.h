#ifndef LDG_MATH_CDT_H
#define LDG_MATH_CDT_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

LDG_EXPORT uint32_t ldg_math_cdt_tetrahedralize(const double *vert, uint32_t vert_cunt, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt);

LDG_EXPORT uint32_t ldg_math_cdt_build(const double *vert, uint32_t vert_cunt, const uint32_t *segment, uint32_t segment_cunt, uint32_t max_steiner, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *segment_pool, ldg_mem_pool_t *predicate_pool, double *out_vert, uint32_t out_vert_cap, uint32_t *out_vert_cunt, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt);

LDG_EXPORT uint32_t ldg_math_cdt_segment_flip_recover(const double *vert, uint32_t node_a, uint32_t node_b, const uint32_t *in_idx, uint32_t in_idx_cunt, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt, uint8_t *recovered);

#endif
