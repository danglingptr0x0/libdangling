#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/arch/dangcsa/dlu.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define DLU_OFF_SW 0x90000000ULL
#define DLU_OFF_LED 0x90000008ULL
#define DLU_OFF_SERIAL_TX 0x9000000CULL
#define DLU_OFF_TIMER0_TGT 0x90000020ULL
#define DLU_OFF_TIMER0_ONESHOT 0x90000028ULL
#define DLU_OFF_TIMER1_TGT 0x90000030ULL
#define DLU_OFF_TIMER1_ONESHOT 0x90000038ULL
#define DLU_OFF_SERIAL_READY 0x90000040ULL
#define DLU_QBIT(pos) (1ULL << (pos))
#define DLU_SIGN_BIT 63
#define DLU_SERIAL_READY 1
#define DLU_TIMER0 0
#define DLU_TIMER1 1
#define DLU_LED_MASK 0xFFFFU
#define DLU_ONESHOT_MASK 0x1U
#define DLU_IO_REG_BYTES 4
#define DLU_IO_REG_BITS 32
#define DLU_IO_REG_ALIGN_MASK 0x3ULL
#define DLU_BYTE_BYTES 1
#define DLU_WORD_BYTES 2
#define DLU_DWORD_BYTES 4
#define DLU_QWORD_BYTES 8
#define DLU_WORD_ALIGN_MASK 0x1ULL
#define DLU_DWORD_ALIGN_MASK 0x3ULL
#define DLU_QWORD_ALIGN_MASK 0x7ULL
#define DLU_INSTR_ALIGN_MASK 0x7ULL
#define DLU_WORD_VAL_MASK 0xFFFFULL
#define DLU_DWORD_VAL_MASK 0xFFFFFFFFULL
#define DLU_LIMB_BITS 32
#define DLU_LIMB_MASK 0xFFFFFFFFULL
#define DLU_CLEAR 0
#define DLU_EMPTY 0

static uint32_t dlu_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void dlu_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static uint32_t dlu_init_ck(const ldg_dangcsa_dlu_t *dlu)
{
    if (LDG_UNLIKELY(!dlu)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dlu->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint64_t dlu_width_bytes_get(uint8_t width)
{
    switch (width)
    {
        case LDG_DANGCSA_WIDTH_BYTE:
            return DLU_BYTE_BYTES;

        case LDG_DANGCSA_WIDTH_WORD:
            return DLU_WORD_BYTES;

        case LDG_DANGCSA_WIDTH_DWORD:
            return DLU_DWORD_BYTES;

        default:
            return DLU_QWORD_BYTES;
    }
}

static uint64_t dlu_width_align_mask_get(uint8_t width)
{
    switch (width)
    {
        case LDG_DANGCSA_WIDTH_BYTE:
            return DLU_CLEAR;

        case LDG_DANGCSA_WIDTH_WORD:
            return DLU_WORD_ALIGN_MASK;

        case LDG_DANGCSA_WIDTH_DWORD:
            return DLU_DWORD_ALIGN_MASK;

        default:
            return DLU_QWORD_ALIGN_MASK;
    }
}

static uint64_t dlu_width_val_mask_get(uint8_t width)
{
    switch (width)
    {
        case LDG_DANGCSA_WIDTH_BYTE:
            return LDG_BYTE_MASK;

        case LDG_DANGCSA_WIDTH_WORD:
            return DLU_WORD_VAL_MASK;

        case LDG_DANGCSA_WIDTH_DWORD:
            return DLU_DWORD_VAL_MASK;

        default:
            return UINT64_MAX;
    }
}

static uint64_t dlu_sext_get(uint64_t val, uint8_t width)
{
    switch (width)
    {
        case LDG_DANGCSA_WIDTH_BYTE:
            return (uint64_t)(int64_t)(int8_t)(uint8_t)val;

        case LDG_DANGCSA_WIDTH_WORD:
            return (uint64_t)(int64_t)(int16_t)(uint16_t)val;

        case LDG_DANGCSA_WIDTH_DWORD:
            return (uint64_t)(int64_t)(int32_t)(uint32_t)val;

        default:
            return val;
    }
}

static uint64_t dlu_sext32_get(uint32_t val)
{
    return (uint64_t)(int64_t)(int32_t)val;
}

static uint64_t dlu_reg_rd(const ldg_dangcsa_dlu_t *dlu, uint32_t idx)
{
    return dlu->reg[idx];
}

static void dlu_reg_wr(ldg_dangcsa_dlu_t *dlu, uint32_t idx, uint64_t val)
{
    dlu->reg[idx] = val;
    dlu->last_reg = idx;
    dlu->last_val = val;
}

static void dlu_loc_advance(ldg_dangcsa_dlu_t *dlu)
{
    dlu->reg[LDG_DANGCSA_LOC] = dlu->reg[LDG_DANGCSA_LOC] + LDG_DANGCSA_INSTR_SIZE;
}

static void dlu_loc_jmp(ldg_dangcsa_dlu_t *dlu, uint64_t loc)
{
    dlu->reg[LDG_DANGCSA_LOC] = loc;
    dlu->last_reg = LDG_DANGCSA_LOC;
    dlu->last_val = loc;
}

static uint32_t dlu_ram_rd(ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint64_t size, uint64_t *out)
{
    const uint8_t *lane = 0x0;
    uint64_t idx = 0;
    uint64_t val = 0;

    *out = DLU_CLEAR;

    if (LDG_UNLIKELY(addr > dlu->conf.ram_size || size > dlu->conf.ram_size - addr))
    {
        dlu->fault = LDG_ERR_BOUNDS;
        dlu->hlt_is = LDG_TRUTH_TRUE;
        return LDG_ERR_BOUNDS;
    }

    lane = dlu->ram + addr;
    while (idx < size)
    {
        val = val | ((uint64_t)lane[idx] << (idx * LDG_BYTE_BITS));
        idx++;
    }

    *out = val;

    return LDG_ERR_AOK;
}

static uint32_t dlu_ram_wr(ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint64_t size, uint64_t val)
{
    uint8_t *lane = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(addr > dlu->conf.ram_size || size > dlu->conf.ram_size - addr))
    {
        dlu->fault = LDG_ERR_BOUNDS;
        dlu->hlt_is = LDG_TRUTH_TRUE;
        return LDG_ERR_BOUNDS;
    }

    lane = dlu->ram + addr;
    while (idx < size)
    {
        lane[idx] = (uint8_t)((val >> (idx * LDG_BYTE_BITS)) & LDG_BYTE_MASK);
        idx++;
    }

    return LDG_ERR_AOK;
}

static uint8_t dlu_io_builtin_is(uint64_t off)
{
    if (off >= DLU_OFF_SW && off <= DLU_OFF_SERIAL_READY) { return LDG_TRUTH_TRUE; }

    if (off == LDG_DANGCSA_IO_EXC_HANDLER_LO_OFF || off == LDG_DANGCSA_IO_EXC_HANDLER_HI_OFF || off == LDG_DANGCSA_IO_EXC_CAUSE_OFF) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t dlu_io_reg_rd(const ldg_dangcsa_dlu_t *dlu, uint64_t off)
{
    if (off == LDG_DANGCSA_IO_EXC_HANDLER_LO_OFF) { return (uint32_t)(dlu->exc_handler & DLU_DWORD_VAL_MASK); }

    if (off == LDG_DANGCSA_IO_EXC_HANDLER_HI_OFF) { return (uint32_t)(dlu->exc_handler >> DLU_IO_REG_BITS); }

    if (off == LDG_DANGCSA_IO_EXC_CAUSE_OFF) { return (uint32_t)(dlu->exc_cause & DLU_DWORD_VAL_MASK); }

    if (off == DLU_OFF_LED) { return dlu->led; }

    if (off == DLU_OFF_SERIAL_READY) { return DLU_SERIAL_READY; }

    if (off == DLU_OFF_TIMER0_TGT) { return dlu->timer_tgt[DLU_TIMER0]; }

    if (off == DLU_OFF_TIMER0_ONESHOT) { return dlu->timer_oneshot[DLU_TIMER0]; }

    if (off == DLU_OFF_TIMER1_TGT) { return dlu->timer_tgt[DLU_TIMER1]; }

    if (off == DLU_OFF_TIMER1_ONESHOT) { return dlu->timer_oneshot[DLU_TIMER1]; }

    return DLU_CLEAR;
}

static void dlu_io_reg_wr(ldg_dangcsa_dlu_t *dlu, uint64_t off, uint32_t val)
{
    if (off == LDG_DANGCSA_IO_EXC_HANDLER_LO_OFF) { dlu->exc_handler = (dlu->exc_handler & ~DLU_DWORD_VAL_MASK) | (uint64_t)val; }

    if (off == LDG_DANGCSA_IO_EXC_HANDLER_HI_OFF) { dlu->exc_handler = (dlu->exc_handler & DLU_DWORD_VAL_MASK) | ((uint64_t)val << DLU_IO_REG_BITS); }

    if (off == LDG_DANGCSA_IO_EXC_CAUSE_OFF) { dlu->exc_cause = val; }

    if (off == DLU_OFF_LED) { dlu->led = val & DLU_LED_MASK; }

    if (off == DLU_OFF_SERIAL_TX && dlu->tx_cunt < dlu->conf.tx_cap)
    {
        dlu->tx[dlu->tx_cunt] = (uint8_t)(val & LDG_BYTE_MASK);
        dlu->tx_cunt++;
    }

    if (off == DLU_OFF_TIMER0_TGT) { dlu->timer_tgt[DLU_TIMER0] = val; }

    if (off == DLU_OFF_TIMER0_ONESHOT) { dlu->timer_oneshot[DLU_TIMER0] = val & DLU_ONESHOT_MASK; }

    if (off == DLU_OFF_TIMER1_TGT) { dlu->timer_tgt[DLU_TIMER1] = val; }

    if (off == DLU_OFF_TIMER1_ONESHOT) { dlu->timer_oneshot[DLU_TIMER1] = val & DLU_ONESHOT_MASK; }
}

static uint32_t dlu_io_rd(ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint8_t width, uint64_t *out)
{
    uint64_t sel = addr >> LDG_DANGCSA_IO_SEL_SHIFT;
    uint64_t off = addr & LDG_DANGCSA_IO_OFF_MASK;
    uint64_t base = off & ~DLU_IO_REG_ALIGN_MASK;
    uint64_t lane = off & DLU_IO_REG_ALIGN_MASK;
    uint64_t val = 0;

    *out = DLU_CLEAR;

    if (sel != LDG_DANGCSA_IO_PAGE_DEV || dlu_io_builtin_is(base) != LDG_TRUTH_TRUE)
    {
        if (dlu->dev_rd_cb) { return dlu->dev_rd_cb(dlu->dev_ctx, addr, width, out); }

        return LDG_ERR_AOK;
    }

    val = dlu_io_reg_rd(dlu, base);
    if (width == LDG_DANGCSA_WIDTH_QWORD) { val = val | ((uint64_t)dlu_io_reg_rd(dlu, base + DLU_IO_REG_BYTES) << DLU_IO_REG_BITS); }

    *out = (val >> (lane * LDG_BYTE_BITS)) & dlu_width_val_mask_get(width);

    return LDG_ERR_AOK;
}

static uint32_t dlu_io_wr(ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint8_t width, uint64_t val)
{
    uint64_t sel = addr >> LDG_DANGCSA_IO_SEL_SHIFT;
    uint64_t off = addr & LDG_DANGCSA_IO_OFF_MASK;
    uint64_t base = off & ~DLU_IO_REG_ALIGN_MASK;
    uint64_t lane = off & DLU_IO_REG_ALIGN_MASK;
    uint64_t shift = lane * LDG_BYTE_BITS;
    uint64_t mask = dlu_width_val_mask_get(width) << shift;
    uint64_t curr = 0;
    uint64_t merged = 0;

    if (sel != LDG_DANGCSA_IO_PAGE_DEV || dlu_io_builtin_is(base) != LDG_TRUTH_TRUE)
    {
        if (dlu->dev_wr_cb) { return dlu->dev_wr_cb(dlu->dev_ctx, addr, width, val); }

        return LDG_ERR_AOK;
    }

    curr = dlu_io_reg_rd(dlu, base);
    if (width == LDG_DANGCSA_WIDTH_QWORD) { curr = curr | ((uint64_t)dlu_io_reg_rd(dlu, base + DLU_IO_REG_BYTES) << DLU_IO_REG_BITS); }

    merged = (curr & ~mask) | ((val << shift) & mask);
    dlu_io_reg_wr(dlu, base, (uint32_t)(merged & DLU_DWORD_VAL_MASK));
    if (width == LDG_DANGCSA_WIDTH_QWORD) { dlu_io_reg_wr(dlu, base + DLU_IO_REG_BYTES, (uint32_t)(merged >> DLU_IO_REG_BITS)); }

    return LDG_ERR_AOK;
}

static uint32_t dlu_mem_rd(ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint8_t width, uint64_t *out)
{
    if ((addr >> LDG_DANGCSA_IO_SEL_SHIFT) == DLU_CLEAR) { return dlu_ram_rd(dlu, addr, dlu_width_bytes_get(width), out); }

    return dlu_io_rd(dlu, addr, width, out);
}

static uint32_t dlu_mem_wr(ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint8_t width, uint64_t val)
{
    if ((addr >> LDG_DANGCSA_IO_SEL_SHIFT) == DLU_CLEAR) { return dlu_ram_wr(dlu, addr, dlu_width_bytes_get(width), val & dlu_width_val_mask_get(width)); }

    return dlu_io_wr(dlu, addr, width, val);
}

static void dlu_trap(ldg_dangcsa_dlu_t *dlu, uint32_t cause)
{
    uint64_t sp = dlu->reg[LDG_DANGCSA_SP];
    uint64_t loc = dlu->reg[LDG_DANGCSA_LOC];
    uint32_t ret = 0;

    dlu->exc_cause = cause;
    dlu->last_exc = cause;

    if (sp < LDG_DANGCSA_STACK_SLOT) { dlu->reg[LDG_DANGCSA_LOC] = loc + LDG_DANGCSA_INSTR_SIZE; }
    else
    {
        ret = dlu_mem_wr(dlu, sp - LDG_DANGCSA_STACK_SLOT, LDG_DANGCSA_WIDTH_QWORD, loc + LDG_DANGCSA_INSTR_SIZE);
        if (ret != LDG_ERR_AOK) { return; }

        dlu->reg[LDG_DANGCSA_SP] = sp - LDG_DANGCSA_STACK_SLOT;
        dlu->reg[LDG_DANGCSA_LOC] = dlu->exc_handler;
    }

    if (dlu->conf.trap_hlt_is == LDG_TRUTH_TRUE) { dlu->hlt_is = LDG_TRUTH_TRUE; }
}

static uint8_t dlu_cond_holds_is(uint64_t lhs, uint64_t rhs, uint8_t cond)
{
    if (cond == LDG_DANGCSA_COND_EQ) { return (lhs == rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (cond == LDG_DANGCSA_COND_NE) { return (lhs != rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (cond == LDG_DANGCSA_COND_LT) { return ((int64_t)lhs < (int64_t)rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (cond == LDG_DANGCSA_COND_GE) { return ((int64_t)lhs >= (int64_t)rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (cond == LDG_DANGCSA_COND_LTU) { return (lhs < rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (cond == LDG_DANGCSA_COND_GEU) { return (lhs >= rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (cond == LDG_DANGCSA_COND_ALWAYS) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint64_t dlu_clz_get(uint64_t val, uint8_t trailing_is)
{
    uint64_t cunt = 0;

    if (val == DLU_CLEAR) { return LDG_DANGCSA_CLZ_ZERO_RES; }

    if (trailing_is == LDG_TRUTH_TRUE)
    {
        while ((val & DLU_QBIT(cunt)) == DLU_CLEAR) { cunt++; }

        return cunt;
    }

    while ((val & DLU_QBIT(DLU_SIGN_BIT - cunt)) == DLU_CLEAR) { cunt++; }

    return cunt;
}

static uint64_t dlu_div_get(uint64_t lhs, uint64_t rhs, uint8_t signed_is, uint8_t remainder_is)
{
    uint8_t lhs_neg = LDG_TRUTH_FALSE;
    uint8_t rhs_neg = LDG_TRUTH_FALSE;
    uint64_t lhs_mag = lhs;
    uint64_t rhs_mag = rhs;
    uint64_t quot = 0;
    uint64_t rem = 0;

    if (signed_is == LDG_TRUTH_TRUE && (lhs & DLU_QBIT(DLU_SIGN_BIT)) != DLU_CLEAR)
    {
        lhs_neg = LDG_TRUTH_TRUE;
        lhs_mag = ~lhs + (uint64_t)LDG_TRUTH_TRUE;
    }

    if (signed_is == LDG_TRUTH_TRUE && (rhs & DLU_QBIT(DLU_SIGN_BIT)) != DLU_CLEAR)
    {
        rhs_neg = LDG_TRUTH_TRUE;
        rhs_mag = ~rhs + (uint64_t)LDG_TRUTH_TRUE;
    }

    quot = lhs_mag / rhs_mag;
    rem = lhs_mag % rhs_mag;

    if (remainder_is == LDG_TRUTH_TRUE)
    {
        if (lhs_neg == LDG_TRUTH_TRUE) { return ~rem + (uint64_t)LDG_TRUTH_TRUE; }

        return rem;
    }

    if (lhs_neg != rhs_neg) { return ~quot + (uint64_t)LDG_TRUTH_TRUE; }

    return quot;
}

static uint64_t dlu_mul_high_get(uint64_t lhs, uint64_t rhs, uint8_t signed_is)
{
    uint64_t lhs_low = lhs & DLU_LIMB_MASK;
    uint64_t lhs_high = lhs >> DLU_LIMB_BITS;
    uint64_t rhs_low = rhs & DLU_LIMB_MASK;
    uint64_t rhs_high = rhs >> DLU_LIMB_BITS;
    uint64_t low_low = lhs_low * rhs_low;
    uint64_t low_high = lhs_low * rhs_high;
    uint64_t high_low = lhs_high * rhs_low;
    uint64_t high_high = lhs_high * rhs_high;
    uint64_t mid = (low_low >> DLU_LIMB_BITS) + (low_high & DLU_LIMB_MASK) + (high_low & DLU_LIMB_MASK);
    uint64_t high = high_high + (low_high >> DLU_LIMB_BITS) + (high_low >> DLU_LIMB_BITS) + (mid >> DLU_LIMB_BITS);

    if (signed_is != LDG_TRUTH_TRUE) { return high; }

    if ((lhs & DLU_QBIT(DLU_SIGN_BIT)) != DLU_CLEAR) { high = high - rhs; }

    if ((rhs & DLU_QBIT(DLU_SIGN_BIT)) != DLU_CLEAR) { high = high - lhs; }

    return high;
}

static uint32_t dlu_cpy_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t dst_val = dlu_reg_rd(dlu, decoded->dst);
    uint64_t src_val = dlu_reg_rd(dlu, decoded->src);
    uint64_t shift = (uint64_t)decoded->quarter * LDG_DANGCSA_QUARTER_BITS;
    uint64_t half = 0;
    uint64_t res = 0;

    if (decoded->f1 != LDG_TRUTH_TRUE) { res = (decoded->f0 == LDG_TRUTH_TRUE) ? dlu_sext32_get(decoded->imm32) : src_val; }
    else
    {
        half = (decoded->f0 == LDG_TRUTH_TRUE) ? ((uint64_t)decoded->imm32 & LDG_DANGCSA_QUARTER_VAL_MASK) : ((src_val >> shift) & LDG_DANGCSA_QUARTER_VAL_MASK);
        res = (dst_val & ~(LDG_DANGCSA_QUARTER_VAL_MASK << shift)) | (half << shift);
    }

    dlu_reg_wr(dlu, decoded->dst, res);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_ld_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t addr = dlu_reg_rd(dlu, decoded->src) + dlu_sext32_get(decoded->imm32);
    uint64_t val = 0;
    uint32_t ret = 0;

    if ((addr & dlu_width_align_mask_get(decoded->width)) != DLU_CLEAR)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_UNALIGNED);
        return LDG_ERR_AOK;
    }

    ret = dlu_mem_rd(dlu, addr, decoded->width, &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_AOK; }

    if (decoded->f2 == LDG_TRUTH_TRUE) { val = dlu_sext_get(val, decoded->width); }

    dlu_reg_wr(dlu, decoded->dst, val);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_st_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t addr = dlu_reg_rd(dlu, decoded->dst) + dlu_sext32_get(decoded->imm32);
    uint64_t src_val = dlu_reg_rd(dlu, decoded->src);
    uint32_t ret = 0;

    if ((addr & dlu_width_align_mask_get(decoded->width)) != DLU_CLEAR)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_UNALIGNED);
        return LDG_ERR_AOK;
    }

    ret = dlu_mem_wr(dlu, addr, decoded->width, src_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_AOK; }

    dlu->last_val = src_val & dlu_width_val_mask_get(decoded->width);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_add_sub_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t lhs = dlu_reg_rd(dlu, decoded->dst);
    uint64_t rhs = (decoded->f1 == LDG_TRUTH_TRUE) ? dlu_sext32_get(decoded->imm32) : dlu_reg_rd(dlu, decoded->src);
    uint64_t carry_in = (decoded->f0 == LDG_TRUTH_TRUE) ? (uint64_t)dlu->carry : DLU_CLEAR;
    uint64_t partial = 0;
    uint64_t res = 0;
    uint8_t out_a = LDG_TRUTH_FALSE;
    uint8_t out_b = LDG_TRUTH_FALSE;

    if (decoded->opcode == LDG_DANGCSA_OP_ADD)
    {
        partial = lhs + rhs;
        res = partial + carry_in;
        out_a = (partial < lhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        out_b = (res < partial) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }
    else
    {
        partial = lhs - rhs;
        res = partial - carry_in;
        out_a = (lhs < rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        out_b = (partial < carry_in) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }

    dlu->carry = (out_a == LDG_TRUTH_TRUE || out_b == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    dlu_reg_wr(dlu, decoded->dst, res);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_logic_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t lhs = dlu_reg_rd(dlu, decoded->dst);
    uint64_t rhs = dlu_reg_rd(dlu, decoded->src);
    uint64_t res = 0;

    if (decoded->opcode == LDG_DANGCSA_OP_AND) { res = lhs & rhs; }
    else if (decoded->opcode == LDG_DANGCSA_OP_OR) { res = lhs | rhs; }
    else { res = lhs ^ rhs; }

    dlu_reg_wr(dlu, decoded->dst, res);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_sh_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t val = dlu_reg_rd(dlu, decoded->dst);
    uint64_t amt = (decoded->f2 == LDG_TRUTH_TRUE) ? (uint64_t)decoded->shift_amt : (dlu_reg_rd(dlu, decoded->src) & LDG_DANGCSA_SHIFT_AMT_MASK);
    uint64_t res = 0;

    if (decoded->f0 != LDG_TRUTH_TRUE) { res = val << amt; }
    else
    {
        res = val >> amt;
        if (decoded->f1 == LDG_TRUTH_TRUE && (val & DLU_QBIT(DLU_SIGN_BIT)) != DLU_CLEAR) { res = res | ~(UINT64_MAX >> amt); }
    }

    dlu_reg_wr(dlu, decoded->dst, res);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_mul_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t lhs = dlu_reg_rd(dlu, decoded->dst);
    uint64_t rhs = dlu_reg_rd(dlu, decoded->src);
    uint64_t res = 0;

    if (decoded->f0 == LDG_TRUTH_TRUE) { res = dlu_mul_high_get(lhs, rhs, decoded->f1); }
    else { res = lhs * rhs; }

    dlu_reg_wr(dlu, decoded->dst, res);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_div_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t lhs = dlu_reg_rd(dlu, decoded->dst);
    uint64_t rhs = dlu_reg_rd(dlu, decoded->src);

    if (rhs == DLU_CLEAR)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_DIV_BY_ZERO);
        return LDG_ERR_AOK;
    }

    dlu_reg_wr(dlu, decoded->dst, dlu_div_get(lhs, rhs, decoded->f0, decoded->f1));
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_clz_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    dlu_reg_wr(dlu, decoded->dst, dlu_clz_get(dlu_reg_rd(dlu, decoded->src), decoded->f0));
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_jmp_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t dst_val = dlu_reg_rd(dlu, decoded->dst);
    uint64_t src_val = dlu_reg_rd(dlu, decoded->src);
    uint64_t loc = dlu->reg[LDG_DANGCSA_LOC];

    if (decoded->cond == LDG_DANGCSA_COND_RESERVED)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_INVALID);
        return LDG_ERR_AOK;
    }

    if (decoded->f3 == LDG_TRUTH_TRUE)
    {
        if (dlu_cond_holds_is(dst_val, src_val, decoded->cond) == LDG_TRUTH_TRUE) { dlu_loc_jmp(dlu, loc + dlu_sext32_get(decoded->imm32) * LDG_DANGCSA_INSTR_SIZE); }
        else { dlu_loc_advance(dlu); }

        return LDG_ERR_AOK;
    }

    if (dlu_cond_holds_is(src_val, DLU_CLEAR, decoded->cond) == LDG_TRUTH_TRUE) { dlu_loc_jmp(dlu, dst_val); }
    else { dlu_loc_advance(dlu); }

    return LDG_ERR_AOK;
}

static uint32_t dlu_setcc_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t lhs = dlu_reg_rd(dlu, decoded->dst);
    uint64_t rhs = dlu_reg_rd(dlu, decoded->src);

    if (decoded->cond == LDG_DANGCSA_COND_RESERVED)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_INVALID);
        return LDG_ERR_AOK;
    }

    dlu_reg_wr(dlu, decoded->dst, (dlu_cond_holds_is(lhs, rhs, decoded->cond) == LDG_TRUTH_TRUE) ? (uint64_t)LDG_TRUTH_TRUE : (uint64_t)LDG_TRUTH_FALSE);
    dlu_loc_advance(dlu);

    return LDG_ERR_AOK;
}

static uint32_t dlu_call_exec(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_decoded_t *decoded)
{
    uint64_t sp = dlu->reg[LDG_DANGCSA_SP];
    uint64_t loc = dlu->reg[LDG_DANGCSA_LOC];
    uint64_t tgt = dlu_reg_rd(dlu, decoded->dst);
    uint32_t ret = 0;

    if (sp < LDG_DANGCSA_STACK_SLOT)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_STACK_UNDERFLOW);
        return LDG_ERR_AOK;
    }

    ret = dlu_mem_wr(dlu, sp - LDG_DANGCSA_STACK_SLOT, LDG_DANGCSA_WIDTH_QWORD, loc + LDG_DANGCSA_INSTR_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_AOK; }

    dlu->reg[LDG_DANGCSA_SP] = sp - LDG_DANGCSA_STACK_SLOT;
    dlu_loc_jmp(dlu, tgt);

    return LDG_ERR_AOK;
}

static uint32_t dlu_ret_exec(ldg_dangcsa_dlu_t *dlu)
{
    uint64_t sp = dlu->reg[LDG_DANGCSA_SP];
    uint64_t tgt = 0;
    uint32_t ret = 0;

    if (sp > LDG_DANGCSA_DLU_STACK_TOP)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_STACK_OVERFLOW);
        return LDG_ERR_AOK;
    }

    ret = dlu_mem_rd(dlu, sp, LDG_DANGCSA_WIDTH_QWORD, &tgt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_AOK; }

    dlu->reg[LDG_DANGCSA_SP] = sp + LDG_DANGCSA_STACK_SLOT;
    dlu_loc_jmp(dlu, tgt);

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_conf_default_get(ldg_dangcsa_dlu_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->ram_size = LDG_DANGCSA_DLU_RAM_SIZE_DEFAULT;
    out->tx_cap = LDG_DANGCSA_DLU_TX_CAP_DEFAULT;
    out->trap_hlt_is = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_init(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_dlu_conf_t *conf)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dlu || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->ram_size < LDG_DANGCSA_INSTR_SIZE || conf->ram_size > LDG_DANGCSA_DLU_RAM_LIMIT || (conf->ram_size & DLU_QWORD_ALIGN_MASK) != DLU_CLEAR)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->tx_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(dlu, sizeof(*dlu));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->conf = *conf;

    ret = dlu_alloc((uint64_t)conf->ram_size, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->ram = (uint8_t *)ptr;

    ret = dlu_alloc((uint64_t)conf->tx_cap, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        dlu_free(dlu->ram);
        return ret;
    }

    dlu->tx = (uint8_t *)ptr;

    dlu->last_reg = LDG_DANGCSA_DLU_LAST_NONE;
    dlu->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_shutdown(ldg_dangcsa_dlu_t *dlu)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu_free(dlu->tx);
    dlu_free(dlu->ram);

    return ldg_mem_zero(dlu, sizeof(*dlu));
}

uint32_t ldg_dangcsa_dlu_rst(ldg_dangcsa_dlu_t *dlu)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(dlu->reg, sizeof(dlu->reg));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(dlu->timer_tgt, sizeof(dlu->timer_tgt));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(dlu->timer_oneshot, sizeof(dlu->timer_oneshot));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->reg[LDG_DANGCSA_LOC] = LDG_DANGCSA_RST_VEC;
    dlu->exc_handler = DLU_CLEAR;
    dlu->exc_cause = LDG_DANGCSA_EXC_NONE;
    dlu->led = DLU_CLEAR;
    dlu->last_exc = LDG_DANGCSA_EXC_NONE;
    dlu->last_reg = LDG_DANGCSA_DLU_LAST_NONE;
    dlu->last_val = DLU_CLEAR;
    dlu->fault = LDG_DANGCSA_DLU_FAULT_NONE;
    dlu->step_cunt = 0;
    dlu->tx_cunt = 0;
    dlu->carry = LDG_TRUTH_FALSE;
    dlu->hlt_is = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_ram_wr(ldg_dangcsa_dlu_t *dlu, uint64_t addr, const uint8_t *src, uint32_t len)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(addr > dlu->conf.ram_size || len > dlu->conf.ram_size - addr)) { return LDG_ERR_BOUNDS; }

    if (len == DLU_EMPTY) { return LDG_ERR_AOK; }

    return ldg_mem_cpy(dlu->ram + addr, src, (uint64_t)len);
}

uint32_t ldg_dangcsa_dlu_ram_rd(const ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint8_t *dst, uint32_t len)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(addr > dlu->conf.ram_size || len > dlu->conf.ram_size - addr)) { return LDG_ERR_BOUNDS; }

    if (len == DLU_EMPTY) { return LDG_ERR_AOK; }

    return ldg_mem_cpy(dst, dlu->ram + addr, (uint64_t)len);
}

uint32_t ldg_dangcsa_dlu_reg_get(const ldg_dangcsa_dlu_t *dlu, uint32_t reg, uint64_t *out)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = DLU_CLEAR;

    if (LDG_UNLIKELY(reg >= LDG_DANGCSA_REG_CUNT)) { return LDG_ERR_BOUNDS; }

    *out = dlu->reg[reg];

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_reg_set(ldg_dangcsa_dlu_t *dlu, uint32_t reg, uint64_t val)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(reg >= LDG_DANGCSA_REG_CUNT)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(reg == LDG_DANGCSA_LOC)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dlu->reg[reg] = val;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_loc_set(ldg_dangcsa_dlu_t *dlu, uint64_t loc)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->reg[LDG_DANGCSA_LOC] = loc;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_exec(ldg_dangcsa_dlu_t *dlu, ldg_qword_t raw)
{
    ldg_dangcsa_decoded_t decoded = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(dlu->hlt_is == LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    ret = ldg_dangcsa_decode(raw, &decoded);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->last_reg = LDG_DANGCSA_DLU_LAST_NONE;
    dlu->last_val = DLU_CLEAR;
    dlu->last_exc = LDG_DANGCSA_EXC_NONE;
    dlu->step_cunt++;

    if (decoded.dst > LDG_DANGCSA_REG_MAX || decoded.src > LDG_DANGCSA_REG_MAX)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_RESERVED_REG);
        return LDG_ERR_AOK;
    }

    if (ldg_dangcsa_dst_writeback_is(decoded.opcode) == LDG_TRUTH_TRUE && decoded.dst == LDG_DANGCSA_LOC)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_RESERVED_REG);
        return LDG_ERR_AOK;
    }

    switch (decoded.opcode)
    {
        case LDG_DANGCSA_OP_CPY:
            ret = dlu_cpy_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_LD:
            ret = dlu_ld_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_ST:
            ret = dlu_st_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_ADD:
        case LDG_DANGCSA_OP_SUB:
            ret = dlu_add_sub_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_AND:
        case LDG_DANGCSA_OP_OR:
        case LDG_DANGCSA_OP_XOR:
            ret = dlu_logic_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_SH:
            ret = dlu_sh_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_MUL:
            ret = dlu_mul_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_DIV:
            ret = dlu_div_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_CLZ:
            ret = dlu_clz_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_JMP:
            ret = dlu_jmp_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_SETCC:
            ret = dlu_setcc_exec(dlu, &decoded);
            break;

        case LDG_DANGCSA_OP_CALL:
            ret = dlu_call_exec(dlu, &decoded);
            break;

        default:
            ret = dlu_ret_exec(dlu);
            break;
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (dlu->dev_tick_cb && dlu->hlt_is != LDG_TRUTH_TRUE) { return dlu->dev_tick_cb(dlu->dev_ctx, dlu); }

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_dev_set(ldg_dangcsa_dlu_t *dlu, ldg_dangcsa_dlu_dev_rd_cb_t rd_cb, ldg_dangcsa_dlu_dev_wr_cb_t wr_cb, ldg_dangcsa_dlu_dev_tick_cb_t tick_cb, void *ctx)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->dev_rd_cb = rd_cb;
    dlu->dev_wr_cb = wr_cb;
    dlu->dev_tick_cb = tick_cb;
    dlu->dev_ctx = ctx;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_hlt(ldg_dangcsa_dlu_t *dlu, uint32_t fault)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->fault = fault;
    dlu->hlt_is = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_step(ldg_dangcsa_dlu_t *dlu)
{
    uint64_t loc = 0;
    uint64_t raw = 0;
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(dlu->hlt_is == LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    loc = dlu->reg[LDG_DANGCSA_LOC];
    if ((loc & DLU_INSTR_ALIGN_MASK) != DLU_CLEAR)
    {
        dlu_trap(dlu, LDG_DANGCSA_EXC_UNALIGNED);
        return LDG_ERR_AOK;
    }

    ret = dlu_mem_rd(dlu, loc, LDG_DANGCSA_WIDTH_QWORD, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_AOK; }

    return ldg_dangcsa_dlu_exec(dlu, raw);
}

uint32_t ldg_dangcsa_dlu_run(ldg_dangcsa_dlu_t *dlu, uint32_t step_max, uint32_t *out_cunt)
{
    uint32_t cunt = 0;
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    while (cunt < step_max && dlu->hlt_is != LDG_TRUTH_TRUE)
    {
        ret = ldg_dangcsa_dlu_step(dlu);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cunt++;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint8_t ldg_dangcsa_dlu_hlt_is(const ldg_dangcsa_dlu_t *dlu)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return dlu->hlt_is;
}

uint32_t ldg_dangcsa_dlu_tx_get(const ldg_dangcsa_dlu_t *dlu, uint8_t *dst, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap < dlu->tx_cunt)) { return LDG_ERR_BOUNDS; }

    if (dlu->tx_cunt == DLU_EMPTY) { return LDG_ERR_AOK; }

    ret = ldg_mem_cpy(dst, dlu->tx, (uint64_t)dlu->tx_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = dlu->tx_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_dangcsa_dlu_tx_rst(ldg_dangcsa_dlu_t *dlu)
{
    uint32_t ret = dlu_init_ck(dlu);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dlu->tx_cunt = 0;

    return LDG_ERR_AOK;
}
