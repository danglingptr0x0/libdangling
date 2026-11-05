#ifndef LDG_NET_GQL_H
#define LDG_NET_GQL_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/net/curl.h>

typedef struct ldg_net_gql_ctx
{
    ldg_net_curl_easy_ctx_t curl_ctx;
    char *endpoint_url;
} ldg_net_gql_ctx_t;

LDG_EXPORT uint32_t ldg_net_gql_ctx_create(ldg_net_gql_ctx_t *ctx, const char *endpoint_url);
LDG_EXPORT void ldg_net_gql_ctx_destroy(ldg_net_gql_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_gql_exec(ldg_net_gql_ctx_t *ctx, const char *body, void *hdrs, ldg_net_curl_resp_t *resp);

#endif
