#include <stdint.h>

#include <dangling/fmt/bmp.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>

#define LDG_FMT_BMP_MAGIC_LEAD 'B'
#define LDG_FMT_BMP_MAGIC_TRAIL 'M'
#define LDG_FMT_BMP_INFO_HDR_SIZE 40U
#define LDG_FMT_BMP_OFF_MAGIC_TRAIL 1U
#define LDG_FMT_BMP_OFF_FILE_SIZE 2U
#define LDG_FMT_BMP_OFF_DATA 10U
#define LDG_FMT_BMP_OFF_INFO_SIZE 14U
#define LDG_FMT_BMP_OFF_WIDTH 18U
#define LDG_FMT_BMP_OFF_HEIGHT 22U
#define LDG_FMT_BMP_OFF_PLANES 26U
#define LDG_FMT_BMP_OFF_BPP 28U
#define LDG_FMT_BMP_OFF_COMPRESSION 30U
#define LDG_FMT_BMP_OFF_IMG_SIZE 34U
#define LDG_FMT_BMP_OFF_RES_X 38U
#define LDG_FMT_BMP_OFF_RES_Y 42U
#define LDG_FMT_BMP_PLANE_CUNT 1U
#define LDG_FMT_BMP_COMPRESSION_RGB 0U
#define LDG_FMT_BMP_PIXEL_PER_METER 2835U
#define LDG_FMT_BMP_ROW_ALIGN 4U
#define LDG_FMT_BMP_ALPHA_OPAQUE 0xFFU
#define LDG_FMT_BMP_DIM_MAX ((uint32_t)INT32_MAX)
#define LDG_FMT_BMP_STEP 1U
#define LDG_FMT_BMP_RED_IDX 0U
#define LDG_FMT_BMP_GREEN_IDX 1U
#define LDG_FMT_BMP_BLUE_IDX 2U
#define LDG_FMT_BMP_ALPHA_IDX 3U
#define LDG_FMT_BMP_LE_THIRD_IDX 2U
#define LDG_FMT_BMP_LE_FOURTH_IDX 3U

static void bmp_le16_put(uint8_t *dst, uint32_t off, uint32_t val)
{
    dst[off] = (uint8_t)(val & LDG_BYTE_MASK);
    dst[off + LDG_FMT_BMP_STEP] = (uint8_t)((val >> LDG_BYTE_SHIFT_1) & LDG_BYTE_MASK);
}

static void bmp_le32_put(uint8_t *dst, uint32_t off, uint32_t val)
{
    dst[off] = (uint8_t)(val & LDG_BYTE_MASK);
    dst[off + LDG_FMT_BMP_STEP] = (uint8_t)((val >> LDG_BYTE_SHIFT_1) & LDG_BYTE_MASK);
    dst[off + LDG_FMT_BMP_LE_THIRD_IDX] = (uint8_t)((val >> LDG_BYTE_SHIFT_2) & LDG_BYTE_MASK);
    dst[off + LDG_FMT_BMP_LE_FOURTH_IDX] = (uint8_t)((val >> LDG_BYTE_SHIFT_3) & LDG_BYTE_MASK);
}

static uint64_t bmp_row_size_get(uint32_t width, uint32_t bpp)
{
    uint64_t row = (uint64_t)width * (bpp / LDG_BYTE_BITS);

    return ((row + LDG_FMT_BMP_ROW_ALIGN - LDG_FMT_BMP_STEP) / LDG_FMT_BMP_ROW_ALIGN) * LDG_FMT_BMP_ROW_ALIGN;
}

uint32_t ldg_fmt_bmp_size_get(uint32_t width, uint32_t height, uint32_t bpp, uint64_t *out)
{
    uint64_t row_size = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(width == 0 || height == 0 || width > LDG_FMT_BMP_DIM_MAX || height > LDG_FMT_BMP_DIM_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(bpp != LDG_FMT_BMP_BPP_RGB && bpp != LDG_FMT_BMP_BPP_RGBA)) { return LDG_ERR_FUNC_ARG_INVALID; }

    row_size = bmp_row_size_get(width, bpp);

    if (LDG_UNLIKELY(row_size > ((uint64_t)UINT32_MAX - LDG_FMT_BMP_HDR_SIZE) / height)) { return LDG_ERR_OVERFLOW; }

    *out = LDG_FMT_BMP_HDR_SIZE + (row_size * height);

    return LDG_ERR_AOK;
}

uint32_t ldg_fmt_bmp_encode(const uint8_t *src, uint32_t width, uint32_t height, uint64_t stride, uint32_t bpp, uint8_t *dst, uint64_t cap, uint64_t *out_len)
{
    const uint8_t *src_px = 0x0;
    uint8_t *dst_row = 0x0;
    uint64_t total = 0;
    uint64_t row_size = 0;
    uint64_t pos = 0;
    uint32_t pixel_size = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src || !dst || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    ret = ldg_fmt_bmp_size_get(width, height, bpp, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(stride < (uint64_t)width * LDG_FMT_BMP_SRC_CHANNEL_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < total)) { return LDG_ERR_FULL; }

    row_size = bmp_row_size_get(width, bpp);

    ret = ldg_mem_zero(dst, total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dst[0] = (uint8_t)LDG_FMT_BMP_MAGIC_LEAD;
    dst[LDG_FMT_BMP_OFF_MAGIC_TRAIL] = (uint8_t)LDG_FMT_BMP_MAGIC_TRAIL;
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_FILE_SIZE, (uint32_t)total);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_DATA, LDG_FMT_BMP_HDR_SIZE);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_INFO_SIZE, LDG_FMT_BMP_INFO_HDR_SIZE);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_WIDTH, width);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_HEIGHT, height);
    bmp_le16_put(dst, LDG_FMT_BMP_OFF_PLANES, LDG_FMT_BMP_PLANE_CUNT);
    bmp_le16_put(dst, LDG_FMT_BMP_OFF_BPP, bpp);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_COMPRESSION, LDG_FMT_BMP_COMPRESSION_RGB);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_IMG_SIZE, (uint32_t)(total - LDG_FMT_BMP_HDR_SIZE));
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_RES_X, LDG_FMT_BMP_PIXEL_PER_METER);
    bmp_le32_put(dst, LDG_FMT_BMP_OFF_RES_Y, LDG_FMT_BMP_PIXEL_PER_METER);

    pixel_size = bpp / LDG_BYTE_BITS;

    for (row = 0; row < height; row++)
    {
        dst_row = dst + LDG_FMT_BMP_HDR_SIZE + ((uint64_t)(height - LDG_FMT_BMP_STEP - row) * row_size);
        pos = 0;

        for (col = 0; col < width; col++)
        {
            src_px = src + ((uint64_t)row * stride) + ((uint64_t)col * LDG_FMT_BMP_SRC_CHANNEL_CUNT);
            dst_row[pos] = src_px[LDG_FMT_BMP_BLUE_IDX];
            dst_row[pos + LDG_FMT_BMP_GREEN_IDX] = src_px[LDG_FMT_BMP_GREEN_IDX];
            dst_row[pos + LDG_FMT_BMP_BLUE_IDX] = src_px[LDG_FMT_BMP_RED_IDX];

            if (bpp == LDG_FMT_BMP_BPP_RGBA) { dst_row[pos + LDG_FMT_BMP_ALPHA_IDX] = (uint8_t)LDG_FMT_BMP_ALPHA_OPAQUE; }

            pos += pixel_size;
        }
    }

    *out_len = total;

    return LDG_ERR_AOK;
}
