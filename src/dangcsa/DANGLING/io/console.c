#include <stdint.h>

#include <dangling/io/console.h>
#include <dangling/sys/handover.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_CONSOLE_SYM_CH_PUT "dangling_con_ch_put"
#define LDG_IO_CONSOLE_SYM_CH_GET "dangling_con_ch_get"
#define LDG_IO_CONSOLE_STEP 1

typedef uint32_t (*ldg_io_console_ch_put_cb_t)(void *kernel, uint8_t ch);
typedef uint32_t (*ldg_io_console_ch_get_cb_t)(void *kernel, uint8_t *out);

typedef struct ldg_io_console_state
{
    void *kernel;
    ldg_io_console_ch_put_cb_t ch_put;
    ldg_io_console_ch_get_cb_t ch_get;
    uint8_t held;
    uint8_t held_is;
    uint8_t is_bound;
    uint8_t pudding[5];
} ldg_io_console_state_t;

static ldg_io_console_state_t* console_state_get(void)
{
    static ldg_io_console_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint32_t console_bind_ensure(ldg_io_console_state_t *state)
{
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (state->is_bound == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_sys_kernel_get(&state->kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sys_sym_get(LDG_IO_CONSOLE_SYM_CH_PUT, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->ch_put = (ldg_io_console_ch_put_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_IO_CONSOLE_SYM_CH_GET, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->ch_get = (ldg_io_console_ch_get_cb_t)(uintptr_t)addr;

    state->is_bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_byte_wr(uint8_t byte)
{
    ldg_io_console_state_t *state = 0x0;
    uint32_t ret = 0;

    state = console_state_get();

    ret = console_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return state->ch_put(state->kernel, byte);
}

uint32_t ldg_io_console_wr(const uint8_t *data, uint64_t len)
{
    ldg_io_console_state_t *state = 0x0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    state = console_state_get();

    ret = console_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < len; idx++)
    {
        ret = state->ch_put(state->kernel, data[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_console_rd_ready_is(void)
{
    ldg_io_console_state_t *state = 0x0;
    uint8_t byte = 0;
    uint32_t ret = 0;

    state = console_state_get();

    if (state->held_is == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    ret = console_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = state->ch_get(state->kernel, &byte);

    if (ret == LDG_ERR_EMPTY) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    state->held = byte;
    state->held_is = LDG_TRUTH_TRUE;

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_io_console_byte_rd(uint8_t *out)
{
    ldg_io_console_state_t *state = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    state = console_state_get();

    if (state->held_is == LDG_TRUTH_TRUE)
    {
        *out = state->held;
        state->held_is = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    ret = console_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = state->ch_get(state->kernel, out);

    if (ret == LDG_ERR_EMPTY) { return LDG_ERR_WOULD_BLOCK; }

    return ret;
}

uint32_t ldg_io_console_rd(uint8_t *dst, uint64_t cap, uint64_t *out_len)
{
    ldg_io_console_state_t *state = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dst || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    state = console_state_get();

    if (state->held_is == LDG_TRUTH_TRUE && *out_len < cap)
    {
        dst[*out_len] = state->held;
        *out_len += LDG_IO_CONSOLE_STEP;
        state->held_is = LDG_TRUTH_FALSE;
    }

    ret = console_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (*out_len < cap)
    {
        ret = state->ch_get(state->kernel, &dst[*out_len]);

        if (ret == LDG_ERR_EMPTY) { return LDG_ERR_AOK; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_len += LDG_IO_CONSOLE_STEP;
    }

    return LDG_ERR_AOK;
}
