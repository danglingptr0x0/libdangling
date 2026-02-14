#ifndef LDG_NET_GQL_H
#define LDG_NET_GQL_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/net/curl.h>

typedef struct ldg_gql_ctx
{
    ldg_curl_easy_ctx_t curl_ctx;
    char *endpoint_url;
} ldg_gql_ctx_t;

LDG_EXPORT uint32_t ldg_gql_ctx_create(ldg_gql_ctx_t *ctx, const char *endpoint_url);
LDG_EXPORT void ldg_gql_ctx_destroy(ldg_gql_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_gql_exec(ldg_gql_ctx_t *ctx, const char *body, void *headers, ldg_curl_resp_t *resp);

#endif
