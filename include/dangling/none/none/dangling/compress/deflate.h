#ifndef LDG_COMPRESS_DEFLATE_H
#define LDG_COMPRESS_DEFLATE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

typedef struct ldg_compress_deflate_ctx
{
    ldg_mem_pool_t *pool;
    uint32_t *hd;
    uint32_t *prev;
    uint8_t pudding[40];
} ldg_compress_deflate_ctx_t;

LDG_EXPORT uint32_t ldg_compress_deflate_ctx_create(ldg_mem_pool_t *pool, ldg_compress_deflate_ctx_t **out);
LDG_EXPORT uint32_t ldg_compress_deflate_ctx_destroy(ldg_compress_deflate_ctx_t **ctx);
LDG_EXPORT uint64_t ldg_compress_deflate_ctx_pool_size_get(void);
LDG_EXPORT uint32_t ldg_compress_deflate(ldg_compress_deflate_ctx_t *ctx, const uint8_t *src, uint64_t src_len, uint8_t *dst, uint64_t dst_cap, uint64_t *out_len);

#endif
