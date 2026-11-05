#ifndef LDG_TERM_LAYOUT_H
#define LDG_TERM_LAYOUT_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/rational.h>

typedef struct ldg_term_rect
{
    uint32_t col;
    uint32_t row;
    uint32_t col_cunt;
    uint32_t row_cunt;
} ldg_term_rect_t;

typedef enum ldg_term_layout_axis
{
    LDG_TERM_LAYOUT_AXIS_COL = 0,
    LDG_TERM_LAYOUT_AXIS_ROW
} ldg_term_layout_axis_t;

typedef enum ldg_term_layout_size_kind
{
    LDG_TERM_LAYOUT_SIZE_FIXED = 0,
    LDG_TERM_LAYOUT_SIZE_SHARE
} ldg_term_layout_size_kind_t;

typedef struct ldg_term_layout_size
{
    ldg_math_rational_t share;
    uint32_t cell_cunt;
    uint32_t kind;
} ldg_term_layout_size_t;

LDG_EXPORT uint32_t ldg_term_layout_split(const ldg_term_rect_t *rect, uint32_t axis, const ldg_term_layout_size_t *size, uint32_t size_cunt, ldg_term_rect_t *out);

#endif
