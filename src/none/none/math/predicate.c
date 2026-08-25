#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/math/predicate.h>

#define PREDICATE_EPSILON 1.1102230246251565e-16
#define PREDICATE_SPLITTER 134217729.0
#define PREDICATE_COUNTERCLOCKWISE_ERR_BOUND_A ((3.0 + 16.0 * PREDICATE_EPSILON) * PREDICATE_EPSILON)
#define PREDICATE_INCIRCLE_ERR_BOUND_A ((10.0 + 96.0 * PREDICATE_EPSILON) * PREDICATE_EPSILON)
#define PREDICATE_ORIENT3D_ERR_BOUND_A ((7.0 + 56.0 * PREDICATE_EPSILON) * PREDICATE_EPSILON)
#define PREDICATE_INSPHERE_ERR_BOUND_A ((16.0 + 224.0 * PREDICATE_EPSILON) * PREDICATE_EPSILON)

#define PREDICATE_ORIENT2_BUFF 32U

#define PREDICATE_OFF_DETERMINANT_TERM 0U
#define PREDICATE_OFF_DETERMINANT 8U
#define PREDICATE_OFF_DETERMINANT_BUFF 72U
#define PREDICATE_OFF_MINOR 136U
#define PREDICATE_OFF_MINOR_BUFF 392U
#define PREDICATE_OFF_SCALED_A 648U
#define PREDICATE_OFF_SCALED_B 1160U
#define PREDICATE_OFF_LIFT_TERM 2184U
#define PREDICATE_OFF_AXIS 3208U
#define PREDICATE_OFF_AXIS_BUFF 7304U
#define PREDICATE_OFF_TOTAL 11400U
#define PREDICATE_OFF_TOTAL_BUFF 44168U

#define PREDICATE_BLK_DOUBLES 76936U
#define PREDICATE_BLK_SIZE ((uint64_t)PREDICATE_BLK_DOUBLES * (uint64_t)sizeof(double))

static double predicate_absolute_get(double val)
{
    if (val < 0.0) { return -val; }

    return val;
}

static uint8_t predicate_nonzero_is(double val)
{
    if (val < 0.0) { return LDG_TRUTH_TRUE; }

    if (val > 0.0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t predicate_sign_get(double val)
{
    if (val > 0.0) { return LDG_MATH_SIGN_POS; }

    if (val < 0.0) { return LDG_MATH_SIGN_NEG; }

    return LDG_MATH_SIGN_ZERO;
}

static void predicate_two_product_compute(double lhs, double rhs, double *out_high, double *out_low)
{
    double high = 0x0;
    double lhs_high = 0x0;
    double lhs_low = 0x0;
    double rhs_high = 0x0;
    double rhs_low = 0x0;
    double lhs_big = 0x0;
    double rhs_big = 0x0;
    double split = 0x0;
    double err_first = 0x0;
    double err_second = 0x0;
    double err_third = 0x0;
    high = lhs * rhs;
    split = PREDICATE_SPLITTER * lhs;
    lhs_big = split - lhs;
    lhs_high = split - lhs_big;
    lhs_low = lhs - lhs_high;
    split = PREDICATE_SPLITTER * rhs;
    rhs_big = split - rhs;
    rhs_high = split - rhs_big;
    rhs_low = rhs - rhs_high;
    err_first = high - (lhs_high * rhs_high);
    err_second = err_first - (lhs_low * rhs_high);
    err_third = err_second - (lhs_high * rhs_low);
    *out_high = high;
    *out_low = (lhs_low * rhs_low) - err_third;
}

static void predicate_two_sum_compute(double lhs, double rhs, double *out_high, double *out_low)
{
    double high = 0x0;
    double b_virtual = 0x0;
    double a_virtual = 0x0;
    double b_round = 0x0;
    double a_round = 0x0;
    high = lhs + rhs;
    b_virtual = high - lhs;
    a_virtual = high - b_virtual;
    b_round = rhs - b_virtual;
    a_round = lhs - a_virtual;
    *out_high = high;
    *out_low = a_round + b_round;
}

static void predicate_two_difference_compute(double lhs, double rhs, double *out_high, double *out_low)
{
    double high = 0x0;
    double b_virtual = 0x0;
    double a_virtual = 0x0;
    double b_round = 0x0;
    double a_round = 0x0;
    high = lhs - rhs;
    b_virtual = lhs - high;
    a_virtual = high + b_virtual;
    b_round = b_virtual - rhs;
    a_round = lhs - a_virtual;
    *out_high = high;
    *out_low = a_round + b_round;
}

static void predicate_fast_two_sum_compute(double lhs, double rhs, double *out_high, double *out_low)
{
    double high = 0x0;
    double b_virtual = 0x0;
    high = lhs + rhs;
    b_virtual = high - lhs;
    *out_high = high;
    *out_low = rhs - b_virtual;
}

static uint32_t predicate_scale(uint32_t len, const double *expansion, double factor, double *out)
{
    double running = 0x0;
    double sum_high = 0x0;
    double sum_low = 0x0;
    double product_high = 0x0;
    double product_low = 0x0;
    double now = 0x0;
    uint32_t out_cunt = 0x0;
    uint32_t idx = 0x0;
    if (len == 0) { return 0; }

    predicate_two_product_compute(expansion[0], factor, &running, &sum_low);
    if (predicate_nonzero_is(sum_low) == LDG_TRUTH_TRUE)
    {
        out[out_cunt] = sum_low;
        out_cunt++;
    }

    for (idx = 1; idx < len; idx++)
    {
        now = expansion[idx];
        predicate_two_product_compute(now, factor, &product_high, &product_low);
        predicate_two_sum_compute(running, product_low, &sum_high, &sum_low);
        if (predicate_nonzero_is(sum_low) == LDG_TRUTH_TRUE)
        {
            out[out_cunt] = sum_low;
            out_cunt++;
        }

        predicate_fast_two_sum_compute(product_high, sum_high, &running, &sum_low);
        if (predicate_nonzero_is(sum_low) == LDG_TRUTH_TRUE)
        {
            out[out_cunt] = sum_low;
            out_cunt++;
        }
    }
    if (predicate_nonzero_is(running) == LDG_TRUTH_TRUE)
    {
        out[out_cunt] = running;
        out_cunt++;
    }

    return out_cunt;
}

static uint32_t predicate_sum_compute(uint32_t len_a, const double *expansion_a, uint32_t len_b, const double *expansion_b, double *out)
{
    double running = 0x0;
    double running_next = 0x0;
    double tail = 0x0;
    double front_a = 0x0;
    double front_b = 0x0;
    uint32_t idx_a = 0x0;
    uint32_t idx_b = 0x0;
    uint32_t out_cunt = 0x0;
    front_a = expansion_a[0];
    front_b = expansion_b[0];
    if ((front_b > front_a) == (front_b > -front_a))
    {
        running = front_a;
        idx_a++;
    }
    else
    {
        running = front_b;
        idx_b++;
    }

    if ((idx_a < len_a) && (idx_b < len_b))
    {
        front_a = expansion_a[idx_a];
        front_b = expansion_b[idx_b];
        if ((front_b > front_a) == (front_b > -front_a))
        {
            predicate_fast_two_sum_compute(front_a, running, &running_next, &tail);
            idx_a++;
        }
        else
        {
            predicate_fast_two_sum_compute(front_b, running, &running_next, &tail);
            idx_b++;
        }

        running = running_next;
        if (predicate_nonzero_is(tail) == LDG_TRUTH_TRUE)
        {
            out[out_cunt] = tail;
            out_cunt++;
        }

        while ((idx_a < len_a) && (idx_b < len_b))
        {
            front_a = expansion_a[idx_a];
            front_b = expansion_b[idx_b];
            if ((front_b > front_a) == (front_b > -front_a))
            {
                predicate_two_sum_compute(running, front_a, &running_next, &tail);
                idx_a++;
            }
            else
            {
                predicate_two_sum_compute(running, front_b, &running_next, &tail);
                idx_b++;
            }

            running = running_next;
            if (predicate_nonzero_is(tail) == LDG_TRUTH_TRUE)
            {
                out[out_cunt] = tail;
                out_cunt++;
            }
        }
    }

    while (idx_a < len_a)
    {
        front_a = expansion_a[idx_a];
        predicate_two_sum_compute(running, front_a, &running_next, &tail);
        idx_a++;
        running = running_next;
        if (predicate_nonzero_is(tail) == LDG_TRUTH_TRUE)
        {
            out[out_cunt] = tail;
            out_cunt++;
        }
    }
    while (idx_b < len_b)
    {
        front_b = expansion_b[idx_b];
        predicate_two_sum_compute(running, front_b, &running_next, &tail);
        idx_b++;
        running = running_next;
        if (predicate_nonzero_is(tail) == LDG_TRUTH_TRUE)
        {
            out[out_cunt] = tail;
            out_cunt++;
        }
    }
    if (predicate_nonzero_is(running) == LDG_TRUTH_TRUE || (out_cunt == 0))
    {
        out[out_cunt] = running;
        out_cunt++;
    }

    return out_cunt;
}

static void predicate_cum(double *total, uint32_t *total_len, const double *addend, uint32_t addend_len, double *buff)
{
    uint32_t merged_len = 0x0;
    uint32_t idx = 0x0;
    if (addend_len == 0) { return; }

    if (*total_len == 0)
    {
        for (idx = 0; idx < addend_len; idx++) { total[idx] = addend[idx]; }
        *total_len = addend_len;
        return;
    }

    merged_len = predicate_sum_compute(*total_len, total, addend_len, addend, buff);
    for (idx = 0; idx < merged_len; idx++) { total[idx] = buff[idx]; }
    *total_len = merged_len;
}

static void predicate_product_cum(double lhs, double rhs, uint8_t negate, double *total, uint32_t *total_len, double *buff)
{
    double product[2] = LDG_ARR_ZERO_INIT;
    double product_high = 0x0;
    double product_low = 0x0;
    uint32_t product_len = 0x0;
    predicate_two_product_compute(lhs, rhs, &product_high, &product_low);
    if (negate)
    {
        product_low = -product_low;
        product_high = -product_high;
    }

    if (predicate_nonzero_is(product_low) == LDG_TRUTH_TRUE)
    {
        product[product_len] = product_low;
        product_len++;
    }

    if (predicate_nonzero_is(product_high) == LDG_TRUTH_TRUE)
    {
        product[product_len] = product_high;
        product_len++;
    }

    predicate_cum(total, total_len, product, product_len, buff);
}

static uint32_t predicate_orient2d_exact_compute(const double *point_a, const double *point_b, const double *point_c, double *out)
{
    double buff[PREDICATE_ORIENT2_BUFF] = LDG_ARR_ZERO_INIT;
    uint32_t total_len = 0x0;
    predicate_product_cum(point_a[0], point_b[1], 0, out, &total_len, buff);
    predicate_product_cum(point_a[0], point_c[1], 1, out, &total_len, buff);
    predicate_product_cum(point_c[0], point_b[1], 1, out, &total_len, buff);
    predicate_product_cum(point_a[1], point_b[0], 1, out, &total_len, buff);
    predicate_product_cum(point_a[1], point_c[0], 0, out, &total_len, buff);
    predicate_product_cum(point_c[1], point_b[0], 0, out, &total_len, buff);
    return total_len;
}

static uint32_t predicate_triple_compute(double val_a, double val_b, double val_c, uint8_t negate, double *out)
{
    double product[2] = LDG_ARR_ZERO_INIT;
    double product_high = 0x0;
    double product_low = 0x0;
    uint32_t len = 0x0;
    uint32_t idx = 0x0;
    predicate_two_product_compute(val_b, val_c, &product_high, &product_low);
    product[0] = product_low;
    product[1] = product_high;
    len = predicate_scale(2, product, val_a, out);
    if (negate) { for (idx = 0; idx < len; idx++) { out[idx] = -out[idx]; } }

    return len;
}

static uint32_t predicate_determinant3_compute(const double *point_p, const double *point_q, const double *point_r, double *out, double *buff, double *term)
{
    uint32_t total_len = 0x0;
    uint32_t term_len = 0x0;
    term_len = predicate_triple_compute(point_p[0], point_q[1], point_r[2], 0, term);
    predicate_cum(out, &total_len, term, term_len, buff);
    term_len = predicate_triple_compute(point_p[0], point_q[2], point_r[1], 1, term);
    predicate_cum(out, &total_len, term, term_len, buff);
    term_len = predicate_triple_compute(point_p[1], point_q[0], point_r[2], 1, term);
    predicate_cum(out, &total_len, term, term_len, buff);
    term_len = predicate_triple_compute(point_p[1], point_q[2], point_r[0], 0, term);
    predicate_cum(out, &total_len, term, term_len, buff);
    term_len = predicate_triple_compute(point_p[2], point_q[0], point_r[1], 0, term);
    predicate_cum(out, &total_len, term, term_len, buff);
    term_len = predicate_triple_compute(point_p[2], point_q[1], point_r[0], 1, term);
    predicate_cum(out, &total_len, term, term_len, buff);
    return total_len;
}

static uint32_t predicate_orient3d_exact_compute(const double *point_a, const double *point_b, const double *point_c, const double *point_d, double *out, double *out_buff, double *determinant, double *determinant_buff, double *determinant_term)
{
    uint32_t total_len = 0x0;
    uint32_t determinant_len = 0x0;
    uint32_t idx = 0x0;
    determinant_len = predicate_determinant3_compute(point_b, point_c, point_d, determinant, determinant_buff, determinant_term);
    for (idx = 0; idx < determinant_len; idx++) { determinant[idx] = -determinant[idx]; }
    predicate_cum(out, &total_len, determinant, determinant_len, out_buff);
    determinant_len = predicate_determinant3_compute(point_a, point_c, point_d, determinant, determinant_buff, determinant_term);
    predicate_cum(out, &total_len, determinant, determinant_len, out_buff);
    determinant_len = predicate_determinant3_compute(point_a, point_b, point_d, determinant, determinant_buff, determinant_term);
    for (idx = 0; idx < determinant_len; idx++) { determinant[idx] = -determinant[idx]; }
    predicate_cum(out, &total_len, determinant, determinant_len, out_buff);
    determinant_len = predicate_determinant3_compute(point_a, point_b, point_c, determinant, determinant_buff, determinant_term);
    predicate_cum(out, &total_len, determinant, determinant_len, out_buff);
    return total_len;
}

static uint32_t predicate_axis_lift(uint32_t minor_len, const double *minor, double difference_high, double difference_low, double *out, double *out_buff, double *scaled_a, double *scaled_b, double *term)
{
    uint32_t out_len = 0x0;
    uint32_t scaled_a_len = 0x0;
    uint32_t scaled_b_len = 0x0;
    uint32_t term_len = 0x0;
    scaled_a_len = predicate_scale(minor_len, minor, difference_high, scaled_a);
    term_len = predicate_scale(scaled_a_len, scaled_a, difference_high, term);
    predicate_cum(out, &out_len, term, term_len, out_buff);
    scaled_b_len = predicate_scale(scaled_a_len, scaled_a, difference_low, scaled_b);
    term_len = predicate_scale(scaled_b_len, scaled_b, 2.0, term);
    predicate_cum(out, &out_len, term, term_len, out_buff);
    scaled_a_len = predicate_scale(minor_len, minor, difference_low, scaled_a);
    term_len = predicate_scale(scaled_a_len, scaled_a, difference_low, term);
    predicate_cum(out, &out_len, term, term_len, out_buff);
    return out_len;
}

uint32_t ldg_math_2d_orient(const double *point_a, const double *point_b, const double *point_c, ldg_mem_pool_t *pool, uint8_t *out)
{
    double determinant_lhs = 0x0;
    double determinant_rhs = 0x0;
    double determinant = 0x0;
    double determinant_sum = 0x0;
    double err_bound = 0x0;
    double *base = 0x0;
    double *total = 0x0;
    void *blk = 0x0;
    uint32_t total_len = 0x0;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(point_a == 0x0 || point_b == 0x0 || point_c == 0x0 || pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_SIGN_ZERO;
    determinant_lhs = (point_a[0] - point_c[0]) * (point_b[1] - point_c[1]);
    determinant_rhs = (point_a[1] - point_c[1]) * (point_b[0] - point_c[0]);
    determinant = determinant_lhs - determinant_rhs;
    determinant_sum = predicate_absolute_get(determinant_lhs) + predicate_absolute_get(determinant_rhs);
    err_bound = PREDICATE_COUNTERCLOCKWISE_ERR_BOUND_A * determinant_sum;
    if (determinant > err_bound || -determinant > err_bound)
    {
        *out = predicate_sign_get(determinant);
        return LDG_ERR_AOK;
    }

    err = ldg_mem_pool_rst(pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(pool, PREDICATE_BLK_SIZE, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(blk == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    base = blk;
    total = base + PREDICATE_OFF_TOTAL;
    total_len = predicate_orient2d_exact_compute(point_a, point_b, point_c, total);
    if (total_len == 0)
    {
        *out = LDG_MATH_SIGN_ZERO;
        return LDG_ERR_AOK;
    }

    *out = predicate_sign_get(total[total_len - 1]);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_2d_incircle(const double *point_a, const double *point_b, const double *point_c, const double *point_d, ldg_mem_pool_t *pool, uint8_t *out)
{
    double adx = 0x0;
    double ady = 0x0;
    double bdx = 0x0;
    double bdy = 0x0;
    double cdx = 0x0;
    double cdy = 0x0;
    double bdxcdy = 0x0;
    double cdxbdy = 0x0;
    double cdxady = 0x0;
    double adxcdy = 0x0;
    double adxbdy = 0x0;
    double bdxady = 0x0;
    double alift = 0x0;
    double blift = 0x0;
    double clift = 0x0;
    double determinant = 0x0;
    double permanent = 0x0;
    double err_bound = 0x0;
    double difference_high = 0x0;
    double difference_low = 0x0;
    double *base = 0x0;
    double *minor = 0x0;
    double *axis = 0x0;
    double *axis_buff = 0x0;
    double *scaled_a = 0x0;
    double *scaled_b = 0x0;
    double *lift_term = 0x0;
    double *total = 0x0;
    double *total_buff = 0x0;
    void *blk = 0x0;
    uint32_t minor_len = 0x0;
    uint32_t axis_len = 0x0;
    uint32_t total_len = 0x0;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(point_a == 0x0 || point_b == 0x0 || point_c == 0x0 || point_d == 0x0 || pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_SIGN_ZERO;
    adx = point_a[0] - point_d[0];
    ady = point_a[1] - point_d[1];
    bdx = point_b[0] - point_d[0];
    bdy = point_b[1] - point_d[1];
    cdx = point_c[0] - point_d[0];
    cdy = point_c[1] - point_d[1];
    bdxcdy = bdx * cdy;
    cdxbdy = cdx * bdy;
    cdxady = cdx * ady;
    adxcdy = adx * cdy;
    adxbdy = adx * bdy;
    bdxady = bdx * ady;
    alift = adx * adx + ady * ady;
    blift = bdx * bdx + bdy * bdy;
    clift = cdx * cdx + cdy * cdy;
    determinant = alift * (bdxcdy - cdxbdy) + blift * (cdxady - adxcdy) + clift * (adxbdy - bdxady);
    permanent = (predicate_absolute_get(bdxcdy) + predicate_absolute_get(cdxbdy)) * alift + (predicate_absolute_get(cdxady) + predicate_absolute_get(adxcdy)) * blift + (predicate_absolute_get(adxbdy) + predicate_absolute_get(bdxady)) * clift;
    err_bound = PREDICATE_INCIRCLE_ERR_BOUND_A * permanent;
    if (determinant > err_bound || -determinant > err_bound)
    {
        *out = predicate_sign_get(determinant);
        return LDG_ERR_AOK;
    }

    err = ldg_mem_pool_rst(pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(pool, PREDICATE_BLK_SIZE, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(blk == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    base = blk;
    minor = base + PREDICATE_OFF_MINOR;
    axis = base + PREDICATE_OFF_AXIS;
    axis_buff = base + PREDICATE_OFF_AXIS_BUFF;
    scaled_a = base + PREDICATE_OFF_SCALED_A;
    scaled_b = base + PREDICATE_OFF_SCALED_B;
    lift_term = base + PREDICATE_OFF_LIFT_TERM;
    total = base + PREDICATE_OFF_TOTAL;
    total_buff = base + PREDICATE_OFF_TOTAL_BUFF;
    minor_len = predicate_orient2d_exact_compute(point_b, point_c, point_d, minor);
    predicate_two_difference_compute(point_a[0], point_d[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_a[1], point_d[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    minor_len = predicate_orient2d_exact_compute(point_c, point_a, point_d, minor);
    predicate_two_difference_compute(point_b[0], point_d[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_b[1], point_d[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    minor_len = predicate_orient2d_exact_compute(point_a, point_b, point_d, minor);
    predicate_two_difference_compute(point_c[0], point_d[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_c[1], point_d[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    if (total_len == 0)
    {
        *out = LDG_MATH_SIGN_ZERO;
        return LDG_ERR_AOK;
    }

    *out = predicate_sign_get(total[total_len - 1]);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_3d_orient(const double *point_a, const double *point_b, const double *point_c, const double *point_d, ldg_mem_pool_t *pool, uint8_t *out)
{
    double adx = 0x0;
    double ady = 0x0;
    double adz = 0x0;
    double bdx = 0x0;
    double bdy = 0x0;
    double bdz = 0x0;
    double cdx = 0x0;
    double cdy = 0x0;
    double cdz = 0x0;
    double bdxcdy = 0x0;
    double cdxbdy = 0x0;
    double cdxady = 0x0;
    double adxcdy = 0x0;
    double adxbdy = 0x0;
    double bdxady = 0x0;
    double determinant = 0x0;
    double permanent = 0x0;
    double err_bound = 0x0;
    double *base = 0x0;
    double *total = 0x0;
    double *total_buff = 0x0;
    double *determinant_minor = 0x0;
    double *determinant_buff = 0x0;
    double *determinant_term = 0x0;
    void *blk = 0x0;
    uint32_t total_len = 0x0;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(point_a == 0x0 || point_b == 0x0 || point_c == 0x0 || point_d == 0x0 || pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_SIGN_ZERO;
    adx = point_a[0] - point_d[0];
    ady = point_a[1] - point_d[1];
    adz = point_a[2] - point_d[2];
    bdx = point_b[0] - point_d[0];
    bdy = point_b[1] - point_d[1];
    bdz = point_b[2] - point_d[2];
    cdx = point_c[0] - point_d[0];
    cdy = point_c[1] - point_d[1];
    cdz = point_c[2] - point_d[2];
    bdxcdy = bdx * cdy;
    cdxbdy = cdx * bdy;
    cdxady = cdx * ady;
    adxcdy = adx * cdy;
    adxbdy = adx * bdy;
    bdxady = bdx * ady;
    determinant = adz * (bdxcdy - cdxbdy) + bdz * (cdxady - adxcdy) + cdz * (adxbdy - bdxady);
    permanent = (predicate_absolute_get(bdxcdy) + predicate_absolute_get(cdxbdy)) * predicate_absolute_get(adz) + (predicate_absolute_get(cdxady) + predicate_absolute_get(adxcdy)) * predicate_absolute_get(bdz) + (predicate_absolute_get(adxbdy) + predicate_absolute_get(bdxady)) * predicate_absolute_get(cdz);
    err_bound = PREDICATE_ORIENT3D_ERR_BOUND_A * permanent;
    if (determinant > err_bound || -determinant > err_bound)
    {
        *out = predicate_sign_get(determinant);
        return LDG_ERR_AOK;
    }

    err = ldg_mem_pool_rst(pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(pool, PREDICATE_BLK_SIZE, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(blk == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    base = blk;
    total = base + PREDICATE_OFF_TOTAL;
    total_buff = base + PREDICATE_OFF_TOTAL_BUFF;
    determinant_minor = base + PREDICATE_OFF_DETERMINANT;
    determinant_buff = base + PREDICATE_OFF_DETERMINANT_BUFF;
    determinant_term = base + PREDICATE_OFF_DETERMINANT_TERM;
    total_len = predicate_orient3d_exact_compute(point_a, point_b, point_c, point_d, total, total_buff, determinant_minor, determinant_buff, determinant_term);
    if (total_len == 0)
    {
        *out = LDG_MATH_SIGN_ZERO;
        return LDG_ERR_AOK;
    }

    *out = predicate_sign_get(total[total_len - 1]);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_3d_insphere(const double *point_a, const double *point_b, const double *point_c, const double *point_d, const double *point_e, ldg_mem_pool_t *pool, uint8_t *out)
{
    double aex = 0x0;
    double aey = 0x0;
    double aez = 0x0;
    double bex = 0x0;
    double bey = 0x0;
    double bez = 0x0;
    double cex = 0x0;
    double cey = 0x0;
    double cez = 0x0;
    double dex = 0x0;
    double dey = 0x0;
    double dez = 0x0;
    double ab = 0x0;
    double bc = 0x0;
    double cd = 0x0;
    double da = 0x0;
    double ac = 0x0;
    double bd = 0x0;
    double abc = 0x0;
    double bcd = 0x0;
    double cda = 0x0;
    double dab = 0x0;
    double alift = 0x0;
    double blift = 0x0;
    double clift = 0x0;
    double dlift = 0x0;
    double aezplus = 0x0;
    double bezplus = 0x0;
    double cezplus = 0x0;
    double dezplus = 0x0;
    double aexbeyplus = 0x0;
    double bexaeyplus = 0x0;
    double bexceyplus = 0x0;
    double cexbeyplus = 0x0;
    double cexdeyplus = 0x0;
    double dexceyplus = 0x0;
    double dexaeyplus = 0x0;
    double aexdeyplus = 0x0;
    double aexceyplus = 0x0;
    double cexaeyplus = 0x0;
    double bexdeyplus = 0x0;
    double dexbeyplus = 0x0;
    double determinant = 0x0;
    double permanent = 0x0;
    double err_bound = 0x0;
    double difference_high = 0x0;
    double difference_low = 0x0;
    double *base = 0x0;
    double *minor = 0x0;
    double *minor_buff = 0x0;
    double *determinant_minor = 0x0;
    double *determinant_buff = 0x0;
    double *determinant_term = 0x0;
    double *axis = 0x0;
    double *axis_buff = 0x0;
    double *scaled_a = 0x0;
    double *scaled_b = 0x0;
    double *lift_term = 0x0;
    double *total = 0x0;
    double *total_buff = 0x0;
    void *blk = 0x0;
    uint32_t minor_len = 0x0;
    uint32_t axis_len = 0x0;
    uint32_t total_len = 0x0;
    uint32_t err = 0x0;
    uint32_t idx = 0x0;
    if (LDG_UNLIKELY(point_a == 0x0 || point_b == 0x0 || point_c == 0x0 || point_d == 0x0 || point_e == 0x0 || pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_SIGN_ZERO;
    aex = point_a[0] - point_e[0];
    aey = point_a[1] - point_e[1];
    aez = point_a[2] - point_e[2];
    bex = point_b[0] - point_e[0];
    bey = point_b[1] - point_e[1];
    bez = point_b[2] - point_e[2];
    cex = point_c[0] - point_e[0];
    cey = point_c[1] - point_e[1];
    cez = point_c[2] - point_e[2];
    dex = point_d[0] - point_e[0];
    dey = point_d[1] - point_e[1];
    dez = point_d[2] - point_e[2];
    ab = aex * bey - bex * aey;
    bc = bex * cey - cex * bey;
    cd = cex * dey - dex * cey;
    da = dex * aey - aex * dey;
    ac = aex * cey - cex * aey;
    bd = bex * dey - dex * bey;
    abc = aez * bc - bez * ac + cez * ab;
    bcd = bez * cd - cez * bd + dez * bc;
    cda = cez * da + dez * ac + aez * cd;
    dab = dez * ab + aez * bd + bez * da;
    alift = aex * aex + aey * aey + aez * aez;
    blift = bex * bex + bey * bey + bez * bez;
    clift = cex * cex + cey * cey + cez * cez;
    dlift = dex * dex + dey * dey + dez * dez;
    determinant = (dlift * abc - clift * dab) + (blift * cda - alift * bcd);
    aezplus = predicate_absolute_get(aez);
    bezplus = predicate_absolute_get(bez);
    cezplus = predicate_absolute_get(cez);
    dezplus = predicate_absolute_get(dez);
    aexbeyplus = predicate_absolute_get(aex * bey);
    bexaeyplus = predicate_absolute_get(bex * aey);
    bexceyplus = predicate_absolute_get(bex * cey);
    cexbeyplus = predicate_absolute_get(cex * bey);
    cexdeyplus = predicate_absolute_get(cex * dey);
    dexceyplus = predicate_absolute_get(dex * cey);
    dexaeyplus = predicate_absolute_get(dex * aey);
    aexdeyplus = predicate_absolute_get(aex * dey);
    aexceyplus = predicate_absolute_get(aex * cey);
    cexaeyplus = predicate_absolute_get(cex * aey);
    bexdeyplus = predicate_absolute_get(bex * dey);
    dexbeyplus = predicate_absolute_get(dex * bey);
    permanent = ((cexdeyplus + dexceyplus) * bezplus + (dexbeyplus + bexdeyplus) * cezplus + (bexceyplus + cexbeyplus) * dezplus) * alift + ((dexaeyplus + aexdeyplus) * cezplus + (aexceyplus + cexaeyplus) * dezplus + (cexdeyplus + dexceyplus) * aezplus) * blift + ((aexbeyplus + bexaeyplus) * dezplus + (bexdeyplus + dexbeyplus) * aezplus + (dexaeyplus + aexdeyplus) * bezplus) * clift + ((bexceyplus + cexbeyplus) * aezplus + (cexaeyplus + aexceyplus) * bezplus + (aexbeyplus + bexaeyplus) * cezplus) * dlift;
    err_bound = PREDICATE_INSPHERE_ERR_BOUND_A * permanent;
    if (determinant > err_bound || -determinant > err_bound)
    {
        *out = predicate_sign_get(determinant);
        return LDG_ERR_AOK;
    }

    err = ldg_mem_pool_rst(pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(pool, PREDICATE_BLK_SIZE, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(blk == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    base = blk;
    minor = base + PREDICATE_OFF_MINOR;
    minor_buff = base + PREDICATE_OFF_MINOR_BUFF;
    determinant_minor = base + PREDICATE_OFF_DETERMINANT;
    determinant_buff = base + PREDICATE_OFF_DETERMINANT_BUFF;
    determinant_term = base + PREDICATE_OFF_DETERMINANT_TERM;
    axis = base + PREDICATE_OFF_AXIS;
    axis_buff = base + PREDICATE_OFF_AXIS_BUFF;
    scaled_a = base + PREDICATE_OFF_SCALED_A;
    scaled_b = base + PREDICATE_OFF_SCALED_B;
    lift_term = base + PREDICATE_OFF_LIFT_TERM;
    total = base + PREDICATE_OFF_TOTAL;
    total_buff = base + PREDICATE_OFF_TOTAL_BUFF;
    minor_len = predicate_orient3d_exact_compute(point_b, point_c, point_d, point_e, minor, minor_buff, determinant_minor, determinant_buff, determinant_term);
    for (idx = 0; idx < minor_len; idx++) { minor[idx] = -minor[idx]; }
    predicate_two_difference_compute(point_a[0], point_e[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_a[1], point_e[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_a[2], point_e[2], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    minor_len = predicate_orient3d_exact_compute(point_a, point_c, point_d, point_e, minor, minor_buff, determinant_minor, determinant_buff, determinant_term);
    predicate_two_difference_compute(point_b[0], point_e[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_b[1], point_e[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_b[2], point_e[2], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    minor_len = predicate_orient3d_exact_compute(point_a, point_b, point_d, point_e, minor, minor_buff, determinant_minor, determinant_buff, determinant_term);
    for (idx = 0; idx < minor_len; idx++) { minor[idx] = -minor[idx]; }
    predicate_two_difference_compute(point_c[0], point_e[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_c[1], point_e[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_c[2], point_e[2], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    minor_len = predicate_orient3d_exact_compute(point_a, point_b, point_c, point_e, minor, minor_buff, determinant_minor, determinant_buff, determinant_term);
    predicate_two_difference_compute(point_d[0], point_e[0], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_d[1], point_e[1], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    predicate_two_difference_compute(point_d[2], point_e[2], &difference_high, &difference_low);
    axis_len = predicate_axis_lift(minor_len, minor, difference_high, difference_low, axis, axis_buff, scaled_a, scaled_b, lift_term);
    predicate_cum(total, &total_len, axis, axis_len, total_buff);
    if (total_len == 0)
    {
        *out = LDG_MATH_SIGN_ZERO;
        return LDG_ERR_AOK;
    }

    *out = predicate_sign_get(total[total_len - 1]);
    return LDG_ERR_AOK;
}

static uint8_t predicate_sign_flip(uint8_t sign)
{
    if (sign == LDG_MATH_SIGN_POS) { return LDG_MATH_SIGN_NEG; }

    if (sign == LDG_MATH_SIGN_NEG) { return LDG_MATH_SIGN_POS; }

    return LDG_MATH_SIGN_ZERO;
}

static uint8_t predicate_parity_apply(uint8_t sign, uint32_t parity)
{
    if ((parity & 1) != 0) { return predicate_sign_flip(sign); }

    return sign;
}

static uint8_t predicate_permutation3_sign_get(uint32_t coord_a, uint32_t coord_b, uint32_t coord_c)
{
    if (coord_a == 0 && coord_b == 1 && coord_c == 2) { return LDG_MATH_SIGN_POS; }

    if (coord_a == 1 && coord_b == 2 && coord_c == 0) { return LDG_MATH_SIGN_POS; }

    if (coord_a == 2 && coord_b == 0 && coord_c == 1) { return LDG_MATH_SIGN_POS; }

    return LDG_MATH_SIGN_NEG;
}

static uint8_t predicate_orient2d_proj_sign_get(const double *point_p, const double *point_q, const double *point_r, uint32_t axis_a, uint32_t axis_b)
{
    double proj_p[2] = LDG_ARR_ZERO_INIT;
    double proj_q[2] = LDG_ARR_ZERO_INIT;
    double proj_r[2] = LDG_ARR_ZERO_INIT;
    double expansion[PREDICATE_ORIENT2_BUFF] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0x0;
    proj_p[0] = point_p[axis_a];
    proj_p[1] = point_p[axis_b];
    proj_q[0] = point_q[axis_a];
    proj_q[1] = point_q[axis_b];
    proj_r[0] = point_r[axis_a];
    proj_r[1] = point_r[axis_b];
    len = predicate_orient2d_exact_compute(proj_p, proj_q, proj_r, expansion);
    if (len == 0) { return LDG_MATH_SIGN_ZERO; }

    return predicate_sign_get(expansion[len - 1]);
}

static uint8_t predicate_perturb_term1_eval(const double **corner, uint32_t row, uint32_t coord)
{
    const double *rest[3] = LDG_ARR_ZERO_INIT;
    uint32_t axis_a = 0x0;
    uint32_t axis_b = 0x0;
    uint32_t fill = 0x0;
    uint32_t idx = 0x0;
    uint8_t sign = 0x0;
    for (idx = 0; idx < 4; idx++) { if (idx != row)
        {
            rest[fill] = corner[idx];
            fill++;
        }
    }
    if (coord == 0)
    {
        axis_a = 1;
        axis_b = 2;
    }
    else if (coord == 1)
    {
        axis_a = 0;
        axis_b = 2;
    }
    else
    {
        axis_a = 0;
        axis_b = 1;
    }

    sign = predicate_orient2d_proj_sign_get(rest[0], rest[1], rest[2], axis_a, axis_b);
    return predicate_parity_apply(sign, row + coord);
}

static uint8_t predicate_perturb_term2_eval(const double **corner, uint32_t row_a, uint32_t row_b, uint32_t coord_a, uint32_t coord_b)
{
    const double *rest[2] = LDG_ARR_ZERO_INIT;
    uint32_t fill = 0x0;
    uint32_t idx = 0x0;
    uint32_t free_coord = 0x0;
    uint32_t parity = 0x0;
    uint8_t sign = 0x0;
    for (idx = 0; idx < 4; idx++) { if (idx != row_a && idx != row_b)
        {
            rest[fill] = corner[idx];
            fill++;
        }
    }
    free_coord = 3 - coord_a - coord_b;
    if (rest[0][free_coord] > rest[1][free_coord]) { sign = LDG_MATH_SIGN_POS; }
    else if (rest[0][free_coord] < rest[1][free_coord]) { sign = LDG_MATH_SIGN_NEG; }
    else { return LDG_MATH_SIGN_ZERO; }

    parity = row_a + row_b + coord_a + coord_b;
    if (coord_a > coord_b) { parity++; }

    return predicate_parity_apply(sign, parity);
}

uint32_t ldg_math_3d_orient_perturb(const double *vert, uint32_t idx_a, uint32_t idx_b, uint32_t idx_c, uint32_t idx_d, ldg_mem_pool_t *pool, uint8_t *out)
{
    const double *corner[4] = LDG_ARR_ZERO_INIT;
    uint32_t vert_idx[4] = LDG_ARR_ZERO_INIT;
    uint32_t rank[4] = LDG_ARR_ZERO_INIT;
    uint32_t rest[3] = LDG_ARR_ZERO_INIT;
    uint8_t base = 0x0;
    uint8_t best_sign = 0x0;
    uint8_t term_sign = 0x0;
    uint8_t permutation_sign = 0x0;
    uint32_t best_exponent = 0x0;
    uint32_t exponent = 0x0;
    uint32_t err = 0x0;
    uint32_t row = 0x0;
    uint32_t row_a = 0x0;
    uint32_t row_b = 0x0;
    uint32_t coord = 0x0;
    uint32_t coord_a = 0x0;
    uint32_t coord_b = 0x0;
    uint32_t coord_c = 0x0;
    uint32_t idx = 0x0;
    uint32_t fill = 0x0;
    uint32_t keep_row = 0x0;
    if (LDG_UNLIKELY(vert == 0x0 || pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_SIGN_ZERO;
    vert_idx[0] = idx_a;
    vert_idx[1] = idx_b;
    vert_idx[2] = idx_c;
    vert_idx[3] = idx_d;
    corner[0] = vert + 3 * idx_a;
    corner[1] = vert + 3 * idx_b;
    corner[2] = vert + 3 * idx_c;
    corner[3] = vert + 3 * idx_d;
    err = ldg_math_3d_orient(corner[0], corner[1], corner[2], corner[3], pool, &base);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (base != LDG_MATH_SIGN_ZERO)
    {
        *out = base;
        return LDG_ERR_AOK;
    }

    for (row = 0; row < 4; row++) { for (idx = 0; idx < 4; idx++) { if (vert_idx[idx] < vert_idx[row]) { rank[row]++; } } }
    best_exponent = UINT32_MAX;
    best_sign = LDG_MATH_SIGN_ZERO;
    for (row = 0; row < 4; row++)
    {
        for (coord = 0; coord < 3; coord++)
        {
            exponent = 1 << (3 * rank[row] + coord);
            if (exponent >= best_exponent) { continue; }

            term_sign = predicate_perturb_term1_eval(corner, row, coord);
            if (term_sign != LDG_MATH_SIGN_ZERO)
            {
                best_exponent = exponent;
                best_sign = term_sign;
            }
        }
    }
    for (row_a = 0; row_a < 4; row_a++)
    {
        for (row_b = row_a + 1; row_b < 4; row_b++)
        {
            for (coord_a = 0; coord_a < 3; coord_a++)
            {
                for (coord_b = 0; coord_b < 3; coord_b++)
                {
                    if (coord_a == coord_b) { continue; }

                    exponent = (uint32_t)((1 << (3 * rank[row_a] + coord_a)) + (1 << (3 * rank[row_b] + coord_b)));
                    if (exponent >= best_exponent) { continue; }

                    term_sign = predicate_perturb_term2_eval(corner, row_a, row_b, coord_a, coord_b);
                    if (term_sign != LDG_MATH_SIGN_ZERO)
                    {
                        best_exponent = exponent;
                        best_sign = term_sign;
                    }
                }
            }
        }
    }
    for (keep_row = 0; keep_row < 4; keep_row++)
    {
        fill = 0;
        for (idx = 0; idx < 4; idx++) { if (idx != keep_row)
            {
                rest[fill] = idx;
                fill++;
            }
        }
        for (coord_a = 0; coord_a < 3; coord_a++)
        {
            for (coord_b = 0; coord_b < 3; coord_b++)
            {
                for (coord_c = 0; coord_c < 3; coord_c++)
                {
                    if (coord_a == coord_b || coord_a == coord_c || coord_b == coord_c) { continue; }

                    exponent = (uint32_t)((1 << (3 * rank[rest[0]] + coord_a)) + (1 << (3 * rank[rest[1]] + coord_b)) + (1 << (3 * rank[rest[2]] + coord_c)));
                    if (exponent >= best_exponent) { continue; }

                    permutation_sign = predicate_permutation3_sign_get(coord_a, coord_b, coord_c);
                    term_sign = predicate_parity_apply(permutation_sign, keep_row + 3);
                    best_exponent = exponent;
                    best_sign = term_sign;
                }
            }
        }
    }
    *out = best_sign;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_3d_insphere_perturb(const double *vert, uint32_t idx_a, uint32_t idx_b, uint32_t idx_c, uint32_t idx_d, uint32_t idx_e, ldg_mem_pool_t *pool, uint8_t *out)
{
    const double *corner[5] = LDG_ARR_ZERO_INIT;
    uint32_t vert_idx[5] = LDG_ARR_ZERO_INIT;
    uint32_t rest[4] = LDG_ARR_ZERO_INIT;
    uint8_t base = 0x0;
    uint8_t orient = 0x0;
    uint32_t min_pos = 0x0;
    uint32_t fill = 0x0;
    uint32_t idx = 0x0;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(vert == 0x0 || pool == 0x0 || out == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_SIGN_ZERO;
    vert_idx[0] = idx_a;
    vert_idx[1] = idx_b;
    vert_idx[2] = idx_c;
    vert_idx[3] = idx_d;
    vert_idx[4] = idx_e;
    corner[0] = vert + 3 * idx_a;
    corner[1] = vert + 3 * idx_b;
    corner[2] = vert + 3 * idx_c;
    corner[3] = vert + 3 * idx_d;
    corner[4] = vert + 3 * idx_e;
    err = ldg_math_3d_insphere(corner[0], corner[1], corner[2], corner[3], corner[4], pool, &base);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (base != LDG_MATH_SIGN_ZERO)
    {
        *out = base;
        return LDG_ERR_AOK;
    }

    min_pos = 0;
    for (idx = 1; idx < 5; idx++) { if (vert_idx[idx] < vert_idx[min_pos]) { min_pos = idx; } }
    fill = 0;
    for (idx = 0; idx < 5; idx++) { if (idx != min_pos)
        {
            rest[fill] = vert_idx[idx];
            fill++;
        }
    }
    err = ldg_math_3d_orient_perturb(vert, rest[0], rest[1], rest[2], rest[3], pool, &orient);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out = predicate_parity_apply(orient, min_pos);
    return LDG_ERR_AOK;
}
