#ifndef LDG_PROTO_EMIEMI_H
#define LDG_PROTO_EMIEMI_H

#include <stdint.h>
#include <dangling/core/types.h>
#include <dangling/core/macros.h>

#define LDG_EMIEMI_START_MARKER "<<EMIEMI>"
#define LDG_EMIEMI_END_MARKER "<<EMIEMI>>"
#define LDG_EMIEMI_START_MARKER_LEN 9
#define LDG_EMIEMI_END_MARKER_LEN 10
#define LDG_EMIEMI_SIZE_FIELD_LEN 6
#define LDG_EMIEMI_DELIM '>'
#define LDG_EMIEMI_HDR_LEN 16
#define LDG_EMIEMI_OVERHEAD 26
#define LDG_EMIEMI_MAX_PAYLOAD 0xFFFFFF

#define LDG_EMIEMI_FNV1A_OFFSET 2166136261U
#define LDG_EMIEMI_FNV1A_PRIME 16777619U

typedef int32_t (*ldg_emiemi_rd_byte_cb_t)(void *ctx);
typedef uint32_t (*ldg_emiemi_wr_cb_t)(const ldg_byte_t *data, ldg_dword_t len, void *ctx);

typedef struct ldg_emiemi_io_ctx
{
    ldg_emiemi_rd_byte_cb_t rd_byte;
    ldg_emiemi_wr_cb_t wr;
    void *ctx;
} ldg_emiemi_io_ctx_t;

static inline ldg_dword_t ldg_emiemi_encoded_size_get(ldg_dword_t payload_len)
{
    ldg_dword_t total = 0;

    total = LDG_EMIEMI_OVERHEAD + payload_len;
    if (LDG_UNLIKELY(total < payload_len)) { return 0; }

    return total;
}

// callback streaming
LDG_EXPORT uint32_t ldg_emiemi_hdr_recv(ldg_emiemi_io_ctx_t *io, ldg_dword_t *payload_len);
LDG_EXPORT uint32_t ldg_emiemi_payload_recv(ldg_emiemi_io_ctx_t *io, ldg_byte_t *buff, ldg_dword_t payload_len, ldg_dword_t *checksum);
LDG_EXPORT uint32_t ldg_emiemi_recv(ldg_emiemi_io_ctx_t *io, ldg_byte_t *buff, ldg_dword_t buff_len, ldg_dword_t *payload_len, ldg_dword_t *checksum);
LDG_EXPORT uint32_t ldg_emiemi_send(ldg_emiemi_io_ctx_t *io, const ldg_byte_t *payload, ldg_dword_t payload_len, ldg_dword_t *checksum);

// buff-oriented
LDG_EXPORT uint32_t ldg_emiemi_encode(const ldg_byte_t *payload, ldg_dword_t payload_len, ldg_byte_t *out, ldg_dword_t out_len, ldg_dword_t *checksum);
LDG_EXPORT uint32_t ldg_emiemi_decode(const ldg_byte_t *frame, ldg_dword_t frame_len, const ldg_byte_t **payload, ldg_dword_t *payload_len, ldg_dword_t *checksum);

// utility
LDG_EXPORT ldg_dword_t ldg_emiemi_fnv1a(const ldg_byte_t *data, ldg_dword_t len);

#endif
