#ifndef LDG_NET_CURL_H
#define LDG_NET_CURL_H

#include <stdint.h>
#include <stddef.h>
#include <curl/curl.h>
#include <dangling/core/macros.h>

#define LDG_CURL_RESP_INITIAL_CAP 4096
#define LDG_CURL_RESP_RESIZE_FACTOR 2

typedef struct ldg_curl_resp
{
    char *data;
    size_t size;
    size_t cap;
} ldg_curl_resp_t;

typedef struct ldg_curl_multi_req
{
    void *curl;
    char *url;
    char *post_data;
    struct curl_slist *headers;
    ldg_curl_resp_t response;
    void *user_data;
    int idx;
    curl_off_t dl_total;
    curl_off_t dl_now;
    curl_off_t ul_total;
    curl_off_t ul_now;
    uint64_t last_progress_ms;
    uint8_t pudding[4];
} LDG_ALIGNED ldg_curl_multi_req_t;

typedef struct ldg_curl_multi_ctx
{
    CURLM *multi;
    ldg_curl_multi_req_t *reqs;
    size_t req_cunt;
    size_t req_cap;
    uint8_t pudding[40];
} LDG_ALIGNED ldg_curl_multi_ctx_t;

LDG_EXPORT size_t ldg_curl_resp_write_cb(void *contents, size_t size, size_t nmemb, void *user_data);
LDG_EXPORT void ldg_curl_resp_init(ldg_curl_resp_t *resp);
LDG_EXPORT void ldg_curl_resp_free(ldg_curl_resp_t *resp);

LDG_EXPORT int32_t ldg_curl_multi_ctx_create(ldg_curl_multi_ctx_t *ctx, size_t capacity);
LDG_EXPORT void ldg_curl_multi_ctx_destroy(ldg_curl_multi_ctx_t *ctx);

LDG_EXPORT int32_t ldg_curl_multi_req_add(ldg_curl_multi_ctx_t *ctx, const char *url, const char *post_data, struct curl_slist *headers);
LDG_EXPORT int32_t ldg_curl_multi_perform(ldg_curl_multi_ctx_t *ctx);
LDG_EXPORT double ldg_curl_multi_progress_get(ldg_curl_multi_ctx_t *ctx);

#endif
