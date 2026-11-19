#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/str.h>
#include <dangling/shell/shell.h>
#include <dangling/shell/cmd.h>

static uint8_t shell_cmd_name_eq_is(const char *lhs, const char *rhs)
{
    uint32_t order = 0;

    if (ldg_str_cmp(lhs, rhs, LDG_SHELL_NAME_CAP, &order) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    return (order == LDG_ARITH_ORD_EQ) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t shell_cmd_name_reserved_is(const char *name)
{
    if (shell_cmd_name_eq_is(name, LDG_SHELL_WORD_NOT) == LDG_TRUTH_TRUE || shell_cmd_name_eq_is(name, LDG_SHELL_WORD_AND) == LDG_TRUTH_TRUE || shell_cmd_name_eq_is(name, LDG_SHELL_WORD_XOR) == LDG_TRUTH_TRUE || shell_cmd_name_eq_is(name, LDG_SHELL_WORD_OR) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (shell_cmd_name_eq_is(name, LDG_SHELL_WORD_IMPLIES) == LDG_TRUTH_TRUE || shell_cmd_name_eq_is(name, LDG_SHELL_WORD_IFF) == LDG_TRUTH_TRUE || shell_cmd_name_eq_is(name, LDG_SHELL_WORD_AOK) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_shell_cmd_reg(ldg_shell_t *shell, const ldg_shell_cmd_t *cmd)
{
    uint64_t name_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !cmd)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!cmd->name || !cmd->help || !cmd->cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cmd->kind >= LDG_SHELL_CMD_KIND_CUNT || cmd->remainder > LDG_TRUTH_TRUE || cmd->arg_min > cmd->arg_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_str_len_get(cmd->name, LDG_SHELL_NAME_CAP, &name_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SHELL_NAME_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(shell_cmd_name_reserved_is(cmd->name) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < shell->cmd_cunt; idx++) { if (shell_cmd_name_eq_is(shell->cmd[idx].name, cmd->name) == LDG_TRUTH_TRUE) { return LDG_ERR_EXISTS; } }

    if (LDG_UNLIKELY(shell->cmd_cunt >= shell->conf.cmd_max)) { return LDG_ERR_FULL; }

    ret = ldg_mem_cpy(&shell->cmd[shell->cmd_cunt], cmd, sizeof(ldg_shell_cmd_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shell->cmd_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_cmd_get(const ldg_shell_t *shell, uint32_t idx, const ldg_shell_cmd_t **out)
{
    if (LDG_UNLIKELY(!shell || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(idx >= shell->cmd_cunt)) { return LDG_ERR_BOUNDS; }

    *out = &shell->cmd[idx];

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_cmd_cunt_get(const ldg_shell_t *shell, uint32_t *out)
{
    if (LDG_UNLIKELY(!shell || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = shell->cmd_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_cmd_find(const ldg_shell_t *shell, const char *name, const ldg_shell_cmd_t **out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!shell || !name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (idx = 0; idx < shell->cmd_cunt; idx++)
    {
        if (shell_cmd_name_eq_is(shell->cmd[idx].name, name) != LDG_TRUTH_TRUE) { continue; }

        *out = &shell->cmd[idx];

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}
