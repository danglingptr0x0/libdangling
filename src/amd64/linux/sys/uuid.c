#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <dangling/sys/uuid.h>
#include <dangling/core/err.h>
#include <dangling/str/str.h>

uint32_t ldg_sys_uuid_gen(uint8_t uuid[LDG_UUID_BYTE_SIZE])
{
    int32_t fd = 0;
    int64_t total = 0;
    int64_t result = 0;
    int32_t close_ret = 0;

    if (LDG_UNLIKELY(!uuid)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(uuid, 0, LDG_UUID_BYTE_SIZE) != uuid)) { return LDG_ERR_MEM_BAD; }

    fd = (int32_t)open("/dev/urandom", O_RDONLY);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    while (total < (int64_t)LDG_UUID_BYTE_SIZE)
    {
        result = (int64_t)read(fd, uuid + total, (uint64_t)(LDG_UUID_BYTE_SIZE - (uint64_t)total));
        if (LDG_UNLIKELY(result < 0))
        {
            close(fd);
            return LDG_ERR_IO_RD;
        }

        if (LDG_UNLIKELY(result == 0))
        {
            close(fd);
            return LDG_ERR_IO_RD;
        }

        total += result;
    }

    close_ret = (int32_t)close(fd);
    if (LDG_UNLIKELY(close_ret < 0)) { return LDG_ERR_IO_CLOSE; }

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

    if (LDG_UNLIKELY(memset(str, 0, (uint64_t)str_size) != str)) { return LDG_ERR_MEM_BAD; }

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
