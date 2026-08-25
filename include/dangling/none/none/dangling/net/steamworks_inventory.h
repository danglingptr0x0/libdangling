#ifndef LDG_NET_STEAMWORKS_INVENTORY_H
#define LDG_NET_STEAMWORKS_INVENTORY_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_STEAMWORKS_INVENTORY_ITEM_MAX 256
#define LDG_NET_STEAMWORKS_INVENTORY_PROP_NAME_MAX 64
#define LDG_NET_STEAMWORKS_INVENTORY_PROP_VAL_MAX 256

typedef int32_t ldg_net_steamworks_inventory_res_t;
typedef int32_t ldg_net_steamworks_inventory_itemdef_t;
typedef uint64_t ldg_net_steamworks_inventory_instance_t;

#define LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID ((ldg_net_steamworks_inventory_res_t)-1)
#define LDG_NET_STEAMWORKS_INVENTORY_INSTANCE_INVALID ((ldg_net_steamworks_inventory_instance_t) ~0ULL)

typedef enum ldg_net_steamworks_inventory_status
{
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING = 0,
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_OK = 1,
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_EXPIRED = 2,
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_INVALID = 3,
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_SERVICE_UNAVAIL = 4,
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_LIMIT_EXCEEDED = 5,
    LDG_NET_STEAMWORKS_INVENTORY_STATUS_FAIL = 6
} ldg_net_steamworks_inventory_status_t;

typedef enum ldg_net_steamworks_inventory_item_flags
{
    LDG_NET_STEAMWORKS_INVENTORY_FLAG_NONE = 0,
    LDG_NET_STEAMWORKS_INVENTORY_FLAG_NO_TRADE = (1 << 0),
    LDG_NET_STEAMWORKS_INVENTORY_FLAG_REMOVED = (1 << 8),
    LDG_NET_STEAMWORKS_INVENTORY_FLAG_CONSUMED = (1 << 9)
} ldg_net_steamworks_inventory_item_flags_t;

typedef struct ldg_net_steamworks_inventory_item
{
    ldg_net_steamworks_inventory_instance_t instance_id;
    ldg_net_steamworks_inventory_itemdef_t def;
    uint16_t quantity;
    uint16_t flags;
} ldg_net_steamworks_inventory_item_t;

typedef struct ldg_net_steamworks_inventory_ctx
{
    ldg_net_steamworks_inventory_res_t res;
    uint32_t item_cunt;
    ldg_net_steamworks_inventory_status_t status;
    uint32_t timestamp;
    ldg_net_steamworks_inventory_item_t item[LDG_NET_STEAMWORKS_INVENTORY_ITEM_MAX];
    uint8_t pudding[48];
} LDG_ALIGNED ldg_net_steamworks_inventory_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_inventory_init(ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_inventory_shutdown(ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_def_ld(void);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_all_get(ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_item_by_id_get(ldg_net_steamworks_inventory_ctx_t *ctx, const ldg_net_steamworks_inventory_instance_t *id, uint32_t cunt);
LDG_EXPORT ldg_net_steamworks_inventory_status_t ldg_net_steamworks_inventory_status_get(ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_item_cunt_get(const ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT const ldg_net_steamworks_inventory_item_t* ldg_net_steamworks_inventory_item_get(const ldg_net_steamworks_inventory_ctx_t *ctx, uint32_t idx);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_item_consume(ldg_net_steamworks_inventory_ctx_t *ctx, ldg_net_steamworks_inventory_instance_t instance, uint32_t quantity);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_item_drop(ldg_net_steamworks_inventory_ctx_t *ctx, ldg_net_steamworks_inventory_itemdef_t droplist_def);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_item_generate(ldg_net_steamworks_inventory_ctx_t *ctx, const ldg_net_steamworks_inventory_itemdef_t *def, const uint32_t *quantity, uint32_t cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_item_xchg(ldg_net_steamworks_inventory_ctx_t *ctx, ldg_net_steamworks_inventory_itemdef_t output_def, const ldg_net_steamworks_inventory_instance_t *input_id, const uint32_t *input_quantity, uint32_t input_cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_promo_grant(ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_inventory_def_prop_get(ldg_net_steamworks_inventory_itemdef_t def, const char *prop_name, char *out_val, uint32_t *out_len);
LDG_EXPORT void ldg_net_steamworks_inventory_res_destroy(ldg_net_steamworks_inventory_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_inventory_poll(ldg_net_steamworks_inventory_ctx_t *ctx);

#endif
