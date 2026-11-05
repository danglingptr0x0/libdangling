#ifndef LDG_FMT_BMP_H
#define LDG_FMT_BMP_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_FMT_BMP_BPP_RGB 24U
#define LDG_FMT_BMP_BPP_RGBA 32U
#define LDG_FMT_BMP_SRC_CHANNEL_CUNT 3U
#define LDG_FMT_BMP_HDR_SIZE 54U

LDG_EXPORT uint32_t ldg_fmt_bmp_size_get(uint32_t width, uint32_t height, uint32_t bpp, uint64_t *out);
LDG_EXPORT uint32_t ldg_fmt_bmp_encode(const uint8_t *src, uint32_t width, uint32_t height, uint64_t stride, uint32_t bpp, uint8_t *dst, uint64_t cap, uint64_t *out_len);

#endif
