#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define DENSITY_NONE 0.0
#define DENSITY_WEIGHT_FULL 1.0
#define DENSITY_EXPONENT_SCALE (-0.5)
#define DENSITY_GRID_MIN 2
#define DENSITY_ADJACENT_OFF 1
#define DENSITY_POINT_DIM 2
#define DENSITY_AXIS_X 0
#define DENSITY_AXIS_Y 1

static uint32_t density_elem_ck(const double *elem, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t density_axis_ck(const ldg_math_stat_density_axis_t *axis)
{
    if (LDG_UNLIKELY(ldg_math_finite_is(axis->lower) != LDG_TRUTH_TRUE || ldg_math_finite_is(axis->upper) != LDG_TRUTH_TRUE || ldg_math_finite_is(axis->bandwidth) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(axis->upper <= axis->lower || axis->bandwidth <= DENSITY_NONE || axis->grid_cunt < DENSITY_GRID_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static double density_step_get(const ldg_math_stat_density_axis_t *axis)
{
    return (axis->upper - axis->lower) / (double)(axis->grid_cunt - DENSITY_ADJACENT_OFF);
}

static double density_kernel_get(double diff, double bandwidth)
{
    double ratio = diff / bandwidth;
    double exponent = DENSITY_EXPONENT_SCALE * ratio * ratio;

    if (exponent < LDG_MATH_EXP_ARG_MIN) { return DENSITY_NONE; }

    return ldg_math_exp_get(exponent);
}

static void density_axis_cum(const ldg_math_stat_density_axis_t *axis, double val, double weight, double *dst)
{
    double step = density_step_get(axis);
    uint32_t grid = 0;

    while (grid < axis->grid_cunt)
    {
        dst[grid] += weight * density_kernel_get((axis->lower + ((double)grid * step)) - val, axis->bandwidth);
        grid++;
    }
}

static void density_buff_scale(double *buff, uintptr_t cunt, double scale)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        buff[idx] *= scale;
        idx++;
    }
}

static void density_buff_rst(double *buff, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        buff[idx] = DENSITY_NONE;
        idx++;
    }
}

static void density_point_cum(const ldg_math_stat_density_axis_t *axis_x, const ldg_math_stat_density_axis_t *axis_y, const double *point, const double *buff, double *dst)
{
    double *row_ptr = dst;
    double step = density_step_get(axis_y);
    double weight = DENSITY_NONE;
    uint32_t grid_x = 0;
    uint32_t grid_y = 0;

    while (grid_y < axis_y->grid_cunt)
    {
        weight = density_kernel_get((axis_y->lower + ((double)grid_y * step)) - point[DENSITY_AXIS_Y], axis_y->bandwidth);
        grid_x = 0;

        while (grid_x < axis_x->grid_cunt)
        {
            row_ptr[grid_x] += weight * buff[grid_x];
            grid_x++;
        }

        row_ptr += axis_x->grid_cunt;
        grid_y++;
    }
}

uint32_t ldg_math_stat_density_1d_fill(const double *val, uint32_t cunt, const ldg_math_stat_density_axis_t *axis, double *dst, uint32_t cap)
{
    double weight = DENSITY_NONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!val || !axis || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = density_axis_ck(axis);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < axis->grid_cunt)) { return LDG_ERR_OVERFLOW; }

    ret = density_elem_ck(val, (uintptr_t)cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    weight = ldg_math_sqrt_get(LDG_MATH_TAU) * axis->bandwidth * (double)cunt;
    density_buff_rst(dst, (uintptr_t)axis->grid_cunt);

    while (idx < cunt)
    {
        density_axis_cum(axis, val[idx], DENSITY_WEIGHT_FULL, dst);
        idx++;
    }

    density_buff_scale(dst, (uintptr_t)axis->grid_cunt, DENSITY_WEIGHT_FULL / weight);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_density_2d_fill(const double *point, uint32_t cunt, const ldg_math_stat_density_axis_t *axis_x, const ldg_math_stat_density_axis_t *axis_y, double *buff, uint32_t buff_cunt, double *dst, uintptr_t cap)
{
    const double *point_ptr = point;
    double weight = DENSITY_NONE;
    uintptr_t grid_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!point || !axis_x || !axis_y || !buff || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = density_axis_ck(axis_x);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = density_axis_ck(axis_y);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    grid_cunt = (uintptr_t)axis_x->grid_cunt * (uintptr_t)axis_y->grid_cunt;

    if (LDG_UNLIKELY(buff_cunt < axis_x->grid_cunt || cap < grid_cunt)) { return LDG_ERR_OVERFLOW; }

    ret = density_elem_ck(point, (uintptr_t)cunt * (uintptr_t)DENSITY_POINT_DIM);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    weight = LDG_MATH_TAU * axis_x->bandwidth * axis_y->bandwidth * (double)cunt;
    density_buff_rst(dst, grid_cunt);

    while (idx < cunt)
    {
        density_buff_rst(buff, (uintptr_t)axis_x->grid_cunt);
        density_axis_cum(axis_x, point_ptr[DENSITY_AXIS_X], DENSITY_WEIGHT_FULL, buff);
        density_point_cum(axis_x, axis_y, point_ptr, buff, dst);
        point_ptr += DENSITY_POINT_DIM;
        idx++;
    }

    density_buff_scale(dst, grid_cunt, DENSITY_WEIGHT_FULL / weight);

    return LDG_ERR_AOK;
}
