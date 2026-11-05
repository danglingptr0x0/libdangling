#ifndef LDG_TERM_EMIT_H
#define LDG_TERM_EMIT_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/mem/alloc.h>
#include <dangling/term/term.h>

#define LDG_TERM_EMIT_COLOR_TRUE 0U
#define LDG_TERM_EMIT_COLOR_PALETTE 1U
#define LDG_TERM_EMIT_BUFF_CAP_DEFAULT 262144U
#define LDG_TERM_EMIT_CELL_ENCODED_MAX 96U
#define LDG_TERM_EMIT_PALETTE_CUNT 256U

typedef struct ldg_term_emit_conf
{
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t buff_cap;
    uint8_t color_mode;
    uint8_t sync;
    uint8_t fg_only;
    uint8_t pudding[1];
} ldg_term_emit_conf_t;

typedef struct ldg_term_emit
{
    ldg_mem_pool_t *pool;
    ldg_term_cell_t *prev;
    ldg_term_cell_t *quant;
    int32_t *diffuse;
    uint8_t *buff;
    uint8_t *palette;
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t buff_cap;
    uint32_t cursor_col;
    uint32_t cursor_row;
    uint8_t last_fg[LDG_TERM_RGB_CUNT];
    uint8_t last_bg[LDG_TERM_RGB_CUNT];
    uint8_t color_mode;
    uint8_t sync;
    uint8_t full_redraw;
    uint8_t color_known;
    uint8_t fg_only;
    uint8_t last_attr;
} ldg_term_emit_t;

LDG_EXPORT uint32_t ldg_term_emit_conf_default_get(ldg_term_emit_conf_t *out);
LDG_EXPORT uint32_t ldg_term_emit_pool_size_get(const ldg_term_emit_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_term_emit_create(ldg_mem_pool_t *pool, const ldg_term_emit_conf_t *conf, ldg_term_emit_t **out);
LDG_EXPORT uint32_t ldg_term_emit_destroy(ldg_term_emit_t **emit);
LDG_EXPORT uint32_t ldg_term_emit_rst(ldg_term_emit_t *emit);

LDG_EXPORT uint32_t ldg_term_emit_screen_enter_encode(ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_emit_screen_exit_encode(ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_emit_cell_arr_encode(ldg_term_emit_t *emit, const ldg_term_cell_t *cell, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_emit_cell_arr_wr(ldg_term_emit_t *emit, const ldg_term_cell_t *cell);
LDG_EXPORT uint32_t ldg_term_emit_screen_enter_wr(void);
LDG_EXPORT uint32_t ldg_term_emit_screen_exit_wr(void);

#endif
