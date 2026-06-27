#ifndef LDG_COMPRESS_INFLATE_H
#define LDG_COMPRESS_INFLATE_H

#include <stdint.h>

#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_compress_inflate(const uint8_t *src, uint64_t src_len, uint8_t *dst, uint64_t dst_cap, uint64_t *out_len);

#endif
