#ifndef LDG_NET_STEAMWORKS_LBOARD_H
#define LDG_NET_STEAMWORKS_LBOARD_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_STEAMWORKS_LBOARD_ENTRY_MAX 16
#define LDG_NET_STEAMWORKS_LBOARD_NAME_MAX 64
#define LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX 4

typedef enum ldg_net_steamworks_lboard_sort
{
    LDG_NET_STEAMWORKS_LBOARD_SORT_NONE = 0,
    LDG_NET_STEAMWORKS_LBOARD_SORT_ASC = 1,
    LDG_NET_STEAMWORKS_LBOARD_SORT_DESC = 2
} ldg_net_steamworks_lboard_sort_t;

typedef enum ldg_net_steamworks_lboard_display
{
    LDG_NET_STEAMWORKS_LBOARD_DISPLAY_NONE = 0,
    LDG_NET_STEAMWORKS_LBOARD_DISPLAY_NUMERIC = 1,
    LDG_NET_STEAMWORKS_LBOARD_DISPLAY_SECONDS = 2,
    LDG_NET_STEAMWORKS_LBOARD_DISPLAY_MS = 3
} ldg_net_steamworks_lboard_display_t;

typedef enum ldg_net_steamworks_lboard_req
{
    LDG_NET_STEAMWORKS_LBOARD_REQ_GLOBAL = 0,
    LDG_NET_STEAMWORKS_LBOARD_REQ_AROUND_USER = 1,
    LDG_NET_STEAMWORKS_LBOARD_REQ_FRIENDS = 2
} ldg_net_steamworks_lboard_req_t;

typedef enum ldg_net_steamworks_lboard_upload
{
    LDG_NET_STEAMWORKS_LBOARD_UPLOAD_NONE = 0,
    LDG_NET_STEAMWORKS_LBOARD_UPLOAD_KEEP_BEST = 1,
    LDG_NET_STEAMWORKS_LBOARD_UPLOAD_FORCE = 2
} ldg_net_steamworks_lboard_upload_t;

typedef enum ldg_net_steamworks_lboard_state
{
    LDG_NET_STEAMWORKS_LBOARD_STATE_IDLE = 0,
    LDG_NET_STEAMWORKS_LBOARD_STATE_FINDING = 1,
    LDG_NET_STEAMWORKS_LBOARD_STATE_DOWNLOADING = 2,
    LDG_NET_STEAMWORKS_LBOARD_STATE_UPLOADING = 3,
    LDG_NET_STEAMWORKS_LBOARD_STATE_READY = 4,
    LDG_NET_STEAMWORKS_LBOARD_STATE_ERR = 5
} ldg_net_steamworks_lboard_state_t;

typedef struct ldg_net_steamworks_lboard_entry
{
    uint64_t steam_id;
    int32_t global_rank;
    int32_t score;
    int32_t detail[LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX];
    int32_t detail_cunt;
    uint8_t pudding[4];
} LDG_ALIGNED ldg_net_steamworks_lboard_entry_t;

typedef struct ldg_net_steamworks_lboard_ctx
{
    uint64_t handle;
    uint64_t entry_handle;
    uint64_t api_call;
    char name[LDG_NET_STEAMWORKS_LBOARD_NAME_MAX];
    ldg_net_steamworks_lboard_entry_t entry[LDG_NET_STEAMWORKS_LBOARD_ENTRY_MAX];
    int32_t entry_cunt;
    int32_t total_cunt;
    ldg_net_steamworks_lboard_state_t state;
    uint8_t upload_success;
    uint8_t score_changed;
    int32_t new_rank;
    int32_t prev_rank;
    uint8_t pudding[4];
} LDG_ALIGNED ldg_net_steamworks_lboard_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_lboard_init(ldg_net_steamworks_lboard_ctx_t *lboard);
LDG_EXPORT void ldg_net_steamworks_lboard_shutdown(ldg_net_steamworks_lboard_ctx_t *lboard);
LDG_EXPORT uint32_t ldg_net_steamworks_lboard_find(ldg_net_steamworks_lboard_ctx_t *lboard, const char *name);
LDG_EXPORT uint32_t ldg_net_steamworks_lboard_download(ldg_net_steamworks_lboard_ctx_t *lboard, ldg_net_steamworks_lboard_req_t type, int32_t start, int32_t end);
LDG_EXPORT uint32_t ldg_net_steamworks_lboard_upload(ldg_net_steamworks_lboard_ctx_t *lboard, int32_t score, ldg_net_steamworks_lboard_upload_t method, const int32_t *detail, uint32_t detail_cunt);
LDG_EXPORT ldg_net_steamworks_lboard_state_t ldg_net_steamworks_lboard_state_get(const ldg_net_steamworks_lboard_ctx_t *lboard);
LDG_EXPORT uint32_t ldg_net_steamworks_lboard_entry_cunt_get(const ldg_net_steamworks_lboard_ctx_t *lboard);
LDG_EXPORT const ldg_net_steamworks_lboard_entry_t* ldg_net_steamworks_lboard_entry_get(const ldg_net_steamworks_lboard_ctx_t *lboard, uint32_t idx);
LDG_EXPORT void ldg_net_steamworks_lboard_call_handle(ldg_net_steamworks_lboard_ctx_t *lboard, int32_t pipe, uint64_t call);

#endif
