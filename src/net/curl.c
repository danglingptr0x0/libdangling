#include <dangling/net/curl.h>
#include <dangling/core/err.h>
#include <dangling/str/str.h>
#include <dangling/log/log.h>

#include <stdlib.h>
#include <string.h>

size_t ldg_curl_resp_write_cb(void *contents, size_t size, size_t nmemb, void *user_data)
{
    size_t size_real = 0;
    ldg_curl_resp_t *resp = NULL;
    size_t new_len = 0;
    size_t new_cap = 0;
    void *new_data = NULL;

    if (LDG_UNLIKELY(!contents || !user_data)) { return 0; }

    size_real = size * nmemb;
    resp = (ldg_curl_resp_t *)user_data;

    new_len = resp->size + size_real;

    if (!resp->data || new_len + LDG_STR_TERM_SIZE > resp->cap)
    {
        new_cap = resp->cap ? resp->cap * LDG_CURL_RESP_RESIZE_FACTOR : LDG_CURL_RESP_INITIAL_CAP;
        while (new_cap < new_len + LDG_STR_TERM_SIZE) { new_cap *= LDG_CURL_RESP_RESIZE_FACTOR; }

        if (LDG_UNLIKELY(posix_memalign(&new_data, LDG_CACHE_LINE_WIDTH, new_cap) != 0))
        {
            LDG_LOG_ERROR("ldg_curl: couldn't memalign resp buff");
            return 0;
        }

        (void)memset(new_data, 0, new_cap);

        if (resp->data)
        {
            (void)memcpy(new_data, resp->data, resp->size);
            free(resp->data);
        }

        resp->data = new_data;
        resp->cap = new_cap;
    }

    (void)memcpy(resp->data + resp->size, contents, size_real);
    resp->size = new_len;
    resp->data[resp->size] = LDG_STR_TERM;

    return size_real;
}

void ldg_curl_resp_init(ldg_curl_resp_t *resp)
{
    if (LDG_UNLIKELY(!resp)) { return; }

    (void)memset(resp, 0, sizeof(ldg_curl_resp_t));
}

void ldg_curl_resp_free(ldg_curl_resp_t *resp)
{
    if (LDG_UNLIKELY(!resp)) { return; }

    if (resp->data)
    {
        free(resp->data);
        resp->data = NULL;
    }

    resp->size = 0;
    resp->cap = 0;
}

int32_t ldg_curl_multi_ctx_create(ldg_curl_multi_ctx_t *ctx, size_t capacity)
{
    if (LDG_UNLIKELY(!ctx || capacity == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(ctx, 0, sizeof(ldg_curl_multi_ctx_t));

    ctx->multi = curl_multi_init();
    if (LDG_UNLIKELY(!ctx->multi)) { return LDG_ERR_NET_INIT; }

    if (LDG_UNLIKELY(posix_memalign((void **)&ctx->reqs, LDG_CACHE_LINE_WIDTH, capacity * sizeof(ldg_curl_multi_req_t)) != 0))
    {
        curl_multi_cleanup(ctx->multi);
        ctx->multi = NULL;
        return LDG_ERR_ALLOC_NULL;
    }

    (void)memset(ctx->reqs, 0, capacity * sizeof(ldg_curl_multi_req_t));
    ctx->req_cap = capacity;
    ctx->req_cunt = 0;

    return LDG_ERR_AOK;
}

void ldg_curl_multi_ctx_destroy(ldg_curl_multi_ctx_t *ctx)
{
    size_t i = 0;

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

        ldg_curl_resp_free(&ctx->reqs[i].response);
    }

    if (ctx->reqs) { free(ctx->reqs); }

    if (ctx->multi) { curl_multi_cleanup(ctx->multi); }

    (void)memset(ctx, 0, sizeof(ldg_curl_multi_ctx_t));
}

int32_t ldg_curl_multi_req_add(ldg_curl_multi_ctx_t *ctx, const char *url, const char *post_data, struct curl_slist *headers)
{
    ldg_curl_multi_req_t *req = NULL;
    size_t url_len = 0;
    size_t data_len = 0;
    int32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ctx || !url)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->req_cunt >= ctx->req_cap)) { return LDG_ERR_FULL; }

    req = &ctx->reqs[ctx->req_cunt];
    (void)memset(req, 0, sizeof(ldg_curl_multi_req_t));

    url_len = strlen(url) + LDG_STR_TERM_SIZE;
    if (LDG_UNLIKELY(posix_memalign((void **)&req->url, LDG_CACHE_LINE_WIDTH, url_len) != 0)) { return LDG_ERR_ALLOC_NULL; }

    ret = ldg_strrbrcpy(req->url, url, url_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        free(req->url);
        req->url = NULL;
        return ret;
    }

    if (post_data)
    {
        data_len = strlen(post_data) + LDG_STR_TERM_SIZE;
        if (LDG_UNLIKELY(posix_memalign((void **)&req->post_data, LDG_CACHE_LINE_WIDTH, data_len) != 0))
        {
            free(req->url);
            req->url = NULL;
            return LDG_ERR_ALLOC_NULL;
        }

        ret = ldg_strrbrcpy(req->post_data, post_data, data_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
        {
            free(req->url);
            req->url = NULL;
            free(req->post_data);
            req->post_data = NULL;
            return ret;
        }
    }

    req->headers = headers;
    req->idx = (int)ctx->req_cunt;

    ldg_curl_resp_init(&req->response);

    req->curl = curl_easy_init();
    if (LDG_UNLIKELY(!req->curl))
    {
        free(req->url);
        req->url = NULL;
        if (req->post_data) { free(req->post_data); req->post_data = NULL; }

        return LDG_ERR_NET_INIT;
    }

    (void)curl_easy_setopt(req->curl, CURLOPT_URL, req->url);
    (void)curl_easy_setopt(req->curl, CURLOPT_WRITEFUNCTION, ldg_curl_resp_write_cb);
    (void)curl_easy_setopt(req->curl, CURLOPT_WRITEDATA, &req->response);
    (void)curl_easy_setopt(req->curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_3);
    (void)curl_easy_setopt(req->curl, CURLOPT_SSL_VERIFYPEER, 1L);

    if (req->headers) { (void)curl_easy_setopt(req->curl, CURLOPT_HTTPHEADER, req->headers); }

    if (req->post_data) { (void)curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, req->post_data); }

    curl_multi_add_handle(ctx->multi, req->curl);
    ctx->req_cunt++;

    return LDG_ERR_AOK;
}

int32_t ldg_curl_multi_perform(ldg_curl_multi_ctx_t *ctx)
{
    int still_running = 0;
    int numfds = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    do
    {
        curl_multi_perform(ctx->multi, &still_running);

        if (still_running) { curl_multi_wait(ctx->multi, NULL, 0, 1000, &numfds); }
    } while (still_running);

    return LDG_ERR_AOK;
}

double ldg_curl_multi_progress_get(ldg_curl_multi_ctx_t *ctx)
{
    size_t i = 0;
    curl_off_t total_dl = 0;
    curl_off_t total_now = 0;

    if (LDG_UNLIKELY(!ctx)) { return 0.0; }

    for (i = 0; i < ctx->req_cunt; i++)
    {
        if (ctx->reqs[i].curl)
        {
            curl_easy_getinfo(ctx->reqs[i].curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &ctx->reqs[i].dl_total);
            curl_easy_getinfo(ctx->reqs[i].curl, CURLINFO_SIZE_DOWNLOAD_T, &ctx->reqs[i].dl_now);
        }

        if (ctx->reqs[i].dl_total > 0)
        {
            total_dl += ctx->reqs[i].dl_total;
            total_now += ctx->reqs[i].dl_now;
        }
    }

    if (total_dl <= 0) { return 0.0; }

    return (double)total_now / (double)total_dl;
}

int32_t ldg_curl_headers_append(struct curl_slist **list, const char *header)
{
    struct curl_slist *tmp = NULL;

    if (LDG_UNLIKELY(!list || !header)) { return LDG_ERR_FUNC_ARG_NULL; }

    tmp = curl_slist_append(*list, header);
    if (LDG_UNLIKELY(!tmp)) { return LDG_ERR_ALLOC_NULL; }

    *list = tmp;

    return LDG_ERR_AOK;
}

void ldg_curl_headers_destroy(struct curl_slist **list)
{
    if (LDG_UNLIKELY(!list || !*list)) { return; }

    curl_slist_free_all(*list);
    *list = NULL;
}

int32_t ldg_curl_easy_ctx_create(ldg_curl_easy_ctx_t *ctx)
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
        ctx->curl = NULL;
    }

    ctx->is_init = 0;
}

int32_t ldg_curl_easy_get(ldg_curl_easy_ctx_t *ctx, const char *url, struct curl_slist *headers, ldg_curl_resp_t *resp)
{
    CURLcode res = CURLE_OK;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    curl_easy_reset(ctx->curl);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(ctx->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, ldg_curl_resp_write_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, resp);

    if (headers) { curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers); }

    res = curl_easy_perform(ctx->curl);
    if (LDG_UNLIKELY(res != CURLE_OK))
    {
        ldg_curl_resp_free(resp);
        return LDG_ERR_NET_PERFORM;
    }

    return LDG_ERR_AOK;
}

int32_t ldg_curl_easy_post(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, struct curl_slist *headers, ldg_curl_resp_t *resp)
{
    CURLcode res = CURLE_OK;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    curl_easy_reset(ctx->curl);
    curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
    curl_easy_setopt(ctx->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, ldg_curl_resp_write_cb);
    curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, resp);

    if (data) { curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, data); }
    else { curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, ""); }

    if (headers) { curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers); }

    res = curl_easy_perform(ctx->curl);
    if (LDG_UNLIKELY(res != CURLE_OK))
    {
        ldg_curl_resp_free(resp);
        return LDG_ERR_NET_PERFORM;
    }

    return LDG_ERR_AOK;
}
