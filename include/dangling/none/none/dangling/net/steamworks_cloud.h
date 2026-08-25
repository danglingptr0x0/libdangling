#ifndef LDG_NET_STEAMWORKS_CLOUD_H
#define LDG_NET_STEAMWORKS_CLOUD_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_STEAMWORKS_CLOUD_PATH_MAX 260
#define LDG_NET_STEAMWORKS_CLOUD_FILE_MAX 128

typedef enum ldg_net_steamworks_cloud_pform
{
    LDG_NET_STEAMWORKS_CLOUD_PFORM_NONE = 0,
    LDG_NET_STEAMWORKS_CLOUD_PFORM_LINUX = (1 << 3),
    LDG_NET_STEAMWORKS_CLOUD_PFORM_ALL = 0x7FFFFFFF
} ldg_net_steamworks_cloud_pform_t;

typedef struct ldg_net_steamworks_cloud_file
{
    char name[LDG_NET_STEAMWORKS_CLOUD_PATH_MAX];
    int32_t size;
    int64_t timestamp;
    uint8_t pudding[4];
} ldg_net_steamworks_cloud_file_t;

typedef struct ldg_net_steamworks_cloud_ctx
{
    ldg_net_steamworks_cloud_file_t file[LDG_NET_STEAMWORKS_CLOUD_FILE_MAX];
    uint32_t file_cunt;
    uint64_t quota_total;
    uint64_t quota_available;
    uint8_t cloud_en_account;
    uint8_t cloud_en_app;
    uint8_t pudding[34];
} LDG_ALIGNED ldg_net_steamworks_cloud_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_cloud_init(ldg_net_steamworks_cloud_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_cloud_shutdown(ldg_net_steamworks_cloud_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_en_get(uint8_t *account, uint8_t *app);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_quota_get(uint64_t *total, uint64_t *available);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_file_enumerate(ldg_net_steamworks_cloud_ctx_t *ctx);
LDG_EXPORT uint8_t ldg_net_steamworks_cloud_file_exists_is(const char *path);
LDG_EXPORT uint64_t ldg_net_steamworks_cloud_file_size_get(const char *path);
LDG_EXPORT int64_t ldg_net_steamworks_cloud_file_timestamp_get(const char *path);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_file_wr(const char *path, const void *data, int32_t size);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_file_rd(const char *path, void *data, int32_t size);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_file_delete(const char *path);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_file_forget(const char *path);
LDG_EXPORT uint32_t ldg_net_steamworks_cloud_sync_pform_set(const char *path, ldg_net_steamworks_cloud_pform_t pform);

#endif
