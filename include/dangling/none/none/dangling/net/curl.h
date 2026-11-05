#ifndef LDG_NET_CURL_H
#define LDG_NET_CURL_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_CURL_RESP_INITIAL_CAP 4096
#define LDG_NET_CURL_RESP_RESIZE_FACTOR 2
#define LDG_NET_CURL_RESP_MAX_CAP (256 * LDG_MIB)

typedef struct ldg_net_curl_resp
{
    char *data;
    uint64_t size;
    uint64_t cap;
    uint32_t status;
    uint8_t pudding[4];
} ldg_net_curl_resp_t;

typedef struct ldg_net_curl_multi_req
{
    void *curl;
    char *url;
    char *post_data;
    void *hdrs;
    ldg_net_curl_resp_t resp;
    void *user_data;
    int64_t dl_total;
    int64_t dl_now;
    int64_t ul_total;
    int64_t ul_now;
    uint64_t last_progress_ms;
    uint32_t idx;
    uint8_t pudding[12];
} LDG_ALIGNED ldg_net_curl_multi_req_t;

typedef struct ldg_net_curl_multi_ctx
{
    void *multi;
    ldg_net_curl_multi_req_t *reqs;
    uint64_t req_cunt;
    uint64_t req_cap;
    uint8_t pudding[32];
} LDG_ALIGNED ldg_net_curl_multi_ctx_t;

typedef struct ldg_net_curl_easy_ctx
{
    void *curl;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_net_curl_easy_ctx_t;

typedef uint64_t (*ldg_net_curl_stream_cb_t)(void *chunk, uint64_t size, void *user_data);

typedef struct ldg_net_curl_emiemi_res
{
    const uint8_t *payload;
    uint32_t payload_len;
    uint32_t req_checksum;
    uint32_t resp_checksum;
    uint8_t pudding[4];
} ldg_net_curl_emiemi_res_t;

LDG_EXPORT void ldg_net_curl_resp_init(ldg_net_curl_resp_t *resp);
LDG_EXPORT void ldg_net_curl_resp_free(ldg_net_curl_resp_t *resp);

LDG_EXPORT uint32_t ldg_net_curl_multi_ctx_create(ldg_net_curl_multi_ctx_t *ctx, uint64_t cap);
LDG_EXPORT void ldg_net_curl_multi_ctx_destroy(ldg_net_curl_multi_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_net_curl_multi_req_add(ldg_net_curl_multi_ctx_t *ctx, const char *url, const char *post_data, void *hdrs);
LDG_EXPORT uint32_t ldg_net_curl_multi_get(ldg_net_curl_multi_ctx_t *ctx, const char *url, void *hdrs);
LDG_EXPORT uint32_t ldg_net_curl_multi_post(ldg_net_curl_multi_ctx_t *ctx, const char *url, const char *data, void *hdrs);
LDG_EXPORT uint32_t ldg_net_curl_multi_perform(ldg_net_curl_multi_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_curl_multi_progress_get(ldg_net_curl_multi_ctx_t *ctx, double *out);

LDG_EXPORT uint32_t ldg_net_curl_hdr_append(void **list, const char *hdr);
LDG_EXPORT uint32_t ldg_net_curl_hdr_cpy(void *src, void **dst);
LDG_EXPORT void ldg_net_curl_hdr_destroy(void **list);

LDG_EXPORT uint32_t ldg_net_curl_easy_ctx_create(ldg_net_curl_easy_ctx_t *ctx);
LDG_EXPORT void ldg_net_curl_easy_ctx_destroy(ldg_net_curl_easy_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_curl_easy_get(ldg_net_curl_easy_ctx_t *ctx, const char *url, void *hdrs, ldg_net_curl_resp_t *resp);
LDG_EXPORT uint32_t ldg_net_curl_easy_post(ldg_net_curl_easy_ctx_t *ctx, const char *url, const char *data, void *hdrs, ldg_net_curl_resp_t *resp);
LDG_EXPORT uint32_t ldg_net_curl_easy_raw_post(ldg_net_curl_easy_ctx_t *ctx, const char *url, const void *data, uint64_t len, void *hdrs, ldg_net_curl_resp_t *resp);
LDG_EXPORT uint32_t ldg_net_curl_emiemi_post(ldg_net_curl_easy_ctx_t *ctx, const char *url, const uint8_t *payload, uint32_t payload_len, void *hdrs, ldg_net_curl_resp_t *resp, ldg_net_curl_emiemi_res_t *out);
LDG_EXPORT uint32_t ldg_net_curl_easy_get_stream(ldg_net_curl_easy_ctx_t *ctx, const char *url, void *hdrs, ldg_net_curl_stream_cb_t cb, void *user_data);
LDG_EXPORT uint32_t ldg_net_curl_easy_post_stream(ldg_net_curl_easy_ctx_t *ctx, const char *url, const char *data, void *hdrs, ldg_net_curl_stream_cb_t cb, void *user_data);

#endif
