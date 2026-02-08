#ifndef LDG_PROTO_EMIRU_H
#define LDG_PROTO_EMIRU_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/types.h>
#include <dangling/core/macros.h>

#define LDG_EMIRU_MAGIC "EMIRU"
#define LDG_EMIRU_MAGIC_LEN 6
#define LDG_EMIRU_HDR_SIZE 32
#define LDG_EMIRU_REV 1
#define LDG_EMIRU_FLAG_PIC (1 << 0)
#define LDG_EMIRU_RING_MAX 3

typedef struct ldg_emiru_hdr
{
    ldg_byte_t magic[6];
    ldg_byte_t rev;
    ldg_byte_t ring;
    ldg_byte_t flags;
    ldg_byte_t reserved0;
    ldg_word_t prog_rev;
    ldg_dword_t entry;
    ldg_dword_t text_size;
    ldg_dword_t data_size;
    ldg_dword_t bss_size;
    ldg_dword_t reserved1;
} ldg_emiru_hdr_t;

typedef struct ldg_emiru_decoded
{
    const ldg_emiru_hdr_t *hdr;
    const ldg_byte_t *text;
    const ldg_byte_t *data;
} ldg_emiru_decoded_t;

static inline ldg_dword_t ldg_emiru_encoded_size_get(ldg_dword_t text_len, ldg_dword_t data_len)
{
    ldg_dword_t payload = 0;
    ldg_dword_t total = 0;

    payload = text_len + data_len;
    if (LDG_UNLIKELY(payload < text_len)) { return 0; }

    total = LDG_EMIRU_HDR_SIZE + payload;
    if (LDG_UNLIKELY(total < payload)) { return 0; }

    return total;
}

LDG_EXPORT uint32_t ldg_emiru_hdr_validate(const ldg_byte_t *buff, ldg_dword_t buff_len);
LDG_EXPORT uint32_t ldg_emiru_decode(const ldg_byte_t *buff, ldg_dword_t buff_len, ldg_emiru_decoded_t *out);
LDG_EXPORT uint32_t ldg_emiru_encode(const ldg_emiru_hdr_t *hdr, const ldg_byte_t *text, ldg_dword_t text_len, const ldg_byte_t *data, ldg_dword_t data_len, ldg_byte_t *out, ldg_dword_t out_len);

#endif
