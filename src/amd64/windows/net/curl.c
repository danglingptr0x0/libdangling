#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <malloc.h>
#include <windows.h>
#include <winhttp.h>

#include <dangling/net/curl.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/proto/emiemi.h>

typedef struct ldg_curl_hdr_node
{
    struct ldg_curl_hdr_node *next;
    char *text;
} ldg_curl_hdr_node_t;

static uint32_t resp_grow_append(ldg_curl_resp_t *resp, const void *contents, uint64_t size_real)
{
    uint64_t new_len = 0;
    uint64_t new_cap = 0;
    void *new_data = 0x0;

    new_len = resp->size + size_real;
    if (LDG_UNLIKELY(new_len < resp->size || new_len > LDG_CURL_RESP_MAX_CAP)) { return LDG_ERR_OVERFLOW; }

    if (!resp->data || new_len + LDG_STR_TERM_SIZE > resp->cap)
    {
        new_cap = resp->cap ? resp->cap * LDG_CURL_RESP_RESIZE_FACTOR : LDG_CURL_RESP_INITIAL_CAP;
        while (new_cap < new_len + LDG_STR_TERM_SIZE) { new_cap *= LDG_CURL_RESP_RESIZE_FACTOR; }

        new_data = _aligned_malloc(new_cap, LDG_AMD64_CACHE_LINE_WIDTH);
        if (LDG_UNLIKELY(!new_data)) { return LDG_ERR_ALLOC_NULL; }

        if (LDG_UNLIKELY(memset(new_data, 0, new_cap) != new_data))
        {
            _aligned_free(new_data);
            return LDG_ERR_MEM_BAD;
        }

        if (resp->data)
        {
            if (LDG_UNLIKELY(ldg_mem_secure_cpy(new_data, resp->data, resp->size) != LDG_ERR_AOK))
            {
                _aligned_free(new_data);
                return LDG_ERR_MEM_BAD;
            }

            _aligned_free(resp->data);
        }

        resp->data = new_data;
        resp->cap = new_cap;
    }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(resp->data + resp->size, contents, size_real) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    resp->size = new_len;
    resp->data[resp->size] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t utf8_to_wide(const char *in, wchar_t **out)
{
    uint64_t byte_len = 0;
    int32_t wide_len = 0;
    void *tmp = 0x0;
    wchar_t *buff = 0x0;

    if (LDG_UNLIKELY(!in || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    byte_len = (uint64_t)strlen(in) + 1;

    wide_len = (int32_t)MultiByteToWideChar(CP_UTF8, 0, in, (int32_t)byte_len, 0x0, 0);
    if (LDG_UNLIKELY(wide_len <= 0)) { return LDG_ERR_NET_PROTOCOL; }

    tmp = _aligned_malloc((uint64_t)wide_len * sizeof(wchar_t), LDG_AMD64_CACHE_LINE_WIDTH);
    if (LDG_UNLIKELY(!tmp)) { return LDG_ERR_ALLOC_NULL; }

    buff = (wchar_t *)tmp;

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, 0, in, (int32_t)byte_len, buff, wide_len) <= 0))
    {
        _aligned_free(buff);
        return LDG_ERR_NET_PROTOCOL;
    }

    *out = buff;

    return LDG_ERR_AOK;
}

static uint32_t hdr_arr_to_wide(void *hdr_arr, wchar_t **out, uint32_t *wide_len_out)
{
    ldg_curl_hdr_node_t *node = 0x0;
    uint64_t total = 0;
    uint64_t pos = 0;
    uint64_t text_len = 0;
    void *tmp = 0x0;
    char *joined = 0x0;
    uint32_t ret = 0;

    *out = 0x0;
    *wide_len_out = 0;

    if (!hdr_arr) { return LDG_ERR_AOK; }

    node = (ldg_curl_hdr_node_t *)hdr_arr;
    while (node)
    {
        total += (uint64_t)strlen(node->text) + 2;
        node = node->next;
    }

    if (total == 0) { return LDG_ERR_AOK; }

    tmp = _aligned_malloc(total + LDG_STR_TERM_SIZE, LDG_AMD64_CACHE_LINE_WIDTH);
    if (LDG_UNLIKELY(!tmp)) { return LDG_ERR_ALLOC_NULL; }

    joined = (char *)tmp;

    node = (ldg_curl_hdr_node_t *)hdr_arr;
    while (node)
    {
        text_len = (uint64_t)strlen(node->text);
        if (text_len > 0)
        {
            if (LDG_UNLIKELY(ldg_mem_secure_cpy(joined + pos, node->text, text_len) != LDG_ERR_AOK))
            {
                _aligned_free(joined);
                return LDG_ERR_MEM_BAD;
            }

            pos += text_len;
        }

        joined[pos] = '\r';
        pos++;
        joined[pos] = '\n';
        pos++;
        node = node->next;
    }

    joined[pos] = LDG_STR_TERM;

    ret = utf8_to_wide(joined, out);
    _aligned_free(joined);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *wide_len_out = (uint32_t)wcslen(*out);

    return LDG_ERR_AOK;
}

static uint32_t winhttp_sesh_open(HINTERNET *out)
{
    HINTERNET sesh = 0x0;
    DWORD secure_protocol = 0;

    *out = 0x0;

    sesh = WinHttpOpen(L"libdangling/4.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (LDG_UNLIKELY(!sesh)) { return LDG_ERR_NET_INIT; }

#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
    secure_protocol |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#endif
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
    secure_protocol |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
    if (secure_protocol != 0) { WinHttpSetOption(sesh, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocol, (DWORD)sizeof(secure_protocol)); }

    *out = sesh;

    return LDG_ERR_AOK;
}

static uint32_t winhttp_run(HINTERNET sesh, const wchar_t *method, const char *url, const char *body, uint64_t body_len, void *hdr_arr, ldg_curl_resp_t *resp, ldg_curl_stream_cb_t cb, void *cb_user, int64_t *content_len_out)
{
    wchar_t host[256] = LDG_ARR_ZERO_INIT;
    wchar_t *url_wide = 0x0;
    wchar_t *hdr_wide = 0x0;
    HINTERNET conn = 0x0;
    HINTERNET req = 0x0;
    URL_COMPONENTS url_comp = LDG_STRUCT_ZERO_INIT;
    DWORD flags = 0;
    uint32_t hdr_len = 0;
    DWORD avail = 0;
    DWORD rd = 0;
    DWORD content_len = 0;
    DWORD content_len_size = 0;
    void *chunk = 0x0;
    uint32_t ret = LDG_ERR_AOK;
    BOOL ok = FALSE;

    if (content_len_out) { *content_len_out = 0; }

    ret = utf8_to_wide(url, &url_wide);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    url_comp.dwStructSize = (DWORD)sizeof(url_comp);
    url_comp.lpszHostName = host;
    url_comp.dwHostNameLength = (DWORD)(sizeof(host) / sizeof(wchar_t));
    url_comp.lpszUrlPath = 0x0;
    url_comp.dwUrlPathLength = 1;

    if (LDG_UNLIKELY(!WinHttpCrackUrl(url_wide, 0, 0, &url_comp)))
    {
        _aligned_free(url_wide);
        return LDG_ERR_NET_PROTOCOL;
    }

    conn = WinHttpConnect(sesh, host, url_comp.nPort, 0);
    if (LDG_UNLIKELY(!conn))
    {
        _aligned_free(url_wide);
        return LDG_ERR_NET_CONN;
    }

    flags = (url_comp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;

    req = WinHttpOpenRequest(conn, method, ((url_comp.lpszUrlPath != 0x0 && url_comp.lpszUrlPath[0] != 0) ? url_comp.lpszUrlPath : L"/"), 0x0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (LDG_UNLIKELY(!req))
    {
        WinHttpCloseHandle(conn);
        _aligned_free(url_wide);
        return LDG_ERR_NET_CONN;
    }

    ret = hdr_arr_to_wide(hdr_arr, &hdr_wide, &hdr_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        _aligned_free(url_wide);
        return ret;
    }

    ok = WinHttpSendRequest(req, (hdr_wide ? hdr_wide : WINHTTP_NO_ADDITIONAL_HEADERS), (hdr_wide ? hdr_len : 0), (body ? (void *)body : 0x0), (DWORD)body_len, (DWORD)body_len, 0);

    if (hdr_wide) { _aligned_free(hdr_wide); }

    if (LDG_UNLIKELY(!ok))
    {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        _aligned_free(url_wide);
        return LDG_ERR_NET_SEND;
    }

    if (LDG_UNLIKELY(!WinHttpReceiveResponse(req, 0x0)))
    {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        _aligned_free(url_wide);
        return LDG_ERR_NET_RECV;
    }

    if (content_len_out)
    {
        content_len_size = (DWORD)sizeof(content_len);
        if (WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &content_len, &content_len_size, WINHTTP_NO_HEADER_INDEX)) { *content_len_out = (int64_t)content_len; }
    }

    for (;;)
    {
        avail = 0;
        if (LDG_UNLIKELY(!WinHttpQueryDataAvailable(req, &avail)))
        {
            ret = LDG_ERR_NET_RECV;
            break;
        }

        if (avail == 0) { break; }

        chunk = _aligned_malloc(avail, LDG_AMD64_CACHE_LINE_WIDTH);
        if (LDG_UNLIKELY(!chunk))
        {
            ret = LDG_ERR_ALLOC_NULL;
            break;
        }

        rd = 0;
        if (LDG_UNLIKELY(!WinHttpReadData(req, chunk, avail, &rd)))
        {
            _aligned_free(chunk);
            ret = LDG_ERR_NET_RECV;
            break;
        }

        if (rd == 0)
        {
            _aligned_free(chunk);
            break;
        }

        if (cb) { cb(chunk, (uint64_t)rd, cb_user); }
        else if (resp)
        {
            ret = resp_grow_append(resp, chunk, (uint64_t)rd);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                _aligned_free(chunk);
                break;
            }
        }

        _aligned_free(chunk);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(conn);
    _aligned_free(url_wide);

    return ret;
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
        _aligned_free(resp->data);
        resp->data = 0x0;
    }

    resp->size = 0;
    resp->cap = 0;
}

uint32_t ldg_curl_multi_ctx_create(ldg_curl_multi_ctx_t *ctx, uint64_t cap)
{
    HINTERNET sesh = 0x0;
    void *reqs_tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || cap == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_curl_multi_ctx_t)) != ctx)) { return LDG_ERR_MEM_BAD; }

    ret = winhttp_sesh_open(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->multi = sesh;

    reqs_tmp = _aligned_malloc(cap * sizeof(ldg_curl_multi_req_t), LDG_AMD64_CACHE_LINE_WIDTH);
    if (LDG_UNLIKELY(!reqs_tmp))
    {
        WinHttpCloseHandle(ctx->multi);
        ctx->multi = 0x0;
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->reqs = (ldg_curl_multi_req_t *)reqs_tmp;

    if (LDG_UNLIKELY(memset(ctx->reqs, 0, cap * sizeof(ldg_curl_multi_req_t)) != ctx->reqs))
    {
        WinHttpCloseHandle(ctx->multi);
        _aligned_free(ctx->reqs);
        return LDG_ERR_MEM_BAD;
    }

    ctx->req_cap = cap;
    ctx->req_cunt = 0;

    return LDG_ERR_AOK;
}

void ldg_curl_multi_ctx_destroy(ldg_curl_multi_ctx_t *ctx)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    for (idx = 0; idx < ctx->req_cunt; idx++)
    {
        if (ctx->reqs[idx].url) { _aligned_free(ctx->reqs[idx].url); }

        if (ctx->reqs[idx].post_data) { _aligned_free(ctx->reqs[idx].post_data); }

        ldg_curl_resp_free(&ctx->reqs[idx].resp);
    }

    if (ctx->reqs) { _aligned_free(ctx->reqs); }

    if (ctx->multi) { WinHttpCloseHandle(ctx->multi); }

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
    url_tmp = _aligned_malloc(url_len, LDG_AMD64_CACHE_LINE_WIDTH);
    if (LDG_UNLIKELY(!url_tmp)) { return LDG_ERR_ALLOC_NULL; }

    req->url = (char *)url_tmp;

    ret = ldg_str_rbrcpy(req->url, url, url_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        _aligned_free(req->url);
        req->url = 0x0;
        return ret;
    }

    if (post_data)
    {
        data_len = (uint64_t)strlen(post_data) + LDG_STR_TERM_SIZE;
        data_tmp = _aligned_malloc(data_len, LDG_AMD64_CACHE_LINE_WIDTH);
        if (LDG_UNLIKELY(!data_tmp))
        {
            _aligned_free(req->url);
            req->url = 0x0;
            return LDG_ERR_ALLOC_NULL;
        }

        req->post_data = (char *)data_tmp;

        ret = ldg_str_rbrcpy(req->post_data, post_data, data_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
        {
            _aligned_free(req->url);
            req->url = 0x0;
            _aligned_free(req->post_data);
            req->post_data = 0x0;
            return ret;
        }
    }

    req->hdrs = hdrs;
    req->idx = (uint32_t)ctx->req_cunt;

    ldg_curl_resp_init(&req->resp);

    ctx->req_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_multi_get(ldg_curl_multi_ctx_t *ctx, const char *url, void *hdrs)
{
    if (LDG_UNLIKELY(!ctx || !url)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_curl_multi_req_add(ctx, url, 0x0, hdrs);
}

uint32_t ldg_curl_multi_post(ldg_curl_multi_ctx_t *ctx, const char *url, const char *data, void *hdrs)
{
    if (LDG_UNLIKELY(!data)) { return ldg_curl_multi_req_add(ctx, url, "", hdrs); }

    return ldg_curl_multi_req_add(ctx, url, data, hdrs);
}

uint32_t ldg_curl_multi_perform(ldg_curl_multi_ctx_t *ctx)
{
    uint64_t idx = 0;
    uint64_t body_len = 0;
    int64_t content_len = 0;
    const wchar_t *method = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < ctx->req_cunt; idx++)
    {
        method = ctx->reqs[idx].post_data ? L"POST" : L"GET";
        body_len = ctx->reqs[idx].post_data ? (uint64_t)strlen(ctx->reqs[idx].post_data) : 0;

        winhttp_run(ctx->multi, method, ctx->reqs[idx].url, ctx->reqs[idx].post_data, body_len, ctx->reqs[idx].hdrs, &ctx->reqs[idx].resp, 0x0, 0x0, &content_len);

        ctx->reqs[idx].dl_total = content_len;
        ctx->reqs[idx].dl_now = (int64_t)ctx->reqs[idx].resp.size;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_multi_progress_get(ldg_curl_multi_ctx_t *ctx, double *out)
{
    uint64_t idx = 0;
    int64_t total_dl = 0;
    int64_t total_now = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    for (idx = 0; idx < ctx->req_cunt; idx++) { if (ctx->reqs[idx].dl_total > 0)
        {
            total_dl += ctx->reqs[idx].dl_total;
            total_now += ctx->reqs[idx].dl_now;
        }
    }

    if (total_dl > 0) { *out = (double)total_now / (double)total_dl; }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_hdr_append(void **list, const char *hdr)
{
    ldg_curl_hdr_node_t *node = 0x0;
    ldg_curl_hdr_node_t *tail = 0x0;
    void *node_tmp = 0x0;
    void *text_tmp = 0x0;
    uint64_t hdr_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!list || !hdr)) { return LDG_ERR_FUNC_ARG_NULL; }

    node_tmp = _aligned_malloc(sizeof(ldg_curl_hdr_node_t), LDG_AMD64_CACHE_LINE_WIDTH);
    if (LDG_UNLIKELY(!node_tmp)) { return LDG_ERR_ALLOC_NULL; }

    node = (ldg_curl_hdr_node_t *)node_tmp;
    node->next = 0x0;
    node->text = 0x0;

    hdr_len = (uint64_t)strlen(hdr) + LDG_STR_TERM_SIZE;
    text_tmp = _aligned_malloc(hdr_len, LDG_AMD64_CACHE_LINE_WIDTH);
    if (LDG_UNLIKELY(!text_tmp))
    {
        _aligned_free(node);
        return LDG_ERR_ALLOC_NULL;
    }

    node->text = (char *)text_tmp;

    ret = ldg_str_rbrcpy(node->text, hdr, hdr_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        _aligned_free(node->text);
        _aligned_free(node);
        return ret;
    }

    if (!*list)
    {
        *list = node;
        return LDG_ERR_AOK;
    }

    tail = (ldg_curl_hdr_node_t *)*list;
    while (tail->next) { tail = tail->next; }
    tail->next = node;

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_hdr_cpy(void *src, void **dst)
{
    ldg_curl_hdr_node_t *node = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (!src) { return LDG_ERR_AOK; }

    node = (ldg_curl_hdr_node_t *)src;
    while (node)
    {
        ret = ldg_curl_hdr_append(dst, node->text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        node = node->next;
    }

    return LDG_ERR_AOK;
}

void ldg_curl_hdr_destroy(void **list)
{
    ldg_curl_hdr_node_t *node = 0x0;
    ldg_curl_hdr_node_t *next = 0x0;

    if (LDG_UNLIKELY(!list || !*list)) { return; }

    node = (ldg_curl_hdr_node_t *)*list;
    while (node)
    {
        next = node->next;
        if (node->text) { _aligned_free(node->text); }

        _aligned_free(node);
        node = next;
    }

    *list = 0x0;
}

uint32_t ldg_curl_easy_ctx_create(ldg_curl_easy_ctx_t *ctx)
{
    HINTERNET sesh = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = winhttp_sesh_open(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx->curl = sesh;
    ctx->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

void ldg_curl_easy_ctx_destroy(ldg_curl_easy_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE)) { return; }

    if (ctx->curl)
    {
        WinHttpCloseHandle(ctx->curl);
        ctx->curl = 0x0;
    }

    ctx->is_init = LDG_TRUTH_FALSE;
}

uint32_t ldg_curl_easy_get(ldg_curl_easy_ctx_t *ctx, const char *url, void *hdrs, ldg_curl_resp_t *resp)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    ret = winhttp_run(ctx->curl, L"GET", url, 0x0, 0, hdrs, resp, 0x0, 0x0, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_curl_resp_free(resp);
        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_easy_post(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, void *hdrs, ldg_curl_resp_t *resp)
{
    uint64_t body_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    body_len = data ? (uint64_t)strlen(data) : 0;

    ret = winhttp_run(ctx->curl, L"POST", url, (data ? data : ""), body_len, hdrs, resp, 0x0, 0x0, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_curl_resp_free(resp);
        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_easy_raw_post(ldg_curl_easy_ctx_t *ctx, const char *url, const void *data, uint64_t len, void *hdrs, ldg_curl_resp_t *resp)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE || !url || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!data && len)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_curl_resp_init(resp);

    ret = winhttp_run(ctx->curl, L"POST", url, (data ? (const char *)data : ""), len, hdrs, resp, 0x0, 0x0, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_curl_resp_free(resp);
        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_emiemi_post(ldg_curl_easy_ctx_t *ctx, const char *url, const uint8_t *payload, uint32_t payload_len, void *hdrs, ldg_curl_resp_t *resp, ldg_curl_emiemi_res_t *out)
{
    uint8_t *frame = 0x0;
    uint32_t frame_len = 0;
    uint32_t err = 0;
    uint32_t post_err = 0;

    if (LDG_UNLIKELY(!ctx || !url || !resp || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!payload && payload_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->payload = 0x0;
    out->payload_len = 0;
    out->req_checksum = 0;
    out->resp_checksum = 0;

    if (LDG_UNLIKELY(payload_len > LDG_EMIEMI_MAX_PAYLOAD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    frame_len = ldg_emiemi_encoded_size_get(payload_len);
    if (LDG_UNLIKELY(!frame_len)) { return LDG_ERR_OVERFLOW; }

    err = ldg_mem_alloc(frame_len, (void **)&frame);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_emiemi_encode(payload, payload_len, frame, frame_len, &out->req_checksum);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(frame);
        return err;
    }

    post_err = ldg_curl_easy_raw_post(ctx, url, frame, frame_len, hdrs, resp);
    ldg_mem_dealloc(frame);
    frame = 0x0;
    if (LDG_UNLIKELY(post_err != LDG_ERR_AOK)) { return post_err; }

    if (LDG_UNLIKELY(resp->size > (uint64_t)LDG_EMIEMI_MAX_PAYLOAD + LDG_EMIEMI_OVERHEAD)) { return LDG_ERR_OVERFLOW; }

    err = ldg_emiemi_decode((const ldg_byte_t *)resp->data, (ldg_dword_t)resp->size, &out->payload, &out->payload_len, &out->resp_checksum);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return LDG_ERR_AOK;
}

uint32_t ldg_curl_easy_get_stream(ldg_curl_easy_ctx_t *ctx, const char *url, void *hdrs, ldg_curl_stream_cb_t cb, void *user_data)
{
    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE || !url || !cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    return winhttp_run(ctx->curl, L"GET", url, 0x0, 0, hdrs, 0x0, cb, user_data, 0x0);
}

uint32_t ldg_curl_easy_post_stream(ldg_curl_easy_ctx_t *ctx, const char *url, const char *data, void *hdrs, ldg_curl_stream_cb_t cb, void *user_data)
{
    uint64_t body_len = 0;

    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE || !url || !cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    body_len = data ? (uint64_t)strlen(data) : 0;

    return winhttp_run(ctx->curl, L"POST", url, (data ? data : ""), body_len, hdrs, 0x0, cb, user_data, 0x0);
}
