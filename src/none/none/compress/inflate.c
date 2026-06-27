#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/compress/inflate.h>

#define LDG_INFLATE_MAX_BITS 15
#define LDG_INFLATE_MAX_LIT 288
#define LDG_INFLATE_MAX_DIST 30
#define LDG_INFLATE_MAX_CODE_LEN 19
#define LDG_INFLATE_ADLER_BASE 65521
#define LDG_INFLATE_ADLER_NMAX 5552
#define LDG_INFLATE_ZLIB_CM_DEFLATE 8
#define LDG_INFLATE_NOINLINE __attribute__((noinline))

typedef struct inflate_state
{
    const uint8_t *in;
    uint8_t *out;
    uint64_t inlen;
    uint64_t incnt;
    uint64_t outlen;
    uint64_t outcnt;
    uint32_t bitbuf;
    uint32_t bitcnt;
} inflate_state_t;

static uint32_t inflate_bits(inflate_state_t *st, uint32_t need, uint32_t *out_val)
{
    uint32_t val = 0x0;

    val = st->bitbuf;
    while (st->bitcnt < need)
    {
        if (LDG_UNLIKELY(st->incnt >= st->inlen)) { return LDG_ERR_INVALID; }

        val |= (uint32_t)st->in[st->incnt] << st->bitcnt;
        st->incnt++;
        st->bitcnt += 8;
    }

    st->bitbuf = val >> need;
    st->bitcnt -= need;
    *out_val = val & (((uint32_t)1 << need) - 1);
    return LDG_ERR_AOK;
}

static uint32_t inflate_decode(inflate_state_t *st, const uint16_t *cunt, const uint16_t *sym, int32_t *out_sym)
{
    uint32_t code = 0x0;
    uint32_t first = 0x0;
    uint32_t idx = 0x0;
    uint32_t len_cunt = 0x0;
    int32_t len = 0x0;
    uint32_t bit = 0x0;
    uint32_t ret = 0x0;

    for (len = 1; len <= LDG_INFLATE_MAX_BITS; len++)
    {
        ret = inflate_bits(st, 1, &bit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        code |= bit;
        len_cunt = (uint32_t)cunt[len];
        if (code < first + len_cunt)
        {
            *out_sym = (int32_t)sym[idx + (code - first)];
            return LDG_ERR_AOK;
        }

        idx += len_cunt;
        first += len_cunt;
        first <<= 1;
        code <<= 1;
    }

    return LDG_ERR_INVALID;
}

static uint32_t inflate_construct(uint16_t *cunt, uint16_t *sym, const uint8_t *code_len, uint32_t num, uint32_t *out_avail)
{
    uint16_t offs[LDG_INFLATE_MAX_BITS + 1] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0x0;
    int32_t len = 0x0;
    uint32_t avail = 0x0;

    for (len = 0; len <= LDG_INFLATE_MAX_BITS; len++) { cunt[len] = 0; }

    for (idx = 0; idx < num; idx++) { cunt[code_len[idx]]++; }

    if (cunt[0] == num)
    {
        *out_avail = 0;
        return LDG_ERR_AOK;
    }

    avail = 1;
    for (len = 1; len <= LDG_INFLATE_MAX_BITS; len++)
    {
        avail <<= 1;
        if (LDG_UNLIKELY((uint32_t)cunt[len] > avail)) { return LDG_ERR_INVALID; }

        avail -= (uint32_t)cunt[len];
    }

    offs[1] = 0;
    for (len = 1; len < LDG_INFLATE_MAX_BITS; len++) { offs[len + 1] = (uint16_t)(offs[len] + cunt[len]); }

    for (idx = 0; idx < num; idx++) { if (code_len[idx] != 0)
        {
            sym[offs[code_len[idx]]] = (uint16_t)idx;
            offs[code_len[idx]]++;
        }
    }

    *out_avail = avail;
    return LDG_ERR_AOK;
}

static LDG_INFLATE_NOINLINE uint32_t inflate_codes(inflate_state_t *st, const uint16_t *len_cunt, const uint16_t *len_sym, const uint16_t *dist_cunt, const uint16_t *dist_sym)
{
    const uint16_t lens[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
    const uint16_t lext[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
    const uint16_t dists[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
    const uint16_t dext[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
    int32_t sym = 0x0;
    uint32_t len = 0x0;
    uint64_t dist = 0x0;
    uint32_t extra = 0x0;
    uint32_t ret = 0x0;

    do
    {
        ret = inflate_decode(st, len_cunt, len_sym, &sym);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (sym < 256)
        {
            if (LDG_UNLIKELY(st->outcnt >= st->outlen)) { return LDG_ERR_BOUNDS; }

            st->out[st->outcnt] = (uint8_t)sym;
            st->outcnt++;
        }
        else if (sym > 256)
        {
            sym -= 257;
            if (LDG_UNLIKELY(sym >= 29)) { return LDG_ERR_INVALID; }

            ret = inflate_bits(st, lext[sym], &extra);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            len = (uint32_t)lens[sym] + extra;

            ret = inflate_decode(st, dist_cunt, dist_sym, &sym);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (LDG_UNLIKELY(sym >= 30)) { return LDG_ERR_INVALID; }

            ret = inflate_bits(st, dext[sym], &extra);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            dist = (uint64_t)dists[sym] + extra;
            if (LDG_UNLIKELY(dist > st->outcnt)) { return LDG_ERR_INVALID; }

            while (len > 0)
            {
                if (LDG_UNLIKELY(st->outcnt >= st->outlen)) { return LDG_ERR_BOUNDS; }

                st->out[st->outcnt] = st->out[st->outcnt - dist];
                st->outcnt++;
                len--;
            }
        }
    } while (sym != 256);

    return LDG_ERR_AOK;
}

static uint32_t inflate_stored(inflate_state_t *st)
{
    uint32_t len = 0x0;
    uint32_t nlen = 0x0;

    st->bitbuf = 0;
    st->bitcnt = 0;

    if (LDG_UNLIKELY(st->incnt + 4 > st->inlen)) { return LDG_ERR_INVALID; }

    len = (uint32_t)st->in[st->incnt];
    len |= (uint32_t)st->in[st->incnt + 1] << 8;
    nlen = (uint32_t)st->in[st->incnt + 2];
    nlen |= (uint32_t)st->in[st->incnt + 3] << 8;
    st->incnt += 4;

    if (LDG_UNLIKELY((len ^ 0xFFFF) != nlen)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(st->incnt + len > st->inlen)) { return LDG_ERR_INVALID; }

    while (len > 0)
    {
        if (LDG_UNLIKELY(st->outcnt >= st->outlen)) { return LDG_ERR_BOUNDS; }

        st->out[st->outcnt] = st->in[st->incnt];
        st->outcnt++;
        st->incnt++;
        len--;
    }

    return LDG_ERR_AOK;
}

static LDG_INFLATE_NOINLINE uint32_t inflate_fixed(inflate_state_t *st, uint16_t *len_cunt, uint16_t *len_sym, uint16_t *dist_cunt, uint16_t *dist_sym)
{
    uint8_t code_len[LDG_INFLATE_MAX_LIT] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0x0;
    uint32_t avail = 0x0;
    uint32_t ret = 0x0;

    for (idx = 0; idx < 144; idx++) { code_len[idx] = 8; }

    for (idx = 144; idx < 256; idx++) { code_len[idx] = 9; }

    for (idx = 256; idx < 280; idx++) { code_len[idx] = 7; }

    for (idx = 280; idx < 288; idx++) { code_len[idx] = 8; }

    ret = inflate_construct(len_cunt, len_sym, code_len, 288, &avail);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < 30; idx++) { code_len[idx] = 5; }

    ret = inflate_construct(dist_cunt, dist_sym, code_len, 30, &avail);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return inflate_codes(st, len_cunt, len_sym, dist_cunt, dist_sym);
}

static LDG_INFLATE_NOINLINE uint32_t inflate_dynamic(inflate_state_t *st, uint16_t *len_cunt, uint16_t *len_sym, uint16_t *dist_cunt, uint16_t *dist_sym)
{
    const uint16_t order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
    uint8_t code_len[LDG_INFLATE_MAX_LIT + LDG_INFLATE_MAX_DIST] = LDG_ARR_ZERO_INIT;
    uint16_t cl_cunt[LDG_INFLATE_MAX_BITS + 1] = LDG_ARR_ZERO_INIT;
    uint16_t cl_sym[LDG_INFLATE_MAX_CODE_LEN] = LDG_ARR_ZERO_INIT;
    int32_t sym = 0x0;
    uint32_t avail = 0x0;
    uint32_t nlen = 0x0;
    uint32_t ndist = 0x0;
    uint32_t ncode = 0x0;
    uint32_t idx = 0x0;
    uint32_t prev = 0x0;
    uint32_t repeat = 0x0;
    uint32_t val = 0x0;
    uint32_t ret = 0x0;

    ret = inflate_bits(st, 5, &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    nlen = val + 257;

    ret = inflate_bits(st, 5, &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ndist = val + 1;

    ret = inflate_bits(st, 4, &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ncode = val + 4;

    if (LDG_UNLIKELY(nlen > LDG_INFLATE_MAX_LIT || ndist > LDG_INFLATE_MAX_DIST)) { return LDG_ERR_INVALID; }

    for (idx = 0; idx < ncode; idx++)
    {
        ret = inflate_bits(st, 3, &val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        code_len[order[idx]] = (uint8_t)val;
    }

    for (idx = ncode; idx < 19; idx++) { code_len[order[idx]] = 0; }

    ret = inflate_construct(cl_cunt, cl_sym, code_len, 19, &avail);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(avail != 0)) { return LDG_ERR_INVALID; }

    idx = 0;
    while (idx < nlen + ndist)
    {
        ret = inflate_decode(st, cl_cunt, cl_sym, &sym);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (sym < 16)
        {
            code_len[idx] = (uint8_t)sym;
            idx++;
        }
        else if (sym == 16)
        {
            if (LDG_UNLIKELY(idx == 0)) { return LDG_ERR_INVALID; }

            prev = code_len[idx - 1];
            ret = inflate_bits(st, 2, &val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            repeat = 3 + val;

            if (LDG_UNLIKELY(idx + repeat > nlen + ndist)) { return LDG_ERR_INVALID; }

            while (repeat > 0)
            {
                code_len[idx] = (uint8_t)prev;
                idx++;
                repeat--;
            }
        }
        else if (sym == 17)
        {
            ret = inflate_bits(st, 3, &val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            repeat = 3 + val;

            if (LDG_UNLIKELY(idx + repeat > nlen + ndist)) { return LDG_ERR_INVALID; }

            while (repeat > 0)
            {
                code_len[idx] = 0;
                idx++;
                repeat--;
            }
        }
        else
        {
            ret = inflate_bits(st, 7, &val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            repeat = 11 + val;

            if (LDG_UNLIKELY(idx + repeat > nlen + ndist)) { return LDG_ERR_INVALID; }

            while (repeat > 0)
            {
                code_len[idx] = 0;
                idx++;
                repeat--;
            }
        }
    }

    if (LDG_UNLIKELY(code_len[256] == 0)) { return LDG_ERR_INVALID; }

    ret = inflate_construct(len_cunt, len_sym, code_len, nlen, &avail);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(avail != 0 && (uint32_t)(len_cunt[0] + len_cunt[1]) != nlen)) { return LDG_ERR_INVALID; }

    ret = inflate_construct(dist_cunt, dist_sym, &code_len[nlen], ndist, &avail);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(avail != 0 && (uint32_t)(dist_cunt[0] + dist_cunt[1]) != ndist)) { return LDG_ERR_INVALID; }

    return inflate_codes(st, len_cunt, len_sym, dist_cunt, dist_sym);
}

static uint32_t inflate_adler32(const uint8_t *data, uint64_t len)
{
    uint32_t sum1 = 1;
    uint32_t sum2 = 0;
    uint64_t idx = 0x0;
    uint64_t remain = 0x0;
    uint64_t block = 0x0;
    uint64_t inner = 0x0;

    remain = len;
    while (remain > 0)
    {
        block = remain;
        if (block > LDG_INFLATE_ADLER_NMAX) { block = LDG_INFLATE_ADLER_NMAX; }

        for (inner = 0; inner < block; inner++)
        {
            sum1 += (uint32_t)data[idx];
            sum2 += sum1;
            idx++;
        }

        sum1 %= LDG_INFLATE_ADLER_BASE;
        sum2 %= LDG_INFLATE_ADLER_BASE;
        remain -= block;
    }

    return (sum2 << 16) | sum1;
}

LDG_EXPORT uint32_t ldg_compress_inflate(const uint8_t *src, uint64_t src_len, uint8_t *dst, uint64_t dst_cap, uint64_t *out_len)
{
    inflate_state_t state = LDG_STRUCT_ZERO_INIT;
    uint16_t len_cunt[LDG_INFLATE_MAX_BITS + 1] = LDG_ARR_ZERO_INIT;
    uint16_t len_sym[LDG_INFLATE_MAX_LIT] = LDG_ARR_ZERO_INIT;
    uint16_t dist_cunt[LDG_INFLATE_MAX_BITS + 1] = LDG_ARR_ZERO_INIT;
    uint16_t dist_sym[LDG_INFLATE_MAX_DIST] = LDG_ARR_ZERO_INIT;
    uint32_t cmf = 0x0;
    uint32_t flg = 0x0;
    uint32_t bfinal = 0x0;
    uint32_t btype = 0x0;
    uint32_t computed = 0x0;
    uint32_t stored_adler = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!src || !dst || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(src_len < 6)) { return LDG_ERR_INVALID; }

    cmf = src[0];
    flg = src[1];
    if (LDG_UNLIKELY((cmf & 0x0F) != LDG_INFLATE_ZLIB_CM_DEFLATE)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY((cmf >> 4) > 7)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(((cmf << 8) | flg) % 31 != 0)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY((flg & 0x20) != 0)) { return LDG_ERR_INVALID; }

    state.in = src;
    state.inlen = src_len - 4;
    state.incnt = 2;
    state.out = dst;
    state.outlen = dst_cap;
    state.outcnt = 0;
    state.bitbuf = 0;
    state.bitcnt = 0;

    do
    {
        ret = inflate_bits(&state, 1, &bfinal);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = inflate_bits(&state, 2, &btype);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (btype == 0) { ret = inflate_stored(&state); }
        else if (btype == 1) { ret = inflate_fixed(&state, len_cunt, len_sym, dist_cunt, dist_sym); }
        else if (btype == 2) { ret = inflate_dynamic(&state, len_cunt, len_sym, dist_cunt, dist_sym); }
        else { return LDG_ERR_INVALID; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    } while (bfinal == 0);

    computed = inflate_adler32(dst, state.outcnt);
    stored_adler = ((uint32_t)src[src_len - 4] << 24) | ((uint32_t)src[src_len - 3] << 16) | ((uint32_t)src[src_len - 2] << 8) | (uint32_t)src[src_len - 1];
    if (LDG_UNLIKELY(computed != stored_adler)) { return LDG_ERR_INVALID; }

    *out_len = state.outcnt;
    return LDG_ERR_AOK;
}
