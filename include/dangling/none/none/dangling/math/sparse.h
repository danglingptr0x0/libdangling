#ifndef LDG_MATH_SPARSE_H
#define LDG_MATH_SPARSE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>

typedef struct ldg_math_sparse
{
    uint64_t *row_ptr;
    uint64_t *col_idx;
    double *vals;
    uint64_t *coo_row;
    uint64_t rows;
    uint64_t cols;
    uint64_t max_nnz;
    uint64_t nnz;
    uint64_t coo_cunt;
    uint8_t finalized;
    uint8_t pudding[7];
} ldg_math_sparse_t;

LDG_EXPORT uint32_t ldg_math_sparse_create(uint64_t rows, uint64_t cols, uint64_t max_nnz, ldg_math_sparse_t **out);
LDG_EXPORT uint32_t ldg_math_sparse_destroy(ldg_math_sparse_t **mat);
LDG_EXPORT uint32_t ldg_math_sparse_entry_add(ldg_math_sparse_t *mat, uint64_t row, uint64_t col, double val);
LDG_EXPORT uint32_t ldg_math_sparse_finalize(ldg_math_sparse_t *mat);
LDG_EXPORT uint32_t ldg_math_sparse_val_rst(ldg_math_sparse_t *mat);
LDG_EXPORT uint32_t ldg_math_sparse_elem_idx_get(const ldg_math_sparse_t *mat, uint64_t row, uint64_t col, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_sparse_mv(const ldg_math_sparse_t *mat, const double *vec, double *out);
LDG_EXPORT uint32_t ldg_math_sparse_diag_get(const ldg_math_sparse_t *mat, double *out);
LDG_EXPORT uint8_t ldg_math_sparse_symmetric_is(const ldg_math_sparse_t *mat, double tol);
LDG_EXPORT uint64_t ldg_math_sparse_nnz_get(const ldg_math_sparse_t *mat);
LDG_EXPORT uint64_t ldg_math_sparse_rows_get(const ldg_math_sparse_t *mat);
LDG_EXPORT uint64_t ldg_math_sparse_cols_get(const ldg_math_sparse_t *mat);

#endif
