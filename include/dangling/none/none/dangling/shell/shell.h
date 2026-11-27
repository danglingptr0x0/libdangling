#ifndef LDG_SHELL_SHELL_H
#define LDG_SHELL_SHELL_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/thread/sync.h>

#define LDG_SHELL_CANDIDATE_CAP 64
#define LDG_SHELL_CANDIDATE_LABEL_SEP '\t'
#define LDG_SHELL_ADDR_MAX 64
#define LDG_SHELL_ADDR_CAP 24
#define LDG_SHELL_ADDR_DIGITS_MIN 5
#define LDG_SHELL_HIST_KIND_CMD 'C'
#define LDG_SHELL_HIST_KIND_ALIAS 'A'
#define LDG_SHELL_HIST_KIND_SEP '\t'
#define LDG_SHELL_HIST_ALIAS_SEP '='
#define LDG_SHELL_HIST_LEAD_LEN 2
#define LDG_SHELL_HIST_ROW_EXTRA 4
#define LDG_SHELL_LN_CAP_DEFAULT 640
#define LDG_SHELL_HIST_MAX_DEFAULT 192
#define LDG_SHELL_CMD_MAX_DEFAULT 96
#define LDG_SHELL_VAR_MAX_DEFAULT 32
#define LDG_SHELL_ALIAS_MAX_DEFAULT 32
#define LDG_SHELL_OUT_LN_MAX_DEFAULT 128
#define LDG_SHELL_OUT_LN_CAP_DEFAULT 144
#define LDG_SHELL_LN_CAP_MIN 64
#define LDG_SHELL_NAME_CAP 32
#define LDG_SHELL_ARG_MAX 16
#define LDG_SHELL_EXPR_ITEM_MAX 64
#define LDG_SHELL_EXPR_DEPTH_MAX 32
#define LDG_SHELL_SEL_NONE UINT32_MAX
#define LDG_SHELL_SCRIPT_EXT ".aww"
#define LDG_SHELL_WORD_NOT "not"
#define LDG_SHELL_WORD_AND "and"
#define LDG_SHELL_WORD_XOR "xor"
#define LDG_SHELL_WORD_OR "or"
#define LDG_SHELL_WORD_IMPLIES "implies"
#define LDG_SHELL_WORD_IFF "iff"
#define LDG_SHELL_WORD_AOK "aok"

typedef uint32_t (*ldg_shell_cmd_cb_t)(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth);
typedef uint32_t (*ldg_shell_out_cb_t)(void *user, const char *text, uint64_t len);

typedef enum ldg_shell_cmd_kind
{
    LDG_SHELL_CMD_PLAIN = 0,
    LDG_SHELL_CMD_PREDICATE,
    LDG_SHELL_CMD_KIND_CUNT
} ldg_shell_cmd_kind_t;

typedef struct ldg_shell_cmd
{
    const char *name;
    const char *help;
    ldg_shell_cmd_cb_t cb;
    void *user;
    uint32_t arg_min;
    uint32_t arg_max;
    uint8_t kind;
    uint8_t remainder;
    uint8_t pudding[6];
} ldg_shell_cmd_t;

typedef struct ldg_shell_hist_io
{
    uint32_t (*ld_cb)(void *user, char *dst, uint64_t cap, uint64_t *out_len);
    uint32_t (*st_cb)(void *user, const char *src, uint64_t len);
    void *user;
} ldg_shell_hist_io_t;

typedef struct ldg_shell_conf
{
    uint32_t ln_cap;
    uint32_t hist_max;
    uint32_t cmd_max;
    uint32_t var_max;
    uint32_t alias_max;
    uint32_t out_ln_max;
    uint32_t out_ln_cap;
    uint8_t pudding[4];
} ldg_shell_conf_t;

typedef struct ldg_shell
{
    ldg_shell_conf_t conf;
    ldg_shell_hist_io_t hist_io;
    char *hist;
    char *hist_blob;
    uint64_t hist_blob_cap;
    ldg_shell_cmd_t *cmd;
    char *var_name;
    char *var_text;
    char *alias_name;
    char *alias_text;
    char *out;
    char *exec_ln;
    char *arg_buff;
    const char *arg[LDG_SHELL_ARG_MAX];
    ldg_shell_out_cb_t out_cb;
    void *out_user;
    ldg_mut_t out_mut;
    uint64_t out_seq;
    uint64_t wipe_cunt;
    char addr[LDG_SHELL_ADDR_MAX][LDG_SHELL_ADDR_CAP];
    uint32_t addr_next;
    uint32_t addr_cunt;
    uint32_t expr_kind[LDG_SHELL_EXPR_ITEM_MAX];
    uint32_t expr_start[LDG_SHELL_EXPR_ITEM_MAX];
    uint32_t expr_len[LDG_SHELL_EXPR_ITEM_MAX];
    uint32_t expr_cunt;
    uint32_t cmd_cunt;
    uint32_t arg_cunt;
    uint32_t out_hd;
    uint32_t out_cunt;
    uint32_t out_open_len;
    uint32_t status;
    uint32_t hist_max;
    uint32_t hist_hd;
    uint32_t hist_cunt;
    uint8_t op_stack[LDG_SHELL_EXPR_DEPTH_MAX];
    uint8_t truth_stack[LDG_SHELL_EXPR_DEPTH_MAX];
    uint8_t truth;
    uint8_t truth_has;
    uint8_t out_open;
    uint8_t pudding[5];
} ldg_shell_t;

LDG_EXPORT uint32_t ldg_shell_conf_default_get(ldg_shell_conf_t *out);
LDG_EXPORT uint32_t ldg_shell_pool_size_get(const ldg_shell_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_shell_create(ldg_mem_pool_t *pool, const ldg_shell_conf_t *conf, ldg_shell_t **out);
LDG_EXPORT uint32_t ldg_shell_uncreate(ldg_shell_t **shell);
LDG_EXPORT uint32_t ldg_shell_rst(ldg_shell_t *shell);
LDG_EXPORT uint32_t ldg_shell_ln_exec(ldg_shell_t *shell, const char *ln);
LDG_EXPORT uint32_t ldg_shell_file_exec(ldg_shell_t *shell, const char *path, const char *src, uint64_t len);
LDG_EXPORT uint32_t ldg_shell_status_get(const ldg_shell_t *shell, uint32_t *out_status, uint8_t *out_truth, uint8_t *out_truth_has);
LDG_EXPORT uint32_t ldg_shell_var_set(ldg_shell_t *shell, const char *name, const char *text);
LDG_EXPORT uint32_t ldg_shell_var_get(const ldg_shell_t *shell, const char *name, const char **out);
LDG_EXPORT uint32_t ldg_shell_var_unset(ldg_shell_t *shell, const char *name);
LDG_EXPORT uint32_t ldg_shell_alias_set(ldg_shell_t *shell, const char *name, const char *text);
LDG_EXPORT uint32_t ldg_shell_alias_get(const ldg_shell_t *shell, const char *name, const char **out);
LDG_EXPORT uint32_t ldg_shell_alias_unset(ldg_shell_t *shell, const char *name);
LDG_EXPORT uint32_t ldg_shell_out_push(ldg_shell_t *shell, const char *text);
LDG_EXPORT uint32_t ldg_shell_out_cb_set(ldg_shell_t *shell, ldg_shell_out_cb_t cb, void *user);
LDG_EXPORT uint32_t ldg_shell_out_seq_get(ldg_shell_t *shell, uint64_t *out);
LDG_EXPORT uint32_t ldg_shell_out_get(ldg_shell_t *shell, uint64_t seq, char *out, uint64_t cap, uint64_t *out_seq);
LDG_EXPORT uint32_t ldg_shell_wipe_cunt_get(ldg_shell_t *shell, uint64_t *out);
LDG_EXPORT uint32_t ldg_shell_candidate_get(ldg_shell_t *shell, const char *text, uint32_t len, char (*out)[LDG_SHELL_CANDIDATE_CAP], uint32_t cap, uint32_t *out_cunt, uint32_t *out_at, uint8_t *out_more);

#endif
