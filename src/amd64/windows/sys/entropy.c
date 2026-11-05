#include <windows.h>
#include <bcrypt.h>

#include <dangling/sys/entropy.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>

uint32_t ldg_sys_entropy_get(uint8_t *out, uint32_t len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_secure_zero(out, (uint64_t)len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(BCryptGenRandom(0x0, out, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}
