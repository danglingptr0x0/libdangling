#include <stdint.h>

#include <dangling/sys/handover.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_HANDOVER_MAGIC 0x474F5250
#define LDG_SYS_CTX_END_SYM "dangling_ctx_term"

typedef uint32_t (*ldg_sys_sym_get_cb_t)(void *kernel, const char *name, uint64_t *out);
typedef uint32_t (*ldg_sys_lib_reg_cb_t)(void *kernel, const void *desc);
typedef uint32_t (*ldg_sys_ctx_end_cb_t)(void *kernel);

typedef struct ldg_sys_handover
{
    uint32_t magic;
    uint32_t rev;
    ldg_sys_sym_get_cb_t sym_get;
    ldg_sys_lib_reg_cb_t lib_reg;
    void *kernel;
    char **argv;
    uint32_t argc;
    uint8_t pudding[4];
} ldg_sys_handover_t;

typedef struct ldg_sys_handover_state
{
    ldg_sys_sym_get_cb_t sym_get;
    ldg_sys_lib_reg_cb_t lib_reg;
    void *kernel;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_sys_handover_state_t;

static ldg_sys_handover_state_t* handover_state_get(void)
{
    static ldg_sys_handover_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

uint32_t ldg_sys_handover_set(const void *handover)
{
    const ldg_sys_handover_t *given = 0x0;
    ldg_sys_handover_state_t *state = 0x0;

    if (LDG_UNLIKELY(!handover)) { return LDG_ERR_FUNC_ARG_NULL; }

    given = (const ldg_sys_handover_t *)handover;

    if (LDG_UNLIKELY(given->magic != LDG_SYS_HANDOVER_MAGIC)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!given->sym_get || !given->kernel)) { return LDG_ERR_FUNC_ARG_INVALID; }

    state = handover_state_get();

    state->sym_get = given->sym_get;
    state->lib_reg = given->lib_reg;
    state->kernel = given->kernel;
    state->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_kernel_get(void **out)
{
    ldg_sys_handover_state_t *state = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    state = handover_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = state->kernel;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_ctx_end(void)
{
    ldg_sys_handover_state_t *state = 0x0;
    ldg_sys_ctx_end_cb_t ctx_end = 0x0;
    uint64_t addr = 0;
    uint32_t ret = 0;

    state = handover_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_sys_sym_get(LDG_SYS_CTX_END_SYM, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx_end = (ldg_sys_ctx_end_cb_t)(uintptr_t)addr;

    return ctx_end(state->kernel);
}

uint32_t ldg_sys_sym_get(const char *name, uint64_t *out)
{
    ldg_sys_handover_state_t *state = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    state = handover_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = state->sym_get(state->kernel, name, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(*out == 0)) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}
