#ifndef LDG_TERM_TERM_H
#define LDG_TERM_TERM_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#define LDG_TERM_RGB_CUNT 3
#define LDG_TERM_COL_MAX 512
#define LDG_TERM_ROW_MAX 256
#define LDG_TERM_SCROLLBACK_MAX 65536
#define LDG_TERM_COL_DEFAULT 80
#define LDG_TERM_ROW_DEFAULT 24
#define LDG_TERM_SCROLLBACK_DEFAULT 4096
#define LDG_TERM_IMG_CAP_DEFAULT 8388608
#define LDG_TERM_BUFF_CAP_DEFAULT 2097152
#define LDG_TERM_IMG_MAX_DEFAULT 16
#define LDG_TERM_PLACEMENT_MAX_DEFAULT 32
#define LDG_TERM_PALETTE_CUNT 256
#define LDG_TERM_LINK_MAX 64
#define LDG_TERM_LINK_URI_MAX 256
#define LDG_TERM_CWD_MAX 512
#define LDG_TERM_LINK_NONE 0

#define LDG_TERM_COLOR_FG 0
#define LDG_TERM_COLOR_BG 1
#define LDG_TERM_COLOR_CURSOR 2
#define LDG_TERM_COLOR_ROLE_CUNT 3

#define LDG_TERM_MARK_PROMPT 1
#define LDG_TERM_MARK_INPUT 2
#define LDG_TERM_MARK_OUTPUT 3

#define LDG_TERM_CHARSET_ASCII 0
#define LDG_TERM_CHARSET_GRAPHIC 1
#define LDG_TERM_CHARSET_UK 2

#define LDG_TERM_LINE_SINGLE 0
#define LDG_TERM_LINE_DOUBLE_TOP 1
#define LDG_TERM_LINE_DOUBLE_BOTTOM 2
#define LDG_TERM_LINE_DOUBLE_WIDTH 3

#define LDG_TERM_TAB_DEFAULT_SPAN 8
#define LDG_TERM_IMG_FMT_PNG 100
#define LDG_TERM_IMG_FMT_RGB 24
#define LDG_TERM_IMG_FMT_RGBA 32

#define LDG_TERM_ATTR_NONE 0x00U
#define LDG_TERM_ATTR_BOLD 0x01U
#define LDG_TERM_ATTR_DIM 0x02U
#define LDG_TERM_ATTR_ITALIC 0x04U
#define LDG_TERM_ATTR_UNDERLINE 0x08U
#define LDG_TERM_ATTR_BLINK 0x10U
#define LDG_TERM_ATTR_REVERSE 0x20U
#define LDG_TERM_ATTR_HIDDEN 0x40U
#define LDG_TERM_ATTR_STRIKE 0x80U

typedef enum ldg_term_erase
{
    LDG_TERM_ERASE_TO_END = 0,
    LDG_TERM_ERASE_TO_START,
    LDG_TERM_ERASE_ALL,
    LDG_TERM_ERASE_SCROLLBACK
} ldg_term_erase_t;

typedef struct ldg_term_cell
{
    uint8_t ch;
    uint8_t attr;
    uint8_t link;
    uint8_t fg[LDG_TERM_RGB_CUNT];
    uint8_t bg[LDG_TERM_RGB_CUNT];
} ldg_term_cell_t;

typedef struct ldg_term_conf
{
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t scrollback_cunt;
    uint32_t img_cap;
    uint32_t img_max;
    uint32_t placement_max;
    uint32_t buff_cap;
    uint8_t fg[LDG_TERM_RGB_CUNT];
    uint8_t bg[LDG_TERM_RGB_CUNT];
    uint8_t pudding[2];
} ldg_term_conf_t;

typedef struct ldg_term_img
{
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t offset;
    uint32_t size;
    uint8_t fmt;
    uint8_t alive;
    uint8_t pudding[2];
} ldg_term_img_t;

typedef struct ldg_term_placement
{
    uint32_t img_id;
    uint32_t placement_id;
    uint32_t col;
    uint32_t row;
    uint32_t col_span;
    uint32_t row_span;
    int32_t layer;
    uint8_t alive;
    uint8_t pudding[3];
} ldg_term_placement_t;

typedef struct ldg_term
{
    void *pool;
    ldg_term_cell_t *cell;
    ldg_term_cell_t *alt_cell;
    ldg_term_cell_t *scrollback;
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t col_cap;
    uint32_t row_cap;
    uint32_t scrollback_cap;
    uint32_t scrollback_hd;
    uint32_t scrollback_used;
    uint32_t cursor_col;
    uint32_t cursor_row;
    uint32_t saved_cursor_col;
    uint32_t saved_cursor_row;
    uint32_t margin_top;
    uint32_t margin_bottom;
    uint32_t margin_lhs;
    uint32_t margin_rhs;
    uint32_t dirty_top;
    uint32_t dirty_bottom;
    uint8_t cur_attr;
    uint8_t cur_fg[LDG_TERM_RGB_CUNT];
    uint8_t cur_bg[LDG_TERM_RGB_CUNT];
    uint8_t saved_attr;
    uint8_t saved_fg[LDG_TERM_RGB_CUNT];
    uint8_t saved_bg[LDG_TERM_RGB_CUNT];
    uint8_t default_fg[LDG_TERM_RGB_CUNT];
    uint8_t default_bg[LDG_TERM_RGB_CUNT];
    uint8_t alt_active;
    uint8_t cursor_visible;
    uint8_t wrap_pending;
    uint8_t orig_mode;
    uint8_t autowrap;
    uint8_t charset_g0;
    uint8_t charset_g1;
    uint8_t charset_shift;
    uint8_t insert_mode;
    uint8_t newline_mode;
    uint8_t reverse_screen;
    uint8_t pudding[1];
    ldg_term_img_t *img;
    ldg_term_placement_t *placement;
    uint8_t *img_pixel;
    uint8_t *tab_stop;
    uint8_t *line_attr;
    uint8_t *buff;
    uint8_t palette[LDG_TERM_PALETTE_CUNT][LDG_TERM_RGB_CUNT];
    uint8_t role_color[LDG_TERM_COLOR_ROLE_CUNT][LDG_TERM_RGB_CUNT];
    char link_uri[LDG_TERM_LINK_MAX][LDG_TERM_LINK_URI_MAX];
    char cwd[LDG_TERM_CWD_MAX];
    uint32_t link_cunt;
    uint32_t link_active;
    uint32_t cwd_len;
    uint32_t mark_kind;
    uint32_t buff_cap;
    uint8_t sync_output;
    uint8_t margin_side;
    uint8_t pudding_tail[2];
    uint32_t img_cap;
    uint32_t img_max;
    uint32_t placement_max;
    uint32_t img_used;
} ldg_term_t;

LDG_EXPORT uint32_t ldg_term_conf_default_get(ldg_term_conf_t *out);
LDG_EXPORT uint32_t ldg_term_pool_size_get(const ldg_term_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_term_create(ldg_mem_pool_t *pool, const ldg_term_conf_t *conf, ldg_term_t **out);
LDG_EXPORT uint32_t ldg_term_destroy(ldg_term_t **term);
LDG_EXPORT uint32_t ldg_term_rst(ldg_term_t *term);

LDG_EXPORT uint32_t ldg_term_ch_put(ldg_term_t *term, uint8_t ch);
LDG_EXPORT uint32_t ldg_term_line_feed(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_carriage_ret(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_backspace(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_tab_advance(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_scroll_up(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_scroll_down(ldg_term_t *term, uint32_t cunt);

LDG_EXPORT uint32_t ldg_term_cursor_set(ldg_term_t *term, uint32_t col, uint32_t row);
LDG_EXPORT uint32_t ldg_term_cursor_get(const ldg_term_t *term, uint32_t *out_col, uint32_t *out_row);
LDG_EXPORT uint32_t ldg_term_cursor_col_mov(ldg_term_t *term, int32_t delta);
LDG_EXPORT uint32_t ldg_term_cursor_row_mov(ldg_term_t *term, int32_t delta);
LDG_EXPORT uint32_t ldg_term_cursor_save(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_cursor_restore(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_cursor_visible_set(ldg_term_t *term, uint8_t visible);
LDG_EXPORT uint8_t ldg_term_cursor_visible_is(const ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_palette_set(ldg_term_t *term, uint32_t idx, uint8_t red, uint8_t green, uint8_t blue);
LDG_EXPORT uint32_t ldg_term_palette_get(const ldg_term_t *term, uint32_t idx, uint8_t *out);
LDG_EXPORT uint32_t ldg_term_palette_rst(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_role_color_set(ldg_term_t *term, uint32_t role, uint8_t red, uint8_t green, uint8_t blue);
LDG_EXPORT uint32_t ldg_term_role_color_get(const ldg_term_t *term, uint32_t role, uint8_t *out);
LDG_EXPORT uint32_t ldg_term_link_open(ldg_term_t *term, const char *uri, uint64_t len);
LDG_EXPORT uint32_t ldg_term_link_close(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_link_uri_get(const ldg_term_t *term, uint32_t id, char *out, uint64_t cap);
LDG_EXPORT uint32_t ldg_term_cwd_set(ldg_term_t *term, const char *cwd, uint64_t len);
LDG_EXPORT uint32_t ldg_term_cwd_get(const ldg_term_t *term, char *out, uint64_t cap);
LDG_EXPORT uint32_t ldg_term_mark_set(ldg_term_t *term, uint32_t kind);
LDG_EXPORT uint32_t ldg_term_mark_get(const ldg_term_t *term, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_sync_set(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint8_t ldg_term_sync_is(const ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_charset_set(ldg_term_t *term, uint8_t slot, uint8_t charset);
LDG_EXPORT uint32_t ldg_term_charset_shift(ldg_term_t *term, uint8_t slot);
LDG_EXPORT uint32_t ldg_term_tab_set(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_tab_clear(ldg_term_t *term, uint8_t all);
LDG_EXPORT uint32_t ldg_term_tab_back(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_tab_advance_cunt(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_insert_mode_set(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint32_t ldg_term_newline_mode_set(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint32_t ldg_term_reverse_set(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint8_t ldg_term_reverse_is(const ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_line_attr_set(ldg_term_t *term, uint8_t attr);
LDG_EXPORT uint32_t ldg_term_line_attr_get(const ldg_term_t *term, uint32_t row, uint8_t *out);
LDG_EXPORT uint32_t ldg_term_align_fill(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_soft_rst(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_img_st(ldg_term_t *term, uint32_t id, uint32_t width, uint32_t height, uint8_t fmt, const uint8_t *pixel, uint32_t size);
LDG_EXPORT uint32_t ldg_term_img_drop(ldg_term_t *term, uint32_t id);
LDG_EXPORT uint32_t ldg_term_img_get(const ldg_term_t *term, uint32_t id, ldg_term_img_t *out);
LDG_EXPORT uint32_t ldg_term_img_pixel_get(const ldg_term_t *term, uint32_t id, const uint8_t **out, uint32_t *out_size);
LDG_EXPORT uint32_t ldg_term_placement_add(ldg_term_t *term, const ldg_term_placement_t *placement);
LDG_EXPORT uint32_t ldg_term_placement_cunt_get(const ldg_term_t *term, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_placement_get(const ldg_term_t *term, uint32_t idx, ldg_term_placement_t *out);
LDG_EXPORT uint32_t ldg_term_placement_clear(ldg_term_t *term);

LDG_EXPORT uint32_t ldg_term_orig_set(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint8_t ldg_term_orig_is(const ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_autowrap_set(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint8_t ldg_term_autowrap_is(const ldg_term_t *term);

LDG_EXPORT uint32_t ldg_term_attr_set(ldg_term_t *term, uint8_t attr);
LDG_EXPORT uint32_t ldg_term_attr_clear(ldg_term_t *term, uint8_t attr);
LDG_EXPORT uint32_t ldg_term_attr_rst(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_fg_set(ldg_term_t *term, uint8_t red, uint8_t green, uint8_t blue);
LDG_EXPORT uint32_t ldg_term_bg_set(ldg_term_t *term, uint8_t red, uint8_t green, uint8_t blue);

LDG_EXPORT uint32_t ldg_term_display_erase(ldg_term_t *term, ldg_term_erase_t mode);
LDG_EXPORT uint32_t ldg_term_line_erase(ldg_term_t *term, ldg_term_erase_t mode);
LDG_EXPORT uint32_t ldg_term_line_insert(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_line_delete(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_ch_delete(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_ch_insert(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_ch_erase(ldg_term_t *term, uint32_t cunt);
LDG_EXPORT uint32_t ldg_term_resize(ldg_term_t *term, uint32_t col_cunt, uint32_t row_cunt);

LDG_EXPORT uint32_t ldg_term_margin_set(ldg_term_t *term, uint32_t top, uint32_t bottom);
LDG_EXPORT uint32_t ldg_term_margin_side_set(ldg_term_t *term, uint32_t lhs, uint32_t rhs);
LDG_EXPORT uint32_t ldg_term_margin_side_en(ldg_term_t *term, uint8_t en);
LDG_EXPORT uint32_t ldg_term_margin_rst(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_alt_enter(ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_alt_exit(ldg_term_t *term);
LDG_EXPORT uint8_t ldg_term_alt_active_is(const ldg_term_t *term);

LDG_EXPORT uint32_t ldg_term_cell_get(const ldg_term_t *term, uint32_t col, uint32_t row, ldg_term_cell_t *out);
LDG_EXPORT uint32_t ldg_term_row_get(const ldg_term_t *term, uint32_t row, ldg_term_cell_t *out, uint32_t cap);
LDG_EXPORT uint32_t ldg_term_scrollback_row_get(const ldg_term_t *term, uint32_t back, ldg_term_cell_t *out, uint32_t cap);
LDG_EXPORT uint32_t ldg_term_scrollback_cunt_get(const ldg_term_t *term, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_dirty_get(const ldg_term_t *term, uint32_t *out_top, uint32_t *out_bottom);
LDG_EXPORT uint32_t ldg_term_dirty_clear(ldg_term_t *term);

#endif
