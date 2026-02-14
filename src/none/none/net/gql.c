#include <string.h>

#include <dangling/net/gql.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

uint32_t ldg_gql_ctx_create(ldg_gql_ctx_t *ctx, const char *endpoint_url)
{
    uint64_t url_len = 0;
    uint32_t ret = 0;
    void *url_tmp = 0x0;

    if (LDG_UNLIKELY(!ctx || !endpoint_url)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_gql_ctx_t)) != ctx)) { return LDG_ERR_MEM_BAD; }

    url_len = (uint64_t)strlen(endpoint_url) + LDG_STR_TERM_SIZE;

    ret = ldg_mem_alloc(url_len, &url_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->endpoint_url = (char *)url_tmp;

    ret = ldg_strrbrcpy(ctx->endpoint_url, endpoint_url, url_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_OVERLAP))
    {
        ldg_mem_dealloc(ctx->endpoint_url);
        ctx->endpoint_url = 0x0;
        return ret;
    }

    ret = ldg_curl_easy_ctx_create(&ctx->curl_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx->endpoint_url);
        ctx->endpoint_url = 0x0;
        return ret;
    }

    return LDG_ERR_AOK;
}

void ldg_gql_ctx_destroy(ldg_gql_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    ldg_curl_easy_ctx_destroy(&ctx->curl_ctx);

    if (ctx->endpoint_url) { ldg_mem_dealloc(ctx->endpoint_url); }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_gql_ctx_t)) != ctx)) { return; }
}

uint32_t ldg_gql_exec(ldg_gql_ctx_t *ctx, const char *body, void *headers, ldg_curl_resp_t *resp)
{
    uint32_t ret = 0;
    void *gql_headers = 0x0;

    if (LDG_UNLIKELY(!ctx || !body || !resp)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_curl_headers_append(&gql_headers, "Content-Type: application/json");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (headers)
    {
        ret = ldg_curl_headers_copy(headers, &gql_headers);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_curl_headers_destroy(&gql_headers);
            return ret;
        }
    }

    ret = ldg_curl_easy_post(&ctx->curl_ctx, ctx->endpoint_url, body, gql_headers, resp);

    ldg_curl_headers_destroy(&gql_headers);

    return ret;
}
