#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/arch/dangcsa/trace.h>

#define TRACE_TOK_OPCODE 0
#define TRACE_TOK_DST 1
#define TRACE_TOK_SRC 2
#define TRACE_TOK_FLAG 3
#define TRACE_TOK_IMM_B3 4
#define TRACE_TOK_IMM_B2 5
#define TRACE_TOK_IMM_B1 6
#define TRACE_TOK_IMM_B0 7
#define TRACE_TOK_REG 0
#define TRACE_TOK_VAL_B7 1
#define TRACE_TOK_VAL_B6 2
#define TRACE_TOK_VAL_B5 3
#define TRACE_TOK_VAL_B4 4
#define TRACE_TOK_VAL_B3 5
#define TRACE_TOK_VAL_B2 6
#define TRACE_TOK_VAL_B1 7
#define TRACE_TOK_VAL_B0 8
#define TRACE_TOK_CARRY 9
#define TRACE_TOK_EXC 10
#define TRACE_DRAW_DST_SHIFT 8
#define TRACE_DRAW_SRC_SHIFT 16
#define TRACE_DRAW_F0_BIT 0
#define TRACE_DRAW_F1_BIT 1
#define TRACE_DRAW_F2_BIT 2
#define TRACE_DRAW_F3_BIT 3
#define TRACE_DRAW_WIDTH_SHIFT 4
#define TRACE_DRAW_QUARTER_SHIFT 4
#define TRACE_DRAW_COND_SHIFT 8
#define TRACE_DRAW_AMT_SHIFT 8
#define TRACE_BYTE_SHIFT_7 56
#define TRACE_BYTE_SHIFT_6 48
#define TRACE_BYTE_SHIFT_5 40
#define TRACE_BYTE_SHIFT_4 32
#define TRACE_DISP_SPAN 256
#define TRACE_DISP_HALF 128
#define TRACE_BRANCH_SPAN 16
#define TRACE_BRANCH_HALF 8
#define TRACE_MASK_OFF 0.0
#define TRACE_MASK_ON 1.0
#define TRACE_FALLBACK_OP LDG_DANGCSA_OP_AND

static uint32_t trace_draw(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    return (uint32_t)rng_cb(rng_ctx);
}

static uint8_t trace_draw_bit_is(uint32_t draw, uint32_t bit)
{
    if ((draw & LDG_BIT(bit)) != LDG_TRUTH_FALSE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint64_t trace_width_bytes_get(uint8_t width)
{
    return (uint64_t)LDG_BIT(width);
}

static uint8_t trace_ram_is(const ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint64_t size)
{
    if (addr <= dlu->conf.ram_size && size <= dlu->conf.ram_size - addr) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t trace_aligned_is(uint64_t addr, uint8_t width)
{
    if ((addr & (trace_width_bytes_get(width) - (uint64_t)LDG_TRUTH_TRUE)) == LDG_TRUTH_FALSE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint64_t trace_sext32_get(uint32_t val)
{
    return (uint64_t)(int64_t)(int32_t)val;
}

static uint32_t trace_disp_draw(uint32_t draw)
{
    return (uint32_t)((int32_t)(draw % TRACE_DISP_SPAN) - TRACE_DISP_HALF);
}

static uint64_t trace_conf_draw(const ldg_dangcsa_dlu_t *dlu, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, uint32_t *opcode, uint32_t dst, uint32_t src)
{
    uint64_t conf = LDG_DANGCSA_CONF_NONE;
    uint32_t draw = trace_draw(rng_cb, rng_ctx);
    uint32_t imm = trace_draw(rng_cb, rng_ctx);
    uint32_t disp = trace_disp_draw(imm);
    uint8_t width = (uint8_t)((draw >> TRACE_DRAW_WIDTH_SHIFT) & LDG_DANGCSA_WIDTH_FIELD_MASK);
    uint8_t quarter = (uint8_t)((draw >> TRACE_DRAW_QUARTER_SHIFT) & LDG_DANGCSA_QUARTER_FIELD_MASK);
    uint8_t cond = (uint8_t)((draw >> TRACE_DRAW_COND_SHIFT) % LDG_DANGCSA_COND_CUNT);
    uint8_t amt = (uint8_t)((draw >> TRACE_DRAW_AMT_SHIFT) & LDG_DANGCSA_SHIFT_AMT_MASK);
    uint64_t addr = 0;
    uint64_t sp = 0;
    int32_t off = 0;

    switch (*opcode)
    {
        case LDG_DANGCSA_OP_CPY:
            if (trace_draw_bit_is(draw, TRACE_DRAW_F0_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_CPY_IMM | (uint64_t)imm; }

            if (trace_draw_bit_is(draw, TRACE_DRAW_F1_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_CPY_PARTIAL | ((uint64_t)quarter << LDG_DANGCSA_QUARTER_SHIFT); }

            break;

        case LDG_DANGCSA_OP_LD:
            addr = dlu->reg[src] + trace_sext32_get(disp);
            if (trace_ram_is(dlu, addr, trace_width_bytes_get(width)) != LDG_TRUTH_TRUE || trace_aligned_is(addr, width) != LDG_TRUTH_TRUE)
            {
                *opcode = TRACE_FALLBACK_OP;
                break;
            }

            conf = ((uint64_t)width << LDG_DANGCSA_WIDTH_SHIFT) | (uint64_t)disp;
            if (trace_draw_bit_is(draw, TRACE_DRAW_F2_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_LD_SIGNED; }

            break;

        case LDG_DANGCSA_OP_ST:
            addr = dlu->reg[dst] + trace_sext32_get(disp);
            if (trace_ram_is(dlu, addr, trace_width_bytes_get(width)) != LDG_TRUTH_TRUE || trace_aligned_is(addr, width) != LDG_TRUTH_TRUE)
            {
                *opcode = TRACE_FALLBACK_OP;
                break;
            }

            conf = ((uint64_t)width << LDG_DANGCSA_WIDTH_SHIFT) | (uint64_t)disp;
            break;

        case LDG_DANGCSA_OP_ADD:
        case LDG_DANGCSA_OP_SUB:
            if (trace_draw_bit_is(draw, TRACE_DRAW_F0_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_ADD_CARRY; }

            if (trace_draw_bit_is(draw, TRACE_DRAW_F1_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_ADD_IMM | (uint64_t)imm; }

            break;

        case LDG_DANGCSA_OP_SH:
            if (trace_draw_bit_is(draw, TRACE_DRAW_F0_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_SH_SHR; }

            if (trace_draw_bit_is(draw, TRACE_DRAW_F1_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_SH_ARITH; }

            if (trace_draw_bit_is(draw, TRACE_DRAW_F2_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_SH_IMM | (uint64_t)amt; }

            break;

        case LDG_DANGCSA_OP_MUL:
            if (trace_draw_bit_is(draw, TRACE_DRAW_F0_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_MUL_HIGH; }

            if (trace_draw_bit_is(draw, TRACE_DRAW_F1_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_MUL_SIGNED; }

            break;

        case LDG_DANGCSA_OP_DIV:
            if (trace_draw_bit_is(draw, TRACE_DRAW_F0_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_DIV_SIGNED; }

            if (trace_draw_bit_is(draw, TRACE_DRAW_F1_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_DIV_REMAINDER; }

            break;

        case LDG_DANGCSA_OP_CLZ:
            if (trace_draw_bit_is(draw, TRACE_DRAW_F0_BIT) == LDG_TRUTH_TRUE) { conf = conf | LDG_DANGCSA_CONF_CLZ_TRAILING; }

            break;

        case LDG_DANGCSA_OP_JMP:
            conf = (uint64_t)cond << LDG_DANGCSA_COND_SHIFT;
            if (trace_draw_bit_is(draw, TRACE_DRAW_F3_BIT) == LDG_TRUTH_TRUE)
            {
                off = (int32_t)(imm % TRACE_BRANCH_SPAN) - TRACE_BRANCH_HALF;
                addr = dlu->reg[LDG_DANGCSA_LOC] + trace_sext32_get((uint32_t)off) * LDG_DANGCSA_INSTR_SIZE;
                if (trace_ram_is(dlu, addr, LDG_DANGCSA_INSTR_SIZE) != LDG_TRUTH_TRUE)
                {
                    *opcode = TRACE_FALLBACK_OP;
                    conf = LDG_DANGCSA_CONF_NONE;
                    break;
                }

                conf = conf | LDG_DANGCSA_CONF_JMP_RELATIVE | (uint64_t)(uint32_t)off;
            }

            break;

        case LDG_DANGCSA_OP_SETCC:
            conf = (uint64_t)cond << LDG_DANGCSA_COND_SHIFT;
            break;

        case LDG_DANGCSA_OP_CALL:
            sp = dlu->reg[LDG_DANGCSA_SP];
            if (sp >= LDG_DANGCSA_STACK_SLOT && trace_ram_is(dlu, sp - LDG_DANGCSA_STACK_SLOT, LDG_DANGCSA_STACK_SLOT) != LDG_TRUTH_TRUE) { *opcode = TRACE_FALLBACK_OP; }

            break;

        case LDG_DANGCSA_OP_RET:
            sp = dlu->reg[LDG_DANGCSA_SP];
            if (sp <= LDG_DANGCSA_DLU_STACK_TOP && trace_ram_is(dlu, sp, LDG_DANGCSA_STACK_SLOT) != LDG_TRUTH_TRUE) { *opcode = TRACE_FALLBACK_OP; }

            break;

        default:
            break;
    }

    return conf;
}

uint32_t ldg_dangcsa_trace_instr_draw(const ldg_dangcsa_dlu_t *dlu, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, ldg_qword_t *out)
{
    uint32_t draw = 0;
    uint32_t opcode = 0;
    uint32_t dst = 0;
    uint32_t src = 0;
    uint64_t conf = 0;

    if (LDG_UNLIKELY(!dlu || !rng_cb || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_DANGCSA_CONF_NONE;

    if (LDG_UNLIKELY(dlu->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    draw = trace_draw(rng_cb, rng_ctx);
    opcode = draw % LDG_DANGCSA_OP_CUNT;
    dst = (draw >> TRACE_DRAW_DST_SHIFT) % LDG_DANGCSA_REG_CUNT;
    src = (draw >> TRACE_DRAW_SRC_SHIFT) % LDG_DANGCSA_REG_CUNT;

    if (ldg_dangcsa_dst_writeback_is((uint8_t)opcode) == LDG_TRUTH_TRUE && dst == LDG_DANGCSA_LOC) { dst = LDG_DANGCSA_DR0; }

    conf = trace_conf_draw(dlu, rng_cb, rng_ctx, &opcode, dst, src);

    *out = ldg_dangcsa_encode((uint8_t)opcode, (uint8_t)dst, (uint8_t)src, conf);

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_trace_instr_encode(ldg_qword_t raw, uint32_t *dst, uint32_t cap)
{
    uint32_t imm = ldg_dangcsa_imm32_get(raw);

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < LDG_DANGCSA_TRACE_INSTR_TOK_CUNT)) { return LDG_ERR_BOUNDS; }

    dst[TRACE_TOK_OPCODE] = ldg_dangcsa_opcode_get(raw);
    dst[TRACE_TOK_DST] = ldg_dangcsa_dst_get(raw);
    dst[TRACE_TOK_SRC] = ldg_dangcsa_src_get(raw);
    dst[TRACE_TOK_FLAG] = ldg_dangcsa_flag_get(raw);
    dst[TRACE_TOK_IMM_B3] = (imm >> LDG_BYTE_SHIFT_3) & LDG_BYTE_MASK;
    dst[TRACE_TOK_IMM_B2] = (imm >> LDG_BYTE_SHIFT_2) & LDG_BYTE_MASK;
    dst[TRACE_TOK_IMM_B1] = (imm >> LDG_BYTE_SHIFT_1) & LDG_BYTE_MASK;
    dst[TRACE_TOK_IMM_B0] = imm & LDG_BYTE_MASK;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_trace_resp_encode(const ldg_dangcsa_dlu_t *dlu, uint32_t *dst, uint32_t cap)
{
    uint64_t val = 0;

    if (LDG_UNLIKELY(!dlu || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dlu->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(cap < LDG_DANGCSA_TRACE_RESP_TOK_CUNT)) { return LDG_ERR_BOUNDS; }

    val = dlu->last_val;

    dst[TRACE_TOK_REG] = dlu->last_reg;
    dst[TRACE_TOK_VAL_B7] = (uint32_t)((val >> TRACE_BYTE_SHIFT_7) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B6] = (uint32_t)((val >> TRACE_BYTE_SHIFT_6) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B5] = (uint32_t)((val >> TRACE_BYTE_SHIFT_5) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B4] = (uint32_t)((val >> TRACE_BYTE_SHIFT_4) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B3] = (uint32_t)((val >> LDG_BYTE_SHIFT_3) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B2] = (uint32_t)((val >> LDG_BYTE_SHIFT_2) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B1] = (uint32_t)((val >> LDG_BYTE_SHIFT_1) & LDG_BYTE_MASK);
    dst[TRACE_TOK_VAL_B0] = (uint32_t)(val & LDG_BYTE_MASK);
    dst[TRACE_TOK_CARRY] = dlu->carry;
    dst[TRACE_TOK_EXC] = dlu->last_exc;
    if (dlu->fault != LDG_DANGCSA_DLU_FAULT_NONE) { dst[TRACE_TOK_EXC] = LDG_DANGCSA_TRACE_EXC_FAULT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_trace_sample_fill(ldg_dangcsa_dlu_t *dlu, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, uint32_t instr_cunt, double *tok_in, double *tgt, double *mask, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t tok[LDG_DANGCSA_TRACE_RESP_TOK_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_qword_t raw = 0;
    uint32_t instr = 0;
    uint32_t idx = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dlu || !rng_cb || !tok_in || !tgt || !mask || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(dlu->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(instr_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < instr_cunt * LDG_DANGCSA_TRACE_TOK_CUNT)) { return LDG_ERR_BOUNDS; }

    while (instr < instr_cunt)
    {
        ret = ldg_dangcsa_trace_instr_draw(dlu, rng_cb, rng_ctx, &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_dangcsa_trace_instr_encode(raw, tok, LDG_DANGCSA_TRACE_RESP_TOK_CUNT);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx = 0;
        while (idx < LDG_DANGCSA_TRACE_INSTR_TOK_CUNT)
        {
            tok_in[pos] = (double)tok[idx];
            tgt[pos] = (double)tok[idx];
            mask[pos] = TRACE_MASK_OFF;
            pos++;
            idx++;
        }

        ret = ldg_dangcsa_dlu_exec(dlu, raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_dangcsa_trace_resp_encode(dlu, tok, LDG_DANGCSA_TRACE_RESP_TOK_CUNT);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx = 0;
        while (idx < LDG_DANGCSA_TRACE_RESP_TOK_CUNT)
        {
            tok_in[pos] = (double)LDG_DANGCSA_TRACE_TOK_MASK;
            tgt[pos] = (double)tok[idx];
            mask[pos] = TRACE_MASK_ON;
            pos++;
            idx++;
        }

        if (dlu->hlt_is == LDG_TRUTH_TRUE)
        {
            ret = ldg_dangcsa_dlu_rst(dlu);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        instr++;
    }

    *out_cunt = pos;

    return LDG_ERR_AOK;
}
