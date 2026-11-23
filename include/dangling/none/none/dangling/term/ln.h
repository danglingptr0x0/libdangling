#ifndef LDG_TERM_LN_H
#define LDG_TERM_LN_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/mem/alloc.h>
#include <dangling/term/render.h>
#include <dangling/term/vt.h>

#define LDG_TERM_LN_CAP_DEFAULT 640
#define LDG_TERM_LN_CAP_MIN 64
#define LDG_TERM_LN_CANDIDATE_MAX_DEFAULT 256
#define LDG_TERM_LN_CANDIDATE_CAP 64
#define LDG_TERM_LN_CANDIDATE_LABEL_SEP '\t'
#define LDG_TERM_LN_HINT_CAP 192
#define LDG_TERM_LN_PROMPT_CAP 32
#define LDG_TERM_LN_PROMPT_DEFAULT "aww # "
#define LDG_TERM_LN_SEL_NONE UINT32_MAX

typedef uint32_t (*ldg_term_ln_hist_cb_t)(void *user, uint32_t idx, const char **out);
typedef uint32_t (*ldg_term_ln_candidate_cb_t)(void *user, const char *text, uint32_t len, char (*out)[LDG_TERM_LN_CANDIDATE_CAP], uint32_t cap, uint32_t *out_cunt, uint32_t *out_at, uint8_t *out_more);

typedef struct ldg_term_ln_conf
{
    uint32_t ln_cap;
    uint32_t candidate_max;
} ldg_term_ln_conf_t;

typedef struct ldg_term_ln_view
{
    const char *prompt;
    const char *text;
    const char *ghost;
    const char *hint;
    const char (*candidate)[LDG_TERM_LN_CANDIDATE_CAP];
    uint32_t prompt_len;
    uint32_t text_len;
    uint32_t ghost_len;
    uint32_t hint_len;
    uint32_t cursor;
    uint32_t candidate_cunt;
    uint32_t candidate_sel;
    uint8_t search_on;
    uint8_t candidate_on;
    uint8_t candidate_more;
    uint8_t pudding[1];
} ldg_term_ln_view_t;

typedef struct ldg_term_ln
{
    char *text;
    char *ghost;
    char *draft;
    char *search;
    char *taken;
    char (*candidate)[LDG_TERM_LN_CANDIDATE_CAP];
    ldg_term_ln_hist_cb_t hist_cb;
    void *hist_user;
    ldg_term_ln_candidate_cb_t candidate_cb;
    void *candidate_user;
    char hint[LDG_TERM_LN_HINT_CAP];
    char prompt[LDG_TERM_LN_PROMPT_CAP];
    uint32_t cap;
    uint32_t len;
    uint32_t cursor;
    uint32_t ghost_len;
    uint32_t search_len;
    uint32_t hist_sel;
    uint32_t search_sel;
    uint32_t candidate_max;
    uint32_t candidate_cunt;
    uint32_t candidate_sel;
    uint32_t candidate_at;
    uint32_t draw_row;
    uint8_t rdy;
    uint8_t search_on;
    uint8_t candidate_on;
    uint8_t candidate_more;
    uint8_t ghost_hidden;
    uint8_t finish_due;
    uint8_t pudding[2];
} ldg_term_ln_t;

LDG_EXPORT uint32_t ldg_term_ln_conf_default_get(ldg_term_ln_conf_t *out);
LDG_EXPORT uint32_t ldg_term_ln_pool_size_get(const ldg_term_ln_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_term_ln_create(ldg_mem_pool_t *pool, const ldg_term_ln_conf_t *conf, ldg_term_ln_t **out);
LDG_EXPORT uint32_t ldg_term_ln_uncreate(ldg_term_ln_t **ln);
LDG_EXPORT uint32_t ldg_term_ln_rst(ldg_term_ln_t *ln);
LDG_EXPORT uint32_t ldg_term_ln_key_put(ldg_term_ln_t *ln, const ldg_term_key_event_t *event, uint8_t *out_consumed);
LDG_EXPORT uint32_t ldg_term_ln_text_get(const ldg_term_ln_t *ln, const char **out, uint32_t *out_len);
LDG_EXPORT uint32_t ldg_term_ln_text_set(ldg_term_ln_t *ln, const char *text);
LDG_EXPORT uint32_t ldg_term_ln_take(ldg_term_ln_t *ln, char *out, uint64_t cap);
LDG_EXPORT uint32_t ldg_term_ln_prompt_set(ldg_term_ln_t *ln, const char *prompt);
LDG_EXPORT uint32_t ldg_term_ln_hist_cb_set(ldg_term_ln_t *ln, ldg_term_ln_hist_cb_t cb, void *user);
LDG_EXPORT uint32_t ldg_term_ln_candidate_cb_set(ldg_term_ln_t *ln, ldg_term_ln_candidate_cb_t cb, void *user);
LDG_EXPORT uint32_t ldg_term_ln_view_get(const ldg_term_ln_t *ln, ldg_term_ln_view_t *out);
LDG_EXPORT uint32_t ldg_term_ln_draw(const ldg_term_ln_t *ln, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t row_cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *ghost_fg, const ldg_term_render_color_t *bg);
LDG_EXPORT uint32_t ldg_term_ln_encode(ldg_term_ln_t *ln, uint32_t col_cunt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);

#endif
