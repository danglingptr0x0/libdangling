#include <stdint.h>
#include <string.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_econ.h>

#define STEAM_CB_MARKET_ELIGIBILITY 166

typedef struct econ_eligibility_res
{
    uint8_t allowed;
    uint8_t pudding0[3];
    int32_t not_allowed_reason;
    uint32_t allowed_at_time;
    int32_t guard_day_cunt;
    int32_t new_dev_day_cunt;
} LDG_PACKED econ_eligibility_res_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

STEAMWORKS_LAYOUT_CK(eligibility_res_size, sizeof(econ_eligibility_res_t) == 20);

extern void* SteamAPI_SteamUser_v023(void);
extern uint64_t SteamAPI_ISteamUser_GetMarketEligibility(void *self);
extern void* SteamAPI_SteamInventory_v003(void);
extern uint8_t SteamAPI_ISteamInventory_TradeItems(void *self, int32_t *res, uint64_t partner, const uint64_t *give_id, const uint32_t *give_qty, uint32_t give_cunt, const uint64_t *get_id, const uint32_t *get_qty, uint32_t get_cunt);
extern uint8_t SteamAPI_ManualDispatch_GetAPICallResult(int32_t pipe, uint64_t call, void *cb, int32_t cb_size, int32_t cb_expected, uint8_t *failed);

uint32_t ldg_net_steamworks_econ_init(ldg_net_steamworks_econ_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_econ_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->state = LDG_NET_STEAMWORKS_ECON_STATE_IDLE;

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_econ_shutdown(ldg_net_steamworks_econ_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_econ_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("net: econ ctx zero failed"); }
}

uint32_t ldg_net_steamworks_econ_market_ck(ldg_net_steamworks_econ_ctx_t *ctx)
{
    void *user = 0x0;
    uint64_t api_call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    user = SteamAPI_SteamUser_v023();
    if (LDG_UNLIKELY(!user)) { return LDG_ERR_NET_RPC_FAIL; }

    api_call = SteamAPI_ISteamUser_GetMarketEligibility(user);
    if (api_call == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->api_call = api_call;
    ctx->state = LDG_NET_STEAMWORKS_ECON_STATE_PENDING;
    ctx->market_allowed = 0;
    ctx->blk_reason = 0;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_econ_state_t ldg_net_steamworks_econ_state_get(const ldg_net_steamworks_econ_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_ECON_STATE_IDLE; }

    return ctx->state;
}

uint8_t ldg_net_steamworks_econ_market_allowed_is(const ldg_net_steamworks_econ_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->market_allowed != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_net_steamworks_econ_blk_reason_get(const ldg_net_steamworks_econ_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ctx->blk_reason;
}

uint32_t ldg_net_steamworks_econ_trade(ldg_net_steamworks_inventory_ctx_t *inv_ctx, uint64_t partner_steam_id, const ldg_net_steamworks_inventory_instance_t *give_id, const uint32_t *give_qty, uint32_t give_cunt, const ldg_net_steamworks_inventory_instance_t *get_id, const uint32_t *get_qty, uint32_t get_cunt)
{
    void *inv = 0x0;
    int32_t handle = LDG_NET_STEAMWORKS_INVENTORY_RES_INVALID;

    if (LDG_UNLIKELY(!inv_ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (partner_steam_id == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = SteamAPI_SteamInventory_v003();
    if (LDG_UNLIKELY(!inv)) { return LDG_ERR_NET_RPC_FAIL; }

    ldg_net_steamworks_inventory_res_destroy(inv_ctx);

    if (SteamAPI_ISteamInventory_TradeItems(inv, &handle, partner_steam_id, give_id, give_qty, give_cunt, get_id, get_qty, get_cunt) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    inv_ctx->res = handle;
    inv_ctx->status = LDG_NET_STEAMWORKS_INVENTORY_STATUS_PENDING;

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_econ_call_handle(ldg_net_steamworks_econ_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    uint8_t failed = 0;
    econ_eligibility_res_t res = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->api_call == 0 || call != ctx->api_call) { return; }

    if (ctx->state != LDG_NET_STEAMWORKS_ECON_STATE_PENDING) { return; }

    if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, ctx->api_call, &res, (int32_t)sizeof(res), STEAM_CB_MARKET_ELIGIBILITY, &failed) == 0) { return; }

    if (failed == 0)
    {
        ctx->market_allowed = (res.allowed != 0) ? 1 : 0;
        ctx->blk_reason = (uint32_t)res.not_allowed_reason;
        ctx->state = LDG_NET_STEAMWORKS_ECON_STATE_READY;
    }
    else { ctx->state = LDG_NET_STEAMWORKS_ECON_STATE_ERR; }

    ctx->api_call = 0;
}
