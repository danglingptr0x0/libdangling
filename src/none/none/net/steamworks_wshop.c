#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/io/path.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_wshop.h>

#define STEAM_ITEM_STATE_SUBSCRIBED 1
#define STEAM_ITEM_STATE_INSTALLED 4

#define STEAM_CB_CREATE_ITEM 3403
#define STEAM_CB_SUBMIT_ITEM_UPDATE 3404
#define STEAM_UGC_FILE_TYPE_COMMUNITY 0
#define STEAM_UGC_VISIBILITY_PUBLIC 0
#define STEAM_RES_OK 1
#define STEAM_CK_PATH_SIZE 512

typedef struct wshop_create_res
{
    int32_t res;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t file_id;
    uint8_t needs_agree;
#ifdef _WIN32
    uint8_t pudding1[7];
#else
    uint8_t pudding1[3];
#endif
} LDG_PACKED wshop_create_res_t;

typedef struct wshop_submit_res
{
    int32_t res;
    uint8_t needs_agree;
    uint8_t pudding0[3];
    uint64_t file_id;
} LDG_PACKED wshop_submit_res_t;

typedef struct wshop_tag_param
{
    const char **strings;
    int32_t cunt;
} wshop_tag_param_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

#ifdef _WIN32
STEAMWORKS_LAYOUT_CK(create_res_size, sizeof(wshop_create_res_t) == 24);
#else
STEAMWORKS_LAYOUT_CK(create_res_size, sizeof(wshop_create_res_t) == 16);
#endif
STEAMWORKS_LAYOUT_CK(submit_res_size, sizeof(wshop_submit_res_t) == 16);

extern void* SteamAPI_SteamUGC_v021(void);
extern uint32_t SteamAPI_ISteamUGC_GetNumSubscribedItems(void *self, uint8_t include_disabled);
extern uint32_t SteamAPI_ISteamUGC_GetSubscribedItems(void *self, uint64_t *id, uint32_t max, uint8_t include_disabled);
extern uint32_t SteamAPI_ISteamUGC_GetItemState(void *self, uint64_t file_id);
extern uint8_t SteamAPI_ISteamUGC_GetItemInstallInfo(void *self, uint64_t file_id, uint64_t *size, char *folder, uint32_t folder_size, uint32_t *timestamp);
extern uint8_t SteamAPI_ISteamUGC_DownloadItem(void *self, uint64_t file_id, uint8_t high_priority);
extern uint64_t SteamAPI_ISteamUGC_CreateItem(void *self, uint32_t app_id, int32_t file_type);
extern uint64_t SteamAPI_ISteamUGC_StartItemUpdate(void *self, uint32_t app_id, uint64_t file_id);
extern uint8_t SteamAPI_ISteamUGC_SetItemTitle(void *self, uint64_t handle, const char *title);
extern uint8_t SteamAPI_ISteamUGC_SetItemDescription(void *self, uint64_t handle, const char *desc);
extern uint8_t SteamAPI_ISteamUGC_SetItemContent(void *self, uint64_t handle, const char *folder);
extern uint8_t SteamAPI_ISteamUGC_SetItemPreview(void *self, uint64_t handle, const char *preview);
extern uint8_t SteamAPI_ISteamUGC_SetItemTags(void *self, uint64_t handle, const void *tag);
extern uint8_t SteamAPI_ISteamUGC_SetItemVisibility(void *self, uint64_t handle, int32_t visibility);
extern uint64_t SteamAPI_ISteamUGC_SubmitItemUpdate(void *self, uint64_t handle, const char *notes);
extern uint8_t SteamAPI_ISteamUGC_GetItemUpdateProgress(void *self, uint64_t handle, uint64_t *bytes_done, uint64_t *bytes_total);
extern uint64_t SteamAPI_ISteamUGC_StartPlaytimeTracking(void *self, uint64_t *file_id, uint32_t cunt);
extern uint64_t SteamAPI_ISteamUGC_StopPlaytimeTracking(void *self, uint64_t *file_id, uint32_t cunt);
extern uint64_t SteamAPI_ISteamUGC_StopPlaytimeTrackingForAllItems(void *self);
extern uint8_t SteamAPI_ManualDispatch_GetAPICallResult(int32_t pipe, uint64_t call, void *cb, int32_t cb_size, int32_t cb_expected, uint8_t *failed);
extern void* SteamAPI_SteamUtils_v010(void);
extern uint32_t SteamAPI_ISteamUtils_GetAppID(void *self);

static uint32_t app_id_get(void)
{
    void *utils = 0x0;

    utils = SteamAPI_SteamUtils_v010();
    if (LDG_UNLIKELY(!utils)) { return 0; }

    return SteamAPI_ISteamUtils_GetAppID(utils);
}

static ldg_net_steamworks_wshop_type_t item_type_detect(const ldg_net_steamworks_wshop_ctx_t *wshop, const char *path)
{
    char ck_path[STEAM_CK_PATH_SIZE] = LDG_ARR_ZERO_INIT;
    int32_t written = 0;
    uint32_t iter = 0;

    for (iter = 0; iter < wshop->type_cunt; iter = iter + 1)
    {
        written = snprintf(ck_path, sizeof(ck_path) - 1, "%.400s/%.63s", path, wshop->type_reg[iter].marker);
        if (LDG_UNLIKELY(written < 0)) { continue; }

        if (ldg_io_path_exists_is(ck_path) == LDG_TRUTH_TRUE) { return iter + 1; }
    }

    return LDG_NET_STEAMWORKS_WSHOP_TYPE_UNKNOWN;
}

static uint32_t item_find(const ldg_net_steamworks_wshop_ctx_t *wshop, uint64_t file_id)
{
    uint32_t iter = 0;

    for (iter = 0; iter < wshop->item_cunt; iter = iter + 1) { if (wshop->item[iter].file_id == file_id) { return iter; } }

    return UINT32_MAX;
}

static uint32_t item_fill(ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_item_t *item, void *ugc, uint64_t file_id, uint32_t state)
{
    uint64_t size = 0;
    uint32_t timestamp = 0;
    uint32_t err = 0;
    char folder[LDG_NET_STEAMWORKS_WSHOP_PATH_MAX] = LDG_ARR_ZERO_INIT;

    item->file_id = file_id;
    item->state = state;

    if (state & STEAM_ITEM_STATE_INSTALLED) { if (SteamAPI_ISteamUGC_GetItemInstallInfo(ugc, file_id, &size, folder, sizeof(folder), &timestamp) != 0)
        {
            item->installed = 1;
            item->size = size;
            item->timestamp = timestamp;
            err = ldg_str_rbrcpy(item->path, folder, sizeof(item->path));
            if (LDG_UNLIKELY(err != LDG_ERR_AOK && err != LDG_ERR_STR_TRUNC)) { return err; }

            item->type = item_type_detect(wshop, folder);
        }
    }
    else if (state & STEAM_ITEM_STATE_SUBSCRIBED) { if (LDG_UNLIKELY(SteamAPI_ISteamUGC_DownloadItem(ugc, file_id, 0) == 0)) { item->installed = 0; } }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_wshop_init(ldg_net_steamworks_wshop_ctx_t *wshop, const ldg_net_steamworks_wshop_type_desc_t *type_desc, uint32_t type_cunt)
{
    void *ugc = 0x0;
    uint64_t file_id[LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t subscribed_cunt = 0;
    uint32_t got_cunt = 0;
    uint32_t err = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!wshop)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(type_cunt > 0 && !type_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(type_cunt > LDG_NET_STEAMWORKS_WSHOP_TYPE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (wshop->is_init) { return LDG_ERR_AOK; }

    err = ldg_mem_secure_zero(wshop, sizeof(ldg_net_steamworks_wshop_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx < type_cunt; idx = idx + 1)
    {
        if (LDG_UNLIKELY(!type_desc[idx].tag || !type_desc[idx].marker)) { return LDG_ERR_FUNC_ARG_NULL; }

        err = ldg_str_rbrcpy(wshop->type_reg[idx].tag, type_desc[idx].tag, sizeof(wshop->type_reg[idx].tag));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_str_rbrcpy(wshop->type_reg[idx].marker, type_desc[idx].marker, sizeof(wshop->type_reg[idx].marker));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    wshop->type_cunt = type_cunt;

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    subscribed_cunt = SteamAPI_ISteamUGC_GetNumSubscribedItems(ugc, 0);
    if (subscribed_cunt == 0)
    {
        wshop->is_init = 1;
        return LDG_ERR_AOK;
    }

    if (subscribed_cunt > LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX) { subscribed_cunt = LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX; }

    got_cunt = SteamAPI_ISteamUGC_GetSubscribedItems(ugc, file_id, subscribed_cunt, 0);

    for (idx = 0; idx < got_cunt && wshop->item_cunt < LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX; idx = idx + 1)
    {
        uint32_t state = 0;
        ldg_net_steamworks_wshop_item_t *item = 0x0;

        state = SteamAPI_ISteamUGC_GetItemState(ugc, file_id[idx]);
        item = &wshop->item[wshop->item_cunt];

        err = ldg_mem_secure_zero(item, sizeof(ldg_net_steamworks_wshop_item_t));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = item_fill(wshop, item, ugc, file_id[idx], state);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        wshop->item_cunt = wshop->item_cunt + 1;
    }

    wshop->is_init = 1;
    return LDG_ERR_AOK;
}

void ldg_net_steamworks_wshop_shutdown(ldg_net_steamworks_wshop_ctx_t *wshop)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!wshop)) { return; }

    err = ldg_mem_secure_zero(wshop, sizeof(ldg_net_steamworks_wshop_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("net: wshop zero failed"); }
}

uint32_t ldg_net_steamworks_wshop_refresh(ldg_net_steamworks_wshop_ctx_t *wshop)
{
    void *ugc = 0x0;
    uint64_t file_id[LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t seen[LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t subscribed_cunt = 0;
    uint32_t got_cunt = 0;
    uint32_t err = 0;
    uint32_t iter = 0;

    if (LDG_UNLIKELY(!wshop)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (!wshop->is_init) { return LDG_ERR_NOT_INIT; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    subscribed_cunt = SteamAPI_ISteamUGC_GetNumSubscribedItems(ugc, 0);
    if (subscribed_cunt > LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX) { subscribed_cunt = LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX; }

    got_cunt = SteamAPI_ISteamUGC_GetSubscribedItems(ugc, file_id, subscribed_cunt, 0);

    for (iter = 0; iter < got_cunt; iter = iter + 1)
    {
        uint32_t idx = 0;
        uint32_t state = 0;
        ldg_net_steamworks_wshop_item_t *item = 0x0;

        idx = item_find(wshop, file_id[iter]);

        if (idx != UINT32_MAX)
        {
            seen[idx] = 1;
            item = &wshop->item[idx];
            state = SteamAPI_ISteamUGC_GetItemState(ugc, file_id[iter]);
            item->state = state;

            if (!item->installed && (state & STEAM_ITEM_STATE_INSTALLED))
            {
                err = item_fill(wshop, item, ugc, file_id[iter], state);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
            }
        }
        else if (wshop->item_cunt < LDG_NET_STEAMWORKS_WSHOP_ITEM_MAX)
        {
            state = SteamAPI_ISteamUGC_GetItemState(ugc, file_id[iter]);
            item = &wshop->item[wshop->item_cunt];

            err = ldg_mem_secure_zero(item, sizeof(ldg_net_steamworks_wshop_item_t));
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = item_fill(wshop, item, ugc, file_id[iter], state);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            seen[wshop->item_cunt] = 1;
            wshop->item_cunt = wshop->item_cunt + 1;
        }
    }

    for (iter = wshop->item_cunt; iter > 0; iter = iter - 1)
    {
        if (!seen[iter - 1])
        {
            if (iter - 1 < wshop->item_cunt - 1)
            {
                wshop->item[iter - 1] = wshop->item[wshop->item_cunt - 1];
                seen[iter - 1] = seen[wshop->item_cunt - 1];
            }

            wshop->item_cunt = wshop->item_cunt - 1;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_wshop_item_cunt_get(const ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_type_t type)
{
    uint32_t cunt = 0;
    uint32_t iter = 0;

    if (LDG_UNLIKELY(!wshop)) { return 0; }

    if (type == LDG_NET_STEAMWORKS_WSHOP_TYPE_UNKNOWN) { return wshop->item_cunt; }

    for (iter = 0; iter < wshop->item_cunt; iter = iter + 1) { if (wshop->item[iter].type == type) { cunt = cunt + 1; } }

    return cunt;
}

const ldg_net_steamworks_wshop_item_t* ldg_net_steamworks_wshop_item_get(const ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_type_t type, uint32_t idx)
{
    uint32_t cunt = 0;
    uint32_t iter = 0;

    if (LDG_UNLIKELY(!wshop)) { return 0x0; }

    if (type == LDG_NET_STEAMWORKS_WSHOP_TYPE_UNKNOWN)
    {
        if (idx < wshop->item_cunt) { return &wshop->item[idx]; }

        return 0x0;
    }

    for (iter = 0; iter < wshop->item_cunt; iter = iter + 1) { if (wshop->item[iter].type == type)
        {
            if (cunt == idx) { return &wshop->item[iter]; }

            cunt = cunt + 1;
        }
    }

    return 0x0;
}

uint32_t ldg_net_steamworks_wshop_item_download(uint64_t file_id)
{
    void *ugc = 0x0;

    if (file_id == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUGC_DownloadItem(ugc, file_id, 1) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_wshop_poll(ldg_net_steamworks_wshop_ctx_t *wshop)
{
    void *ugc = 0x0;
    uint32_t err = 0;
    uint32_t iter = 0;

    if (LDG_UNLIKELY(!wshop)) { return; }

    if (!wshop->is_init) { return; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return; }

    for (iter = 0; iter < wshop->item_cunt; iter = iter + 1)
    {
        ldg_net_steamworks_wshop_item_t *item = 0x0;
        uint32_t state = 0;
        uint64_t size = 0;
        uint32_t timestamp = 0;
        char folder[LDG_NET_STEAMWORKS_WSHOP_PATH_MAX] = LDG_ARR_ZERO_INIT;

        item = &wshop->item[iter];
        if (item->installed) { continue; }

        state = SteamAPI_ISteamUGC_GetItemState(ugc, item->file_id);
        item->state = state;

        if (state & STEAM_ITEM_STATE_INSTALLED) { if (SteamAPI_ISteamUGC_GetItemInstallInfo(ugc, item->file_id, &size, folder, sizeof(folder), &timestamp) != 0)
            {
                item->installed = 1;
                item->size = size;
                item->timestamp = timestamp;
                err = ldg_str_rbrcpy(item->path, folder, sizeof(item->path));
                if (LDG_UNLIKELY(err != LDG_ERR_AOK && err != LDG_ERR_STR_TRUNC)) { item->path[0] = LDG_STR_TERM; }

                item->type = item_type_detect(wshop, folder);
            }
        }
    }
}

uint32_t ldg_net_steamworks_wshop_upload_init(ldg_net_steamworks_wshop_upload_ctx_t *upload)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!upload)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(upload, sizeof(ldg_net_steamworks_wshop_upload_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_IDLE;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_wshop_upload_start(ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_upload_ctx_t *upload, ldg_net_steamworks_wshop_type_t type, const char *title, const char *desc, const char *content_path, const char *preview_path)
{
    void *ugc = 0x0;
    uint64_t api_call = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!wshop || !upload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!title || !content_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (type == LDG_NET_STEAMWORKS_WSHOP_TYPE_UNKNOWN || type > wshop->type_cunt) { return LDG_ERR_FUNC_ARG_INVALID; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    upload->type = type;
    err = ldg_str_rbrcpy(upload->title, title, sizeof(upload->title));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK && err != LDG_ERR_STR_TRUNC)) { return err; }

    if (desc)
    {
        err = ldg_str_rbrcpy(upload->desc, desc, sizeof(upload->desc));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK && err != LDG_ERR_STR_TRUNC)) { return err; }
    }

    err = ldg_str_rbrcpy(upload->content_path, content_path, sizeof(upload->content_path));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK && err != LDG_ERR_STR_TRUNC)) { return err; }

    if (preview_path)
    {
        err = ldg_str_rbrcpy(upload->preview_path, preview_path, sizeof(upload->preview_path));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK && err != LDG_ERR_STR_TRUNC)) { return err; }
    }

    api_call = SteamAPI_ISteamUGC_CreateItem(ugc, app_id_get(), STEAM_UGC_FILE_TYPE_COMMUNITY);
    if (api_call == 0) { return LDG_ERR_NET_RPC_FAIL; }

    upload->api_call = api_call;
    upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_CREATING;
    wshop->upload_pending = upload;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_wshop_upload_update(ldg_net_steamworks_wshop_ctx_t *wshop, ldg_net_steamworks_wshop_upload_ctx_t *upload, const char *notes)
{
    void *ugc = 0x0;
    uint64_t handle = 0;
    uint64_t api_call = 0;
    uint8_t set_ok = 0;
    const char *tag_arr[2] = { 0x0, 0x0 };
    wshop_tag_param_t tag_param = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!wshop || !upload)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (upload->file_id == 0) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(upload->type == LDG_NET_STEAMWORKS_WSHOP_TYPE_UNKNOWN || upload->type > wshop->type_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    handle = SteamAPI_ISteamUGC_StartItemUpdate(ugc, app_id_get(), upload->file_id);
    if (handle == 0) { return LDG_ERR_NET_RPC_FAIL; }

    upload->update_handle = handle;

    set_ok = SteamAPI_ISteamUGC_SetItemTitle(ugc, handle, upload->title);
    if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("net: wshop set title failed"); }

    if (upload->desc[0] != LDG_STR_TERM)
    {
        set_ok = SteamAPI_ISteamUGC_SetItemDescription(ugc, handle, upload->desc);
        if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("net: wshop set desc failed"); }
    }

    set_ok = SteamAPI_ISteamUGC_SetItemContent(ugc, handle, upload->content_path);
    if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("net: wshop set content failed"); }

    if (upload->preview_path[0] != LDG_STR_TERM)
    {
        set_ok = SteamAPI_ISteamUGC_SetItemPreview(ugc, handle, upload->preview_path);
        if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("net: wshop set preview failed"); }
    }

    tag_arr[0] = wshop->type_reg[upload->type - 1].tag;
    tag_param.strings = tag_arr;
    tag_param.cunt = 1;
    set_ok = SteamAPI_ISteamUGC_SetItemTags(ugc, handle, &tag_param);
    if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("net: wshop set tag failed"); }

    set_ok = SteamAPI_ISteamUGC_SetItemVisibility(ugc, handle, STEAM_UGC_VISIBILITY_PUBLIC);
    if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("net: wshop set visibility failed"); }

    api_call = SteamAPI_ISteamUGC_SubmitItemUpdate(ugc, handle, notes);
    if (api_call == 0)
    {
        upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR;
        return LDG_ERR_NET_RPC_FAIL;
    }

    upload->api_call = api_call;
    upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_UPDATING;
    wshop->upload_pending = upload;

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_wshop_call_handle(ldg_net_steamworks_wshop_ctx_t *wshop, int32_t pipe, uint64_t call)
{
    void *ugc = 0x0;
    uint8_t failed = 0;
    ldg_net_steamworks_wshop_upload_ctx_t *upload = 0x0;

    if (LDG_UNLIKELY(!wshop)) { return; }

    upload = wshop->upload_pending;
    if (upload == 0x0) { return; }

    if (upload->api_call == 0 || call != upload->api_call) { return; }

    if (upload->state != LDG_NET_STEAMWORKS_WSHOP_UPLOAD_CREATING && upload->state != LDG_NET_STEAMWORKS_WSHOP_UPLOAD_UPDATING) { return; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return; }

    if (upload->state == LDG_NET_STEAMWORKS_WSHOP_UPLOAD_CREATING)
    {
        wshop_create_res_t create_res = LDG_STRUCT_ZERO_INIT;

        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, upload->api_call, &create_res, (int32_t)sizeof(create_res), STEAM_CB_CREATE_ITEM, &failed) != 0)
        {
            uint32_t update_err = 0;

            if (failed != 0 || create_res.res != STEAM_RES_OK || create_res.file_id == 0)
            {
                upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR;
                wshop->upload_pending = 0x0;
                return;
            }

            upload->file_id = create_res.file_id;
            update_err = ldg_net_steamworks_wshop_upload_update(wshop, upload, "Initial upload");
            if (LDG_UNLIKELY(update_err != LDG_ERR_AOK)) { upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR; }
        }
    }
    else if (upload->state == LDG_NET_STEAMWORKS_WSHOP_UPLOAD_UPDATING)
    {
        wshop_submit_res_t submit_res = LDG_STRUCT_ZERO_INIT;
        uint8_t progress_ok = 0;

        progress_ok = SteamAPI_ISteamUGC_GetItemUpdateProgress(ugc, upload->update_handle, &upload->bytes_done, &upload->bytes_total);
        if (LDG_UNLIKELY(progress_ok == 0)) { LDG_ERRLOG_WARN("net: wshop update progress query failed"); }

        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, upload->api_call, &submit_res, (int32_t)sizeof(submit_res), STEAM_CB_SUBMIT_ITEM_UPDATE, &failed) != 0)
        {
            if (failed != 0 || submit_res.res != STEAM_RES_OK)
            {
                upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR;
                wshop->upload_pending = 0x0;
                return;
            }

            upload->state = LDG_NET_STEAMWORKS_WSHOP_UPLOAD_DONE;
            wshop->upload_pending = 0x0;
        }
    }
}

ldg_net_steamworks_wshop_upload_state_t ldg_net_steamworks_wshop_upload_state_get(const ldg_net_steamworks_wshop_upload_ctx_t *upload)
{
    if (LDG_UNLIKELY(!upload)) { return LDG_NET_STEAMWORKS_WSHOP_UPLOAD_ERR; }

    return upload->state;
}

uint32_t ldg_net_steamworks_wshop_playtime_start(uint64_t *file_id, uint32_t cunt)
{
    void *ugc = 0x0;

    if (LDG_UNLIKELY(!file_id || cunt == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUGC_StartPlaytimeTracking(ugc, file_id, cunt) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_wshop_playtime_stop(uint64_t *file_id, uint32_t cunt)
{
    void *ugc = 0x0;

    if (LDG_UNLIKELY(!file_id || cunt == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUGC_StopPlaytimeTracking(ugc, file_id, cunt) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_wshop_playtime_all_stop(void)
{
    void *ugc = 0x0;

    ugc = SteamAPI_SteamUGC_v021();
    if (LDG_UNLIKELY(!ugc)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUGC_StopPlaytimeTrackingForAllItems(ugc) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}
