#ifndef LDG_MATH_TRIANGULATE_H
#define LDG_MATH_TRIANGULATE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

LDG_EXPORT uint32_t ldg_math_polygon_triangulate(const double *vert, uint32_t vert_cunt, const uint32_t *ring_start, uint32_t ring_cunt, ldg_mem_pool_t *pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt);

LDG_EXPORT uint32_t ldg_math_delaunay_triangulate(const double *vert, uint32_t vert_cunt, const uint32_t *segment, uint32_t segment_cunt, ldg_mem_pool_t *pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt);

#endif
