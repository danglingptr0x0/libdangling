#ifndef LDG_SHELL_HIST_H
#define LDG_SHELL_HIST_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/shell/shell.h>

LDG_EXPORT uint32_t ldg_shell_hist_push(ldg_shell_t *shell, const char *text);
LDG_EXPORT uint32_t ldg_shell_hist_get(const ldg_shell_t *shell, uint32_t idx, const char **out);
LDG_EXPORT uint32_t ldg_shell_hist_cunt_get(const ldg_shell_t *shell, uint32_t *out);
LDG_EXPORT uint32_t ldg_shell_hist_ld(ldg_shell_t *shell);
LDG_EXPORT uint32_t ldg_shell_hist_st(ldg_shell_t *shell);
LDG_EXPORT uint32_t ldg_shell_hist_io_set(ldg_shell_t *shell, const ldg_shell_hist_io_t *io);

#endif
