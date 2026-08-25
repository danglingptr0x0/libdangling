#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/mem/mem.h>
#include <dangling/mem/alloc.h>
#include <dangling/compress/deflate.h>

#define LDG_DEFLATE_WINDOW 32768
#define LDG_DEFLATE_WINDOW_MASK 32767
#define LDG_DEFLATE_CTX_BLK_CUNT 3
#define LDG_DEFLATE_HASH_SIZE 32768
#define LDG_DEFLATE_HASH_MASK 32767
#define LDG_DEFLATE_HASH_SHIFT_FIRST 10
#define LDG_DEFLATE_HASH_SHIFT_SECOND 5
#define LDG_DEFLATE_HASH_OFF_SECOND 1
#define LDG_DEFLATE_HASH_OFF_THIRD 2
#define LDG_DEFLATE_MIN_MATCH 3
#define LDG_DEFLATE_MAX_MATCH 258
#define LDG_DEFLATE_CHAIN_MAX 128
#define LDG_DEFLATE_NIL UINT32_MAX
#define LDG_DEFLATE_ADLER_BASE 65521
#define LDG_DEFLATE_ADLER_NMAX 5552
#define LDG_DEFLATE_ADLER_INIT 1
#define LDG_DEFLATE_ADLER_SHIFT_MID 16
#define LDG_DEFLATE_ZLIB_CMF 0x78
#define LDG_DEFLATE_ZLIB_FLG 0x9C
#define LDG_DEFLATE_ZLIB_CMF_OFF 0
#define LDG_DEFLATE_ZLIB_FLG_OFF 1
#define LDG_DEFLATE_ZLIB_HDR_SIZE 2
#define LDG_DEFLATE_ZLIB_TRAILER_SIZE 4
#define LDG_DEFLATE_LEN_SYM_CUNT 29
#define LDG_DEFLATE_DIST_SYM_CUNT 30
#define LDG_DEFLATE_LEN_SYM_BASE 257
#define LDG_DEFLATE_EOB_SYM 256
#define LDG_DEFLATE_DIST_CODE_BITS 5
#define LDG_DEFLATE_LIT8_END 144
#define LDG_DEFLATE_LIT9_END 256
#define LDG_DEFLATE_LIT7_END 280
#define LDG_DEFLATE_LIT8_BASE 0x30
#define LDG_DEFLATE_LIT9_BASE 0x190
#define LDG_DEFLATE_LIT7_BASE 0x0
#define LDG_DEFLATE_LIT8_HIGH_BASE 0xC0
#define LDG_DEFLATE_LIT7_BITS 7
#define LDG_DEFLATE_LIT8_BITS 8
#define LDG_DEFLATE_LIT9_BITS 9
#define LDG_DEFLATE_BFINAL_BITS 1
#define LDG_DEFLATE_BTYPE_BITS 2
#define LDG_DEFLATE_BTYPE_FIXED 1
#define LDG_DEFLATE_BFINAL_LAST 1
#define LDG_DEFLATE_BIT_ONE 1
#define LDG_DEFLATE_LIT_ADVANCE 1
#define LDG_DEFLATE_EMPTY 0

#define LDG_DEFLATE_LEN_BASE_INIT { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 }
#define LDG_DEFLATE_LEN_EXTRA_INIT { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 }
#define LDG_DEFLATE_DIST_BASE_INIT { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 }
#define LDG_DEFLATE_DIST_EXTRA_INIT { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 }

typedef struct deflate_state
{
    uint8_t *out;
    uint64_t out_cap;
    uint64_t out_cnt;
    uint32_t bitbuff;
    uint32_t bitcnt;
} deflate_state_t;

static uint32_t deflate_bits_put(deflate_state_t *st, uint32_t val, uint32_t cunt)
{
    st->bitbuff = st->bitbuff | (val << st->bitcnt);
    st->bitcnt = st->bitcnt + cunt;

    while (st->bitcnt >= LDG_BYTE_BITS)
    {
        if (LDG_UNLIKELY(st->out_cnt >= st->out_cap)) { return LDG_ERR_BOUNDS; }

        st->out[st->out_cnt] = (uint8_t)(st->bitbuff & LDG_BYTE_MASK);
        st->out_cnt++;
        st->bitbuff = st->bitbuff >> LDG_BYTE_BITS;
        st->bitcnt = st->bitcnt - LDG_BYTE_BITS;
    }

    return LDG_ERR_AOK;
}

static uint32_t deflate_code_put(deflate_state_t *st, uint32_t code, uint32_t cunt)
{
    uint32_t rev = 0;
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++) { rev = (rev << LDG_DEFLATE_BIT_ONE) | ((code >> idx) & LDG_DEFLATE_BIT_ONE); }

    return deflate_bits_put(st, rev, cunt);
}

static uint32_t deflate_lit_put(deflate_state_t *st, uint32_t sym)
{
    uint32_t code = 0;
    uint32_t cunt = 0;

    if (sym < LDG_DEFLATE_LIT8_END)
    {
        code = LDG_DEFLATE_LIT8_BASE + sym;
        cunt = LDG_DEFLATE_LIT8_BITS;
    }
    else if (sym < LDG_DEFLATE_LIT9_END)
    {
        code = LDG_DEFLATE_LIT9_BASE + (sym - LDG_DEFLATE_LIT8_END);
        cunt = LDG_DEFLATE_LIT9_BITS;
    }
    else if (sym < LDG_DEFLATE_LIT7_END)
    {
        code = LDG_DEFLATE_LIT7_BASE + (sym - LDG_DEFLATE_LIT9_END);
        cunt = LDG_DEFLATE_LIT7_BITS;
    }
    else
    {
        code = LDG_DEFLATE_LIT8_HIGH_BASE + (sym - LDG_DEFLATE_LIT7_END);
        cunt = LDG_DEFLATE_LIT8_BITS;
    }

    return deflate_code_put(st, code, cunt);
}

static uint32_t deflate_match_put(deflate_state_t *st, uint32_t len, uint64_t dist)
{
    const uint16_t len_base[LDG_DEFLATE_LEN_SYM_CUNT] = LDG_DEFLATE_LEN_BASE_INIT;
    const uint16_t len_extra[LDG_DEFLATE_LEN_SYM_CUNT] = LDG_DEFLATE_LEN_EXTRA_INIT;
    const uint16_t dist_base[LDG_DEFLATE_DIST_SYM_CUNT] = LDG_DEFLATE_DIST_BASE_INIT;
    const uint16_t dist_extra[LDG_DEFLATE_DIST_SYM_CUNT] = LDG_DEFLATE_DIST_EXTRA_INIT;
    uint32_t len_sym = 0;
    uint32_t dist_sym = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < LDG_DEFLATE_LEN_SYM_CUNT; idx++) { if (len >= len_base[idx]) { len_sym = idx; } }

    for (idx = 0; idx < LDG_DEFLATE_DIST_SYM_CUNT; idx++) { if (dist >= dist_base[idx]) { dist_sym = idx; } }

    ret = deflate_lit_put(st, LDG_DEFLATE_LEN_SYM_BASE + len_sym);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (len_extra[len_sym] > 0)
    {
        ret = deflate_bits_put(st, len - len_base[len_sym], len_extra[len_sym]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = deflate_code_put(st, dist_sym, LDG_DEFLATE_DIST_CODE_BITS);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (dist_extra[dist_sym] > 0)
    {
        ret = deflate_bits_put(st, (uint32_t)(dist - dist_base[dist_sym]), dist_extra[dist_sym]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t deflate_hash_get(const uint8_t *src, uint64_t pos)
{
    uint32_t hash = 0;

    hash = ((uint32_t)src[pos] << LDG_DEFLATE_HASH_SHIFT_FIRST) ^ ((uint32_t)src[pos + LDG_DEFLATE_HASH_OFF_SECOND] << LDG_DEFLATE_HASH_SHIFT_SECOND) ^ (uint32_t)src[pos + LDG_DEFLATE_HASH_OFF_THIRD];

    return hash & LDG_DEFLATE_HASH_MASK;
}

static void deflate_match_find(const uint8_t *src, uint64_t src_len, uint64_t pos, const uint32_t *hd, const uint32_t *prev, uint32_t *out_len, uint64_t *out_dist)
{
    uint64_t cand = 0;
    uint64_t limit = 0;
    uint32_t chain = 0;
    uint32_t best_len = 0;
    uint64_t best_dist = 0;
    uint32_t span = 0;
    uint32_t max_len = 0;

    *out_len = LDG_DEFLATE_EMPTY;
    *out_dist = LDG_DEFLATE_EMPTY;

    max_len = (uint32_t)(src_len - pos);
    if (max_len > LDG_DEFLATE_MAX_MATCH) { max_len = LDG_DEFLATE_MAX_MATCH; }

    if (max_len < LDG_DEFLATE_MIN_MATCH) { return; }

    limit = (pos > LDG_DEFLATE_WINDOW) ? (pos - LDG_DEFLATE_WINDOW) : LDG_DEFLATE_EMPTY;
    cand = hd[deflate_hash_get(src, pos)];

    while (cand != LDG_DEFLATE_NIL && chain < LDG_DEFLATE_CHAIN_MAX)
    {
        if (cand < limit) { break; }

        span = LDG_DEFLATE_EMPTY;
        while (span < max_len && src[cand + span] == src[pos + span]) { span++; }

        if (span > best_len)
        {
            best_len = span;
            best_dist = pos - cand;
        }

        if (best_len >= max_len) { break; }

        if (cand == 0) { break; }

        cand = prev[cand & LDG_DEFLATE_WINDOW_MASK];
        chain++;
    }

    if (best_len >= LDG_DEFLATE_MIN_MATCH)
    {
        *out_len = best_len;
        *out_dist = best_dist;
    }
}

static uint32_t deflate_adler32(const uint8_t *data, uint64_t len)
{
    uint32_t lo = LDG_DEFLATE_ADLER_INIT;
    uint32_t hi = 0;
    uint64_t idx = 0;
    uint64_t span = 0;

    while (len > 0)
    {
        span = (len < LDG_DEFLATE_ADLER_NMAX) ? len : LDG_DEFLATE_ADLER_NMAX;
        len = len - span;

        for (idx = 0; idx < span; idx++)
        {
            lo = lo + data[idx];
            hi = hi + lo;
        }

        data = data + span;
        lo = lo % LDG_DEFLATE_ADLER_BASE;
        hi = hi % LDG_DEFLATE_ADLER_BASE;
    }

    return (hi << LDG_DEFLATE_ADLER_SHIFT_MID) | lo;
}

static uint32_t deflate_stream_wr(deflate_state_t *st, const uint8_t *src, uint64_t src_len, uint32_t *hd, uint32_t *prev)
{
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint64_t dist = 0;
    uint32_t hash = 0;
    uint32_t len = 0;
    uint32_t ret = 0;

    ret = deflate_bits_put(st, LDG_DEFLATE_BFINAL_LAST, LDG_DEFLATE_BFINAL_BITS);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = deflate_bits_put(st, LDG_DEFLATE_BTYPE_FIXED, LDG_DEFLATE_BTYPE_BITS);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (pos < src_len)
    {
        len = LDG_DEFLATE_EMPTY;
        dist = LDG_DEFLATE_EMPTY;

        if (src_len - pos >= LDG_DEFLATE_MIN_MATCH) { deflate_match_find(src, src_len, pos, hd, prev, &len, &dist); }

        if (len >= LDG_DEFLATE_MIN_MATCH)
        {
            ret = deflate_match_put(st, len, dist);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else
        {
            len = LDG_DEFLATE_LIT_ADVANCE;

            ret = deflate_lit_put(st, src[pos]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        for (idx = 0; idx < len; idx++)
        {
            if (src_len - (pos + idx) < LDG_DEFLATE_MIN_MATCH) { break; }

            hash = deflate_hash_get(src, pos + idx);
            prev[(pos + idx) & LDG_DEFLATE_WINDOW_MASK] = hd[hash];
            hd[hash] = (uint32_t)(pos + idx);
        }

        pos = pos + len;
    }

    ret = deflate_lit_put(st, LDG_DEFLATE_EOB_SYM);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (st->bitcnt > 0)
    {
        if (LDG_UNLIKELY(st->out_cnt >= st->out_cap)) { return LDG_ERR_BOUNDS; }

        st->out[st->out_cnt] = (uint8_t)(st->bitbuff & LDG_BYTE_MASK);
        st->out_cnt++;
        st->bitbuff = LDG_DEFLATE_EMPTY;
        st->bitcnt = LDG_DEFLATE_EMPTY;
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_compress_deflate_ctx_pool_size_get(void)
{
    uint64_t total = 0;

    total = (uint64_t)sizeof(ldg_compress_deflate_ctx_t);
    total += LDG_DEFLATE_HASH_SIZE * (uint64_t)sizeof(uint32_t);
    total += LDG_DEFLATE_WINDOW * (uint64_t)sizeof(uint32_t);
    total += LDG_MEM_POOL_VAR_ALIGN * LDG_DEFLATE_CTX_BLK_CUNT;

    return total;
}

LDG_EXPORT uint32_t ldg_compress_deflate_ctx_create(ldg_mem_pool_t *pool, ldg_compress_deflate_ctx_t **out)
{
    ldg_compress_deflate_ctx_t *ctx = 0x0;
    void *ctx_tmp = 0x0;
    void *hd_tmp = 0x0;
    void *prev_tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(ldg_compress_deflate_ctx_t), &ctx_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, LDG_DEFLATE_HASH_SIZE * (uint64_t)sizeof(uint32_t), &hd_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, LDG_DEFLATE_WINDOW * (uint64_t)sizeof(uint32_t), &prev_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx = (ldg_compress_deflate_ctx_t *)ctx_tmp;
    ctx->pool = pool;
    ctx->hd = (uint32_t *)hd_tmp;
    ctx->prev = (uint32_t *)prev_tmp;

    *out = ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_compress_deflate_ctx_destroy(ldg_compress_deflate_ctx_t **ctx)
{
    ldg_mem_pool_t *pool = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    pool = (*ctx)->pool;
    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_NOT_INIT; }

    if (ldg_mem_pool_var_is(pool) == LDG_TRUTH_TRUE)
    {
        *ctx = 0x0;

        return ldg_mem_pool_rst(pool);
    }

    ret = ldg_mem_pool_dealloc(pool, (*ctx)->prev);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_dealloc(pool, (*ctx)->hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_dealloc(pool, *ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *ctx = 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_compress_deflate(ldg_compress_deflate_ctx_t *ctx, const uint8_t *src, uint64_t src_len, uint8_t *dst, uint64_t dst_cap, uint64_t *out_len)
{
    deflate_state_t state = LDG_STRUCT_ZERO_INIT;
    uint32_t *hd = 0x0;
    uint32_t *prev = 0x0;
    uint64_t idx = 0;
    uint64_t shift = 0;
    uint32_t adler = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src || !dst || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = LDG_DEFLATE_EMPTY;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->hd || !ctx->prev)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(src_len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(src_len > UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(dst_cap < LDG_DEFLATE_ZLIB_HDR_SIZE + LDG_DEFLATE_ZLIB_TRAILER_SIZE)) { return LDG_ERR_BOUNDS; }

    hd = ctx->hd;
    prev = ctx->prev;

    for (idx = 0; idx < LDG_DEFLATE_HASH_SIZE; idx++) { hd[idx] = LDG_DEFLATE_NIL; }

    for (idx = 0; idx < LDG_DEFLATE_WINDOW; idx++) { prev[idx] = LDG_DEFLATE_NIL; }

    dst[LDG_DEFLATE_ZLIB_CMF_OFF] = LDG_DEFLATE_ZLIB_CMF;
    dst[LDG_DEFLATE_ZLIB_FLG_OFF] = LDG_DEFLATE_ZLIB_FLG;

    state.out = dst;
    state.out_cap = dst_cap - LDG_DEFLATE_ZLIB_TRAILER_SIZE;
    state.out_cnt = LDG_DEFLATE_ZLIB_HDR_SIZE;
    state.bitbuff = LDG_DEFLATE_EMPTY;
    state.bitcnt = LDG_DEFLATE_EMPTY;

    ret = deflate_stream_wr(&state, src, src_len, hd, prev);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adler = deflate_adler32(src, src_len);

    for (idx = 0; idx < LDG_DEFLATE_ZLIB_TRAILER_SIZE; idx++)
    {
        shift = (LDG_DEFLATE_ZLIB_TRAILER_SIZE - idx - LDG_DEFLATE_BIT_ONE) * LDG_BYTE_BITS;
        dst[state.out_cnt + idx] = (uint8_t)((adler >> shift) & LDG_BYTE_MASK);
    }

    *out_len = state.out_cnt + LDG_DEFLATE_ZLIB_TRAILER_SIZE;

    return LDG_ERR_AOK;
}
