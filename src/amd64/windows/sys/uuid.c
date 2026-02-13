#include <string.h>
#include <windows.h>
#include <bcrypt.h>

#include <dangling/sys/uuid.h>
#include <dangling/core/err.h>
#include <dangling/str/str.h>

uint32_t ldg_sys_uuid_gen(uint8_t uuid[LDG_UUID_BYTE_SIZE])
{
    if (LDG_UNLIKELY(!uuid)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(uuid, 0, LDG_UUID_BYTE_SIZE) != uuid)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(BCryptGenRandom(0x0, uuid, LDG_UUID_BYTE_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)) { return LDG_ERR_IO_RD; }

    uuid[6] = (uuid[6] & 0x0F) | 0x40;
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_uuid_to_str(const uint8_t uuid[LDG_UUID_BYTE_SIZE], char *str, uint64_t str_size)
{
    uint32_t i = 0;
    uint32_t pos = 0;
    char hex[3] = { 0 };

    if (LDG_UNLIKELY(!uuid || !str)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(str_size < LDG_UUID_STR_SIZE)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(memset(str, 0, (size_t)str_size) != str)) { return LDG_ERR_MEM_BAD; }

    for (i = 0; i < LDG_UUID_BYTE_SIZE; i++)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
        {
            str[pos] = '-';
            pos++;
        }

        ldg_byte_to_hex(uuid[i], hex);
        str[pos] = hex[0];
        str[pos + 1] = hex[1];
        pos += 2;
    }

    str[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}
