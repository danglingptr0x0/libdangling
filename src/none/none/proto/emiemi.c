#include <dangling/proto/emiemi.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>

#define LDG_EMIEMI_MARKER_FIRST_MATCH 1
#define LDG_EMIEMI_ARCHIVE_NAME_LEN_OFF 0
#define LDG_EMIEMI_ARCHIVE_CHECKSUM_OFF 2
#define LDG_EMIEMI_ARCHIVE_SHIFT_MID 2
#define LDG_EMIEMI_ARCHIVE_SHIFT_HI 3

static void emiemi_size_to_hex_convert(ldg_dword_t val, ldg_byte_t *out)
{
    ldg_dword_t idx = 0;
    ldg_dword_t nibble = 0;

    for (idx = 0; idx < LDG_EMIEMI_SIZE_FIELD_LEN; idx++)
    {
        nibble = (val >> (4 * (LDG_EMIEMI_SIZE_FIELD_LEN - 1 - idx))) & 0x0F;
        out[idx] = (ldg_byte_t)((nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10));
    }
}

static uint32_t emiemi_hex_to_size_convert(const ldg_byte_t *hex, ldg_dword_t *out)
{
    ldg_dword_t val = 0;
    ldg_dword_t idx = 0;
    ldg_byte_t ch = 0;

    for (idx = 0; idx < LDG_EMIEMI_SIZE_FIELD_LEN; idx++)
    {
        ch = hex[idx];
        val <<= 4;
        if (ch >= '0' && ch <= '9') { val |= (uint32_t)(ch - '0'); }
        else if (ch >= 'A' && ch <= 'F') { val |= (uint32_t)(ch - 'A' + 10); }
        else { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }
    }

    *out = val;
    return LDG_ERR_AOK;
}

LDG_EXPORT ldg_dword_t ldg_emiemi_fnv1a_compute(const ldg_byte_t *data, ldg_dword_t len)
{
    ldg_dword_t hash = LDG_EMIEMI_FNV1A_OFFSET;
    ldg_dword_t idx = 0;

    if (!data) { return 0; }

    for (idx = 0; idx < len; idx++)
    {
        hash ^= data[idx];
        hash *= LDG_EMIEMI_FNV1A_PRIME;
    }

    return hash;
}

LDG_EXPORT uint32_t ldg_emiemi_hdr_recv(ldg_emiemi_io_ctx_t *io, ldg_dword_t *payload_len)
{
    const ldg_byte_t *marker = (const ldg_byte_t *)LDG_EMIEMI_START_MARKER;
    ldg_byte_t hex[LDG_EMIEMI_SIZE_FIELD_LEN] = LDG_ARR_ZERO_INIT;
    ldg_dword_t idx = 0;
    ldg_dword_t iter = 0;
    ldg_dword_t consumed = 0;
    uint8_t ch = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->rd_byte)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < LDG_EMIEMI_START_MARKER_LEN)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (ret == LDG_ERR_EOF && consumed == 0) { return LDG_ERR_EOF; }

            return LDG_ERR_PROTO_EMIEMI_IO_RD;
        }

        consumed++;

        if (ch == marker[idx]) { idx++; }
        else if (ch == marker[0]) { idx = LDG_EMIEMI_MARKER_FIRST_MATCH; }
        else { idx = 0; }
    }

    for (iter = 0; iter < LDG_EMIEMI_SIZE_FIELD_LEN; iter++)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        hex[iter] = ch;
    }

    ret = emiemi_hex_to_size_convert(hex, payload_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(*payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    ret = io->rd_byte(io->ctx, &ch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

    if (LDG_UNLIKELY(ch != LDG_EMIEMI_DELIM)) { return LDG_ERR_PROTO_EMIEMI_BAD_DELIM; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_payload_recv(ldg_emiemi_io_ctx_t *io, ldg_byte_t *buff, ldg_dword_t buff_len, ldg_dword_t payload_len, ldg_dword_t *checksum)
{
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    ldg_dword_t hash = LDG_EMIEMI_FNV1A_OFFSET;
    ldg_dword_t idx = 0;
    uint8_t ch = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->rd_byte)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_len < payload_len)) { return LDG_ERR_PROTO_EMIEMI_BUFF_TOO_SMALL; }

    for (idx = 0; idx < payload_len; idx++)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        buff[idx] = ch;
        hash ^= ch;
        hash *= LDG_EMIEMI_FNV1A_PRIME;
    }

    for (idx = 0; idx < LDG_EMIEMI_END_MARKER_LEN; idx++)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        if (LDG_UNLIKELY(ch != end[idx])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; }
    }

    if (checksum) { *checksum = hash; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_recv(ldg_emiemi_io_ctx_t *io, ldg_byte_t *buff, ldg_dword_t buff_len, ldg_dword_t *payload_len, ldg_dword_t *checksum)
{
    ldg_dword_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_emiemi_hdr_recv(io, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(buff_len < len)) { return LDG_ERR_PROTO_EMIEMI_BUFF_TOO_SMALL; }

    ret = ldg_emiemi_payload_recv(io, buff, buff_len, len, checksum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *payload_len = len;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_send(ldg_emiemi_io_ctx_t *io, const ldg_byte_t *payload, ldg_dword_t payload_len, ldg_dword_t *checksum)
{
    ldg_byte_t hdr[LDG_EMIEMI_HDR_LEN] = LDG_ARR_ZERO_INIT;
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(hdr, LDG_EMIEMI_START_MARKER, LDG_EMIEMI_START_MARKER_LEN) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    emiemi_size_to_hex_convert(payload_len, hdr + LDG_EMIEMI_START_MARKER_LEN);
    hdr[LDG_EMIEMI_HDR_LEN - 1] = LDG_EMIEMI_DELIM;

    ret = io->wr(hdr, LDG_EMIEMI_HDR_LEN, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    ret = io->wr(payload, payload_len, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    ret = io->wr(end, LDG_EMIEMI_END_MARKER_LEN, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    if (checksum) { *checksum = ldg_emiemi_fnv1a_compute(payload, payload_len); }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_encode(const ldg_byte_t *payload, ldg_dword_t payload_len, ldg_byte_t *out, ldg_dword_t out_len, ldg_dword_t *checksum)
{
    ldg_dword_t required = 0;
    ldg_dword_t offset = 0;

    if (LDG_UNLIKELY(!payload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    required = ldg_emiemi_encoded_size_get(payload_len);
    if (LDG_UNLIKELY(required == 0)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(out_len < required)) { return LDG_ERR_PROTO_EMIEMI_BUFF_TOO_SMALL; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(out, LDG_EMIEMI_START_MARKER, LDG_EMIEMI_START_MARKER_LEN) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    offset = LDG_EMIEMI_START_MARKER_LEN;

    emiemi_size_to_hex_convert(payload_len, out + offset);
    offset += LDG_EMIEMI_SIZE_FIELD_LEN;

    out[offset] = LDG_EMIEMI_DELIM;
    offset++;

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(out + offset, payload, payload_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    offset += payload_len;

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(out + offset, LDG_EMIEMI_END_MARKER, LDG_EMIEMI_END_MARKER_LEN) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    if (checksum) { *checksum = ldg_emiemi_fnv1a_compute(payload, payload_len); }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_decode(const ldg_byte_t *frame, ldg_dword_t frame_len, const ldg_byte_t **payload, ldg_dword_t *payload_len, ldg_dword_t *checksum)
{
    const ldg_byte_t *start = (const ldg_byte_t *)LDG_EMIEMI_START_MARKER;
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    ldg_dword_t len = 0;
    ldg_dword_t expected_len = 0;
    ldg_dword_t end_offset = 0;
    ldg_dword_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(frame_len < LDG_EMIEMI_OVERHEAD)) { return LDG_ERR_PROTO_EMIEMI_TRUNC; }

    for (idx = 0; idx < LDG_EMIEMI_START_MARKER_LEN; idx++) { if (LDG_UNLIKELY(frame[idx] != start[idx])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; } }

    ret = emiemi_hex_to_size_convert(frame + LDG_EMIEMI_START_MARKER_LEN, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    if (LDG_UNLIKELY(frame[LDG_EMIEMI_HDR_LEN - 1] != LDG_EMIEMI_DELIM)) { return LDG_ERR_PROTO_EMIEMI_BAD_DELIM; }

    expected_len = LDG_EMIEMI_OVERHEAD + len;
    if (LDG_UNLIKELY(expected_len < len)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(frame_len < expected_len)) { return LDG_ERR_PROTO_EMIEMI_TRUNC; }

    end_offset = LDG_EMIEMI_HDR_LEN + len;
    for (idx = 0; idx < LDG_EMIEMI_END_MARKER_LEN; idx++) { if (LDG_UNLIKELY(frame[end_offset + idx] != end[idx])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; } }

    *payload = frame + LDG_EMIEMI_HDR_LEN;
    *payload_len = len;

    if (checksum) { *checksum = ldg_emiemi_fnv1a_compute(frame + LDG_EMIEMI_HDR_LEN, len); }

    return LDG_ERR_AOK;
}

static void emiemi_archive_word_put(ldg_byte_t *out, ldg_dword_t val)
{
    out[0] = (ldg_byte_t)(val & LDG_BYTE_MASK);
    out[1] = (ldg_byte_t)((val >> LDG_BYTE_BITS) & LDG_BYTE_MASK);
}

static ldg_dword_t emiemi_archive_word_get(const ldg_byte_t *in)
{
    return (ldg_dword_t)in[0] | ((ldg_dword_t)in[1] << LDG_BYTE_BITS);
}

static void emiemi_archive_dword_put(ldg_byte_t *out, ldg_dword_t val)
{
    out[0] = (ldg_byte_t)(val & LDG_BYTE_MASK);
    out[1] = (ldg_byte_t)((val >> LDG_BYTE_BITS) & LDG_BYTE_MASK);
    out[2] = (ldg_byte_t)((val >> (LDG_BYTE_BITS * LDG_EMIEMI_ARCHIVE_SHIFT_MID)) & LDG_BYTE_MASK);
    out[3] = (ldg_byte_t)((val >> (LDG_BYTE_BITS * LDG_EMIEMI_ARCHIVE_SHIFT_HI)) & LDG_BYTE_MASK);
}

static ldg_dword_t emiemi_archive_dword_get(const ldg_byte_t *in)
{
    return (ldg_dword_t)in[0] | ((ldg_dword_t)in[1] << LDG_BYTE_BITS) | ((ldg_dword_t)in[2] << (LDG_BYTE_BITS * LDG_EMIEMI_ARCHIVE_SHIFT_MID)) | ((ldg_dword_t)in[3] << (LDG_BYTE_BITS * LDG_EMIEMI_ARCHIVE_SHIFT_HI));
}

static uint32_t emiemi_bytes_rd(ldg_emiemi_io_ctx_t *io, ldg_byte_t *dst, ldg_dword_t len)
{
    ldg_dword_t idx = 0;
    uint8_t ch = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < len; idx++)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        dst[idx] = ch;
    }

    return LDG_ERR_AOK;
}

static uint32_t emiemi_bytes_discard(ldg_emiemi_io_ctx_t *io, ldg_dword_t len)
{
    ldg_dword_t idx = 0;
    uint8_t ch = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < len; idx++)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_archive_member_add(ldg_emiemi_io_ctx_t *io, const char *name, const ldg_byte_t *data, ldg_dword_t len)
{
    ldg_byte_t frame_hdr[LDG_EMIEMI_HDR_LEN] = LDG_ARR_ZERO_INIT;
    ldg_byte_t arch_hdr[LDG_EMIEMI_ARCHIVE_HDR_LEN] = LDG_ARR_ZERO_INIT;
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    ldg_dword_t name_len = 0;
    ldg_dword_t payload_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!data && len > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (name[name_len] != LDG_STR_TERM && name_len <= LDG_EMIEMI_ARCHIVE_NAME_MAX) { name_len++; }

    if (LDG_UNLIKELY(name_len == 0 || name_len > LDG_EMIEMI_ARCHIVE_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(len > LDG_EMIEMI_MAX_PAYLOAD - LDG_EMIEMI_ARCHIVE_HDR_LEN - name_len)) { return LDG_ERR_OVERFLOW; }

    payload_len = LDG_EMIEMI_ARCHIVE_HDR_LEN + name_len + len;

    emiemi_archive_word_put(arch_hdr + LDG_EMIEMI_ARCHIVE_NAME_LEN_OFF, name_len);
    emiemi_archive_dword_put(arch_hdr + LDG_EMIEMI_ARCHIVE_CHECKSUM_OFF, ldg_emiemi_fnv1a_compute(data, len));

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(frame_hdr, LDG_EMIEMI_START_MARKER, LDG_EMIEMI_START_MARKER_LEN) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    emiemi_size_to_hex_convert(payload_len, frame_hdr + LDG_EMIEMI_START_MARKER_LEN);
    frame_hdr[LDG_EMIEMI_DELIM_OFF] = LDG_EMIEMI_DELIM;

    ret = io->wr(frame_hdr, LDG_EMIEMI_HDR_LEN, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    ret = io->wr(arch_hdr, LDG_EMIEMI_ARCHIVE_HDR_LEN, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    ret = io->wr((const ldg_byte_t *)name, name_len, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    if (len > 0)
    {
        ret = io->wr(data, len, io->ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }
    }

    ret = io->wr(end, LDG_EMIEMI_END_MARKER_LEN, io->ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_archive_member_get(ldg_emiemi_io_ctx_t *io, ldg_emiemi_archive_member_t *out, ldg_byte_t *buff, ldg_dword_t buff_cap)
{
    ldg_byte_t arch_hdr[LDG_EMIEMI_ARCHIVE_HDR_LEN] = LDG_ARR_ZERO_INIT;
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    ldg_dword_t payload_len = 0;
    ldg_dword_t name_len = 0;
    ldg_dword_t data_len = 0;
    ldg_dword_t checksum_rd = 0;
    ldg_dword_t idx = 0;
    uint8_t ch = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!io || !out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->rd_byte)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_emiemi_hdr_recv(io, &payload_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(payload_len < LDG_EMIEMI_ARCHIVE_HDR_LEN)) { return LDG_ERR_PROTO_EMIEMI_TRUNC; }

    ret = emiemi_bytes_rd(io, arch_hdr, LDG_EMIEMI_ARCHIVE_HDR_LEN);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    name_len = emiemi_archive_word_get(arch_hdr + LDG_EMIEMI_ARCHIVE_NAME_LEN_OFF);
    checksum_rd = emiemi_archive_dword_get(arch_hdr + LDG_EMIEMI_ARCHIVE_CHECKSUM_OFF);

    if (LDG_UNLIKELY(name_len == 0 || name_len > LDG_EMIEMI_ARCHIVE_NAME_MAX)) { return LDG_ERR_PROTO_EMIEMI_TRUNC; }

    if (LDG_UNLIKELY(payload_len < LDG_EMIEMI_ARCHIVE_HDR_LEN + name_len)) { return LDG_ERR_PROTO_EMIEMI_TRUNC; }

    data_len = payload_len - LDG_EMIEMI_ARCHIVE_HDR_LEN - name_len;

    if (LDG_UNLIKELY(buff_cap < data_len))
    {
        ret = emiemi_bytes_discard(io, name_len + data_len + LDG_EMIEMI_END_MARKER_LEN);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return LDG_ERR_PROTO_EMIEMI_BUFF_TOO_SMALL;
    }

    ret = emiemi_bytes_rd(io, (ldg_byte_t *)out->name, name_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->name[name_len] = LDG_STR_TERM;

    if (data_len > 0)
    {
        ret = emiemi_bytes_rd(io, buff, data_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < LDG_EMIEMI_END_MARKER_LEN; idx++)
    {
        ret = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        if (LDG_UNLIKELY(ch != end[idx])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; }
    }

    if (LDG_UNLIKELY(ldg_emiemi_fnv1a_compute(buff, data_len) != checksum_rd)) { return LDG_ERR_PROTO_EMIEMI_BAD_CKSUM; }

    out->data_len = data_len;
    out->checksum = checksum_rd;

    return LDG_ERR_AOK;
}
