#ifndef LDG_NET_STEAMWORKS_WSHOP_H
#define LDG_NET_STEAMWORKS_WSHOP_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX 64
#define LDG_NET_STEAMWORKS_WSHOP_TAG_MAX 32
#define LDG_NET_STEAMWORKS_WSHOP_PATH_MAX 260
#define LDG_NET_STEAMWORKS_WSHOP_TYPE_MAX 8
#define LDG_NET_STEAMWORKS_WSHOP_MARKER_MAX 64

#define LDG_NET_STEAMWORKS_WSHOP_TYPE_UNKNOWN 0

typedef uint32_t ldg_net_steamworks_wshop_type_t;

typedef struct ldg_net_steamworks_wshop_type_desc
{
    const char *tag;
    const char *marker;
} ldg_net_steamworks_wshop_type_desc_t;

typedef struct ldg_net_steamworks_wshop_type_reg
{
    char tag[LDG_NET_STEAMWORKS_WSHOP_TAG_MAX];
    char marker[LDG_NET_STEAMWORKS_WSHOP_MARKER_MAX];
} ldg_net_steamworks_wshop_type_reg_t;

typedef struct ldg_net_steamworks_wshop_item
{
    uint64_t file_id;
    uint64_t size;
    char title[128];
    char path[LDG_NET_STEAMWORKS_WSHOP_PATH_MAX];
    char tag[LDG_NET_STEAMWORKS_WSHOP_TAG_MAX];
    ldg_net_steamworks_wshop_type_t type;
    uint32_t state;
    uint32_t timestamp;
    uint8_t installed;
    uint8_t pudding[63];
} LDG_ALIGNED ldg_net_steamworks_wshop_item_t;

typedef enum ldg_net_steamworks_wshop_upload_state
{
    LDG_NET_STEAMWORKS_WSHOP_UPLOAD_IDLE = 0,
    LDG_NET_STEAMWORKS_WSHOP_UPLOAD_CREATING = 1,
    LDG_NET_STEAMWORKS_WSHOP_UPLOAD_UPDATING = 2,
    LDG_NET_STEAMWORKS_WSHOP_UPLOAD_DONE = 3,
    LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR = 4
} ldg_net_steamworks_wshop_upload_state_t;

typedef struct ldg_net_steamworks_wshop_upload_ctx
{
    uint64_t file_id;
    uint64_t api_call;
    uint64_t update_handle;
    uint64_t bytes_done;
    uint64_t bytes_total;
    ldg_net_steamworks_wshop_upload_state_t state;
    ldg_net_steamworks_wshop_type_t type;
    char title[128];
    char desc[256];
    char content_path[LDG_NET_STEAMWORKS_WSHOP_PATH_MAX];
    char preview_path[LDG_NET_STEAMWORKS_WSHOP_PATH_MAX];
    uint8_t pudding[8];
} LDG_ALIGNED ldg_net_steamworks_wshop_upload_ctx_t;

typedef struct ldg_net_steamworks_wshop_ctx
{
    ldg_net_steamworks_wshop_item_t item[LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX];
    ldg_net_steamworks_wshop_type_reg_t type_reg[LDG_NET_STEAMWORKS_WSHOP_TYPE_MAX];
    ldg_net_steamworks_wshop_upload_ctx_t *upload_pending;
    uint32_t item_cunt;
    uint32_t type_cunt;
    uint8_t is_init;
    uint8_t pudding[47];
} LDG_ALIGNED ldg_net_steamworks_wshop_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_wshop_init(ldg_net_steamworks_wshop_ctx_t *wshop, const ldg_net_steamworks_wshop_type_desc_t *type_desc, uint32_t type_cunt);
LDG_EXPORT void ldg_net_steamworks_wshop_shutdown(ldg_net_steamworks_wshop_ctx_t *wshop);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_refresh(ldg_net_steamworks_wshop_ctx_t *wshop);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_item_cunt_get(const ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_type_t type);
LDG_EXPORT const ldg_net_steamworks_wshop_item_t* ldg_net_steamworks_wshop_item_get(const ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_type_t type, uint32_t idx);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_item_download(uint64_t file_id);
LDG_EXPORT void ldg_net_steamworks_wshop_poll(ldg_net_steamworks_wshop_ctx_t *wshop);

LDG_EXPORT uint32_t ldg_net_steamworks_wshop_upload_init(ldg_net_steamworks_wshop_upload_ctx_t *upload);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_upload_start(ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_upload_ctx_t *upload, ldg_net_steamworks_wshop_type_t type, const char *title, const char *desc, const char *content_path, const char *preview_path);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_upload_update(ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_upload_ctx_t *upload, const char *notes);
LDG_EXPORT void ldg_net_steamworks_wshop_call_handle(ldg_net_steamworks_wshop_ctx_t *wshop, int32_t pipe, uint64_t call);
LDG_EXPORT ldg_net_steamworks_wshop_upload_state_t ldg_net_steamworks_wshop_upload_state_get(const ldg_net_steamworks_wshop_upload_ctx_t *upload);

LDG_EXPORT uint32_t ldg_net_steamworks_wshop_playtime_start(uint64_t *file_id, uint32_t cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_playtime_stop(uint64_t *file_id, uint32_t cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_wshop_playtime_all_stop(void);

#endif
