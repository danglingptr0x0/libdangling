#ifndef LDG_NET_STEAMWORKS_ECON_H
#define LDG_NET_STEAMWORKS_ECON_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/net/steamworks_inventory.h>

typedef enum ldg_net_steamworks_market_blk_reason
{
    LDG_NET_STEAMWORKS_MARKET_BLK_NONE = 0,
    LDG_NET_STEAMWORKS_MARKET_BLK_TEMP_FAIL = (1 << 0),
    LDG_NET_STEAMWORKS_MARKET_BLK_ACCOUNT_DISABLED = (1 << 1),
    LDG_NET_STEAMWORKS_MARKET_BLK_ACCOUNT_LOCKED = (1 << 2),
    LDG_NET_STEAMWORKS_MARKET_BLK_ACCOUNT_LIMITED = (1 << 3),
    LDG_NET_STEAMWORKS_MARKET_BLK_TRADE_BANNED = (1 << 4),
    LDG_NET_STEAMWORKS_MARKET_BLK_NOT_TRUSTED = (1 << 5),
    LDG_NET_STEAMWORKS_MARKET_BLK_NO_GUARD = (1 << 6),
    LDG_NET_STEAMWORKS_MARKET_BLK_GUARD_RECENT = (1 << 7),
    LDG_NET_STEAMWORKS_MARKET_BLK_PASSWORD_RST = (1 << 8),
    LDG_NET_STEAMWORKS_MARKET_BLK_NEW_PAYMENT = (1 << 9),
    LDG_NET_STEAMWORKS_MARKET_BLK_INVALID_COOKIE = (1 << 10),
    LDG_NET_STEAMWORKS_MARKET_BLK_NEW_DEV = (1 << 11),
    LDG_NET_STEAMWORKS_MARKET_BLK_SELF_REFUND = (1 << 12),
    LDG_NET_STEAMWORKS_MARKET_BLK_PAYMENT_UNVERIFIED = (1 << 13),
    LDG_NET_STEAMWORKS_MARKET_BLK_NO_PURCHASES = (1 << 14),
    LDG_NET_STEAMWORKS_MARKET_BLK_WALLET_GIFT = (1 << 15),
    LDG_NET_STEAMWORKS_MARKET_BLK_TRADE_COOLDOWN = (1 << 16)
} ldg_net_steamworks_market_blk_reason_t;

typedef enum ldg_net_steamworks_econ_state
{
    LDG_NET_STEAMWORKS_ECON_STATE_IDLE = 0,
    LDG_NET_STEAMWORKS_ECON_STATE_PENDING = 1,
    LDG_NET_STEAMWORKS_ECON_STATE_READY = 2,
    LDG_NET_STEAMWORKS_ECON_STATE_ERR = 3
} ldg_net_steamworks_econ_state_t;

typedef struct ldg_net_steamworks_econ_ctx
{
    uint64_t api_call;
    ldg_net_steamworks_econ_state_t state;
    uint8_t market_allowed;
    uint8_t pudding0[3];
    uint32_t blk_reason;
    uint8_t pudding[44];
} LDG_ALIGNED ldg_net_steamworks_econ_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_econ_init(ldg_net_steamworks_econ_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_econ_shutdown(ldg_net_steamworks_econ_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_econ_market_ck(ldg_net_steamworks_econ_ctx_t *ctx);
LDG_EXPORT ldg_net_steamworks_econ_state_t ldg_net_steamworks_econ_state_get(const ldg_net_steamworks_econ_ctx_t *ctx);
LDG_EXPORT uint8_t ldg_net_steamworks_econ_market_allowed_is(const ldg_net_steamworks_econ_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_econ_blk_reason_get(const ldg_net_steamworks_econ_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_econ_trade(ldg_net_steamworks_inventory_ctx_t *inv_ctx, uint64_t partner_steam_id, const ldg_net_steamworks_inventory_instance_t *give_id, const uint32_t *give_qty, uint32_t give_cunt, const ldg_net_steamworks_inventory_instance_t *get_id, const uint32_t *get_qty, uint32_t get_cunt);
LDG_EXPORT void ldg_net_steamworks_econ_call_handle(ldg_net_steamworks_econ_ctx_t *ctx, int32_t pipe, uint64_t call);

#endif
