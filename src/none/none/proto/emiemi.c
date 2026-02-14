#include <string.h>
#include <dangling/proto/emiemi.h>
#include <dangling/core/err.h>

// hex

static void emiemi_size_to_hex(ldg_dword_t val, ldg_byte_t *out)
{
    ldg_dword_t i = 0;
    ldg_dword_t nibble = 0;

    for (i = 0; i < LDG_EMIEMI_SIZE_FIELD_LEN; i++)
    {
        nibble = (val >> (4 * (LDG_EMIEMI_SIZE_FIELD_LEN - 1 - i))) & 0x0F;
        out[i] = (ldg_byte_t)((nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10));
    }
}

static uint32_t emiemi_hex_to_size(const ldg_byte_t *hex, ldg_dword_t *out)
{
    ldg_dword_t val = 0;
    ldg_dword_t i = 0;
    ldg_byte_t ch = 0;

    for (i = 0; i < LDG_EMIEMI_SIZE_FIELD_LEN; i++)
    {
        ch = hex[i];
        val <<= 4;
        if (ch >= '0' && ch <= '9') { val |= (uint32_t)(ch - '0'); }
        else if (ch >= 'A' && ch <= 'F') { val |= (uint32_t)(ch - 'A' + 10); }
        else { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }
    }

    *out = val;
    return LDG_ERR_AOK;
}

// fnv1a

LDG_EXPORT ldg_dword_t ldg_emiemi_fnv1a(const ldg_byte_t *data, ldg_dword_t len)
{
    ldg_dword_t hash = LDG_EMIEMI_FNV1A_OFFSET;
    ldg_dword_t i = 0;

    if (!data) { return 0; }

    for (i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= LDG_EMIEMI_FNV1A_PRIME;
    }

    return hash;
}

// callback streaming

LDG_EXPORT uint32_t ldg_emiemi_hdr_recv(ldg_emiemi_io_ctx_t *io, ldg_dword_t *payload_len)
{
    const ldg_byte_t *marker = (const ldg_byte_t *)LDG_EMIEMI_START_MARKER;
    ldg_byte_t hex[LDG_EMIEMI_SIZE_FIELD_LEN] = LDG_ARR_ZERO_INIT;
    ldg_dword_t idx = 0;
    ldg_dword_t i = 0;
    uint8_t ch = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->rd_byte)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < LDG_EMIEMI_START_MARKER_LEN)
    {
        err = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        if (ch == marker[idx]) { idx++; }
        else if (ch == marker[0]) { idx = 1; }
        else { idx = 0; }
    }

    for (i = 0; i < LDG_EMIEMI_SIZE_FIELD_LEN; i++)
    {
        err = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        hex[i] = ch;
    }

    err = emiemi_hex_to_size(hex, payload_len);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(*payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    err = io->rd_byte(io->ctx, &ch);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

    if (LDG_UNLIKELY(ch != LDG_EMIEMI_DELIM)) { return LDG_ERR_PROTO_EMIEMI_BAD_DELIM; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_payload_recv(ldg_emiemi_io_ctx_t *io, ldg_byte_t *buff, ldg_dword_t payload_len, ldg_dword_t *checksum)
{
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    ldg_dword_t hash = LDG_EMIEMI_FNV1A_OFFSET;
    ldg_dword_t i = 0;
    uint8_t ch = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->rd_byte)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (i = 0; i < payload_len; i++)
    {
        err = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        buff[i] = ch;
        hash ^= ch;
        hash *= LDG_EMIEMI_FNV1A_PRIME;
    }

    for (i = 0; i < LDG_EMIEMI_END_MARKER_LEN; i++)
    {
        err = io->rd_byte(io->ctx, &ch);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_RD; }

        if (LDG_UNLIKELY(ch != end[i])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; }
    }

    if (checksum) { *checksum = hash; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_recv(ldg_emiemi_io_ctx_t *io, ldg_byte_t *buff, ldg_dword_t buff_len, ldg_dword_t *payload_len, ldg_dword_t *checksum)
{
    ldg_dword_t len = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_emiemi_hdr_recv(io, &len);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(buff_len < len)) { return LDG_ERR_PROTO_EMIEMI_BUFF_TOO_SMALL; }

    err = ldg_emiemi_payload_recv(io, buff, len, checksum);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *payload_len = len;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_send(ldg_emiemi_io_ctx_t *io, const ldg_byte_t *payload, ldg_dword_t payload_len, ldg_dword_t *checksum)
{
    ldg_byte_t hdr[LDG_EMIEMI_HDR_LEN] = LDG_ARR_ZERO_INIT;
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!io)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!io->wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    memcpy(hdr, LDG_EMIEMI_START_MARKER, LDG_EMIEMI_START_MARKER_LEN);
    emiemi_size_to_hex(payload_len, hdr + LDG_EMIEMI_START_MARKER_LEN);
    hdr[LDG_EMIEMI_HDR_LEN - 1] = LDG_EMIEMI_DELIM;

    err = io->wr(hdr, LDG_EMIEMI_HDR_LEN, io->ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    err = io->wr(payload, payload_len, io->ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    err = io->wr(end, LDG_EMIEMI_END_MARKER_LEN, io->ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_PROTO_EMIEMI_IO_WR; }

    if (checksum) { *checksum = ldg_emiemi_fnv1a(payload, payload_len); }

    return LDG_ERR_AOK;
}

// buff-oriented

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

    memcpy(out, LDG_EMIEMI_START_MARKER, LDG_EMIEMI_START_MARKER_LEN);
    offset = LDG_EMIEMI_START_MARKER_LEN;

    emiemi_size_to_hex(payload_len, out + offset);
    offset += LDG_EMIEMI_SIZE_FIELD_LEN;

    out[offset] = LDG_EMIEMI_DELIM;
    offset++;

    memcpy(out + offset, payload, payload_len);
    offset += payload_len;

    memcpy(out + offset, LDG_EMIEMI_END_MARKER, LDG_EMIEMI_END_MARKER_LEN);

    if (checksum) { *checksum = ldg_emiemi_fnv1a(payload, payload_len); }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiemi_decode(const ldg_byte_t *frame, ldg_dword_t frame_len, const ldg_byte_t **payload, ldg_dword_t *payload_len, ldg_dword_t *checksum)
{
    const ldg_byte_t *start = (const ldg_byte_t *)LDG_EMIEMI_START_MARKER;
    const ldg_byte_t *end = (const ldg_byte_t *)LDG_EMIEMI_END_MARKER;
    ldg_dword_t len = 0;
    ldg_dword_t expected_len = 0;
    ldg_dword_t end_offset = 0;
    ldg_dword_t i = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(frame_len < LDG_EMIEMI_OVERHEAD)) { return LDG_ERR_PROTO_EMIEMI_TRUNCATED; }

    for (i = 0; i < LDG_EMIEMI_START_MARKER_LEN; i++) { if (LDG_UNLIKELY(frame[i] != start[i])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; } }

    err = emiemi_hex_to_size(frame + LDG_EMIEMI_START_MARKER_LEN, &len);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_PROTO_EMIEMI_BAD_SIZE; }

    if (LDG_UNLIKELY(frame[LDG_EMIEMI_HDR_LEN - 1] != LDG_EMIEMI_DELIM)) { return LDG_ERR_PROTO_EMIEMI_BAD_DELIM; }

    expected_len = LDG_EMIEMI_OVERHEAD + len;
    if (LDG_UNLIKELY(expected_len < len)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(frame_len < expected_len)) { return LDG_ERR_PROTO_EMIEMI_TRUNCATED; }

    end_offset = LDG_EMIEMI_HDR_LEN + len;
    for (i = 0; i < LDG_EMIEMI_END_MARKER_LEN; i++) { if (LDG_UNLIKELY(frame[end_offset + i] != end[i])) { return LDG_ERR_PROTO_EMIEMI_BAD_MARKER; } }

    *payload = frame + LDG_EMIEMI_HDR_LEN;
    *payload_len = len;

    if (checksum) { *checksum = ldg_emiemi_fnv1a(frame + LDG_EMIEMI_HDR_LEN, len); }

    return LDG_ERR_AOK;
}
