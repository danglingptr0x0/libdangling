#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_srv.h>

#define STEAM_TKT_DEC_MAX 1024

extern uint8_t SteamEncryptedAppTicket_BDecryptTicket(const uint8_t *enc, uint32_t enc_len, uint8_t *dec, uint32_t *dec_len, const uint8_t *key, int32_t key_len);
extern void SteamEncryptedAppTicket_GetTicketSteamID(uint8_t *dec, uint32_t dec_len, uint64_t *steam_id);
extern uint32_t SteamEncryptedAppTicket_GetTicketAppID(uint8_t *dec, uint32_t dec_len);
extern uint32_t SteamEncryptedAppTicket_GetTicketIssueTime(uint8_t *dec, uint32_t dec_len);
extern uint8_t SteamEncryptedAppTicket_BUserIsVacBanned(uint8_t *dec, uint32_t dec_len);
extern uint8_t SteamEncryptedAppTicket_BIsLicenseBorrowed(uint8_t *dec, uint32_t dec_len);
extern uint8_t SteamEncryptedAppTicket_BIsLicenseTemporary(uint8_t *dec, uint32_t dec_len);

uint32_t ldg_net_steamworks_srv_tkt_dec(const uint8_t *enc, uint32_t enc_len, const uint8_t *secret, uint32_t secret_len, ldg_net_steamworks_srv_ident_t *out)
{
    uint8_t dec[STEAM_TKT_DEC_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t dec_len = STEAM_TKT_DEC_MAX;
    uint32_t zero_err = 0;

    if (LDG_UNLIKELY(!enc || !secret || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(enc_len == 0 || secret_len != LDG_NET_STEAMWORKS_SRV_SECRET_LEN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    zero_err = ldg_mem_secure_zero(out, sizeof(ldg_net_steamworks_srv_ident_t));
    if (LDG_UNLIKELY(zero_err != LDG_ERR_AOK)) { return zero_err; }

    if (SteamEncryptedAppTicket_BDecryptTicket(enc, enc_len, dec, &dec_len, secret, (int32_t)secret_len) == 0)
    {
        zero_err = ldg_mem_secure_zero(dec, sizeof(dec));
        if (LDG_UNLIKELY(zero_err != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: tkt dec buff zero failed"); }

        return LDG_ERR_NET_TKT_DEC;
    }

    SteamEncryptedAppTicket_GetTicketSteamID(dec, dec_len, &out->steam_id);
    out->app_id = SteamEncryptedAppTicket_GetTicketAppID(dec, dec_len);
    out->issue_time = SteamEncryptedAppTicket_GetTicketIssueTime(dec, dec_len);
    out->vac_banned = (SteamEncryptedAppTicket_BUserIsVacBanned(dec, dec_len) != 0) ? 1 : 0;
    out->license_borrowed = (SteamEncryptedAppTicket_BIsLicenseBorrowed(dec, dec_len) != 0) ? 1 : 0;
    out->license_temporary = (SteamEncryptedAppTicket_BIsLicenseTemporary(dec, dec_len) != 0) ? 1 : 0;

    zero_err = ldg_mem_secure_zero(dec, sizeof(dec));
    if (LDG_UNLIKELY(zero_err != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: tkt dec buff zero failed"); }

    return LDG_ERR_AOK;
}
