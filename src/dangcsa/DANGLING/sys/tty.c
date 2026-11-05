#include <stdint.h>

#include <dangling/sys/tty.h>
#include <dangling/sys/handover.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_TTY_SYM_CLAIM "dangling_con_claim"
#define LDG_SYS_TTY_SYM_RELEASE "dangling_con_release"
#define LDG_SYS_TTY_SYM_VIEW_EN_SET "dangling_con_view_en_set"

typedef uint32_t (*ldg_sys_tty_claim_cb_t)(void *kernel);
typedef uint32_t (*ldg_sys_tty_release_cb_t)(void *kernel);
typedef uint32_t (*ldg_sys_tty_view_en_set_cb_t)(void *kernel, uint8_t en);

typedef struct ldg_sys_tty_state
{
    void *kernel;
    ldg_sys_tty_claim_cb_t claim;
    ldg_sys_tty_release_cb_t release;
    ldg_sys_tty_view_en_set_cb_t view_en_set;
    uint8_t is_bound;
    uint8_t pudding[7];
} ldg_sys_tty_state_t;

static ldg_sys_tty_state_t* tty_state_get(void)
{
    static ldg_sys_tty_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint32_t tty_bind_ensure(ldg_sys_tty_state_t *state)
{
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (state->is_bound == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_sys_kernel_get(&state->kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sys_sym_get(LDG_SYS_TTY_SYM_CLAIM, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->claim = (ldg_sys_tty_claim_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_SYS_TTY_SYM_RELEASE, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->release = (ldg_sys_tty_release_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_SYS_TTY_SYM_VIEW_EN_SET, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->view_en_set = (ldg_sys_tty_view_en_set_cb_t)(uintptr_t)addr;

    state->is_bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint8_t ldg_sys_tty_stdout_is(void)
{
    uint32_t ret = 0;

    ret = tty_bind_ensure(tty_state_get());
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_sys_tty_width_get(uint32_t *cols)
{
    if (LDG_UNLIKELY(!cols)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cols = 0;

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_tty_size_get(uint32_t *out_col_cunt, uint32_t *out_row_cunt)
{
    if (LDG_UNLIKELY(!out_col_cunt || !out_row_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_col_cunt = 0;
    *out_row_cunt = 0;

    return LDG_ERR_UNSUPPORTED;
}

uint8_t ldg_sys_tty_open_is(void)
{
    uint32_t ret = 0;

    ret = tty_bind_ensure(tty_state_get());
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_sys_tty_kb_report_en_set(uint32_t flags, uint8_t en)
{
    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(flags > LDG_SYS_TTY_KB_FLAGS_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_tty_other_keys_en_set(uint8_t en)
{
    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_tty_vt_en_set(uint8_t en)
{
    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_tty_raw_en_set(ldg_sys_tty_raw_t *raw, uint8_t en)
{
    ldg_sys_tty_state_t *state = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!raw)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (raw->is_active == en) { return LDG_ERR_AOK; }

    state = tty_state_get();

    ret = tty_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (en == LDG_TRUTH_TRUE)
    {
        ret = state->claim(state->kernel);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = state->view_en_set(state->kernel, LDG_TRUTH_FALSE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (LDG_UNLIKELY(state->release(state->kernel) != LDG_ERR_AOK)) { return LDG_ERR_BUSY; }

            return ret;
        }

        raw->is_active = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    ret = state->view_en_set(state->kernel, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = state->release(state->kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    raw->is_active = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
