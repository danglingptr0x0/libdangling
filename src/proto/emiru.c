#include <string.h>
#include <dangling/proto/emiru.h>
#include <dangling/core/err.h>

LDG_EXPORT uint32_t ldg_emiru_hdr_validate(const ldg_byte_t *buff, ldg_dword_t buff_len)
{
    const ldg_byte_t *magic = (const ldg_byte_t *)LDG_EMIRU_MAGIC;
    const ldg_emiru_hdr_t *hdr = 0x0;
    ldg_dword_t payload = 0;
    ldg_dword_t required = 0;
    ldg_dword_t i = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_len < LDG_EMIRU_HDR_SIZE)) { return LDG_ERR_PROTO_EMIRU_TRUNCATED; }

    hdr = (const ldg_emiru_hdr_t *)buff;

    for (i = 0; i < LDG_EMIRU_MAGIC_LEN; i++) { if (LDG_UNLIKELY(hdr->magic[i] != magic[i])) { return LDG_ERR_PROTO_EMIRU_BAD_MAGIC; } }

    if (LDG_UNLIKELY(hdr->rev != LDG_EMIRU_REV)) { return LDG_ERR_PROTO_EMIRU_BAD_REV; }

    if (LDG_UNLIKELY(hdr->ring > LDG_EMIRU_RING_MAX)) { return LDG_ERR_PROTO_EMIRU_BAD_RING; }

    if (LDG_UNLIKELY(hdr->text_size == 0)) { return LDG_ERR_PROTO_EMIRU_BAD_ENTRY; }

    if (LDG_UNLIKELY(hdr->entry >= hdr->text_size)) { return LDG_ERR_PROTO_EMIRU_BAD_ENTRY; }

    payload = hdr->text_size + hdr->data_size;
    if (LDG_UNLIKELY(payload < hdr->text_size)) { return LDG_ERR_OVERFLOW; }

    required = LDG_EMIRU_HDR_SIZE + payload;
    if (LDG_UNLIKELY(required < payload)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(buff_len < required)) { return LDG_ERR_PROTO_EMIRU_TRUNCATED; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiru_decode(const ldg_byte_t *buff, ldg_dword_t buff_len, ldg_emiru_decoded_t *out)
{
    uint32_t err = 0;
    const ldg_emiru_hdr_t *hdr = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_emiru_hdr_validate(buff, buff_len);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = (const ldg_emiru_hdr_t *)buff;
    out->hdr = hdr;
    out->text = buff + LDG_EMIRU_HDR_SIZE;
    out->data = (hdr->data_size > 0) ? buff + LDG_EMIRU_HDR_SIZE + hdr->text_size : 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_emiru_encode(const ldg_emiru_hdr_t *hdr, const ldg_byte_t *text, ldg_dword_t text_len, const ldg_byte_t *data, ldg_dword_t data_len, ldg_byte_t *out, ldg_dword_t out_len)
{
    ldg_dword_t required = 0;
    ldg_dword_t offset = 0;

    if (LDG_UNLIKELY(!hdr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(text_len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(data_len > 0 && !data)) { return LDG_ERR_FUNC_ARG_NULL; }

    required = ldg_emiru_encoded_size_get(text_len, data_len);
    if (LDG_UNLIKELY(required == 0)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(out_len < required)) { return LDG_ERR_PROTO_EMIRU_BUFF_TOO_SMALL; }

    if (LDG_UNLIKELY(memcpy(out, hdr, LDG_EMIRU_HDR_SIZE) != out)) { return LDG_ERR_MEM_BAD; }

    offset = LDG_EMIRU_HDR_SIZE;

    if (LDG_UNLIKELY(memcpy(out + offset, text, text_len) != out + offset)) { return LDG_ERR_MEM_BAD; }

    offset += text_len;

    if (data_len > 0) { if (LDG_UNLIKELY(memcpy(out + offset, data, data_len) != out + offset)) { return LDG_ERR_MEM_BAD; } }

    return LDG_ERR_AOK;
}
