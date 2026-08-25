#include <stdint.h>

#include <dangling/sys/handover.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_HANDOVER_MAGIC 0x474F5250
#define LDG_SYS_HANDOVER_REV 1
#define LDG_SYS_KERN_AOK 0

typedef uint32_t (*ldg_sys_sym_get_cb_t)(const char *name, uint32_t *out);
typedef uint32_t (*ldg_sys_lib_reg_cb_t)(const void *desc);

typedef struct ldg_sys_handover
{
    uint32_t magic;
    uint32_t rev;
    ldg_sys_sym_get_cb_t sym_get;
    ldg_sys_lib_reg_cb_t lib_reg;
} ldg_sys_handover_t;

typedef struct ldg_sys_handover_state
{
    ldg_sys_sym_get_cb_t sym_get;
    ldg_sys_lib_reg_cb_t lib_reg;
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

    if (LDG_UNLIKELY(given->rev != LDG_SYS_HANDOVER_REV)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(!given->sym_get)) { return LDG_ERR_FUNC_ARG_INVALID; }

    state = handover_state_get();

    state->sym_get = given->sym_get;
    state->lib_reg = given->lib_reg;
    state->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_sym_get(const char *name, uint32_t *out)
{
    ldg_sys_handover_state_t *state = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    state = handover_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(state->sym_get(name, out) != LDG_SYS_KERN_AOK)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(*out == 0)) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}
