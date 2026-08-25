
#include <dangling/math/sparse.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

static void sparse_destroy_internal(ldg_math_sparse_t *mat)
{
    if (mat->row_ptr) { ldg_mem_dealloc(mat->row_ptr); }

    if (mat->vals) { ldg_mem_dealloc(mat->vals); }

    if (mat->col_idx) { ldg_mem_dealloc(mat->col_idx); }

    if (mat->coo_row) { ldg_mem_dealloc(mat->coo_row); }

    ldg_mem_dealloc(mat);
}

static uint8_t sparse_triplet_gt(uint64_t row_a, uint64_t col_a, uint64_t row_b, uint64_t col_b)
{
    if (row_a != row_b) { return (row_a > row_b) ? 1 : 0; }

    return (col_a > col_b) ? 1 : 0;
}

static void sparse_triplet_swap(uint64_t *rows, uint64_t *cols, double *vals, uint64_t first, uint64_t second)
{
    uint64_t tmp_row = 0;
    uint64_t tmp_col = 0;
    double tmp_val = 0.0;

    tmp_row = rows[first];
    rows[first] = rows[second];
    rows[second] = tmp_row;

    tmp_col = cols[first];
    cols[first] = cols[second];
    cols[second] = tmp_col;

    tmp_val = vals[first];
    vals[first] = vals[second];
    vals[second] = tmp_val;
}

static void sparse_heap_sift(uint64_t *rows, uint64_t *cols, double *vals, uint64_t start, uint64_t cunt)
{
    uint64_t root = 0;
    uint64_t child = 0;
    uint64_t target = 0;

    root = start;

    while ((root * 2 + 1) < cunt)
    {
        child = root * 2 + 1;
        target = root;

        if (sparse_triplet_gt(rows[child], cols[child], rows[target], cols[target])) { target = child; }

        if ((child + 1) < cunt && sparse_triplet_gt(rows[child + 1], cols[child + 1], rows[target], cols[target])) { target = child + 1; }

        if (target == root) { return; }

        sparse_triplet_swap(rows, cols, vals, root, target);
        root = target;
    }
}

static void sparse_heap_sort(uint64_t *rows, uint64_t *cols, double *vals, uint64_t cunt)
{
    uint64_t start = 0;
    uint64_t end = 0;

    if (cunt < 2) { return; }

    start = cunt / 2;
    while (start > 0)
    {
        start--;
        sparse_heap_sift(rows, cols, vals, start, cunt);
    }

    end = cunt;
    while (end > 1)
    {
        end--;
        sparse_triplet_swap(rows, cols, vals, 0, end);
        sparse_heap_sift(rows, cols, vals, 0, end);
    }
}

static uint8_t sparse_find(const ldg_math_sparse_t *mat, uint64_t row, uint64_t col, double *val_out)
{
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t mid = 0;

    start = mat->row_ptr[row];
    end = mat->row_ptr[row + 1];

    while (start < end)
    {
        mid = start + (end - start) / 2;
        if (mat->col_idx[mid] == col)
        {
            *val_out = mat->vals[mid];
            return 1;
        }

        if (mat->col_idx[mid] < col) { start = mid + 1; }
        else { end = mid; }
    }

    return 0;
}

LDG_EXPORT uint32_t ldg_math_sparse_create(uint64_t rows, uint64_t cols, uint64_t max_nnz, ldg_math_sparse_t **out)
{
    ldg_math_sparse_t *mat = 0x0;
    void *tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(rows == 0 || cols == 0 || max_nnz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(rows > UINT64_MAX / (uint64_t)sizeof(uint64_t) - 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_nnz > UINT64_MAX / (uint64_t)sizeof(double))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_sparse_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    mat = (ldg_math_sparse_t *)tmp;
    if (LDG_UNLIKELY(ldg_mem_zero(mat, sizeof(ldg_math_sparse_t)) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(mat);
        return LDG_ERR_ALLOC_NULL;
    }

    mat->rows = rows;
    mat->cols = cols;
    mat->max_nnz = max_nnz;

    tmp = 0x0;
    ret = ldg_mem_alloc(max_nnz * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        sparse_destroy_internal(mat);
        return LDG_ERR_ALLOC_NULL;
    }

    mat->coo_row = (uint64_t *)tmp;

    tmp = 0x0;
    ret = ldg_mem_alloc(max_nnz * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        sparse_destroy_internal(mat);
        return LDG_ERR_ALLOC_NULL;
    }

    mat->col_idx = (uint64_t *)tmp;

    tmp = 0x0;
    ret = ldg_mem_alloc(max_nnz * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        sparse_destroy_internal(mat);
        return LDG_ERR_ALLOC_NULL;
    }

    mat->vals = (double *)tmp;

    tmp = 0x0;
    ret = ldg_mem_alloc((rows + 1) * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        sparse_destroy_internal(mat);
        return LDG_ERR_ALLOC_NULL;
    }

    mat->row_ptr = (uint64_t *)tmp;

    *out = mat;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_destroy(ldg_math_sparse_t **mat)
{
    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!*mat)) { return LDG_ERR_AOK; }

    sparse_destroy_internal(*mat);
    *mat = 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_entry_add(ldg_math_sparse_t *mat, uint64_t row, uint64_t col, double val)
{
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t mid = 0;
    uint64_t found = 0;

    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row >= mat->rows || col >= mat->cols)) { return LDG_ERR_BOUNDS; }

    if (!mat->finalized)
    {
        if (LDG_UNLIKELY(mat->coo_cunt >= mat->max_nnz)) { return LDG_ERR_BOUNDS; }

        mat->coo_row[mat->coo_cunt] = row;
        mat->col_idx[mat->coo_cunt] = col;
        mat->vals[mat->coo_cunt] = val;
        mat->coo_cunt++;

        return LDG_ERR_AOK;
    }

    start = mat->row_ptr[row];
    end = mat->row_ptr[row + 1];
    found = UINT64_MAX;

    while (start < end)
    {
        mid = start + (end - start) / 2;
        if (mat->col_idx[mid] == col)
        {
            found = mid;
            break;
        }

        if (mat->col_idx[mid] < col) { start = mid + 1; }
        else { end = mid; }
    }

    if (LDG_UNLIKELY(found == UINT64_MAX)) { return LDG_ERR_NOT_FOUND; }

    mat->vals[found] += val;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_finalize(ldg_math_sparse_t *mat)
{
    uint64_t rd_idx = 0;
    uint64_t wr_idx = 0;
    uint64_t row = 0;

    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mat->finalized)) { return LDG_ERR_MATH_STATE; }

    sparse_heap_sort(mat->coo_row, mat->col_idx, mat->vals, mat->coo_cunt);

    wr_idx = 0;
    for (rd_idx = 0; rd_idx < mat->coo_cunt; rd_idx++) { if (wr_idx > 0 && mat->coo_row[rd_idx] == mat->coo_row[wr_idx - 1] && mat->col_idx[rd_idx] == mat->col_idx[wr_idx - 1]) { mat->vals[wr_idx - 1] += mat->vals[rd_idx]; }
        else
        {
            mat->coo_row[wr_idx] = mat->coo_row[rd_idx];
            mat->col_idx[wr_idx] = mat->col_idx[rd_idx];
            mat->vals[wr_idx] = mat->vals[rd_idx];
            wr_idx++;
        } }

    mat->nnz = wr_idx;

    for (row = 0; row <= mat->rows; row++) { mat->row_ptr[row] = 0; }
    for (rd_idx = 0; rd_idx < mat->nnz; rd_idx++) { mat->row_ptr[mat->coo_row[rd_idx] + 1]++; }
    for (row = 0; row < mat->rows; row++) { mat->row_ptr[row + 1] += mat->row_ptr[row]; }

    mat->finalized = 1;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_val_rst(ldg_math_sparse_t *mat)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_MATH_STATE; }

    for (idx = 0; idx < mat->nnz; idx++) { mat->vals[idx] = 0.0; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_elem_idx_get(const ldg_math_sparse_t *mat, uint64_t row, uint64_t col, uint64_t *out)
{
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t mid = 0;
    uint64_t next = 0;

    if (LDG_UNLIKELY(!mat || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT64_MAX;

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(row >= mat->rows || col >= mat->cols)) { return LDG_ERR_BOUNDS; }

    next = row;
    next++;

    start = mat->row_ptr[row];
    end = mat->row_ptr[next];

    while (start < end)
    {
        mid = start + (end - start) / 2;

        if (mat->col_idx[mid] == col)
        {
            *out = mid;
            return LDG_ERR_AOK;
        }

        if (mat->col_idx[mid] < col)
        {
            start = mid;
            start++;
        }
        else { end = mid; }
    }

    return LDG_ERR_NOT_FOUND;
}

LDG_EXPORT uint32_t ldg_math_sparse_mv(const ldg_math_sparse_t *mat, const double *vec, double *out)
{
    uint64_t row = 0;
    uint64_t nz = 0;
    uint64_t end = 0;
    double acc = 0.0;

    if (LDG_UNLIKELY(!mat || !vec || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_MATH_STATE; }

    for (row = 0; row < mat->rows; row++)
    {
        acc = 0.0;
        end = mat->row_ptr[row + 1];
        for (nz = mat->row_ptr[row]; nz < end; nz++) { acc += mat->vals[nz] * vec[mat->col_idx[nz]]; }
        out[row] = acc;
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_diag_get(const ldg_math_sparse_t *mat, double *out)
{
    uint64_t row = 0;
    uint64_t nz = 0;
    uint64_t end = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_MATH_STATE; }

    for (row = 0; row < mat->rows; row++)
    {
        out[row] = 0.0;
        end = mat->row_ptr[row + 1];
        for (nz = mat->row_ptr[row]; nz < end; nz++) { if (mat->col_idx[nz] == row)
            {
                out[row] = mat->vals[nz];
                break;
            }
        }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_math_sparse_symmetric_is(const ldg_math_sparse_t *mat, double tol)
{
    uint64_t row = 0;
    uint64_t nz = 0;
    uint64_t end = 0;
    uint64_t col = 0;
    double aij = 0.0;
    double aji = 0.0;
    double diff = 0.0;

    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_NOT_INIT; }

    if (mat->rows != mat->cols) { return LDG_TRUTH_FALSE; }

    for (row = 0; row < mat->rows; row++)
    {
        end = mat->row_ptr[row + 1];
        for (nz = mat->row_ptr[row]; nz < end; nz++)
        {
            col = mat->col_idx[nz];
            aij = mat->vals[nz];
            aji = 0.0;
            if (!sparse_find(mat, col, row, &aji)) { return LDG_TRUTH_FALSE; }

            diff = __builtin_fabs(aij - aji);
            if (diff > tol) { return LDG_TRUTH_FALSE; }
        }
    }

    return LDG_TRUTH_TRUE;
}

LDG_EXPORT uint64_t ldg_math_sparse_nnz_get(const ldg_math_sparse_t *mat)
{
    if (LDG_UNLIKELY(!mat)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(!mat->finalized)) { return UINT64_MAX; }

    return mat->nnz;
}

LDG_EXPORT uint64_t ldg_math_sparse_rows_get(const ldg_math_sparse_t *mat)
{
    if (LDG_UNLIKELY(!mat)) { return UINT64_MAX; }

    return mat->rows;
}

LDG_EXPORT uint64_t ldg_math_sparse_cols_get(const ldg_math_sparse_t *mat)
{
    if (LDG_UNLIKELY(!mat)) { return UINT64_MAX; }

    return mat->cols;
}
