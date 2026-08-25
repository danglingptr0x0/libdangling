#include <stdint.h>

#include <dangling/io/console.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_CONSOLE_BASE 0x90000000U

#define LDG_IO_CONSOLE_TX_OFF 0x0CU
#define LDG_IO_CONSOLE_RX_AVAIL_OFF 0x10U
#define LDG_IO_CONSOLE_RX_OFF 0x14U
#define LDG_IO_CONSOLE_TX_READY_OFF 0x40U

#define LDG_IO_CONSOLE_TX_SPIN_MAX 1048576

static uint32_t console_reg_get(uint32_t off)
{
    return *(volatile uint32_t *)(uintptr_t)(LDG_IO_CONSOLE_BASE + off);
}

static void console_reg_wr(uint32_t off, uint32_t val)
{
    *(volatile uint32_t *)(uintptr_t)(LDG_IO_CONSOLE_BASE + off) = val;
}

uint32_t ldg_io_console_byte_wr(uint8_t byte)
{
    uint32_t spin = 0;

    for (spin = 0; spin < LDG_IO_CONSOLE_TX_SPIN_MAX; spin++) { if (console_reg_get(LDG_IO_CONSOLE_TX_READY_OFF) != 0)
        {
            console_reg_wr(LDG_IO_CONSOLE_TX_OFF, (uint32_t)byte);
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_TIMEOUT;
}

uint32_t ldg_io_console_wr(const uint8_t *data, uint64_t len)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < len; idx++)
    {
        ret = ldg_io_console_byte_wr(data[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_console_rd_ready_is(void)
{
    if (console_reg_get(LDG_IO_CONSOLE_RX_AVAIL_OFF) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_io_console_byte_rd(uint8_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (ldg_io_console_rd_ready_is() != LDG_TRUTH_TRUE) { return LDG_ERR_WOULD_BLOCK; }

    *out = (uint8_t)console_reg_get(LDG_IO_CONSOLE_RX_OFF);

    return LDG_ERR_AOK;
}
