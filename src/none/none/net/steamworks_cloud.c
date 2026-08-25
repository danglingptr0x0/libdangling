#include <stdint.h>
#include <string.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_cloud.h>

extern void* SteamAPI_SteamRemoteStorage_v016(void);
extern uint8_t SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount(void *self);
extern uint8_t SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp(void *self);
extern uint8_t SteamAPI_ISteamRemoteStorage_GetQuota(void *self, uint64_t *total, uint64_t *available);
extern int32_t SteamAPI_ISteamRemoteStorage_GetFileCount(void *self);
extern const char* SteamAPI_ISteamRemoteStorage_GetFileNameAndSize(void *self, int32_t idx, int32_t *size);
extern int64_t SteamAPI_ISteamRemoteStorage_GetFileTimestamp(void *self, const char *path);
extern uint8_t SteamAPI_ISteamRemoteStorage_FileExists(void *self, const char *path);
extern int32_t SteamAPI_ISteamRemoteStorage_GetFileSize(void *self, const char *path);
extern uint8_t SteamAPI_ISteamRemoteStorage_FileWrite(void *self, const char *path, const void *data, int32_t size);
extern int32_t SteamAPI_ISteamRemoteStorage_FileRead(void *self, const char *path, void *data, int32_t size);
extern uint8_t SteamAPI_ISteamRemoteStorage_FileDelete(void *self, const char *path);
extern uint8_t SteamAPI_ISteamRemoteStorage_FileForget(void *self, const char *path);
extern uint8_t SteamAPI_ISteamRemoteStorage_SetSyncPlatforms(void *self, const char *path, int32_t pform);

uint32_t ldg_net_steamworks_cloud_init(ldg_net_steamworks_cloud_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_cloud_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_cloud_shutdown(ldg_net_steamworks_cloud_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_cloud_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("net: cloud ctx zero failed"); }
}

uint32_t ldg_net_steamworks_cloud_en_get(uint8_t *account, uint8_t *app)
{
    void *storage = 0x0;

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    if (account) { *account = (SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount(storage) != 0) ? 1 : 0; }

    if (app) { *app = (SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp(storage) != 0) ? 1 : 0; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_cloud_quota_get(uint64_t *total, uint64_t *available)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!total || !available)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamRemoteStorage_GetQuota(storage, total, available) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_cloud_file_enumerate(ldg_net_steamworks_cloud_ctx_t *ctx)
{
    void *storage = 0x0;
    int32_t cunt = 0;
    int32_t iter = 0;
    uint8_t quota_ok = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->file_cunt = 0;
    cunt = SteamAPI_ISteamRemoteStorage_GetFileCount(storage);

    for (iter = 0; iter < cunt && ctx->file_cunt < LDG_NET_STEAMWORKS_CLOUD_FILE_MAX; iter = iter + 1)
    {
        int32_t size = 0;
        const char *name = 0x0;

        name = SteamAPI_ISteamRemoteStorage_GetFileNameAndSize(storage, iter, &size);

        if (name && name[0] != LDG_STR_TERM)
        {
            ldg_net_steamworks_cloud_file_t *file = &ctx->file[ctx->file_cunt];
            uint32_t copy_err = 0;

            copy_err = ldg_str_rbrcpy(file->name, name, sizeof(file->name));
            if (LDG_UNLIKELY(copy_err != LDG_ERR_AOK && copy_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("net: cloud file name copy failed"); }

            file->size = size;
            file->timestamp = SteamAPI_ISteamRemoteStorage_GetFileTimestamp(storage, name);
            ctx->file_cunt = ctx->file_cunt + 1;
        }
    }

    quota_ok = SteamAPI_ISteamRemoteStorage_GetQuota(storage, &ctx->quota_total, &ctx->quota_available);
    if (LDG_UNLIKELY(quota_ok == 0)) { LDG_ERRLOG_WARN("net: cloud quota query failed"); }

    ctx->cloud_en_account = (SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount(storage) != 0) ? 1 : 0;
    ctx->cloud_en_app = (SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp(storage) != 0) ? 1 : 0;

    return LDG_ERR_AOK;
}

uint8_t ldg_net_steamworks_cloud_file_exists_is(const char *path)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NOT_INIT; }

    if (SteamAPI_ISteamRemoteStorage_FileExists(storage, path) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint64_t ldg_net_steamworks_cloud_file_size_get(const char *path)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path)) { return UINT64_MAX; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return UINT64_MAX; }

    return (uint64_t)SteamAPI_ISteamRemoteStorage_GetFileSize(storage, path);
}

int64_t ldg_net_steamworks_cloud_file_timestamp_get(const char *path)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path)) { return 0; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return 0; }

    return SteamAPI_ISteamRemoteStorage_GetFileTimestamp(storage, path);
}

uint32_t ldg_net_steamworks_cloud_file_wr(const char *path, const void *data, int32_t size)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path || !data || size <= 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamRemoteStorage_FileWrite(storage, path, data, size) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_cloud_file_rd(const char *path, void *data, int32_t size)
{
    void *storage = 0x0;
    int32_t rd_cunt = 0;

    if (LDG_UNLIKELY(!path || !data || size <= 0)) { return UINT32_MAX; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return UINT32_MAX; }

    rd_cunt = SteamAPI_ISteamRemoteStorage_FileRead(storage, path, data, size);
    if (rd_cunt <= 0) { return UINT32_MAX; }

    return (uint32_t)rd_cunt;
}

uint32_t ldg_net_steamworks_cloud_file_delete(const char *path)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamRemoteStorage_FileDelete(storage, path) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_cloud_file_forget(const char *path)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamRemoteStorage_FileForget(storage, path) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_cloud_sync_pform_set(const char *path, ldg_net_steamworks_cloud_pform_t pform)
{
    void *storage = 0x0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    storage = SteamAPI_SteamRemoteStorage_v016();
    if (LDG_UNLIKELY(!storage)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamRemoteStorage_SetSyncPlatforms(storage, path, (int32_t)pform) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}
