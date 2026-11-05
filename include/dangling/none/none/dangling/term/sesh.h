#ifndef LDG_TERM_SESH_H
#define LDG_TERM_SESH_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/term/render.h>
#include <dangling/term/emit.h>

#define LDG_TERM_SESH_TITLE_MAX 256U
#define LDG_TERM_SESH_COL_MAX_DEFAULT 160U
#define LDG_TERM_SESH_ROW_MAX_DEFAULT 64U

typedef struct ldg_term_sesh_conf
{
    uint64_t wr_period;
    ldg_term_render_conf_t render;
    ldg_term_emit_conf_t emit;
    uint32_t col_max;
    uint32_t row_max;
} ldg_term_sesh_conf_t;

typedef struct ldg_term_sesh ldg_term_sesh_t;

LDG_EXPORT uint32_t ldg_term_sesh_conf_default_get(ldg_term_sesh_conf_t *out);
LDG_EXPORT uint32_t ldg_term_sesh_create(const ldg_term_sesh_conf_t *conf, ldg_term_sesh_t **out);
LDG_EXPORT uint32_t ldg_term_sesh_destroy(ldg_term_sesh_t *sesh);
LDG_EXPORT uint32_t ldg_term_sesh_render_get(const ldg_term_sesh_t *sesh, ldg_term_render_t **out);
LDG_EXPORT uint32_t ldg_term_sesh_wr(ldg_term_sesh_t *sesh);
LDG_EXPORT uint32_t ldg_term_sesh_resize(ldg_term_sesh_t *sesh, uint32_t col_cunt, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_term_sesh_title_set(ldg_term_sesh_t *sesh, const char *title, uint64_t len);
LDG_EXPORT uint32_t ldg_term_sesh_raw_wr(ldg_term_sesh_t *sesh, const uint8_t *src, uint64_t len);
LDG_EXPORT uint32_t ldg_term_sesh_dt_get(const ldg_term_sesh_t *sesh, uint64_t *out);

#endif
