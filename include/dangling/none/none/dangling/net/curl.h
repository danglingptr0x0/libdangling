#ifndef LDG_NET_CURL_H
#define LDG_NET_CURL_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_CURL_RESP_INITIAL_CAP 4096
#define LDG_CURL_RESP_RESIZE_FACTOR 2
#define LDG_CURL_RESP_MAX_CAP (256 * LDG_MIB)

typedef struct ldg_curl_resp
{
    char *data;
    uint64_t size;
    uint64_t cap;
} ldg_curl_resp_t;

typedef struct ldg_curl_multi_req
{
    void *curl;
    char *url;
    char *post_data;
    void *headers;
    ldg_curl_resp_t response;
    void *user_data;
    uint32_t idx;
    int64_t dl_total;
    int64_t dl_now;
    int64_t ul_total;
    int64_t ul_now;
    uint64_t last_progress_ms;
    uint8_t pudding[4];
} LDG_ALIGNED ldg_curl_multi_req_t;

typedef struct ldg_curl_multi_ctx
{
    void *multi;
    ldg_curl_multi_req_t *reqs;
    uint64_t req_cunt;
    uint64_t req_cap;
    uint8_t pudding[40];
} LDG_ALIGNED ldg_curl_multi_ctx_t;

typedef struct ldg_curl_easy_ctx
{
    void *curl;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_curl_easy_ctx_t;

typedef uint64_t (*ldg_curl_stream_cb_t)(void *chunk, uint64_t size, void *user_data);

LDG_EXPORT void ldg_curl_resp_init(ldg_curl_resp_t *resp);
LDG_EXPORT void ldg_curl_resp_free(ldg_curl_resp_t *resp);

LDG_EXPORT uint32_t ldg_curl_multi_ctx_create(ldg_curl_multi_ctx_t *ctx, uint64_t capacity);
LDG_EXPORT void ldg_curl_multi_ctx_destroy(ldg_curl_multi_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_curl_multi_req_add(ldg_curl_multi_ctx_t *ctx, const char *url, const char *post_data, void *headers);
LDG_EXPORT uint32_t ldg_curl_multi_get(ldg_curl_multi_ctx_t *ctx, const char *url, void *headers);
LDG_EXPORT uint32_t ldg_curl_multi_post(ldg_curl_multi_ctx_t *ctx, const char *url, const char *data, void *headers);
LDG_EXPORT uint32_t ldg_curl_multi_perform(ldg_curl_multi_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_curl_multi_progress_get(ldg_curl_multi_ctx_t *ctx, double *out);

LDG_EXPORT uint32_t ldg_curl_headers_append(void **list, const char *header);
LDG_EXPORT uint32_t ldg_curl_headers_copy(void *src, void **dst);
LDG_EXPORT void ldg_curl_headers_destroy(void **list);

LDG_EXPORT uint32_t ldg_curl_easy_ctx_create(ldg_curl_easy_ctx_t *ctx);
LDG_EXPORT void ldg_curl_easy_ctx_destroy(ldg_curl_easy_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_curl_easy_get(ldg_curl_easy_ctx_t *ctx, const char *url, void *headers, ldg_curl_resp_t *resp);
LDG_EXPORT uint32_t ldg_curl_easy_post(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, void *headers, ldg_curl_resp_t *resp);
LDG_EXPORT uint32_t ldg_curl_easy_get_stream(ldg_curl_easy_ctx_t *ctx, const char *url, void *headers, ldg_curl_stream_cb_t cb, void *user_data);
LDG_EXPORT uint32_t ldg_curl_easy_post_stream(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, void *headers, ldg_curl_stream_cb_t cb, void *user_data);

#endif
