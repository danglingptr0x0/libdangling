#ifndef LDG_SORT_SORT_H
#define LDG_SORT_SORT_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef uint32_t (*ldg_sort_cmp_cb_t)(const void *lhs, const void *rhs, void *user_ctx);

LDG_EXPORT uint32_t ldg_sort_run(void *base, uint64_t cunt, uint64_t item_size, ldg_sort_cmp_cb_t cmp, void *user_ctx);
LDG_EXPORT uint32_t ldg_sort_binary_find(const void *base, uint64_t cunt, uint64_t item_size, const void *key, ldg_sort_cmp_cb_t cmp, void *user_ctx, uint64_t *out_idx);
LDG_EXPORT uint8_t ldg_sort_ordered_is(const void *base, uint64_t cunt, uint64_t item_size, ldg_sort_cmp_cb_t cmp, void *user_ctx);

#endif
