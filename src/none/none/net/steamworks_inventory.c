#include <stdint.h>
#include <string.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_inventory.h>

#define STEAM_ERES_OK 1
#define STEAM_ERES_FAIL 3
#define STEAM_ERES_INVALID_PARAM 8
#define STEAM_ERES_SERVICE_UNAVAILABLE 20
#define STEAM_ERES_LIMIT_EXCEEDED 25
#define STEAM_ERES_EXPIRED 27

extern void* SteamAPI_SteamInventory_v003(void);
extern uint8_t SteamAPI_ISteamInventory_LoadItemDefinitions(void *self);
extern uint8_t SteamAPI_ISteamInventory_GetAllItems(void *self, int32_t *res);
extern uint8_t SteamAPI_ISteamInventory_GetItemsByID(void *self, int32_t *res, const uint64_t *id, uint32_t cunt);
extern int32_t SteamAPI_ISteamInventory_GetResultStatus(void *self, int32_t res);
extern uint8_t SteamAPI_ISteamInventory_GetResultItems(void *self, int32_t res, void *item, uint32_t *cunt);
extern uint32_t SteamAPI_ISteamInventory_GetResultTimestamp(void *self, int32_t res);
extern void SteamAPI_ISteamInventory_DestroyResult(void *self, int32_t res);
extern uint8_t SteamAPI_ISteamInventory_ConsumeItem(void *self, int32_t *res, uint64_t instance, uint32_t quantity);
extern uint8_t SteamAPI_ISteamInventory_TriggerItemDrop(void *self, int32_t *res, int32_t droplist_def);
extern uint8_t SteamAPI_ISteamInventory_GenerateItems(void *self, int32_t *res, const int32_t *def, const uint32_t *quantity, uint32_t cunt);
extern uint8_t SteamAPI_ISteamInventory_ExchangeItems(void *self, int32_t *res, const int32_t *output_def, const uint32_t *output_qty, uint32_t output_cunt, const uint64_t *input_id, const uint32_t *input_qty, uint32_t input_cunt);
extern uint8_t SteamAPI_ISteamInventory_GrantPromoItems(void *self, int32_t *res);
extern uint8_t SteamAPI_ISteamInventory_GetItemDefinitionProperty(void *self, int32_t def, const char *prop_name, char *out_val, uint32_t *out_len);

typedef struct inventory_item_steam
{
    uint64_t instance_id;
    int32_t def;
    uint16_t quantity;
    uint16_t flags;
} inventory_item_steam_t;

static ldg_net_steamworks_inventory_status_t status_convert(int32_t eres)
{
    switch (eres)
    {
        case STEAM_ERES_OK:
            return LDG_NET_STEAMWORKS_INVENTORY_STATUS_OK;
        case STEAM_ERES_EXPIRED:
            return LDG_NET_STEAMWORKS_INVENTORY_STATUS_EXPIRED;
        case STEAM_ERES_INVALID_PARAM:
            return LDG_NET_STEAMWORKS_INVENTORY_STATUS_INVALID;
        case STEAM_ERES_FAIL:
        case STEAM_ERES_SERVICE_UNAVAILABLE:
            return LDG_NET_STEAMWORKS_INVENTORY_STATUS_SERVICE_UNAVAIL;
        case STEAM_ERES_LIMIT_EXCEEDED:
            return LDG_NET_STEAMWORKS_INVENTORY_STATUS_LIMIT_EXCEEDED;
        default:
            return LDG_NET_STEAMWORKS_INVENTORY_STATUS_FAIL;
    }
}

uint32_t ldg_net_steamworks_inventory_init(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_inventory_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->res = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_inventory_shutdown(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    ldg_net_steamworks_inventory_res_destroy(ctx);
    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_inventory_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("net: inventory ctx zero failed"); }
}

uint32_t ldg_net_steamworks_inventory_def_ld(void)
{
    void *inv = 0x0;

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamInventory_LoadItemDefinitions(inv) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_all_get(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_GetAllItems(inv, &handle) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;
    ctx->item_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_item_by_id_get(ldg_net_steamworks_inventory_ctx_t *ctx, const ldg_net_steamworks_inventory_instance_t *id, uint32_t cunt)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!id || cunt == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_GetItemsByID(inv, &handle, id, cunt) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;
    ctx->item_cunt = 0;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_inventory_status_t ldg_net_steamworks_inventory_status_get(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    void *inv = 0x0;
    int32_t eres = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_INVENTORY_STATUS_FAIL; }

    if (ctx->res == LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID) { return LDG_NET_STEAMWORKS_INVENTORY_STATUS_FAIL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_NET_STEAMWORKS_INVENTORY_STATUS_FAIL; }

    eres = SteamAPI_ISteamInventory_GetResultStatus(inv, ctx->res);
    ctx->status = status_convert(eres);

    return ctx->status;
}

uint32_t ldg_net_steamworks_inventory_item_cunt_get(const ldg_net_steamworks_inventory_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ctx->item_cunt;
}

const ldg_net_steamworks_inventory_item_t* ldg_net_steamworks_inventory_item_get(const ldg_net_steamworks_inventory_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0x0; }

    if (idx >= ctx->item_cunt) { return 0x0; }

    return &ctx->item[idx];
}

uint32_t ldg_net_steamworks_inventory_item_consume(ldg_net_steamworks_inventory_ctx_t *ctx, ldg_net_steamworks_inventory_instance_t instance, uint32_t quantity)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (instance == LDG_NET_STEAMWORKS_INVENTORY_INSTANCE_INVALID) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_ConsumeItem(inv, &handle, instance, quantity) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_item_drop(ldg_net_steamworks_inventory_ctx_t *ctx, ldg_net_steamworks_inventory_itemdef_t droplist_def)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_TriggerItemDrop(inv, &handle, droplist_def) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_item_generate(ldg_net_steamworks_inventory_ctx_t *ctx, const ldg_net_steamworks_inventory_itemdef_t *def, const uint32_t *quantity, uint32_t cunt)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!def || cunt == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_GenerateItems(inv, &handle, def, quantity, cunt) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_item_xchg(ldg_net_steamworks_inventory_ctx_t *ctx, ldg_net_steamworks_inventory_itemdef_t output_def, const ldg_net_steamworks_inventory_instance_t *input_id, const uint32_t *input_quantity, uint32_t input_cunt)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;
    uint32_t output_qty = 1;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!input_id || !input_quantity || input_cunt == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_ExchangeItems(inv, &handle, &output_def, &output_qty, 1, input_id, input_quantity, input_cunt) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_promo_grant(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(ctx);

    if (SteamAPI_ISteamInventory_GrantPromoItems(inv, &handle) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->res = handle;
    ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_inventory_def_prop_get(ldg_net_steamworks_inventory_itemdef_t def, const char *prop_name, char *out_val, uint32_t *out_len)
{
    void *inv = 0x0;

    if (LDG_UNLIKELY(!out_val || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamInventory_GetItemDefinitionProperty(inv, def, prop_name, out_val, out_len) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_inventory_res_destroy(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    void *inv = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->res == LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID) { return; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_LIKELY(inv)) { SteamAPI_ISteamInventory_DestroyResult(inv, ctx->res); }

    ctx->res = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;
}

void ldg_net_steamworks_inventory_poll(ldg_net_steamworks_inventory_ctx_t *ctx)
{
    void *inv = 0x0;
    int32_t eres = 0;
    uint32_t cunt = 0;
    uint32_t iter = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->res == LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID) { return; }

    if (ctx->status != LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING) { return; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return; }

    eres = SteamAPI_ISteamInventory_GetResultStatus(inv, ctx->res);
    if (eres == 0) { return; }

    ctx->status = status_convert(eres);

    if (ctx->status == LDG_NET_STEAMWORKS_INVENTORY_STATUS_OK || ctx->status == LDG_NET_STEAMWORKS_INVENTORY_STATUS_EXPIRED)
    {
        ctx->timestamp = SteamAPI_ISteamInventory_GetResultTimestamp(inv, ctx->res);

        cunt = 0;
        if (SteamAPI_ISteamInventory_GetResultItems(inv, ctx->res, 0x0, &cunt) != 0)
        {
            inventory_item_steam_t *steam_item = 0x0;
            uint32_t fetch_cunt = 0;

            if (cunt > LDG_NET_STEAMWORKS_INVENTORY_ITEM_MAX) { cunt = LDG_NET_STEAMWORKS_INVENTORY_ITEM_MAX; }

            fetch_cunt = cunt;

            if (cunt > 0 && ldg_mem_alloc(sizeof(inventory_item_steam_t) * cunt, (void **)&steam_item) == LDG_ERR_AOK)
            {
                if (SteamAPI_ISteamInventory_GetResultItems(inv, ctx->res, steam_item, &fetch_cunt) != 0)
                {
                    ctx->item_cunt = fetch_cunt;
                    for (iter = 0; iter < fetch_cunt; iter = iter + 1)
                    {
                        ctx->item[iter].instance_id = steam_item[iter].instance_id;
                        ctx->item[iter].def = steam_item[iter].def;
                        ctx->item[iter].quantity = steam_item[iter].quantity;
                        ctx->item[iter].flags = steam_item[iter].flags;
                    }
                }

                ldg_mem_dealloc(steam_item);
                steam_item = 0x0;
            }
        }
    }
}
