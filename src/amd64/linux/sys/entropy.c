#include <fcntl.h>
#include <unistd.h>

#include <dangling/sys/entropy.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>

uint32_t ldg_sys_entropy_get(uint8_t *out, uint32_t len)
{
    uint32_t ret = 0;
    int32_t fd = 0;
    uint64_t total = 0;
    int64_t res = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_secure_zero(out, (uint64_t)len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fd = (int32_t)open("/dev/random", O_RDONLY | O_CLOEXEC);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    while (total < (uint64_t)len)
    {
        res = (int64_t)read(fd, out + total, (uint64_t)len - total);
        if (LDG_UNLIKELY(res <= 0))
        {
            ret = LDG_ERR_IO_RD;
            if (LDG_UNLIKELY(close(fd) < 0)) { ret = LDG_ERR_IO_CLOSE; }

            if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)len) != LDG_ERR_AOK)) { ret = LDG_ERR_MEM_BAD; }

            return ret;
        }

        total += (uint64_t)res;
    }

    if (LDG_UNLIKELY(close(fd) < 0)) { return LDG_ERR_IO_CLOSE; }

    return LDG_ERR_AOK;
}
