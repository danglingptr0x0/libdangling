#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include <dangling/net/curl.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/core/macros.h>

static size_t ldg_curl_resp_wr_cb(void *contents, size_t size, size_t nmemb, void *user_data)
{
    uint64_t size_real = 0;
    ldg_curl_resp_t *resp = 0x0;
    uint64_t new_len = 0;
    uint64_t new_cap = 0;
    void *new_data = 0x0;

    if (LDG_UNLIKELY(!contents || !user_data)) { return 0; }

    size_real = (uint64_t)size * (uint64_t)nmemb;
    if (LDG_UNLIKELY(nmemb != 0 && size_real / (uint64_t)nmemb != (uint64_t)size)) { return 0; }

    resp = (ldg_curl_resp_t *)user_data;

    new_len = resp->size + size_real;
    if (LDG_UNLIKELY(new_len < resp->size || new_len > LDG_CURL_RESP_MAX_CAP)) { return 0; }

    if (!resp->data || new_len + LDG_STR_TERM_SIZE > resp->cap)
    {
        new_cap = resp->cap ? resp->cap * LDG_CURL_RESP_RESIZE_FACTOR : LDG_CURL_RESP_INITIAL_CAP;
        while (new_cap < new_len + LDG_STR_TERM_SIZE) { new_cap *= LDG_CURL_RESP_RESIZE_FACTOR; }

        if (LDG_UNLIKELY(posix_memalign(&new_data, LDG_AMD64_CACHE_LINE_WIDTH, new_cap) != 0)) { return 0; }

        if (LDG_UNLIKELY(memset(new_data, 0, new_cap) != new_data)) { free(new_data); return 0; }

        if (resp->data)
        {
            if (LDG_UNLIKELY(ldg_mem_secure_copy(new_data, resp->data, resp->size) != LDG_ERR_AOK)) { free(new_data); return 0; }

            free(resp->data);
        }

        resp->data = new_data;
        resp->cap = new_cap;
    }

    if (LDG_UNLIKELY(ldg_mem_secure_copy(resp->data + resp->size, contents, size_real) != LDG_ERR_AOK)) { return 0; }

    resp->size = new_len;
    resp->data[resp->size] = LDG_STR_TERM;

    return (size_t)size_real;
}

void ldg_curl_resp_init(ldg_curl_resp_t *resp)
{
    if (LDG_UNLIKELY(!resp)) { return; }

    if (LDG_UNLIKELY(memset(resp, 0, sizeof(ldg_curl_resp_t)) != resp)) { return; }
}

void ldg_curl_resp_free(ldg_curl_resp_t *resp)
{
    if (LDG_UNLIKELY(!resp)) { return; }

    if (resp->data)
    {
        free(resp->data);
        resp->data = 0x0;
    }

    resp->size = 0;
    resp->cap = 0;
}

typedef struct ldg_curl_stream_ctx
{
    ldg_curl_stream_cb_t cb;
    void *user_data;
} ldg_curl_stream_ctx_t;

static size_t ldg_curl_stream_wr_cb(void *contents, size_t size, size_t nmemb, void *user_data)
{
    ldg_curl_stream_ctx_t *ctx = 0x0;
    uint64_t size_real = 0;

    if (LDG_UNLIKELY(!contents || !user_data)) { return 0; }

    ctx = (ldg_curl_stream_ctx_t *)user_data;
    size_real = (uint64_t)size * (uint64_t)nmemb;

    if (LDG_UNLIKELY(!ctx->cb)) { return 0; }

    return (size_t)ctx->cb(contents, size_real, ctx->user_data);
}

uint32_t ldg_curl_multi_ctx_create(ldg_curl_multi_ctx_t *ctx, uint64_t cap)
{
    void *reqs_tmp = 0x0;

    if (LDG_UNLIKELY(!ctx || cap == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_curl_multi_ctx_t)) != ctx)) { return LDG_ERR_MEM_BAD; }

    ctx->multi = curl_multi_init();
    if (LDG_UNLIKELY(!ctx->multi)) { return LDG_ERR_NET_INIT; }

    if (LDG_UNLIKELY(posix_memalign(&reqs_tmp, LDG_AMD64_CACHE_LINE_WIDTH, cap * sizeof(ldg_curl_multi_req_t)) != 0))
    {
        curl_multi_cleanup(ctx->multi);
        ctx->multi = 0x0;
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->reqs = (ldg_curl_multi_req_t *)reqs_tmp;

    if (LDG_UNLIKELY(memset(ctx->reqs, 0, cap * sizeof(ldg_curl_multi_req_t)) != ctx->reqs)) { curl_multi_cleanup(ctx->multi); free(ctx->reqs); return LDG_ERR_MEM_BAD; }

    ctx->req_cap = cap;
    ctx->req_cunt = 0;

    return LDG_ERR_AOK;
}

void ldg_curl_multi_ctx_destroy(ldg_curl_multi_ctx_t *ctx)
{
    uint64_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    for (i = 0; i < ctx->req_cunt; i++)
    {
        if (ctx->reqs[i].curl)
        {
            curl_multi_remove_handle(ctx->multi, ctx->reqs[i].curl);
            curl_easy_cleanup(ctx->reqs[i].curl);
        }

        if (ctx->reqs[i].url) { free(ctx->reqs[i].url); }

        if (ctx->reqs[i].post_data) { free(ctx->reqs[i].post_data); }

        ldg_curl_resp_free(&ctx->reqs[i].resp);
    }

    if (ctx->reqs) { free(ctx->reqs); }

    if (ctx->multi) { curl_multi_cleanup(ctx->multi); }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_curl_multi_ctx_t)) != ctx)) { return; }
}

uint32_t ldg_curl_multi_req_add(ldg_curl_multi_ctx_t *ctx, const char *url, const char *post_data, void *hdrs)
{
    ldg_curl_multi_req_t *req = 0x0;
    uint64_t url_len = 0;
    uint64_t data_len = 0;
    uint32_t ret = LDG_ERR_AOK;
    void *url_tmp = 0x0;
    void *data_tmp = 0x0;

    if (LDG_UNLIKELY(!ctx || !url)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->req_cunt >= ctx->req_cap)) { return LDG_ERR_FULL; }

    req = &ctx->reqs[ctx->req_cunt];
    if (LDG_UNLIKELY(memset(req, 0, sizeof(ldg_curl_multi_req_t)) != req)) { return LDG_ERR_MEM_BAD; }

    url_len = (uint64_t)strlen(url) + LDG_STR_TERM_SIZE;
    if (LDG_UNLIKELY(posix_memalign(&url_tmp, LDG_AMD64_CACHE_LINE_WIDTH, url_len) != 0)) { return LDG_ERR_ALLOC_NULL; }

    req->url = (char *)url_tmp;

    ret = ldg_strrbrcpy(req->url, url, url_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        free(req->url);
        req->url = 0x0;
        return ret;
    }

    if (post_data)
    {
        data_len = (uint64_t)strlen(post_data) + LDG_STR_TERM_SIZE;
        if (LDG_UNLIKELY(posix_memalign(&data_tmp, LDG_AMD64_CACHE_LINE_WIDTH, data_len) != 0))
        {
            free(req->url);
            req->url = 0x0;
            return LDG_ERR_ALLOC_NULL;
        }

        req->post_data = (char *)data_tmp;

        ret = ldg_strrbrcpy(req->post_data, post_data, data_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
        {
            free(req->url);
            req->url = 0x0;
            free(req->post_data);
            req->post_data = 0x0;
            return ret;
        }
    }

    req->hdrs = hdrs;
    req->idx = (uint32_t)ctx->req_cunt;

    ldg_curl_resp_init(&req->resp);

    req->curl = curl_easy_init();
    if (LDG_UNLIKELY(!req->curl))
    {
        free(req->url);
        req->url = 0x0;
        if (req->post_data) { free(req->post_data); req->post_data = 0x0; }

        return LDG_ERR_NET_INIT;
    }

    if (LDG_UNLIKELY(curl_easy_setopt(req->curl, CURLOPT_URL, req->url) != CURLE_OK || curl_easy_setopt(req->curl, CURLOPT_WRITEFUNCTION, ldg_curl_resp_wr_cb) != CURLE_OK || curl_easy_setopt(req->curl, CURLOPT_WRITEDATA, &req->resp) != CURLE_OK || curl_easy_setopt(req->curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_3) != CURLE_OK || curl_easy_setopt(req->curl, CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK || (req->hdrs && curl_easy_setopt(req->curl, CURLOPT_HTTPHEADER, req->hdrs) != CURLE_OK) || (req->post_data && curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, req->post_data) != CURLE_OK)))
    {
        curl_easy_cleanup(req->curl);
        req->curl = 0x0;
        free(req->url);
        req->url = 0x0;
        if (req->post_data) { free(req->post_data); req->post_data = 0x0; }

        return LDG_ERR_NET_INIT;
    }

    curl_multi_add_handle(ctx->multi, req->curl);
    ctx->req_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_multi_get(ldg_curl_multi_ctx_t *ctx, const char *url, void *hdrs)
{
    uint32_t ret = 0;
    ldg_curl_multi_req_t *req = 0x0;

    ret = ldg_curl_multi_req_add(ctx, url, 0x0, hdrs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    req = &ctx->reqs[ctx->req_cunt - 1];
    if (LDG_UNLIKELY(curl_easy_setopt(req->curl, CURLOPT_HTTPGET, 1L) != CURLE_OK)) { return LDG_ERR_NET_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_multi_post(ldg_curl_multi_ctx_t *ctx, const char *url, const char *data, void *hdrs)
{
    uint32_t ret = 0;
    ldg_curl_multi_req_t *req = 0x0;

    ret = ldg_curl_multi_req_add(ctx, url, data, hdrs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    req = &ctx->reqs[ctx->req_cunt - 1];
    if (LDG_UNLIKELY(curl_easy_setopt(req->curl, CURLOPT_POST, 1L) != CURLE_OK)) { return LDG_ERR_NET_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_multi_perform(ldg_curl_multi_ctx_t *ctx)
{
    int32_t still_running = 0;
    int32_t numfds = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    do
    {
        curl_multi_perform(ctx->multi, &still_running);

        if (still_running) { curl_multi_wait(ctx->multi, 0x0, 0, 1000, &numfds); }
    } while (still_running);

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_multi_progress_get(ldg_curl_multi_ctx_t *ctx, double *out)
{
    uint64_t i = 0;
    int64_t total_dl = 0;
    int64_t total_now = 0;
    curl_off_t curl_dl = 0;
    curl_off_t curl_now = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    for (i = 0; i < ctx->req_cunt; i++)
    {
        if (ctx->reqs[i].curl)
        {
            curl_easy_getinfo(ctx->reqs[i].curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &curl_dl);
            curl_easy_getinfo(ctx->reqs[i].curl, CURLINFO_SIZE_DOWNLOAD_T, &curl_now);
            ctx->reqs[i].dl_total = (int64_t)curl_dl;
            ctx->reqs[i].dl_now = (int64_t)curl_now;
        }

        if (ctx->reqs[i].dl_total > 0)
        {
            total_dl += ctx->reqs[i].dl_total;
            total_now += ctx->reqs[i].dl_now;
        }
    }

    if (total_dl > 0) { *out = (double)total_now / (double)total_dl; }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_hdrs_append(void **list, const char *hdr)
{
    struct curl_slist *tmp = 0x0;

    if (LDG_UNLIKELY(!list || !hdr)) { return LDG_ERR_FUNC_ARG_NULL; }

    tmp = curl_slist_append((struct curl_slist *)*list, hdr);
    if (LDG_UNLIKELY(!tmp)) { return LDG_ERR_ALLOC_NULL; }

    *list = tmp;

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_hdrs_copy(void *src, void **dst)
{
    struct curl_slist *node = 0x0;
    struct curl_slist *tmp = 0x0;

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (!src) { return LDG_ERR_AOK; }

    node = (struct curl_slist *)src;
    while (node)
    {
        tmp = curl_slist_append((struct curl_slist *)*dst, node->data);
        if (LDG_UNLIKELY(!tmp)) { return LDG_ERR_ALLOC_NULL; }

        *dst = tmp;
        node = node->next;
    }

    return LDG_ERR_AOK;
}

void ldg_curl_hdrs_destroy(void **list)
{
    if (LDG_UNLIKELY(!list || !*list)) { return; }

    curl_slist_free_all((struct curl_slist *)*list);
    *list = 0x0;
}

uint32_t ldg_curl_easy_ctx_create(ldg_curl_easy_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->curl = curl_easy_init();
    if (LDG_UNLIKELY(!ctx->curl)) { return LDG_ERR_ALLOC_NULL; }

    ctx->is_init = 1;

    return LDG_ERR_AOK;
}

void ldg_curl_easy_ctx_destroy(ldg_curl_easy_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return; }

    if (ctx->curl)
    {
        curl_easy_cleanup(ctx->curl);
        ctx->curl = 0x0;
    }

    ctx->is_init = 0;
}

uint32_t ldg_curl_easy_get(ldg_curl_easy_ctx_t *ctx, const char *url, void *hdrs, ldg_curl_resp_t *resp)
{
    CURLcode res = CURLE_OK;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    curl_easy_reset(ctx->curl);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(ctx->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, ldg_curl_resp_wr_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, resp);

    if (hdrs) { curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, hdrs); }

    res = curl_easy_perform(ctx->curl);
    if (LDG_UNLIKELY(res != CURLE_OK))
    {
        ldg_curl_resp_free(resp);
        return LDG_ERR_NET_PERFORM;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_easy_post(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, void *hdrs, ldg_curl_resp_t *resp)
{
    CURLcode res = CURLE_OK;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    curl_easy_reset(ctx->curl);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(ctx->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, ldg_curl_resp_wr_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, resp);

    if (data) { curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, data); }
    else { curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, ""); }

    if (hdrs) { curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, hdrs); }

    res = curl_easy_perform(ctx->curl);
    if (LDG_UNLIKELY(res != CURLE_OK))
    {
        ldg_curl_resp_free(resp);
        return LDG_ERR_NET_PERFORM;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_easy_get_stream(ldg_curl_easy_ctx_t *ctx, const char *url, void *hdrs, ldg_curl_stream_cb_t cb, void *user_data)
{
    CURLcode res = CURLE_OK;
    ldg_curl_stream_ctx_t stream_ctx = { 0 };

    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !url || !cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    stream_ctx.cb = cb;
    stream_ctx.user_data = user_data;

    curl_easy_reset(ctx->curl);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(ctx->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, ldg_curl_stream_wr_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, &stream_ctx);

    if (hdrs) { curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, hdrs); }

    res = curl_easy_perform(ctx->curl);
    if (LDG_UNLIKELY(res != CURLE_OK)) { return LDG_ERR_NET_PERFORM; }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_easy_post_stream(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, void *hdrs, ldg_curl_stream_cb_t cb, void *user_data)
{
    CURLcode res = CURLE_OK;
    ldg_curl_stream_ctx_t stream_ctx = { 0 };

    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !url || !cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    stream_ctx.cb = cb;
    stream_ctx.user_data = user_data;

    curl_easy_reset(ctx->curl);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(ctx->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, ldg_curl_stream_wr_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, &stream_ctx);

    if (data) { curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, data); }
    else { curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, ""); }

    if (hdrs) { curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, hdrs); }

    res = curl_easy_perform(ctx->curl);
    if (LDG_UNLIKELY(res != CURLE_OK)) { return LDG_ERR_NET_PERFORM; }

    return LDG_ERR_AOK;
}
