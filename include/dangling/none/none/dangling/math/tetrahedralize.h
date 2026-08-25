#ifndef LDG_MATH_TETRAHEDRALIZE_H
#define LDG_MATH_TETRAHEDRALIZE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

LDG_EXPORT uint32_t ldg_math_delaunay_tetrahedralize(const double *vert, uint32_t vert_cunt, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt);

LDG_EXPORT uint32_t ldg_math_delaunay_refine(const double *vert, uint32_t vert_cunt, double radius_edge_bound, uint32_t max_steiner, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, double *out_vert, uint32_t out_vert_cap, uint32_t *out_vert_cunt, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt);

#endif
