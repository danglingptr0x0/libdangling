#include <stdint.h>

#include <dangling/time/time.h>
#include <dangling/sys/handover.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TIME_SYM_DEPCH_GET "dangling_time_depch_get"
#define LDG_TIME_SYM_TIMER_MS_GET "dangling_time_timer_ms_get"

typedef uint32_t (*ldg_time_ms_get_cb_t)(void *kernel, uint64_t *out);

typedef struct ldg_time_state
{
    void *kernel;
    ldg_time_ms_get_cb_t depch_get;
    ldg_time_ms_get_cb_t timer_ms_get;
    uint8_t is_bound;
    uint8_t pudding[7];
} ldg_time_state_t;

static ldg_time_state_t* time_state_get(void)
{
    static ldg_time_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint32_t time_bind_ensure(ldg_time_state_t *state)
{
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (state->is_bound == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_sys_kernel_get(&state->kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sys_sym_get(LDG_TIME_SYM_DEPCH_GET, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->depch_get = (ldg_time_ms_get_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_TIME_SYM_TIMER_MS_GET, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->timer_ms_get = (ldg_time_ms_get_cb_t)(uintptr_t)addr;

    state->is_bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t time_depch_ms_rd(uint64_t *out)
{
    ldg_time_state_t *state = 0x0;
    uint32_t ret = 0;

    *out = 0;

    state = time_state_get();

    ret = time_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return state->depch_get(state->kernel, out);
}

static uint32_t time_timer_ms_rd(uint64_t *out)
{
    ldg_time_state_t *state = 0x0;
    uint32_t ret = 0;

    *out = 0;

    state = time_state_get();

    ret = time_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return state->timer_ms_get(state->kernel, out);
}

uint64_t ldg_time_epoch_ms_get(void)
{
    uint64_t ms = 0;

    if (LDG_UNLIKELY(time_depch_ms_rd(&ms) != LDG_ERR_AOK)) { return UINT64_MAX; }

    return ms;
}

uint64_t ldg_time_epoch_ns_get(void)
{
    uint64_t ms = 0;

    if (LDG_UNLIKELY(time_depch_ms_rd(&ms) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ms > UINT64_MAX / LDG_NS_PER_MS)) { return UINT64_MAX; }

    return ms * LDG_NS_PER_MS;
}

uint64_t ldg_time_cycle_get(void)
{
    return UINT64_MAX;
}

uint32_t ldg_time_ns_delay(uint64_t duration)
{
    uint64_t start = 0;
    uint64_t now = 0;
    uint32_t ret = 0;

    if (duration == 0) { return LDG_ERR_AOK; }

    ret = time_timer_ms_rd(&start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    now = start;

    while ((now - start) * LDG_NS_PER_MS < duration)
    {
        ret = time_timer_ms_rd(&now);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_time_monotonic_get(uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return time_timer_ms_rd(out);
}

uint32_t ldg_time_monotonic_ns_get(uint64_t *out)
{
    uint64_t ms = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = time_timer_ms_rd(&ms);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ms > UINT64_MAX / LDG_NS_PER_MS)) { return LDG_ERR_OVERFLOW; }

    *out = ms * LDG_NS_PER_MS;

    return LDG_ERR_AOK;
}
