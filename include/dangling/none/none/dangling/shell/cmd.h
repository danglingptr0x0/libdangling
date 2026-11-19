#ifndef LDG_SHELL_CMD_H
#define LDG_SHELL_CMD_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/shell/shell.h>

LDG_EXPORT uint32_t ldg_shell_cmd_reg(ldg_shell_t *shell, const ldg_shell_cmd_t *cmd);
LDG_EXPORT uint32_t ldg_shell_cmd_get(const ldg_shell_t *shell, uint32_t idx, const ldg_shell_cmd_t **out);
LDG_EXPORT uint32_t ldg_shell_cmd_cunt_get(const ldg_shell_t *shell, uint32_t *out);
LDG_EXPORT uint32_t ldg_shell_cmd_find(const ldg_shell_t *shell, const char *name, const ldg_shell_cmd_t **out);

#endif
