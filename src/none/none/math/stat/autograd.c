#include <stdint.h>

#include <dangling/math/stat.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define AUTOGRAD_TANH_SAT_LIMIT 20.0
#define AUTOGRAD_TANH_SAT_FLOOR (-20.0)
#define AUTOGRAD_TANH_MAX 1.0
#define AUTOGRAD_TANH_MIN (-1.0)
#define AUTOGRAD_TANH_EXP_FACTOR 2.0
#define AUTOGRAD_SIGMOID_HALF 0.5
#define AUTOGRAD_SQUARE_GRAD_FACTOR 2.0
#define AUTOGRAD_ROOT_SEED 1.0
#define AUTOGRAD_RATIO_CENTER 1.0
#define AUTOGRAD_NO_SCALAR 0.0
#define AUTOGRAD_RELU_FLOOR 0.0
#define AUTOGRAD_SCALAR_ROW 1
#define AUTOGRAD_SCALAR_COL 1
#define AUTOGRAD_SCALAR_ELEM 1
#define AUTOGRAD_RHS_NONE UINT32_MAX
#define AUTOGRAD_AUX_NONE 0
#define AUTOGRAD_MASK_ON 1.0
#define AUTOGRAD_MASK_NEG (-1000000000.0)
#define AUTOGRAD_POOL_EMPTY 0.0
#define AUTOGRAD_SYNAPSE_SRC 0
#define AUTOGRAD_SYNAPSE_DST 1
#define AUTOGRAD_NEURON_KIND 0
#define AUTOGRAD_NEURON_ACT 1
#define AUTOGRAD_NEURON_LEAK 2
#define AUTOGRAD_NEURON_THRESHOLD 3
#define AUTOGRAD_NEURON_RST 4
#define AUTOGRAD_NEURON_REFRACTORY 5
#define AUTOGRAD_NEURON_MEMBRANE 0
#define AUTOGRAD_NEURON_REFRACT 1
#define AUTOGRAD_NEURON_DERIV 2
#define AUTOGRAD_SURROGATE_SLOPE 1.0
#define AUTOGRAD_UNIT_GRAD 1.0
#define AUTOGRAD_SPIKE_UNIT 1.0
#define AUTOGRAD_REFRACT_UNIT 1.0
#define AUTOGRAD_REFRACT_NONE 0.0
#define AUTOGRAD_FIRE_GAP 0.0
#define AUTOGRAD_KWIN_SILENT 0.0
#define AUTOGRAD_CONV_IN_H 0
#define AUTOGRAD_CONV_IN_W 1
#define AUTOGRAD_CONV_IN_C 2
#define AUTOGRAD_CONV_OUT_C 3
#define AUTOGRAD_CONV_KERNEL_H 4
#define AUTOGRAD_CONV_KERNEL_W 5
#define AUTOGRAD_CONV_STRIDE 6
#define AUTOGRAD_CONV_PAD 7
#define AUTOGRAD_CONV_WIN_ORIG 1
#define AUTOGRAD_PAIR_CUM_SEED 0.0
#define AUTOGRAD_EUCLID_GRAD_FACTOR 2.0
#define AUTOGRAD_POOL_IN_H 0
#define AUTOGRAD_POOL_IN_W 1
#define AUTOGRAD_POOL_IN_C 2
#define AUTOGRAD_POOL_KERNEL_H 3
#define AUTOGRAD_POOL_KERNEL_W 4
#define AUTOGRAD_POOL_STRIDE 5
#define AUTOGRAD_POOL_MAX_ON 1.0
#define AUTOGRAD_POOL_MAX_OFF 0.0
#define AUTOGRAD_POOL_WIN_FIRST 0
#define AUTOGRAD_CONV_PAD_SIDE_CUNT 2
#define AUTOGRAD_GELU_SQRT_2_OVER_PI 0.7978845608
#define AUTOGRAD_GELU_CUBIC 0.044715
#define AUTOGRAD_GELU_HALF 0.5
#define AUTOGRAD_GELU_CUBIC_GRAD_FACTOR 3.0

typedef enum autograd_op
{
    AUTOGRAD_OP_LEAF = 0,
    AUTOGRAD_OP_MATMUL,
    AUTOGRAD_OP_BIAS_ADD,
    AUTOGRAD_OP_ROW_MUL,
    AUTOGRAD_OP_ADD,
    AUTOGRAD_OP_SUB,
    AUTOGRAD_OP_MUL,
    AUTOGRAD_OP_MIN,
    AUTOGRAD_OP_GATHER,
    AUTOGRAD_OP_SCALE,
    AUTOGRAD_OP_SHIFT,
    AUTOGRAD_OP_CLIP,
    AUTOGRAD_OP_TANH,
    AUTOGRAD_OP_SIGMOID,
    AUTOGRAD_OP_RELU,
    AUTOGRAD_OP_EXP,
    AUTOGRAD_OP_SQUARE,
    AUTOGRAD_OP_LOG_SOFTMAX,
    AUTOGRAD_OP_ROW_SUM,
    AUTOGRAD_OP_MEAN,
    AUTOGRAD_OP_ROW_NORM,
    AUTOGRAD_OP_ROW_REPEAT,
    AUTOGRAD_OP_ROW_POOL,
    AUTOGRAD_OP_ATTN_SCORE,
    AUTOGRAD_OP_ATTN_MASK,
    AUTOGRAD_OP_ATTN_APPLY,
    AUTOGRAD_OP_COL_MUL,
    AUTOGRAD_OP_COL_CONCAT,
    AUTOGRAD_OP_SCALAR_MUL,
    AUTOGRAD_OP_SYNAPSE,
    AUTOGRAD_OP_NEURON,
    AUTOGRAD_OP_KWIN,
    AUTOGRAD_OP_CONV,
    AUTOGRAD_OP_GROUP_BIAS,
    AUTOGRAD_OP_COSINE,
    AUTOGRAD_OP_EUCLID,
    AUTOGRAD_OP_POOL,
    AUTOGRAD_OP_EMBEDDING,
    AUTOGRAD_OP_GELU,
    AUTOGRAD_OP_RESHAPE,
    AUTOGRAD_OP_TILE,
    AUTOGRAD_OP_ROW_RMS,
    AUTOGRAD_OP_CUNT
} autograd_op_t;

typedef struct autograd_conv_geom
{
    uint32_t in_h;
    uint32_t in_w;
    uint32_t in_c;
    uint32_t out_c;
    uint32_t kernel_h;
    uint32_t kernel_w;
    uint32_t stride;
    uint32_t pad;
    uint32_t out_h;
    uint32_t out_w;
} autograd_conv_geom_t;

static double autograd_tanh_get(double val)
{
    double scaled = 0.0;

    if (val > AUTOGRAD_TANH_SAT_LIMIT) { return AUTOGRAD_TANH_MAX; }

    if (val < AUTOGRAD_TANH_SAT_FLOOR) { return AUTOGRAD_TANH_MIN; }

    scaled = ldg_math_exp_get(val * AUTOGRAD_TANH_EXP_FACTOR);

    return (scaled - AUTOGRAD_TANH_MAX) / (scaled + AUTOGRAD_TANH_MAX);
}

static uintptr_t autograd_node_elem_get(const ldg_math_stat_autograd_t *graph, uint32_t node)
{
    return (uintptr_t)graph->node[node].row_cunt * (uintptr_t)graph->node[node].col_cunt;
}

static double* autograd_val_ptr_get(const ldg_math_stat_autograd_t *graph, uint32_t node)
{
    return graph->val + graph->node[node].val_off;
}

static double* autograd_grad_ptr_get(const ldg_math_stat_autograd_t *graph, uint32_t node)
{
    return graph->grad + graph->node[node].grad_off;
}

static uint32_t autograd_node_valid_ck(const ldg_math_stat_autograd_t *graph, uint32_t node)
{
    if (LDG_UNLIKELY(node >= graph->node_cunt)) { return LDG_ERR_BOUNDS; }

    return LDG_ERR_AOK;
}

static uint32_t autograd_node_push(ldg_math_stat_autograd_t *graph, uint32_t row_cunt, uint32_t col_cunt, uint8_t grad_is, uint32_t *out)
{
    uintptr_t elem = 0;

    if (LDG_UNLIKELY(row_cunt == 0 || col_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node_cunt >= graph->node_cap)) { return LDG_ERR_FULL; }

    elem = (uintptr_t)row_cunt * (uintptr_t)col_cunt;

    if (LDG_UNLIKELY(elem > graph->elem_cap - graph->elem_cunt)) { return LDG_ERR_FULL; }

    graph->node[graph->node_cunt].val_off = graph->elem_cunt;
    graph->node[graph->node_cunt].grad_off = graph->elem_cunt;
    graph->node[graph->node_cunt].row_cunt = row_cunt;
    graph->node[graph->node_cunt].col_cunt = col_cunt;
    graph->node[graph->node_cunt].grad_is = grad_is;

    *out = graph->node_cunt;

    graph->node_cunt++;
    graph->elem_cunt = graph->elem_cunt + elem;

    return LDG_ERR_AOK;
}

static uint32_t autograd_tape_push(ldg_math_stat_autograd_t *graph, uint32_t op, uint32_t out, uint32_t lhs, uint32_t rhs, double scalar)
{
    if (LDG_UNLIKELY(graph->tape_cunt >= graph->tape_cap)) { return LDG_ERR_FULL; }

    graph->tape[graph->tape_cunt].op = op;
    graph->tape[graph->tape_cunt].out = out;
    graph->tape[graph->tape_cunt].lhs = lhs;
    graph->tape[graph->tape_cunt].rhs = rhs;
    graph->tape[graph->tape_cunt].scalar = scalar;
    graph->tape[graph->tape_cunt].aux = AUTOGRAD_AUX_NONE;
    graph->tape[graph->tape_cunt].ref = 0x0;

    graph->tape_cunt++;

    return LDG_ERR_AOK;
}

static void autograd_tape_ref_set(ldg_math_stat_autograd_t *graph, const void *ref)
{
    graph->tape[graph->tape_cunt - AUTOGRAD_SCALAR_ELEM].ref = ref;
}

static uint8_t autograd_mask_on_is(const uint8_t *mask_row, uint32_t col)
{
    if (!mask_row || mask_row[col] == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t autograd_mask_ck(const uint8_t *mask, uint32_t row_cunt, uint32_t width)
{
    const uint8_t *mask_row = mask;
    uint32_t allowed = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    if (!mask) { return LDG_ERR_AOK; }

    while (row < row_cunt)
    {
        allowed = 0;
        col = 0;
        while (col < width)
        {
            if (LDG_UNLIKELY(mask_row[col] > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

            allowed = allowed + mask_row[col];
            col++;
        }

        if (LDG_UNLIKELY(allowed == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        mask_row = mask_row + width;
        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t autograd_unary_prep(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return autograd_node_push(graph, graph->node[lhs].row_cunt, graph->node[lhs].col_cunt, LDG_TRUTH_TRUE, out);
}

static uint32_t autograd_pairwise_prep(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[lhs].row_cunt != graph->node[rhs].row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[lhs].col_cunt != graph->node[rhs].col_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return autograd_node_push(graph, graph->node[lhs].row_cunt, graph->node[lhs].col_cunt, LDG_TRUTH_TRUE, out);
}

static uint32_t autograd_row_vec_prep(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != AUTOGRAD_SCALAR_ROW)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[rhs].col_cunt != graph->node[lhs].col_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return autograd_node_push(graph, graph->node[lhs].row_cunt, graph->node[lhs].col_cunt, LDG_TRUTH_TRUE, out);
}

uint32_t ldg_math_stat_autograd_init(ldg_math_stat_autograd_t *graph, uint32_t node_cap, uint32_t tape_cap, uintptr_t elem_cap)
{
    void *val_ptr = 0x0;
    void *grad_ptr = 0x0;
    void *node_ptr = 0x0;
    void *tape_ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(node_cap == 0 || tape_cap == 0 || elem_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_alloc((uint64_t)elem_cap * (uint64_t)sizeof(double), &val_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc((uint64_t)elem_cap * (uint64_t)sizeof(double), &grad_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(val_ptr);
        return ret;
    }

    ret = ldg_mem_alloc((uint64_t)node_cap * (uint64_t)sizeof(ldg_math_stat_autograd_node_t), &node_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(grad_ptr);
        ldg_mem_dealloc(val_ptr);
        return ret;
    }

    ret = ldg_mem_alloc((uint64_t)tape_cap * (uint64_t)sizeof(ldg_math_stat_autograd_tape_t), &tape_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(node_ptr);
        ldg_mem_dealloc(grad_ptr);
        ldg_mem_dealloc(val_ptr);
        return ret;
    }

    graph->val = (double *)val_ptr;
    graph->grad = (double *)grad_ptr;
    graph->node = (ldg_math_stat_autograd_node_t *)node_ptr;
    graph->tape = (ldg_math_stat_autograd_tape_t *)tape_ptr;
    graph->elem_cap = elem_cap;
    graph->elem_cunt = 0;
    graph->elem_mark = 0;
    graph->node_cap = node_cap;
    graph->node_cunt = 0;
    graph->node_mark = 0;
    graph->tape_cap = tape_cap;
    graph->tape_cunt = 0;
    graph->is_init = LDG_TRUTH_TRUE;

    ret = ldg_mem_zero(graph->val, (uint64_t)elem_cap * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(graph->grad, (uint64_t)elem_cap * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_autograd_shutdown(ldg_math_stat_autograd_t *graph)
{
    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (graph->is_init != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ldg_mem_dealloc(graph->tape);
    ldg_mem_dealloc(graph->node);
    ldg_mem_dealloc(graph->grad);
    ldg_mem_dealloc(graph->val);

    graph->val = 0x0;
    graph->grad = 0x0;
    graph->node = 0x0;
    graph->tape = 0x0;
    graph->elem_cap = 0;
    graph->elem_cunt = 0;
    graph->elem_mark = 0;
    graph->node_cap = 0;
    graph->node_cunt = 0;
    graph->node_mark = 0;
    graph->tape_cap = 0;
    graph->tape_cunt = 0;
    graph->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_param_mark_set(ldg_math_stat_autograd_t *graph)
{
    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    graph->node_mark = graph->node_cunt;
    graph->elem_mark = graph->elem_cunt;
    graph->tape_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_tape_rst(ldg_math_stat_autograd_t *graph)
{
    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    graph->node_cunt = graph->node_mark;
    graph->elem_cunt = graph->elem_mark;
    graph->tape_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_rst(ldg_math_stat_autograd_t *graph)
{
    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    graph->node_cunt = 0;
    graph->node_mark = 0;
    graph->elem_cunt = 0;
    graph->elem_mark = 0;
    graph->tape_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_grad_zero(ldg_math_stat_autograd_t *graph)
{
    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_mem_zero(graph->grad, (uint64_t)graph->elem_cunt * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_autograd_tensor_push(ldg_math_stat_autograd_t *graph, uint32_t row_cunt, uint32_t col_cunt, uint8_t grad_is, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_push(graph, row_cunt, col_cunt, grad_is, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(autograd_val_ptr_get(graph, *out), (uint64_t)autograd_node_elem_get(graph, *out) * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_autograd_val_set(ldg_math_stat_autograd_t *graph, uint32_t node, const double *src, uintptr_t cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cunt != autograd_node_elem_get(graph, node))) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_mem_cpy(autograd_val_ptr_get(graph, node), src, (uint64_t)cunt * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_autograd_val_get(const ldg_math_stat_autograd_t *graph, uint32_t node, double *out, uintptr_t cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < autograd_node_elem_get(graph, node))) { return LDG_ERR_BOUNDS; }

    return ldg_mem_cpy(out, autograd_val_ptr_get(graph, node), (uint64_t)autograd_node_elem_get(graph, node) * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_autograd_grad_get(const ldg_math_stat_autograd_t *graph, uint32_t node, double *out, uintptr_t cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < autograd_node_elem_get(graph, node))) { return LDG_ERR_BOUNDS; }

    return ldg_mem_cpy(out, autograd_grad_ptr_get(graph, node), (uint64_t)autograd_node_elem_get(graph, node) * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_autograd_scalar_get(const ldg_math_stat_autograd_t *graph, uint32_t node, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(autograd_node_elem_get(graph, node) != AUTOGRAD_SCALAR_ELEM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = autograd_val_ptr_get(graph, node)[0];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_matmul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_row = 0x0;
    const double *rhs_val = 0x0;
    const double *rhs_row = 0x0;
    double *out_ptr = 0x0;
    double cum = 0.0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t inner = 0;
    uint32_t inner_cunt = 0;
    uint32_t out_col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[lhs].col_cunt != graph->node[rhs].row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inner_cunt = graph->node[lhs].col_cunt;
    out_col = graph->node[rhs].col_cunt;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, out_col, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_row = autograd_val_ptr_get(graph, lhs);
    rhs_val = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    ret = ldg_mem_zero(out_ptr, (uint64_t)autograd_node_elem_get(graph, *out) * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (row < graph->node[lhs].row_cunt)
    {
        inner = 0;
        while (inner < inner_cunt)
        {
            cum = lhs_row[inner];
            if (cum != AUTOGRAD_NO_SCALAR)
            {
                rhs_row = rhs_val + ((uintptr_t)inner * (uintptr_t)out_col);
                col = 0;
                while (col < out_col)
                {
                    out_ptr[col] = out_ptr[col] + (cum * rhs_row[col]);
                    col++;
                }
            }

            inner++;
        }

        lhs_row = lhs_row + inner_cunt;
        out_ptr = out_ptr + out_col;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_MATMUL, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_bias_add_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_val = 0x0;
    double *out_ptr = 0x0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t width = 0;
    uint32_t ret = 0;

    ret = autograd_row_vec_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_val = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    width = graph->node[lhs].col_cunt;

    while (row < graph->node[lhs].row_cunt)
    {
        col = 0;
        while (col < width)
        {
            *out_ptr = *lhs_ptr + rhs_val[col];
            out_ptr++;
            lhs_ptr++;
            col++;
        }

        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_BIAS_ADD, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_row_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_val = 0x0;
    double *out_ptr = 0x0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t width = 0;
    uint32_t ret = 0;

    ret = autograd_row_vec_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_val = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    width = graph->node[lhs].col_cunt;

    while (row < graph->node[lhs].row_cunt)
    {
        col = 0;
        while (col < width)
        {
            *out_ptr = *lhs_ptr * rhs_val[col];
            out_ptr++;
            lhs_ptr++;
            col++;
        }

        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_ROW_MUL, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_add_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_pairwise_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_ptr = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr + *rhs_ptr;
        out_ptr++;
        lhs_ptr++;
        rhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_ADD, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_sub_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_pairwise_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_ptr = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr - *rhs_ptr;
        out_ptr++;
        lhs_ptr++;
        rhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_SUB, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_pairwise_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_ptr = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr * *rhs_ptr;
        out_ptr++;
        lhs_ptr++;
        rhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_MUL, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_min_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_pairwise_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_ptr = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = ldg_math_min_get(*lhs_ptr, *rhs_ptr);
        out_ptr++;
        lhs_ptr++;
        rhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_MIN, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_gather_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_row = 0x0;
    const double *rhs_val = 0x0;
    double *out_ptr = 0x0;
    uint32_t row = 0;
    uint32_t pick = 0;
    uint32_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != graph->node[lhs].row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[rhs].col_cunt != AUTOGRAD_SCALAR_COL)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = graph->node[lhs].col_cunt;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, AUTOGRAD_SCALAR_COL, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_row = autograd_val_ptr_get(graph, lhs);
    rhs_val = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[*out].row_cunt)
    {
        pick = (uint32_t)rhs_val[row];
        if (LDG_UNLIKELY(pick >= width)) { return LDG_ERR_BOUNDS; }

        out_ptr[row] = lhs_row[pick];
        lhs_row = lhs_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_GATHER, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_scale_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double scalar, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr * scalar;
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_SCALE, *out, lhs, AUTOGRAD_RHS_NONE, scalar);
}

uint32_t ldg_math_stat_autograd_shift_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double scalar, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr + scalar;
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_SHIFT, *out, lhs, AUTOGRAD_RHS_NONE, scalar);
}

uint32_t ldg_math_stat_autograd_clip_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double eps, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    double floor_val = 0.0;
    double ceil_val = 0.0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);
    floor_val = AUTOGRAD_RATIO_CENTER - eps;
    ceil_val = AUTOGRAD_RATIO_CENTER + eps;

    while (out_ptr < out_end)
    {
        *out_ptr = ldg_math_max_get(floor_val, ldg_math_min_get(ceil_val, *lhs_ptr));
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_CLIP, *out, lhs, AUTOGRAD_RHS_NONE, eps);
}

uint32_t ldg_math_stat_autograd_tanh_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = autograd_tanh_get(*lhs_ptr);
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_TANH, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_sigmoid_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = (autograd_tanh_get(*lhs_ptr * AUTOGRAD_SIGMOID_HALF) + AUTOGRAD_TANH_MAX) * AUTOGRAD_SIGMOID_HALF;
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_SIGMOID, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_relu_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = ldg_math_max_get(AUTOGRAD_RELU_FLOOR, *lhs_ptr);
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_RELU, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

static double autograd_gelu_inner_get(double val)
{
    return (double)AUTOGRAD_GELU_SQRT_2_OVER_PI * (val + ((double)AUTOGRAD_GELU_CUBIC * val * val * val));
}

uint32_t ldg_math_stat_autograd_gelu_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = (double)AUTOGRAD_GELU_HALF * *lhs_ptr * (AUTOGRAD_TANH_MAX + autograd_tanh_get(autograd_gelu_inner_get(*lhs_ptr)));
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_GELU, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_reshape_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t row_cunt, uint32_t col_cunt, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY((uintptr_t)row_cunt * (uintptr_t)col_cunt != autograd_node_elem_get(graph, lhs))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, row_cunt, col_cunt, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_cpy(autograd_val_ptr_get(graph, *out), autograd_val_ptr_get(graph, lhs), (uint64_t)autograd_node_elem_get(graph, lhs) * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return autograd_tape_push(graph, AUTOGRAD_OP_RESHAPE, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_exp_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = ldg_math_exp_get(*lhs_ptr);
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_EXP, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_square_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr * *lhs_ptr;
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_SQUARE, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_log_softmax_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, const uint8_t *mask, uint32_t *out)
{
    const double *lhs_row = 0x0;
    const uint8_t *mask_row = mask;
    double *out_row = 0x0;
    double peak = 0.0;
    double cum = 0.0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_mask_ck(mask, graph->node[lhs].row_cunt, graph->node[lhs].col_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_row = autograd_val_ptr_get(graph, *out);
    width = graph->node[lhs].col_cunt;

    while (row < graph->node[lhs].row_cunt)
    {
        peak = AUTOGRAD_MASK_NEG;
        col = 0;
        while (col < width)
        {
            if (autograd_mask_on_is(mask_row, col) == LDG_TRUTH_TRUE) { peak = ldg_math_max_get(peak, lhs_row[col]); }

            col++;
        }

        cum = 0.0;
        col = 0;
        while (col < width)
        {
            if (autograd_mask_on_is(mask_row, col) == LDG_TRUTH_TRUE) { cum = cum + ldg_math_exp_get(lhs_row[col] - peak); }

            col++;
        }

        cum = peak + ldg_math_log_get(cum);
        col = 0;
        while (col < width)
        {
            out_row[col] = AUTOGRAD_MASK_NEG;
            if (autograd_mask_on_is(mask_row, col) == LDG_TRUTH_TRUE) { out_row[col] = lhs_row[col] - cum; }

            col++;
        }

        lhs_row = lhs_row + width;
        out_row = out_row + width;
        if (mask_row) { mask_row = mask_row + width; }

        row++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_LOG_SOFTMAX, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_ref_set(graph, mask);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_row_sum_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_row = 0x0;
    double *out_ptr = 0x0;
    double cum = 0.0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[lhs].col_cunt;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, AUTOGRAD_SCALAR_COL, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[*out].row_cunt)
    {
        cum = 0.0;
        col = 0;
        while (col < width)
        {
            cum = cum + lhs_row[col];
            col++;
        }

        out_ptr[row] = cum;
        lhs_row = lhs_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_ROW_SUM, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_mean_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *lhs_end = 0x0;
    double *out_ptr = 0x0;
    double cum = 0.0;
    uintptr_t elem = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    elem = autograd_node_elem_get(graph, lhs);

    ret = autograd_node_push(graph, AUTOGRAD_SCALAR_ROW, AUTOGRAD_SCALAR_COL, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    lhs_end = lhs_ptr + elem;
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (lhs_ptr < lhs_end)
    {
        cum = cum + *lhs_ptr;
        lhs_ptr++;
    }

    out_ptr[0] = cum / (double)elem;

    return autograd_tape_push(graph, AUTOGRAD_OP_MEAN, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
}

static void autograd_tape_aux_set(ldg_math_stat_autograd_t *graph, uint32_t aux)
{
    graph->tape[graph->tape_cunt - AUTOGRAD_SCALAR_ELEM].aux = aux;
}

uint32_t ldg_math_stat_autograd_tile_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rep_cunt, uint32_t *out)
{
    const double *lhs_val = 0x0;
    double *out_row = 0x0;
    uintptr_t blk = 0;
    uint32_t rep = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(rep_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt * rep_cunt, graph->node[lhs].col_cunt, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_val = autograd_val_ptr_get(graph, lhs);
    out_row = autograd_val_ptr_get(graph, *out);
    blk = autograd_node_elem_get(graph, lhs);

    while (rep < rep_cunt)
    {
        ret = ldg_mem_cpy(out_row, lhs_val, (uint64_t)blk * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out_row = out_row + blk;
        rep++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_TILE, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, rep_cunt);

    return LDG_ERR_AOK;
}

static uint32_t autograd_group_ck(const ldg_math_stat_autograd_t *graph, uint32_t node, uint32_t group_len)
{
    if (LDG_UNLIKELY(group_len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[node].row_cunt % group_len != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static void autograd_row_stat_get(const double *row, uint32_t width, double eps, double *out_mean, double *out_inv)
{
    double cum = 0.0;
    double var = 0.0;
    double diff = 0.0;
    uint32_t col = 0;

    while (col < width)
    {
        cum = cum + row[col];
        col++;
    }

    *out_mean = cum / (double)width;

    col = 0;
    while (col < width)
    {
        diff = row[col] - *out_mean;
        var = var + (diff * diff);
        col++;
    }

    *out_inv = AUTOGRAD_MASK_ON / ldg_math_sqrt_get((var / (double)width) + eps);
}

uint32_t ldg_math_stat_autograd_row_norm_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double eps, uint32_t *out)
{
    const double *lhs_row = 0x0;
    double *out_row = 0x0;
    double mean = 0.0;
    double inv = 0.0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[lhs].col_cunt;
    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_row = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        autograd_row_stat_get(lhs_row, width, eps, &mean, &inv);

        col = 0;
        while (col < width)
        {
            out_row[col] = (lhs_row[col] - mean) * inv;
            col++;
        }

        lhs_row = lhs_row + width;
        out_row = out_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_ROW_NORM, *out, lhs, AUTOGRAD_RHS_NONE, eps);
}

static double autograd_row_rms_inv_get(const double *row, uint32_t width, double eps)
{
    double cum = 0.0;
    uint32_t col = 0;

    while (col < width)
    {
        cum = cum + (row[col] * row[col]);
        col++;
    }

    return AUTOGRAD_MASK_ON / ldg_math_sqrt_get((cum / (double)width) + eps);
}

uint32_t ldg_math_stat_autograd_row_rms_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double eps, uint32_t *out)
{
    const double *lhs_row = 0x0;
    double *out_row = 0x0;
    double inv = 0.0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[lhs].col_cunt;
    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_row = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        inv = autograd_row_rms_inv_get(lhs_row, width, eps);

        col = 0;
        while (col < width)
        {
            out_row[col] = lhs_row[col] * inv;
            col++;
        }

        lhs_row = lhs_row + width;
        out_row = out_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_ROW_RMS, *out, lhs, AUTOGRAD_RHS_NONE, eps);
}

uint32_t ldg_math_stat_autograd_row_repeat_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rep_cunt, uint32_t *out)
{
    const double *lhs_row = 0x0;
    double *out_row = 0x0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t rep = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(rep_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[lhs].col_cunt;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt * rep_cunt, width, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_row = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        rep = 0;
        while (rep < rep_cunt)
        {
            ret = ldg_mem_cpy(out_row, lhs_row, (uint64_t)width * (uint64_t)sizeof(double));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            out_row = out_row + width;
            rep++;
        }

        lhs_row = lhs_row + width;
        row++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_ROW_REPEAT, *out, lhs, AUTOGRAD_RHS_NONE, AUTOGRAD_NO_SCALAR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, rep_cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_row_pool_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t *out)
{
    const double *lhs_row = 0x0;
    const double *mask = 0x0;
    double *out_row = 0x0;
    double valid = 0.0;
    uint32_t width = 0;
    uint32_t group_cunt = 0;
    uint32_t group = 0;
    uint32_t member = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_group_ck(graph, lhs, group_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != graph->node[lhs].row_cunt || graph->node[rhs].col_cunt != AUTOGRAD_SCALAR_COL)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = graph->node[lhs].col_cunt;
    group_cunt = graph->node[lhs].row_cunt / group_len;

    ret = autograd_node_push(graph, group_cunt, width, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_row = autograd_val_ptr_get(graph, lhs);
    mask = autograd_val_ptr_get(graph, rhs);
    out_row = autograd_val_ptr_get(graph, *out);

    while (group < group_cunt)
    {
        valid = 0.0;
        col = 0;
        while (col < width)
        {
            out_row[col] = AUTOGRAD_POOL_EMPTY;
            col++;
        }

        member = 0;
        while (member < group_len)
        {
            if (*mask == AUTOGRAD_MASK_ON)
            {
                valid = valid + AUTOGRAD_MASK_ON;
                col = 0;
                while (col < width)
                {
                    out_row[col] = out_row[col] + lhs_row[col];
                    col++;
                }
            }

            lhs_row = lhs_row + width;
            mask++;
            member++;
        }

        if (valid > AUTOGRAD_POOL_EMPTY)
        {
            col = 0;
            while (col < width)
            {
                out_row[col] = out_row[col] / valid;
                col++;
            }
        }

        out_row = out_row + width;
        group++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_ROW_POOL, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, group_len);

    return LDG_ERR_AOK;
}

static uint32_t autograd_attn_prep(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(hd_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return autograd_group_ck(graph, rhs, group_len);
}

uint32_t ldg_math_stat_autograd_attn_score_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt, uint32_t *out)
{
    const double *q_val = 0x0;
    const double *k_val = 0x0;
    const double *q_row = 0x0;
    const double *k_row = 0x0;
    double *out_ptr = 0x0;
    double scale = 0.0;
    double cum = 0.0;
    uint32_t width = 0;
    uint32_t hd_dim = 0;
    uint32_t group_cunt = 0;
    uint32_t group = 0;
    uint32_t hd = 0;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t dim = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = autograd_attn_prep(graph, lhs, rhs, group_len, hd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[lhs].row_cunt != graph->node[rhs].row_cunt || graph->node[lhs].col_cunt != graph->node[rhs].col_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = graph->node[lhs].col_cunt;

    if (LDG_UNLIKELY(width % hd_cunt != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hd_dim = width / hd_cunt;
    group_cunt = graph->node[lhs].row_cunt / group_len;
    scale = AUTOGRAD_MASK_ON / ldg_math_sqrt_get((double)hd_dim);

    ret = autograd_node_push(graph, group_cunt * hd_cunt * group_len, group_len, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    q_val = autograd_val_ptr_get(graph, lhs);
    k_val = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (group < group_cunt)
    {
        hd = 0;
        while (hd < hd_cunt)
        {
            query = 0;
            while (query < group_len)
            {
                q_row = q_val + ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)query) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                key = 0;
                while (key < group_len)
                {
                    k_row = k_val + ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)key) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                    cum = 0.0;
                    dim = 0;
                    while (dim < hd_dim)
                    {
                        cum = cum + (q_row[dim] * k_row[dim]);
                        dim++;
                    }

                    *out_ptr = cum * scale;
                    out_ptr++;
                    key++;
                }

                query++;
            }

            hd++;
        }

        group++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_ATTN_SCORE, *out, lhs, rhs, (double)hd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, group_len);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_attn_mask_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt, uint32_t *out)
{
    const double *score = 0x0;
    const double *mask = 0x0;
    double *out_ptr = 0x0;
    uint32_t group_cunt = 0;
    uint32_t group = 0;
    uint32_t hd = 0;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = autograd_attn_prep(graph, lhs, rhs, group_len, hd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    group_cunt = graph->node[rhs].row_cunt / group_len;

    if (LDG_UNLIKELY(graph->node[rhs].col_cunt != AUTOGRAD_SCALAR_COL)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[lhs].row_cunt != group_cunt * hd_cunt * group_len || graph->node[lhs].col_cunt != group_len)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, group_len, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    score = autograd_val_ptr_get(graph, lhs);
    mask = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (group < group_cunt)
    {
        hd = 0;
        while (hd < hd_cunt)
        {
            query = 0;
            while (query < group_len)
            {
                key = 0;
                while (key < group_len)
                {
                    *out_ptr = *score;
                    if (mask[((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)key] != AUTOGRAD_MASK_ON) { *out_ptr = *score + AUTOGRAD_MASK_NEG; }

                    out_ptr++;
                    score++;
                    key++;
                }

                query++;
            }

            hd++;
        }

        group++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_ATTN_MASK, *out, lhs, rhs, (double)hd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, group_len);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_attn_apply_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt, uint32_t *out)
{
    const double *prob = 0x0;
    const double *v_val = 0x0;
    const double *v_row = 0x0;
    double *out_row = 0x0;
    double weight = 0.0;
    uint32_t width = 0;
    uint32_t hd_dim = 0;
    uint32_t group_cunt = 0;
    uint32_t group = 0;
    uint32_t hd = 0;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t dim = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = autograd_attn_prep(graph, lhs, rhs, group_len, hd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[rhs].col_cunt;

    if (LDG_UNLIKELY(width % hd_cunt != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hd_dim = width / hd_cunt;
    group_cunt = graph->node[rhs].row_cunt / group_len;

    if (LDG_UNLIKELY(graph->node[lhs].row_cunt != group_cunt * hd_cunt * group_len || graph->node[lhs].col_cunt != group_len)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, graph->node[rhs].row_cunt, width, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    prob = autograd_val_ptr_get(graph, lhs);
    v_val = autograd_val_ptr_get(graph, rhs);

    ret = ldg_mem_zero(autograd_val_ptr_get(graph, *out), (uint64_t)autograd_node_elem_get(graph, *out) * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (group < group_cunt)
    {
        hd = 0;
        while (hd < hd_cunt)
        {
            query = 0;
            while (query < group_len)
            {
                out_row = autograd_val_ptr_get(graph, *out) + ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)query) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                key = 0;
                while (key < group_len)
                {
                    weight = *prob;
                    prob++;

                    if (weight != AUTOGRAD_NO_SCALAR)
                    {
                        v_row = v_val + ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)key) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                        dim = 0;
                        while (dim < hd_dim)
                        {
                            out_row[dim] = out_row[dim] + (weight * v_row[dim]);
                            dim++;
                        }
                    }

                    key++;
                }

                query++;
            }

            hd++;
        }

        group++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_ATTN_APPLY, *out, lhs, rhs, (double)hd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, group_len);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_col_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *col = 0x0;
    double *out_ptr = 0x0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != graph->node[lhs].row_cunt || graph->node[rhs].col_cunt != AUTOGRAD_SCALAR_COL)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = graph->node[lhs].col_cunt;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, width, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    col = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        idx = 0;
        while (idx < width)
        {
            *out_ptr = *lhs_ptr * col[row];
            out_ptr++;
            lhs_ptr++;
            idx++;
        }

        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_COL_MUL, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_col_concat_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *rhs_ptr = 0x0;
    double *out_ptr = 0x0;
    uint32_t lhs_width = 0;
    uint32_t rhs_width = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != graph->node[lhs].row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    lhs_width = graph->node[lhs].col_cunt;
    rhs_width = graph->node[rhs].col_cunt;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, lhs_width + rhs_width, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    rhs_ptr = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        ret = ldg_mem_cpy(out_ptr, lhs_ptr, (uint64_t)lhs_width * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_mem_cpy(out_ptr + lhs_width, rhs_ptr, (uint64_t)rhs_width * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out_ptr = out_ptr + lhs_width + rhs_width;
        lhs_ptr = lhs_ptr + lhs_width;
        rhs_ptr = rhs_ptr + rhs_width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_COL_CONCAT, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_scalar_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    double *out_ptr = 0x0;
    const double *out_end = 0x0;
    double factor = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(autograd_node_elem_get(graph, rhs) != AUTOGRAD_SCALAR_ELEM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, graph->node[lhs].col_cunt, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    factor = autograd_val_ptr_get(graph, rhs)[0];
    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    out_end = out_ptr + autograd_node_elem_get(graph, *out);

    while (out_ptr < out_end)
    {
        *out_ptr = *lhs_ptr * factor;
        out_ptr++;
        lhs_ptr++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_SCALAR_MUL, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

uint32_t ldg_math_stat_autograd_synapse_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t idx, uint32_t out_dim, uint32_t *out)
{
    const double *lhs_val = 0x0;
    const double *weight = 0x0;
    const double *table = 0x0;
    double *out_ptr = 0x0;
    uintptr_t src_width = 0;
    uint32_t rows = 0;
    uint32_t synapse_cunt = 0;
    uint32_t src = 0;
    uint32_t dst = 0;
    uint32_t syn = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(out_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[rhs].col_cunt != AUTOGRAD_SCALAR_COL || graph->node[idx].col_cunt != LDG_MATH_STAT_SYNAPSE_IDX_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[idx].row_cunt != graph->node[rhs].row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    rows = graph->node[lhs].row_cunt;
    src_width = graph->node[lhs].col_cunt;
    synapse_cunt = graph->node[rhs].row_cunt;
    table = autograd_val_ptr_get(graph, idx);

    while (syn < synapse_cunt)
    {
        src = (uint32_t)table[(syn * LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + AUTOGRAD_SYNAPSE_SRC];
        dst = (uint32_t)table[(syn * LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + AUTOGRAD_SYNAPSE_DST];
        if (LDG_UNLIKELY(src >= src_width || dst >= out_dim)) { return LDG_ERR_BOUNDS; }

        syn++;
    }

    ret = autograd_node_push(graph, rows, out_dim, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_val = autograd_val_ptr_get(graph, lhs);
    weight = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    ret = ldg_mem_zero(out_ptr, (uint64_t)rows * (uint64_t)out_dim * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    syn = 0;
    while (syn < synapse_cunt)
    {
        src = (uint32_t)table[(syn * LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + AUTOGRAD_SYNAPSE_SRC];
        dst = (uint32_t)table[(syn * LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + AUTOGRAD_SYNAPSE_DST];

        row = 0;
        while (row < rows)
        {
            out_ptr[((uintptr_t)row * (uintptr_t)out_dim) + dst] = out_ptr[((uintptr_t)row * (uintptr_t)out_dim) + dst] + (weight[syn] * lhs_val[((uintptr_t)row * src_width) + src]);
            row++;
        }

        syn++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_SYNAPSE, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, idx);

    return LDG_ERR_AOK;
}

static double autograd_rate_get(uint32_t act, double in, double *out_deriv)
{
    double val = 0.0;

    if (act == LDG_MATH_STAT_LAYER_ACT_TANH)
    {
        val = autograd_tanh_get(in);
        *out_deriv = AUTOGRAD_TANH_MAX - (val * val);

        return val;
    }

    if (act == LDG_MATH_STAT_LAYER_ACT_SIGMOID)
    {
        val = (autograd_tanh_get(in * AUTOGRAD_SIGMOID_HALF) + AUTOGRAD_TANH_MAX) * AUTOGRAD_SIGMOID_HALF;
        *out_deriv = val * (AUTOGRAD_TANH_MAX - val);

        return val;
    }

    if (act == LDG_MATH_STAT_LAYER_ACT_RELU)
    {
        *out_deriv = AUTOGRAD_RELU_FLOOR;
        if (in > AUTOGRAD_RELU_FLOOR) { *out_deriv = AUTOGRAD_UNIT_GRAD; }

        if (in > AUTOGRAD_RELU_FLOOR) { return in; }

        return AUTOGRAD_RELU_FLOOR;
    }

    *out_deriv = AUTOGRAD_UNIT_GRAD;

    return in;
}

static double autograd_spike_get(const double *param, double in, uint32_t tick_cunt, double *membrane, double *refract, double *out_deriv)
{
    double leak = param[AUTOGRAD_NEURON_LEAK];
    double threshold = param[AUTOGRAD_NEURON_THRESHOLD];
    double rst = param[AUTOGRAD_NEURON_RST];
    double refractory = param[AUTOGRAD_NEURON_REFRACTORY];
    double volt = *membrane;
    double wait = *refract;
    double spike_cum = 0.0;
    double deriv_cum = 0.0;
    double gap = 0.0;
    double mag = 0.0;
    double slope_gap = 0.0;
    uint32_t tick = 0;

    while (tick < tick_cunt)
    {
        if (wait > AUTOGRAD_REFRACT_NONE)
        {
            wait = wait - AUTOGRAD_REFRACT_UNIT;
            volt = rst;
        }
        else
        {
            volt = (volt * leak) + in;
            gap = volt - threshold;
            mag = gap;
            if (mag < AUTOGRAD_FIRE_GAP) { mag = -mag; }

            slope_gap = AUTOGRAD_UNIT_GRAD + (mag * AUTOGRAD_SURROGATE_SLOPE);
            deriv_cum = deriv_cum + (AUTOGRAD_UNIT_GRAD / (slope_gap * slope_gap));

            if (gap >= AUTOGRAD_FIRE_GAP)
            {
                spike_cum = spike_cum + AUTOGRAD_SPIKE_UNIT;
                volt = rst;
                wait = refractory;
            }
        }

        tick++;
    }

    *membrane = volt;
    *refract = wait;
    *out_deriv = deriv_cum / (double)tick_cunt;

    return spike_cum / (double)tick_cunt;
}

uint32_t ldg_math_stat_autograd_neuron_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t state, uint32_t tick_cunt, uint32_t *out)
{
    const double *lhs_val = 0x0;
    const double *param = 0x0;
    double *state_row = 0x0;
    double *out_ptr = 0x0;
    uintptr_t dim = 0;
    uint32_t rows = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(tick_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    rows = graph->node[lhs].row_cunt;
    dim = graph->node[lhs].col_cunt;

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != dim || graph->node[rhs].col_cunt != LDG_MATH_STAT_NEURON_PARAM_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[state].row_cunt != rows || graph->node[state].col_cunt != LDG_MATH_STAT_NEURON_STATE_CUNT * dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, rows, (uint32_t)dim, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_val = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);
    state_row = autograd_val_ptr_get(graph, state);

    while (row < rows)
    {
        idx = 0;
        while (idx < dim)
        {
            param = autograd_val_ptr_get(graph, rhs) + ((uintptr_t)idx * (uintptr_t)LDG_MATH_STAT_NEURON_PARAM_CUNT);

            if ((uint32_t)param[AUTOGRAD_NEURON_KIND] == LDG_MATH_STAT_NEURON_SPIKE) { *out_ptr = autograd_spike_get(param, *lhs_val, tick_cunt, &state_row[((uintptr_t)AUTOGRAD_NEURON_MEMBRANE * dim) + idx], &state_row[((uintptr_t)AUTOGRAD_NEURON_REFRACT * dim) + idx], &state_row[((uintptr_t)AUTOGRAD_NEURON_DERIV * dim) + idx]); }
            else { *out_ptr = autograd_rate_get((uint32_t)param[AUTOGRAD_NEURON_ACT], *lhs_val, &state_row[((uintptr_t)AUTOGRAD_NEURON_DERIV * dim) + idx]); }

            out_ptr++;
            lhs_val++;
            idx++;
        }

        state_row = state_row + ((uintptr_t)LDG_MATH_STAT_NEURON_STATE_CUNT * dim);
        row++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_NEURON, *out, lhs, rhs, (double)tick_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, state);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_kwin_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t keep_cunt, uint32_t *out)
{
    const double *lhs_row = 0x0;
    double *out_row = 0x0;
    double best_mag = 0.0;
    double mag = 0.0;
    uint32_t width = 0;
    uint32_t best = 0;
    uint32_t pass = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = autograd_unary_prep(graph, lhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(keep_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = graph->node[lhs].col_cunt;
    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_row = autograd_val_ptr_get(graph, *out);

    if (keep_cunt >= width)
    {
        ret = ldg_mem_cpy(out_row, lhs_row, (uint64_t)autograd_node_elem_get(graph, lhs) * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return autograd_tape_push(graph, AUTOGRAD_OP_KWIN, *out, lhs, AUTOGRAD_RHS_NONE, (double)keep_cunt);
    }

    while (row < graph->node[lhs].row_cunt)
    {
        idx = 0;
        while (idx < width)
        {
            out_row[idx] = AUTOGRAD_KWIN_SILENT;
            idx++;
        }

        pass = 0;
        while (pass < keep_cunt)
        {
            best = UINT32_MAX;
            best_mag = AUTOGRAD_KWIN_SILENT;

            idx = 0;
            while (idx < width)
            {
                if (out_row[idx] == AUTOGRAD_KWIN_SILENT)
                {
                    mag = lhs_row[idx];
                    if (mag < AUTOGRAD_KWIN_SILENT) { mag = -mag; }

                    if (mag > best_mag)
                    {
                        best = idx;
                        best_mag = mag;
                    }
                }

                idx++;
            }

            if (best == UINT32_MAX) { pass = keep_cunt; }
            else
            {
                out_row[best] = lhs_row[best];
                pass++;
            }
        }

        lhs_row = lhs_row + width;
        out_row = out_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_KWIN, *out, lhs, AUTOGRAD_RHS_NONE, (double)keep_cunt);
}

static uint32_t autograd_conv_geom_get(const ldg_math_stat_autograd_t *graph, uint32_t geom, autograd_conv_geom_t *out)
{
    const double *val = autograd_val_ptr_get(graph, geom);
    uint32_t span = 0;

    out->in_h = (uint32_t)val[AUTOGRAD_CONV_IN_H];
    out->in_w = (uint32_t)val[AUTOGRAD_CONV_IN_W];
    out->in_c = (uint32_t)val[AUTOGRAD_CONV_IN_C];
    out->out_c = (uint32_t)val[AUTOGRAD_CONV_OUT_C];
    out->kernel_h = (uint32_t)val[AUTOGRAD_CONV_KERNEL_H];
    out->kernel_w = (uint32_t)val[AUTOGRAD_CONV_KERNEL_W];
    out->stride = (uint32_t)val[AUTOGRAD_CONV_STRIDE];
    out->pad = (uint32_t)val[AUTOGRAD_CONV_PAD];

    if (LDG_UNLIKELY(out->in_h == 0 || out->in_w == 0 || out->in_c == 0 || out->out_c == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(out->kernel_h == 0 || out->kernel_w == 0 || out->stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    span = out->in_h + (AUTOGRAD_CONV_PAD_SIDE_CUNT * out->pad);
    if (LDG_UNLIKELY(span < out->kernel_h)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->out_h = ((span - out->kernel_h) / out->stride) + AUTOGRAD_CONV_WIN_ORIG;

    span = out->in_w + (AUTOGRAD_CONV_PAD_SIDE_CUNT * out->pad);
    if (LDG_UNLIKELY(span < out->kernel_w)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->out_w = ((span - out->kernel_w) / out->stride) + AUTOGRAD_CONV_WIN_ORIG;

    return LDG_ERR_AOK;
}

static uint32_t autograd_conv_src_off_get(const autograd_conv_geom_t *geom, uint32_t out_pos, uint32_t kernel_pos, uint32_t limit, uint32_t *out)
{
    uint32_t shifted = (out_pos * geom->stride) + kernel_pos;

    if (shifted < geom->pad) { return LDG_ERR_BOUNDS; }

    *out = shifted - geom->pad;

    if (*out >= limit) { return LDG_ERR_BOUNDS; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_conv_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t geom, uint32_t *out)
{
    autograd_conv_geom_t desc = LDG_STRUCT_ZERO_INIT;
    const double *lhs_val = 0x0;
    const double *weight = 0x0;
    double *out_ptr = 0x0;
    uintptr_t src_width = 0;
    uintptr_t tap_width = 0;
    double cum = 0.0;
    uint32_t src_row = 0;
    uint32_t src_col = 0;
    uint32_t row = 0;
    uint32_t out_row = 0;
    uint32_t out_col = 0;
    uint32_t out_chan = 0;
    uint32_t kernel_row = 0;
    uint32_t kernel_col = 0;
    uint32_t chan = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, geom);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(autograd_node_elem_get(graph, geom) < LDG_MATH_STAT_CONV_GEOM_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_conv_geom_get(graph, geom, &desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    src_width = (uintptr_t)desc.in_h * (uintptr_t)desc.in_w * (uintptr_t)desc.in_c;
    tap_width = (uintptr_t)desc.kernel_h * (uintptr_t)desc.kernel_w * (uintptr_t)desc.in_c;

    if (LDG_UNLIKELY(graph->node[lhs].col_cunt != src_width)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != tap_width || graph->node[rhs].col_cunt != desc.out_c)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, desc.out_h * desc.out_w * desc.out_c, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_val = autograd_val_ptr_get(graph, lhs);
    weight = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        out_row = 0;
        while (out_row < desc.out_h)
        {
            out_col = 0;
            while (out_col < desc.out_w)
            {
                out_chan = 0;
                while (out_chan < desc.out_c)
                {
                    cum = 0.0;
                    kernel_row = 0;
                    while (kernel_row < desc.kernel_h)
                    {
                        kernel_col = 0;
                        while (kernel_col < desc.kernel_w)
                        {
                            if (autograd_conv_src_off_get(&desc, out_row, kernel_row, desc.in_h, &src_row) == LDG_ERR_AOK && autograd_conv_src_off_get(&desc, out_col, kernel_col, desc.in_w, &src_col) == LDG_ERR_AOK)
                            {
                                chan = 0;
                                while (chan < desc.in_c)
                                {
                                    cum = cum + (lhs_val[(((uintptr_t)src_row * (uintptr_t)desc.in_w) + (uintptr_t)src_col) * (uintptr_t)desc.in_c + (uintptr_t)chan] * weight[((((uintptr_t)kernel_row * (uintptr_t)desc.kernel_w) + (uintptr_t)kernel_col) * (uintptr_t)desc.in_c + (uintptr_t)chan) * (uintptr_t)desc.out_c + (uintptr_t)out_chan]);
                                    chan++;
                                }
                            }

                            kernel_col++;
                        }

                        kernel_row++;
                    }

                    *out_ptr = cum;
                    out_ptr++;
                    out_chan++;
                }

                out_col++;
            }

            out_row++;
        }

        lhs_val = lhs_val + src_width;
        row++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_CONV, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, geom);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_group_bias_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *bias = 0x0;
    double *out_ptr = 0x0;
    uint32_t width = 0;
    uint32_t group_cunt = 0;
    uint32_t row = 0;
    uint32_t group = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[rhs].row_cunt != AUTOGRAD_SCALAR_ROW)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = graph->node[rhs].col_cunt;

    if (LDG_UNLIKELY(width == 0 || graph->node[lhs].col_cunt % width != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    group_cunt = graph->node[lhs].col_cunt / width;

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, graph->node[lhs].col_cunt, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    bias = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        group = 0;
        while (group < group_cunt)
        {
            idx = 0;
            while (idx < width)
            {
                *out_ptr = *lhs_ptr + bias[idx];
                out_ptr++;
                lhs_ptr++;
                idx++;
            }

            group++;
        }

        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_GROUP_BIAS, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

static uint32_t autograd_proto_prep(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(graph->node[lhs].col_cunt != graph->node[rhs].col_cunt || graph->node[lhs].col_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return autograd_node_push(graph, graph->node[lhs].row_cunt, graph->node[rhs].row_cunt, LDG_TRUTH_TRUE, out);
}

static double autograd_row_len_get(const double *row, uint32_t width, double eps)
{
    const double *end = row + width;
    double cum = eps;

    while (row < end)
    {
        cum = cum + (*row * *row);
        row++;
    }

    return ldg_math_sqrt_get(cum);
}

uint32_t ldg_math_stat_autograd_cosine_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, double eps, uint32_t *out)
{
    const double *lhs_row = 0x0;
    const double *rhs_row = 0x0;
    double *out_ptr = 0x0;
    double lhs_len = 0.0;
    double cum = 0.0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t proto = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    ret = autograd_proto_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[lhs].col_cunt;
    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        lhs_len = autograd_row_len_get(lhs_row, width, eps);
        rhs_row = autograd_val_ptr_get(graph, rhs);

        proto = 0;
        while (proto < graph->node[rhs].row_cunt)
        {
            cum = AUTOGRAD_PAIR_CUM_SEED;
            col = 0;
            while (col < width)
            {
                cum = cum + (lhs_row[col] * rhs_row[col]);
                col++;
            }

            *out_ptr = cum / (lhs_len * autograd_row_len_get(rhs_row, width, eps));
            out_ptr++;
            rhs_row = rhs_row + width;
            proto++;
        }

        lhs_row = lhs_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_COSINE, *out, lhs, rhs, eps);
}

uint32_t ldg_math_stat_autograd_euclid_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_row = 0x0;
    const double *rhs_row = 0x0;
    double *out_ptr = 0x0;
    double diff = 0.0;
    double cum = 0.0;
    uint32_t width = 0;
    uint32_t row = 0;
    uint32_t proto = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    ret = autograd_proto_prep(graph, lhs, rhs, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    width = graph->node[lhs].col_cunt;
    lhs_row = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        rhs_row = autograd_val_ptr_get(graph, rhs);

        proto = 0;
        while (proto < graph->node[rhs].row_cunt)
        {
            cum = AUTOGRAD_PAIR_CUM_SEED;
            col = 0;
            while (col < width)
            {
                diff = lhs_row[col] - rhs_row[col];
                cum = cum + (diff * diff);
                col++;
            }

            *out_ptr = -cum;
            out_ptr++;
            rhs_row = rhs_row + width;
            proto++;
        }

        lhs_row = lhs_row + width;
        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_EUCLID, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

typedef struct autograd_pool_geom
{
    uint32_t in_h;
    uint32_t in_w;
    uint32_t in_c;
    uint32_t kernel_h;
    uint32_t kernel_w;
    uint32_t stride;
    uint32_t out_h;
    uint32_t out_w;
} autograd_pool_geom_t;

static uint32_t autograd_pool_geom_get(const ldg_math_stat_autograd_t *graph, uint32_t geom, autograd_pool_geom_t *out)
{
    const double *val = autograd_val_ptr_get(graph, geom);

    out->in_h = (uint32_t)val[AUTOGRAD_POOL_IN_H];
    out->in_w = (uint32_t)val[AUTOGRAD_POOL_IN_W];
    out->in_c = (uint32_t)val[AUTOGRAD_POOL_IN_C];
    out->kernel_h = (uint32_t)val[AUTOGRAD_POOL_KERNEL_H];
    out->kernel_w = (uint32_t)val[AUTOGRAD_POOL_KERNEL_W];
    out->stride = (uint32_t)val[AUTOGRAD_POOL_STRIDE];

    if (LDG_UNLIKELY(out->in_h == 0 || out->in_w == 0 || out->in_c == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(out->kernel_h == 0 || out->kernel_w == 0 || out->stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(out->in_h < out->kernel_h || out->in_w < out->kernel_w)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->out_h = ((out->in_h - out->kernel_h) / out->stride) + AUTOGRAD_CONV_WIN_ORIG;
    out->out_w = ((out->in_w - out->kernel_w) / out->stride) + AUTOGRAD_CONV_WIN_ORIG;

    return LDG_ERR_AOK;
}

static uintptr_t autograd_pool_src_idx_get(const autograd_pool_geom_t *geom, uint32_t out_row, uint32_t out_col, uint32_t kernel_row, uint32_t kernel_col, uint32_t chan)
{
    uintptr_t src_row = ((uintptr_t)out_row * (uintptr_t)geom->stride) + (uintptr_t)kernel_row;
    uintptr_t src_col = ((uintptr_t)out_col * (uintptr_t)geom->stride) + (uintptr_t)kernel_col;

    return (((src_row * (uintptr_t)geom->in_w) + src_col) * (uintptr_t)geom->in_c) + (uintptr_t)chan;
}

static uintptr_t autograd_pool_win_pick_get(const autograd_pool_geom_t *geom, const double *src, uint32_t out_row, uint32_t out_col, uint32_t chan, uint8_t max_is, double *out_val)
{
    uintptr_t best_idx = autograd_pool_src_idx_get(geom, out_row, out_col, AUTOGRAD_POOL_WIN_FIRST, AUTOGRAD_POOL_WIN_FIRST, chan);
    uintptr_t idx = 0;
    double cum = AUTOGRAD_PAIR_CUM_SEED;
    double best = src[best_idx];
    uint32_t kernel_row = 0;
    uint32_t kernel_col = 0;

    while (kernel_row < geom->kernel_h)
    {
        kernel_col = 0;
        while (kernel_col < geom->kernel_w)
        {
            idx = autograd_pool_src_idx_get(geom, out_row, out_col, kernel_row, kernel_col, chan);
            cum = cum + src[idx];
            if (src[idx] > best)
            {
                best = src[idx];
                best_idx = idx;
            }

            kernel_col++;
        }

        kernel_row++;
    }

    *out_val = cum / (double)(geom->kernel_h * geom->kernel_w);
    if (max_is == LDG_TRUTH_TRUE) { *out_val = best; }

    return best_idx;
}

uint32_t ldg_math_stat_autograd_pool_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t geom, uint8_t max_is, uint32_t *out)
{
    autograd_pool_geom_t desc = LDG_STRUCT_ZERO_INIT;
    const double *lhs_val = 0x0;
    double *out_ptr = 0x0;
    double flag = AUTOGRAD_POOL_MAX_OFF;
    uintptr_t src_width = 0;
    uintptr_t pick = 0;
    uint32_t row = 0;
    uint32_t out_row = 0;
    uint32_t out_col = 0;
    uint32_t chan = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(max_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, geom);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(autograd_node_elem_get(graph, geom) != LDG_MATH_STAT_POOL_GEOM_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_pool_geom_get(graph, geom, &desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    src_width = (uintptr_t)desc.in_h * (uintptr_t)desc.in_w * (uintptr_t)desc.in_c;

    if (LDG_UNLIKELY(graph->node[lhs].col_cunt != src_width)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, desc.out_h * desc.out_w * desc.in_c, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (max_is == LDG_TRUTH_TRUE) { flag = AUTOGRAD_POOL_MAX_ON; }

    lhs_val = autograd_val_ptr_get(graph, lhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        out_row = 0;
        while (out_row < desc.out_h)
        {
            out_col = 0;
            while (out_col < desc.out_w)
            {
                chan = 0;
                while (chan < desc.in_c)
                {
                    pick = autograd_pool_win_pick_get(&desc, lhs_val, out_row, out_col, chan, max_is, out_ptr);
                    if (pick >= src_width) { return LDG_ERR_BOUNDS; }

                    out_ptr++;
                    chan++;
                }

                out_col++;
            }

            out_row++;
        }

        lhs_val = lhs_val + src_width;
        row++;
    }

    ret = autograd_tape_push(graph, AUTOGRAD_OP_POOL, *out, lhs, AUTOGRAD_RHS_NONE, flag);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    autograd_tape_aux_set(graph, geom);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_autograd_embedding_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    const double *lhs_ptr = 0x0;
    const double *table = 0x0;
    double *out_ptr = 0x0;
    uintptr_t width = 0;
    uintptr_t pick = 0;
    uint32_t vocab = 0;
    uint32_t lane_cunt = 0;
    uint32_t row = 0;
    uint32_t lane = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = autograd_node_valid_ck(graph, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    vocab = graph->node[rhs].row_cunt;
    width = graph->node[rhs].col_cunt;
    lane_cunt = graph->node[lhs].col_cunt;

    if (LDG_UNLIKELY(vocab == 0 || width == 0 || lane_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = autograd_node_push(graph, graph->node[lhs].row_cunt, (uint32_t)(lane_cunt * width), LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_ptr = autograd_val_ptr_get(graph, lhs);
    table = autograd_val_ptr_get(graph, rhs);
    out_ptr = autograd_val_ptr_get(graph, *out);

    while (row < graph->node[lhs].row_cunt)
    {
        lane = 0;
        while (lane < lane_cunt)
        {
            if (LDG_UNLIKELY(*lhs_ptr < AUTOGRAD_PAIR_CUM_SEED || (uintptr_t)*lhs_ptr >= vocab)) { return LDG_ERR_BOUNDS; }

            pick = (uintptr_t)*lhs_ptr * width;

            col = 0;
            while (col < width)
            {
                out_ptr[col] = table[pick + col];
                col++;
            }

            out_ptr = out_ptr + width;
            lhs_ptr++;
            lane++;
        }

        row++;
    }

    return autograd_tape_push(graph, AUTOGRAD_OP_EMBEDDING, *out, lhs, rhs, AUTOGRAD_NO_SCALAR);
}

static void autograd_col_mul_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *col = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *col_grad = autograd_grad_ptr_get(graph, entry->rhs);
    double cum = 0.0;
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t idx = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        cum = 0.0;
        idx = 0;
        while (idx < width)
        {
            *lhs_grad = *lhs_grad + (*out_grad * col[row]);
            cum = cum + (*out_grad * *lhs_val);
            out_grad++;
            lhs_grad++;
            lhs_val++;
            idx++;
        }

        col_grad[row] = col_grad[row] + cum;
        row++;
    }
}

static void autograd_col_concat_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uint32_t lhs_width = graph->node[entry->lhs].col_cunt;
    uint32_t rhs_width = graph->node[entry->rhs].col_cunt;
    uint32_t row = 0;
    uint32_t idx = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        idx = 0;
        while (idx < lhs_width)
        {
            *lhs_grad = *lhs_grad + *out_grad;
            lhs_grad++;
            out_grad++;
            idx++;
        }

        idx = 0;
        while (idx < rhs_width)
        {
            *rhs_grad = *rhs_grad + *out_grad;
            rhs_grad++;
            out_grad++;
            idx++;
        }

        row++;
    }
}

static void autograd_scalar_mul_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *factor_grad = autograd_grad_ptr_get(graph, entry->rhs);
    const double *grad_end = out_grad + autograd_node_elem_get(graph, entry->out);
    double factor = autograd_val_ptr_get(graph, entry->rhs)[0];
    double cum = 0.0;

    while (out_grad < grad_end)
    {
        *lhs_grad = *lhs_grad + (*out_grad * factor);
        cum = cum + (*out_grad * *lhs_val);
        out_grad++;
        lhs_grad++;
        lhs_val++;
    }

    factor_grad[0] = factor_grad[0] + cum;
}

static void autograd_synapse_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *weight = autograd_val_ptr_get(graph, entry->rhs);
    const double *table = autograd_val_ptr_get(graph, entry->aux);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *weight_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uintptr_t src_width = graph->node[entry->lhs].col_cunt;
    uintptr_t out_dim = graph->node[entry->out].col_cunt;
    uint32_t rows = graph->node[entry->out].row_cunt;
    uint32_t synapse_cunt = graph->node[entry->rhs].row_cunt;
    double gap = 0.0;
    uint32_t src = 0;
    uint32_t dst = 0;
    uint32_t syn = 0;
    uint32_t row = 0;

    while (syn < synapse_cunt)
    {
        src = (uint32_t)table[(syn * LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + AUTOGRAD_SYNAPSE_SRC];
        dst = (uint32_t)table[(syn * LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + AUTOGRAD_SYNAPSE_DST];

        row = 0;
        while (row < rows)
        {
            gap = out_grad[((uintptr_t)row * out_dim) + dst];
            weight_grad[syn] = weight_grad[syn] + (gap * lhs_val[((uintptr_t)row * src_width) + src]);
            lhs_grad[((uintptr_t)row * src_width) + src] = lhs_grad[((uintptr_t)row * src_width) + src] + (gap * weight[syn]);
            row++;
        }

        syn++;
    }
}

static void autograd_neuron_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    const double *state_row = autograd_val_ptr_get(graph, entry->aux);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    uintptr_t dim = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t idx = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        idx = 0;
        while (idx < dim)
        {
            *lhs_grad = *lhs_grad + (*out_grad * state_row[((uintptr_t)AUTOGRAD_NEURON_DERIV * dim) + idx]);
            lhs_grad++;
            out_grad++;
            idx++;
        }

        state_row = state_row + ((uintptr_t)LDG_MATH_STAT_NEURON_STATE_CUNT * dim);
        row++;
    }
}

static void autograd_kwin_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_val = autograd_val_ptr_get(graph, entry->out);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    const double *grad_end = out_grad + autograd_node_elem_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);

    while (out_grad < grad_end)
    {
        if (*out_val <= *lhs_val && *out_val >= *lhs_val) { *lhs_grad = *lhs_grad + *out_grad; }

        out_grad++;
        out_val++;
        lhs_val++;
        lhs_grad++;
    }
}

static void autograd_conv_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    autograd_conv_geom_t desc = LDG_STRUCT_ZERO_INIT;
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *weight = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *weight_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uintptr_t src_width = 0;
    uintptr_t src_off = 0;
    uintptr_t tap_off = 0;
    double gap = 0.0;
    uint32_t src_row = 0;
    uint32_t src_col = 0;
    uint32_t row = 0;
    uint32_t out_row = 0;
    uint32_t out_col = 0;
    uint32_t out_chan = 0;
    uint32_t kernel_row = 0;
    uint32_t kernel_col = 0;
    uint32_t chan = 0;
    uint32_t ret = 0;

    ret = autograd_conv_geom_get(graph, entry->aux, &desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return; }

    src_width = (uintptr_t)desc.in_h * (uintptr_t)desc.in_w * (uintptr_t)desc.in_c;

    while (row < graph->node[entry->out].row_cunt)
    {
        out_row = 0;
        while (out_row < desc.out_h)
        {
            out_col = 0;
            while (out_col < desc.out_w)
            {
                out_chan = 0;
                while (out_chan < desc.out_c)
                {
                    gap = *out_grad;
                    kernel_row = 0;
                    while (kernel_row < desc.kernel_h)
                    {
                        kernel_col = 0;
                        while (kernel_col < desc.kernel_w)
                        {
                            if (autograd_conv_src_off_get(&desc, out_row, kernel_row, desc.in_h, &src_row) == LDG_ERR_AOK && autograd_conv_src_off_get(&desc, out_col, kernel_col, desc.in_w, &src_col) == LDG_ERR_AOK)
                            {
                                chan = 0;
                                while (chan < desc.in_c)
                                {
                                    src_off = ((((uintptr_t)src_row * (uintptr_t)desc.in_w) + (uintptr_t)src_col) * (uintptr_t)desc.in_c) + (uintptr_t)chan;
                                    tap_off = (((((uintptr_t)kernel_row * (uintptr_t)desc.kernel_w) + (uintptr_t)kernel_col) * (uintptr_t)desc.in_c) + (uintptr_t)chan) * (uintptr_t)desc.out_c + (uintptr_t)out_chan;

                                    lhs_grad[src_off] = lhs_grad[src_off] + (gap * weight[tap_off]);
                                    weight_grad[tap_off] = weight_grad[tap_off] + (gap * lhs_val[src_off]);
                                    chan++;
                                }
                            }

                            kernel_col++;
                        }

                        kernel_row++;
                    }

                    out_grad++;
                    out_chan++;
                }

                out_col++;
            }

            out_row++;
        }

        lhs_val = lhs_val + src_width;
        lhs_grad = lhs_grad + src_width;
        row++;
    }
}

static void autograd_group_bias_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *bias_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uint32_t width = graph->node[entry->rhs].col_cunt;
    uint32_t group_cunt = graph->node[entry->lhs].col_cunt / width;
    uint32_t row = 0;
    uint32_t group = 0;
    uint32_t idx = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        group = 0;
        while (group < group_cunt)
        {
            idx = 0;
            while (idx < width)
            {
                *lhs_grad = *lhs_grad + *out_grad;
                bias_grad[idx] = bias_grad[idx] + *out_grad;
                out_grad++;
                lhs_grad++;
                idx++;
            }

            group++;
        }

        row++;
    }
}

static void autograd_cosine_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_row = autograd_val_ptr_get(graph, entry->lhs);
    const double *rhs_row = 0x0;
    const double *out_val = autograd_val_ptr_get(graph, entry->out);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = 0x0;
    double lhs_len = 0.0;
    double rhs_len = 0.0;
    double scale = 0.0;
    double lhs_share = 0.0;
    double rhs_share = 0.0;
    uint32_t width = graph->node[entry->lhs].col_cunt;
    uint32_t row = 0;
    uint32_t proto = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        lhs_len = autograd_row_len_get(lhs_row, width, entry->scalar);
        rhs_row = autograd_val_ptr_get(graph, entry->rhs);
        rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);

        proto = 0;
        while (proto < graph->node[entry->rhs].row_cunt)
        {
            rhs_len = autograd_row_len_get(rhs_row, width, entry->scalar);
            scale = *out_grad / (lhs_len * rhs_len);
            lhs_share = (*out_grad * *out_val) / (lhs_len * lhs_len);
            rhs_share = (*out_grad * *out_val) / (rhs_len * rhs_len);

            col = 0;
            while (col < width)
            {
                lhs_grad[col] = lhs_grad[col] + (scale * rhs_row[col]) - (lhs_share * lhs_row[col]);
                rhs_grad[col] = rhs_grad[col] + (scale * lhs_row[col]) - (rhs_share * rhs_row[col]);
                col++;
            }

            out_grad++;
            out_val++;
            rhs_row = rhs_row + width;
            rhs_grad = rhs_grad + width;
            proto++;
        }

        lhs_row = lhs_row + width;
        lhs_grad = lhs_grad + width;
        row++;
    }
}

static void autograd_euclid_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_row = autograd_val_ptr_get(graph, entry->lhs);
    const double *rhs_row = 0x0;
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = 0x0;
    double pull = 0.0;
    uint32_t width = graph->node[entry->lhs].col_cunt;
    uint32_t row = 0;
    uint32_t proto = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        rhs_row = autograd_val_ptr_get(graph, entry->rhs);
        rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);

        proto = 0;
        while (proto < graph->node[entry->rhs].row_cunt)
        {
            col = 0;
            while (col < width)
            {
                pull = AUTOGRAD_EUCLID_GRAD_FACTOR * *out_grad * (lhs_row[col] - rhs_row[col]);
                lhs_grad[col] = lhs_grad[col] - pull;
                rhs_grad[col] = rhs_grad[col] + pull;
                col++;
            }

            out_grad++;
            rhs_row = rhs_row + width;
            rhs_grad = rhs_grad + width;
            proto++;
        }

        lhs_row = lhs_row + width;
        lhs_grad = lhs_grad + width;
        row++;
    }
}

static void autograd_pool_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    autograd_pool_geom_t desc = LDG_STRUCT_ZERO_INIT;
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double share = 0.0;
    double picked = 0.0;
    uintptr_t src_width = 0;
    uintptr_t pick = 0;
    uint32_t row = 0;
    uint32_t out_row = 0;
    uint32_t out_col = 0;
    uint32_t chan = 0;
    uint32_t kernel_row = 0;
    uint32_t kernel_col = 0;
    uint8_t max_is = LDG_TRUTH_FALSE;

    if (autograd_pool_geom_get(graph, entry->aux, &desc) != LDG_ERR_AOK) { return; }

    if (entry->scalar > AUTOGRAD_POOL_MAX_OFF) { max_is = LDG_TRUTH_TRUE; }

    src_width = (uintptr_t)desc.in_h * (uintptr_t)desc.in_w * (uintptr_t)desc.in_c;

    while (row < graph->node[entry->out].row_cunt)
    {
        out_row = 0;
        while (out_row < desc.out_h)
        {
            out_col = 0;
            while (out_col < desc.out_w)
            {
                chan = 0;
                while (chan < desc.in_c)
                {
                    if (max_is == LDG_TRUTH_TRUE)
                    {
                        pick = autograd_pool_win_pick_get(&desc, lhs_val, out_row, out_col, chan, max_is, &picked);
                        lhs_grad[pick] = lhs_grad[pick] + *out_grad;
                    }
                    else
                    {
                        share = *out_grad / (double)(desc.kernel_h * desc.kernel_w);

                        kernel_row = 0;
                        while (kernel_row < desc.kernel_h)
                        {
                            kernel_col = 0;
                            while (kernel_col < desc.kernel_w)
                            {
                                pick = autograd_pool_src_idx_get(&desc, out_row, out_col, kernel_row, kernel_col, chan);
                                lhs_grad[pick] = lhs_grad[pick] + share;
                                kernel_col++;
                            }

                            kernel_row++;
                        }
                    }

                    out_grad++;
                    chan++;
                }

                out_col++;
            }

            out_row++;
        }

        lhs_val = lhs_val + src_width;
        lhs_grad = lhs_grad + src_width;
        row++;
    }
}

static void autograd_embedding_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_ptr = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *table_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uintptr_t width = graph->node[entry->rhs].col_cunt;
    uintptr_t pick = 0;
    uint32_t lane_cunt = graph->node[entry->lhs].col_cunt;
    uint32_t row = 0;
    uint32_t lane = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        lane = 0;
        while (lane < lane_cunt)
        {
            pick = (uintptr_t)*lhs_ptr * width;

            col = 0;
            while (col < width)
            {
                table_grad[pick + col] = table_grad[pick + col] + out_grad[col];
                col++;
            }

            out_grad = out_grad + width;
            lhs_ptr++;
            lane++;
        }

        row++;
    }
}

static void autograd_row_norm_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_row = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_row = autograd_val_ptr_get(graph, entry->out);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double mean = 0.0;
    double inv = 0.0;
    double grad_mean = 0.0;
    double grad_dot = 0.0;
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        autograd_row_stat_get(lhs_row, width, entry->scalar, &mean, &inv);

        grad_mean = 0.0;
        grad_dot = 0.0;
        col = 0;
        while (col < width)
        {
            grad_mean = grad_mean + out_grad[col];
            grad_dot = grad_dot + (out_grad[col] * out_row[col]);
            col++;
        }

        grad_mean = grad_mean / (double)width;
        grad_dot = grad_dot / (double)width;

        col = 0;
        while (col < width)
        {
            lhs_grad[col] = lhs_grad[col] + (inv * (out_grad[col] - grad_mean - (out_row[col] * grad_dot)));
            col++;
        }

        lhs_row = lhs_row + width;
        out_row = out_row + width;
        out_grad = out_grad + width;
        lhs_grad = lhs_grad + width;
        row++;
    }
}

static void autograd_row_rms_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_row = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_row = autograd_val_ptr_get(graph, entry->out);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double inv = 0.0;
    double grad_dot = 0.0;
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        inv = autograd_row_rms_inv_get(lhs_row, width, entry->scalar);

        grad_dot = 0.0;
        col = 0;
        while (col < width)
        {
            grad_dot = grad_dot + (out_grad[col] * out_row[col]);
            col++;
        }

        grad_dot = grad_dot / (double)width;

        col = 0;
        while (col < width)
        {
            lhs_grad[col] = lhs_grad[col] + (inv * (out_grad[col] - (out_row[col] * grad_dot)));
            col++;
        }

        lhs_row = lhs_row + width;
        out_row = out_row + width;
        out_grad = out_grad + width;
        lhs_grad = lhs_grad + width;
        row++;
    }
}

static void autograd_row_repeat_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t rep = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->lhs].row_cunt)
    {
        rep = 0;
        while (rep < entry->aux)
        {
            col = 0;
            while (col < width)
            {
                lhs_grad[col] = lhs_grad[col] + out_grad[col];
                col++;
            }

            out_grad = out_grad + width;
            rep++;
        }

        lhs_grad = lhs_grad + width;
        row++;
    }
}

static void autograd_tile_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    uintptr_t blk = autograd_node_elem_get(graph, entry->lhs);
    uintptr_t idx = 0;
    uint32_t rep = 0;

    while (rep < entry->aux)
    {
        idx = 0;
        while (idx < blk)
        {
            lhs_grad[idx] = lhs_grad[idx] + out_grad[idx];
            idx++;
        }

        out_grad = out_grad + blk;
        rep++;
    }
}

static void autograd_row_pool_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *mask = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    const double *mask_row = 0x0;
    double valid = 0.0;
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t group = 0;
    uint32_t member = 0;
    uint32_t col = 0;

    while (group < graph->node[entry->out].row_cunt)
    {
        valid = 0.0;
        member = 0;
        while (member < entry->aux)
        {
            if (mask[member] == AUTOGRAD_MASK_ON) { valid = valid + AUTOGRAD_MASK_ON; }

            member++;
        }

        mask_row = mask;
        member = 0;
        while (member < entry->aux)
        {
            if (*mask_row == AUTOGRAD_MASK_ON)
            {
                col = 0;
                while (col < width)
                {
                    lhs_grad[col] = lhs_grad[col] + (out_grad[col] / valid);
                    col++;
                }
            }

            lhs_grad = lhs_grad + width;
            mask_row++;
            member++;
        }

        mask = mask + entry->aux;
        out_grad = out_grad + width;
        group++;
    }
}

static void autograd_attn_score_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *q_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *k_val = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *q_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *k_grad = autograd_grad_ptr_get(graph, entry->rhs);
    const double *q_row = 0x0;
    const double *k_row = 0x0;
    double *q_grad_row = 0x0;
    double *k_grad_row = 0x0;
    double seed = 0.0;
    double scale = 0.0;
    uint32_t width = graph->node[entry->lhs].col_cunt;
    uint32_t hd_cunt = (uint32_t)entry->scalar;
    uint32_t hd_dim = width / hd_cunt;
    uint32_t group_len = entry->aux;
    uint32_t group_cunt = graph->node[entry->lhs].row_cunt / group_len;
    uint32_t group = 0;
    uint32_t hd = 0;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t dim = 0;
    uintptr_t q_off = 0;
    uintptr_t k_off = 0;

    scale = AUTOGRAD_MASK_ON / ldg_math_sqrt_get((double)hd_dim);

    while (group < group_cunt)
    {
        hd = 0;
        while (hd < hd_cunt)
        {
            query = 0;
            while (query < group_len)
            {
                q_off = ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)query) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                q_row = q_val + q_off;
                q_grad_row = q_grad + q_off;
                key = 0;
                while (key < group_len)
                {
                    seed = *out_grad * scale;
                    out_grad++;

                    if (seed != AUTOGRAD_NO_SCALAR)
                    {
                        k_off = ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)key) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                        k_row = k_val + k_off;
                        k_grad_row = k_grad + k_off;
                        dim = 0;
                        while (dim < hd_dim)
                        {
                            q_grad_row[dim] = q_grad_row[dim] + (seed * k_row[dim]);
                            k_grad_row[dim] = k_grad_row[dim] + (seed * q_row[dim]);
                            dim++;
                        }
                    }

                    key++;
                }

                query++;
            }

            hd++;
        }

        group++;
    }
}

static void autograd_attn_mask_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    const double *grad_end = out_grad + autograd_node_elem_get(graph, entry->out);

    while (out_grad < grad_end)
    {
        *lhs_grad = *lhs_grad + *out_grad;
        out_grad++;
        lhs_grad++;
    }
}

static void autograd_attn_apply_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *prob = autograd_val_ptr_get(graph, entry->lhs);
    const double *v_val = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *prob_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *v_grad = autograd_grad_ptr_get(graph, entry->rhs);
    const double *out_grad_row = 0x0;
    const double *v_row = 0x0;
    double *v_grad_row = 0x0;
    double weight = 0.0;
    double cum = 0.0;
    uint32_t width = graph->node[entry->rhs].col_cunt;
    uint32_t hd_cunt = (uint32_t)entry->scalar;
    uint32_t hd_dim = width / hd_cunt;
    uint32_t group_len = entry->aux;
    uint32_t group_cunt = graph->node[entry->rhs].row_cunt / group_len;
    uint32_t group = 0;
    uint32_t hd = 0;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t dim = 0;
    uintptr_t v_off = 0;

    while (group < group_cunt)
    {
        hd = 0;
        while (hd < hd_cunt)
        {
            query = 0;
            while (query < group_len)
            {
                out_grad_row = out_grad + ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)query) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                key = 0;
                while (key < group_len)
                {
                    v_off = ((((uintptr_t)group * (uintptr_t)group_len) + (uintptr_t)key) * (uintptr_t)width) + ((uintptr_t)hd * (uintptr_t)hd_dim);
                    v_row = v_val + v_off;
                    v_grad_row = v_grad + v_off;
                    weight = *prob;
                    cum = 0.0;
                    dim = 0;
                    while (dim < hd_dim)
                    {
                        cum = cum + (out_grad_row[dim] * v_row[dim]);
                        v_grad_row[dim] = v_grad_row[dim] + (weight * out_grad_row[dim]);
                        dim++;
                    }

                    *prob_grad = *prob_grad + cum;
                    prob++;
                    prob_grad++;
                    key++;
                }

                query++;
            }

            hd++;
        }

        group++;
    }
}

static void autograd_matmul_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_row = autograd_val_ptr_get(graph, entry->lhs);
    const double *rhs_val = autograd_val_ptr_get(graph, entry->rhs);
    const double *rhs_row = 0x0;
    const double *out_grad_row = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad_row = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);
    double *rhs_grad_row = 0x0;
    double seed = 0.0;
    double cum = 0.0;
    uint32_t inner_cunt = graph->node[entry->lhs].col_cunt;
    uint32_t out_col = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t inner = 0;
    uint8_t lhs_grad_is = graph->node[entry->lhs].grad_is;

    while (row < graph->node[entry->out].row_cunt)
    {
        inner = 0;
        while (inner < inner_cunt)
        {
            seed = lhs_row[inner];

            if (seed != AUTOGRAD_NO_SCALAR)
            {
                rhs_grad_row = rhs_grad + ((uintptr_t)inner * (uintptr_t)out_col);
                col = 0;
                while (col < out_col)
                {
                    rhs_grad_row[col] = rhs_grad_row[col] + (out_grad_row[col] * seed);
                    col++;
                }
            }

            if (lhs_grad_is == LDG_TRUTH_TRUE)
            {
                rhs_row = rhs_val + ((uintptr_t)inner * (uintptr_t)out_col);
                cum = 0.0;
                col = 0;
                while (col < out_col)
                {
                    cum = cum + (out_grad_row[col] * rhs_row[col]);
                    col++;
                }

                lhs_grad_row[inner] = lhs_grad_row[inner] + cum;
            }

            inner++;
        }

        lhs_row = lhs_row + inner_cunt;
        lhs_grad_row = lhs_grad_row + inner_cunt;
        out_grad_row = out_grad_row + out_col;
        row++;
    }
}

static void autograd_bias_add_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        col = 0;
        while (col < width)
        {
            *lhs_grad = *lhs_grad + *out_grad;
            rhs_grad[col] = rhs_grad[col] + *out_grad;
            out_grad++;
            lhs_grad++;
            col++;
        }

        row++;
    }
}

static void autograd_row_mul_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *rhs_val = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        col = 0;
        while (col < width)
        {
            *lhs_grad = *lhs_grad + (*out_grad * rhs_val[col]);
            rhs_grad[col] = rhs_grad[col] + (*out_grad * *lhs_val);
            out_grad++;
            lhs_grad++;
            lhs_val++;
            col++;
        }

        row++;
    }
}

static void autograd_log_softmax_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_val = autograd_val_ptr_get(graph, entry->out);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    const uint8_t *mask_row = (const uint8_t *)entry->ref;
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double cum = 0.0;
    uint32_t width = graph->node[entry->out].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        cum = 0.0;
        col = 0;
        while (col < width)
        {
            if (autograd_mask_on_is(mask_row, col) == LDG_TRUTH_TRUE) { cum = cum + out_grad[col]; }

            col++;
        }

        col = 0;
        while (col < width)
        {
            if (autograd_mask_on_is(mask_row, col) == LDG_TRUTH_TRUE) { lhs_grad[col] = lhs_grad[col] + out_grad[col] - (ldg_math_exp_get(out_val[col]) * cum); }

            col++;
        }

        out_val = out_val + width;
        out_grad = out_grad + width;
        lhs_grad = lhs_grad + width;
        if (mask_row) { mask_row = mask_row + width; }

        row++;
    }
}

static void autograd_gather_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *rhs_val = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad_row = autograd_grad_ptr_get(graph, entry->lhs);
    uint32_t width = graph->node[entry->lhs].col_cunt;
    uint32_t row = 0;
    uint32_t pick = 0;

    while (row < graph->node[entry->out].row_cunt)
    {
        pick = (uint32_t)rhs_val[row];
        lhs_grad_row[pick] = lhs_grad_row[pick] + out_grad[row];
        lhs_grad_row = lhs_grad_row + width;
        row++;
    }
}

static void autograd_row_sum_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double seed = 0.0;
    uint32_t width = graph->node[entry->lhs].col_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < graph->node[entry->lhs].row_cunt)
    {
        seed = out_grad[row];
        col = 0;
        while (col < width)
        {
            *lhs_grad = *lhs_grad + seed;
            lhs_grad++;
            col++;
        }

        row++;
    }
}

static void autograd_mean_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    uintptr_t elem = autograd_node_elem_get(graph, entry->lhs);
    const double *lhs_end = lhs_grad + elem;
    double share = out_grad[0] / (double)elem;

    while (lhs_grad < lhs_end)
    {
        *lhs_grad = *lhs_grad + share;
        lhs_grad++;
    }
}

static void autograd_pair_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *rhs_val = autograd_val_ptr_get(graph, entry->rhs);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    double *rhs_grad = autograd_grad_ptr_get(graph, entry->rhs);
    const double *grad_end = out_grad + autograd_node_elem_get(graph, entry->out);

    if (entry->op == AUTOGRAD_OP_ADD)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + *out_grad;
            *rhs_grad = *rhs_grad + *out_grad;
            out_grad++;
            lhs_grad++;
            rhs_grad++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_SUB)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + *out_grad;
            *rhs_grad = *rhs_grad - *out_grad;
            out_grad++;
            lhs_grad++;
            rhs_grad++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_MUL)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + (*out_grad * *rhs_val);
            *rhs_grad = *rhs_grad + (*out_grad * *lhs_val);
            out_grad++;
            lhs_grad++;
            rhs_grad++;
            lhs_val++;
            rhs_val++;
        }

        return;
    }

    while (out_grad < grad_end)
    {
        if (*lhs_val <= *rhs_val) { *lhs_grad = *lhs_grad + *out_grad; }
        else { *rhs_grad = *rhs_grad + *out_grad; }

        out_grad++;
        lhs_grad++;
        rhs_grad++;
        lhs_val++;
        rhs_val++;
    }
}

static void autograd_unary_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    const double *lhs_val = autograd_val_ptr_get(graph, entry->lhs);
    const double *out_val = autograd_val_ptr_get(graph, entry->out);
    const double *out_grad = autograd_grad_ptr_get(graph, entry->out);
    double *lhs_grad = autograd_grad_ptr_get(graph, entry->lhs);
    const double *grad_end = out_grad + autograd_node_elem_get(graph, entry->out);
    double floor_val = AUTOGRAD_RATIO_CENTER - entry->scalar;
    double ceil_val = AUTOGRAD_RATIO_CENTER + entry->scalar;
    double inner = 0.0;
    double slope = 0.0;

    if (entry->op == AUTOGRAD_OP_SCALE)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + (*out_grad * entry->scalar);
            out_grad++;
            lhs_grad++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_SHIFT || entry->op == AUTOGRAD_OP_RESHAPE)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + *out_grad;
            out_grad++;
            lhs_grad++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_CLIP)
    {
        while (out_grad < grad_end)
        {
            if (*lhs_val > floor_val && *lhs_val < ceil_val) { *lhs_grad = *lhs_grad + *out_grad; }

            out_grad++;
            lhs_grad++;
            lhs_val++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_TANH)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + (*out_grad * (AUTOGRAD_TANH_MAX - (*out_val * *out_val)));
            out_grad++;
            lhs_grad++;
            out_val++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_SIGMOID)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + (*out_grad * *out_val * (AUTOGRAD_TANH_MAX - *out_val));
            out_grad++;
            lhs_grad++;
            out_val++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_RELU)
    {
        while (out_grad < grad_end)
        {
            if (*lhs_val > AUTOGRAD_RELU_FLOOR) { *lhs_grad = *lhs_grad + *out_grad; }

            out_grad++;
            lhs_grad++;
            lhs_val++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_EXP)
    {
        while (out_grad < grad_end)
        {
            *lhs_grad = *lhs_grad + (*out_grad * *out_val);
            out_grad++;
            lhs_grad++;
            out_val++;
        }

        return;
    }

    if (entry->op == AUTOGRAD_OP_GELU)
    {
        while (out_grad < grad_end)
        {
            inner = autograd_tanh_get(autograd_gelu_inner_get(*lhs_val));
            slope = ((double)AUTOGRAD_GELU_HALF * (AUTOGRAD_TANH_MAX + inner)) + ((double)AUTOGRAD_GELU_HALF * *lhs_val * (AUTOGRAD_TANH_MAX - (inner * inner)) * (double)AUTOGRAD_GELU_SQRT_2_OVER_PI * (AUTOGRAD_TANH_MAX + ((double)AUTOGRAD_GELU_CUBIC_GRAD_FACTOR * (double)AUTOGRAD_GELU_CUBIC * *lhs_val * *lhs_val)));
            *lhs_grad = *lhs_grad + (*out_grad * slope);
            out_grad++;
            lhs_grad++;
            lhs_val++;
        }

        return;
    }

    while (out_grad < grad_end)
    {
        *lhs_grad = *lhs_grad + (*out_grad * AUTOGRAD_SQUARE_GRAD_FACTOR * *lhs_val);
        out_grad++;
        lhs_grad++;
        lhs_val++;
    }
}

static void autograd_entry_backward(ldg_math_stat_autograd_t *graph, const ldg_math_stat_autograd_tape_t *entry)
{
    if (entry->op == AUTOGRAD_OP_MATMUL) { autograd_matmul_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_BIAS_ADD) { autograd_bias_add_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ROW_MUL) { autograd_row_mul_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_LOG_SOFTMAX) { autograd_log_softmax_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_GATHER) { autograd_gather_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ROW_SUM) { autograd_row_sum_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_MEAN) { autograd_mean_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ROW_NORM) { autograd_row_norm_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ROW_RMS) { autograd_row_rms_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ROW_REPEAT) { autograd_row_repeat_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ROW_POOL) { autograd_row_pool_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ATTN_SCORE) { autograd_attn_score_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ATTN_MASK) { autograd_attn_mask_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ATTN_APPLY) { autograd_attn_apply_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_COL_MUL) { autograd_col_mul_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_COL_CONCAT) { autograd_col_concat_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_SCALAR_MUL) { autograd_scalar_mul_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_SYNAPSE) { autograd_synapse_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_NEURON) { autograd_neuron_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_KWIN) { autograd_kwin_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_CONV) { autograd_conv_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_GROUP_BIAS) { autograd_group_bias_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_COSINE) { autograd_cosine_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_EUCLID) { autograd_euclid_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_POOL) { autograd_pool_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_EMBEDDING) { autograd_embedding_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_TILE) { autograd_tile_backward(graph, entry); return; }

    if (entry->op == AUTOGRAD_OP_ADD || entry->op == AUTOGRAD_OP_SUB || entry->op == AUTOGRAD_OP_MUL || entry->op == AUTOGRAD_OP_MIN)
    {
        autograd_pair_backward(graph, entry);
        return;
    }

    autograd_unary_backward(graph, entry);
}

uint32_t ldg_math_stat_autograd_backward_run(ldg_math_stat_autograd_t *graph, uint32_t root)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = autograd_node_valid_ck(graph, root);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(autograd_node_elem_get(graph, root) != AUTOGRAD_SCALAR_ELEM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    autograd_grad_ptr_get(graph, root)[0] = AUTOGRAD_ROOT_SEED;

    idx = graph->tape_cunt;
    while (idx > 0)
    {
        idx--;
        autograd_entry_backward(graph, &graph->tape[idx]);
    }

    return LDG_ERR_AOK;
}
