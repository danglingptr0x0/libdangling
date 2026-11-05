#ifndef LDG_ARCH_DANGCSA_DANGCSA_H
#define LDG_ARCH_DANGCSA_DANGCSA_H

#include <stdint.h>
#include <dangling/core/types.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>

#define LDG_DANGCSA_OP_CPY 0x0
#define LDG_DANGCSA_OP_LD 0x1
#define LDG_DANGCSA_OP_ST 0x2
#define LDG_DANGCSA_OP_ADD 0x3
#define LDG_DANGCSA_OP_SUB 0x4
#define LDG_DANGCSA_OP_AND 0x5
#define LDG_DANGCSA_OP_OR 0x6
#define LDG_DANGCSA_OP_XOR 0x7
#define LDG_DANGCSA_OP_SH 0x8
#define LDG_DANGCSA_OP_MUL 0x9
#define LDG_DANGCSA_OP_DIV 0xA
#define LDG_DANGCSA_OP_CLZ 0xB
#define LDG_DANGCSA_OP_JMP 0xC
#define LDG_DANGCSA_OP_SETCC 0xD
#define LDG_DANGCSA_OP_CALL 0xE
#define LDG_DANGCSA_OP_RET 0xF
#define LDG_DANGCSA_OP_CUNT 16
#define LDG_DANGCSA_OP_MAX 0xF

#define LDG_DANGCSA_DR0 0
#define LDG_DANGCSA_DR1 1
#define LDG_DANGCSA_DR2 2
#define LDG_DANGCSA_DR3 3
#define LDG_DANGCSA_DR4 4
#define LDG_DANGCSA_DR5 5
#define LDG_DANGCSA_DR6 6
#define LDG_DANGCSA_DR7 7
#define LDG_DANGCSA_DR8 8
#define LDG_DANGCSA_DR9 9
#define LDG_DANGCSA_DR10 10
#define LDG_DANGCSA_DR11 11
#define LDG_DANGCSA_DR12 12
#define LDG_DANGCSA_DR13 13
#define LDG_DANGCSA_DR14 14
#define LDG_DANGCSA_DR15 15
#define LDG_DANGCSA_TDR0 16
#define LDG_DANGCSA_TDR1 17
#define LDG_DANGCSA_TDR2 18
#define LDG_DANGCSA_TDR3 19
#define LDG_DANGCSA_TDR4 20
#define LDG_DANGCSA_TDR5 21
#define LDG_DANGCSA_LOC 22
#define LDG_DANGCSA_SP 23
#define LDG_DANGCSA_REG_GEN_CUNT 16
#define LDG_DANGCSA_REG_TDR_BASE 16
#define LDG_DANGCSA_REG_TDR_CUNT 6
#define LDG_DANGCSA_REG_CUNT 24
#define LDG_DANGCSA_REG_MAX 23
#define LDG_DANGCSA_REG_SPACE 64
#define LDG_DANGCSA_REG_FIELD_MASK 0x3F
#define LDG_DANGCSA_REG_BITS 64

#define LDG_DANGCSA_OPCODE_SHIFT 60
#define LDG_DANGCSA_OPCODE_MASK 0xF000000000000000ULL
#define LDG_DANGCSA_OPCODE_FIELD_MASK 0xF
#define LDG_DANGCSA_DST_SHIFT 54
#define LDG_DANGCSA_DST_MASK 0x0FC0000000000000ULL
#define LDG_DANGCSA_SRC_SHIFT 48
#define LDG_DANGCSA_SRC_MASK 0x003F000000000000ULL
#define LDG_DANGCSA_CONF_MASK 0x0000FFFFFFFFFFFFULL
#define LDG_DANGCSA_CONF_RESERVED_MASK 0x00000FFF00000000ULL
#define LDG_DANGCSA_CONF_NONE 0x0ULL
#define LDG_DANGCSA_SRC_UNUSED 0

#define LDG_DANGCSA_F0_BIT 47
#define LDG_DANGCSA_F1_BIT 46
#define LDG_DANGCSA_F2_BIT 45
#define LDG_DANGCSA_F3_BIT 44
#define LDG_DANGCSA_F0_MASK (1ULL << LDG_DANGCSA_F0_BIT)
#define LDG_DANGCSA_F1_MASK (1ULL << LDG_DANGCSA_F1_BIT)
#define LDG_DANGCSA_F2_MASK (1ULL << LDG_DANGCSA_F2_BIT)
#define LDG_DANGCSA_F3_MASK (1ULL << LDG_DANGCSA_F3_BIT)
#define LDG_DANGCSA_FLAG_SHIFT 44
#define LDG_DANGCSA_FLAG_MASK 0x0000F00000000000ULL
#define LDG_DANGCSA_FLAG_FIELD_MASK 0xF

#define LDG_DANGCSA_CONF_CPY_IMM LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_CPY_PARTIAL LDG_DANGCSA_F1_MASK
#define LDG_DANGCSA_CONF_LD_SIGNED LDG_DANGCSA_F2_MASK
#define LDG_DANGCSA_CONF_ADD_CARRY LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_ADD_IMM LDG_DANGCSA_F1_MASK
#define LDG_DANGCSA_CONF_SUB_BORROW LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_SUB_IMM LDG_DANGCSA_F1_MASK
#define LDG_DANGCSA_CONF_SH_SHR LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_SH_ARITH LDG_DANGCSA_F1_MASK
#define LDG_DANGCSA_CONF_SH_IMM LDG_DANGCSA_F2_MASK
#define LDG_DANGCSA_CONF_MUL_HIGH LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_MUL_SIGNED LDG_DANGCSA_F1_MASK
#define LDG_DANGCSA_CONF_DIV_SIGNED LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_DIV_REMAINDER LDG_DANGCSA_F1_MASK
#define LDG_DANGCSA_CONF_CLZ_TRAILING LDG_DANGCSA_F0_MASK
#define LDG_DANGCSA_CONF_JMP_RELATIVE LDG_DANGCSA_F3_MASK

#define LDG_DANGCSA_WIDTH_SHIFT 46
#define LDG_DANGCSA_WIDTH_MASK 0x0000C00000000000ULL
#define LDG_DANGCSA_WIDTH_FIELD_MASK 0x3
#define LDG_DANGCSA_WIDTH_BYTE 0x0
#define LDG_DANGCSA_WIDTH_WORD 0x1
#define LDG_DANGCSA_WIDTH_DWORD 0x2
#define LDG_DANGCSA_WIDTH_QWORD 0x3
#define LDG_DANGCSA_WIDTH_CUNT 4
#define LDG_DANGCSA_WIDTH_MAX 0x3

#define LDG_DANGCSA_COND_SHIFT 45
#define LDG_DANGCSA_COND_MASK 0x0000E00000000000ULL
#define LDG_DANGCSA_COND_FIELD_MASK 0x7
#define LDG_DANGCSA_COND_EQ 0x0
#define LDG_DANGCSA_COND_NE 0x1
#define LDG_DANGCSA_COND_LT 0x2
#define LDG_DANGCSA_COND_GE 0x3
#define LDG_DANGCSA_COND_LTU 0x4
#define LDG_DANGCSA_COND_GEU 0x5
#define LDG_DANGCSA_COND_ALWAYS 0x6
#define LDG_DANGCSA_COND_RESERVED 0x7
#define LDG_DANGCSA_COND_CUNT 7
#define LDG_DANGCSA_COND_MAX 0x6

#define LDG_DANGCSA_QUARTER_SHIFT 44
#define LDG_DANGCSA_QUARTER_MASK 0x0000300000000000ULL
#define LDG_DANGCSA_QUARTER_FIELD_MASK 0x3
#define LDG_DANGCSA_QUARTER_BITS 16
#define LDG_DANGCSA_QUARTER_CUNT 4
#define LDG_DANGCSA_QUARTER_MAX 0x3
#define LDG_DANGCSA_QUARTER_VAL_MASK 0xFFFFULL

#define LDG_DANGCSA_IMM32_MASK 0x00000000FFFFFFFFULL
#define LDG_DANGCSA_IMM32_BITS 32
#define LDG_DANGCSA_SHIFT_AMT_MASK 0x3F
#define LDG_DANGCSA_SHIFT_AMT_MAX 63
#define LDG_DANGCSA_DISP_MIN INT32_MIN
#define LDG_DANGCSA_DISP_MAX INT32_MAX
#define LDG_DANGCSA_BRANCH_OFF_MIN INT32_MIN
#define LDG_DANGCSA_BRANCH_OFF_MAX INT32_MAX

#define LDG_DANGCSA_INSTR_SIZE 8
#define LDG_DANGCSA_INSTR_BITS 64
#define LDG_DANGCSA_STACK_SLOT 8
#define LDG_DANGCSA_ALIGNMENT 8
#define LDG_DANGCSA_RST_VEC 0x0000000000000000ULL
#define LDG_DANGCSA_CLZ_ZERO_RES 64

#define LDG_DANGCSA_ADDR_MEM_BITS 48
#define LDG_DANGCSA_ADDR_MEM_MASK 0x0000FFFFFFFFFFFFULL
#define LDG_DANGCSA_ADDR_MEM_END 0x0000FFFFFFFFFFFFULL
#define LDG_DANGCSA_IO_SEL_SHIFT 48
#define LDG_DANGCSA_IO_SEL_MASK 0xFFFF
#define LDG_DANGCSA_IO_OFF_MASK 0x0000FFFFFFFFFFFFULL
#define LDG_DANGCSA_IO_PAGE_DEV 1
#define LDG_DANGCSA_IO_EXC_HANDLER_LO_OFF 0x80000000ULL
#define LDG_DANGCSA_IO_EXC_HANDLER_HI_OFF 0x80000004ULL
#define LDG_DANGCSA_IO_EXC_CAUSE_OFF 0x80000008ULL
#define LDG_DANGCSA_IO_DEV_OFF 0x90000000ULL

#define LDG_DANGCSA_EXC_NONE 0x0
#define LDG_DANGCSA_EXC_INVALID 0x1
#define LDG_DANGCSA_EXC_UNALIGNED 0x2
#define LDG_DANGCSA_EXC_STACK_OVERFLOW 0x3
#define LDG_DANGCSA_EXC_STACK_UNDERFLOW 0x4
#define LDG_DANGCSA_EXC_DIV_BY_ZERO 0x5
#define LDG_DANGCSA_EXC_RESERVED_REG 0x6
#define LDG_DANGCSA_EXC_CUNT 7

#define LDG_DANGCSA_DISASM_BUFF_MIN 48
#define LDG_DANGCSA_LD_WORD_CUNT 8
#define LDG_DANGCSA_ST_WORD_CUNT 4
#define LDG_DANGCSA_DIV_WORD_CUNT 4
#define LDG_DANGCSA_SH_WORD_CUNT 3
#define LDG_DANGCSA_MUL_WORD_CUNT 2
#define LDG_DANGCSA_COND_WORD_CUNT 8

#define LDG_DANGCSA_MNEMONIC_CAP 6
#define LDG_DANGCSA_REG_NAME_CAP 5
#define LDG_DANGCSA_LD_WORD_CAP 3
#define LDG_DANGCSA_ST_WORD_CAP 2
#define LDG_DANGCSA_DIV_WORD_CAP 3
#define LDG_DANGCSA_SH_WORD_CAP 3
#define LDG_DANGCSA_MUL_WORD_CAP 3
#define LDG_DANGCSA_COND_WORD_CAP 4

LDG_EXPORT extern const char LDG_DANGCSA_MNEMONIC[LDG_DANGCSA_OP_CUNT][LDG_DANGCSA_MNEMONIC_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_REG_NAME[LDG_DANGCSA_REG_CUNT][LDG_DANGCSA_REG_NAME_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_LD_WORD[LDG_DANGCSA_LD_WORD_CUNT][LDG_DANGCSA_LD_WORD_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_ST_WORD[LDG_DANGCSA_ST_WORD_CUNT][LDG_DANGCSA_ST_WORD_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_DIV_WORD[LDG_DANGCSA_DIV_WORD_CUNT][LDG_DANGCSA_DIV_WORD_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_SH_WORD[LDG_DANGCSA_SH_WORD_CUNT][LDG_DANGCSA_SH_WORD_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_MUL_WORD[LDG_DANGCSA_MUL_WORD_CUNT][LDG_DANGCSA_MUL_WORD_CAP];
LDG_EXPORT extern const char LDG_DANGCSA_COND_WORD[LDG_DANGCSA_COND_WORD_CUNT][LDG_DANGCSA_COND_WORD_CAP];

typedef struct ldg_dangcsa_decoded
{
    ldg_qword_t conf;
    ldg_dword_t imm32;
    uint8_t opcode;
    uint8_t dst;
    uint8_t src;
    uint8_t f0;
    uint8_t f1;
    uint8_t f2;
    uint8_t f3;
    uint8_t cond;
    uint8_t width;
    uint8_t quarter;
    uint8_t shift_amt;
    uint8_t pudding[1];
} ldg_dangcsa_decoded_t;

static inline ldg_qword_t ldg_dangcsa_encode(uint8_t opcode, uint8_t dst, uint8_t src, ldg_qword_t conf)
{
    return ((ldg_qword_t)(opcode & LDG_DANGCSA_OPCODE_FIELD_MASK) << LDG_DANGCSA_OPCODE_SHIFT) | ((ldg_qword_t)(dst & LDG_DANGCSA_REG_FIELD_MASK) << LDG_DANGCSA_DST_SHIFT) | ((ldg_qword_t)(src & LDG_DANGCSA_REG_FIELD_MASK) << LDG_DANGCSA_SRC_SHIFT) | (conf & LDG_DANGCSA_CONF_MASK);
}

static inline uint8_t ldg_dangcsa_opcode_get(ldg_qword_t raw)
{
    return (uint8_t)((raw & LDG_DANGCSA_OPCODE_MASK) >> LDG_DANGCSA_OPCODE_SHIFT);
}

static inline uint8_t ldg_dangcsa_dst_get(ldg_qword_t raw)
{
    return (uint8_t)((raw & LDG_DANGCSA_DST_MASK) >> LDG_DANGCSA_DST_SHIFT);
}

static inline uint8_t ldg_dangcsa_src_get(ldg_qword_t raw)
{
    return (uint8_t)((raw & LDG_DANGCSA_SRC_MASK) >> LDG_DANGCSA_SRC_SHIFT);
}

static inline ldg_qword_t ldg_dangcsa_conf_get(ldg_qword_t raw)
{
    return raw & LDG_DANGCSA_CONF_MASK;
}

static inline uint8_t ldg_dangcsa_flag_get(ldg_qword_t raw)
{
    return (uint8_t)((raw & LDG_DANGCSA_FLAG_MASK) >> LDG_DANGCSA_FLAG_SHIFT);
}

static inline uint8_t ldg_dangcsa_cond_get(ldg_qword_t raw)
{
    return (uint8_t)((raw & LDG_DANGCSA_COND_MASK) >> LDG_DANGCSA_COND_SHIFT);
}

static inline uint8_t ldg_dangcsa_width_get(ldg_qword_t raw)
{
    return (uint8_t)((raw & LDG_DANGCSA_WIDTH_MASK) >> LDG_DANGCSA_WIDTH_SHIFT);
}

static inline ldg_dword_t ldg_dangcsa_imm32_get(ldg_qword_t raw)
{
    return (ldg_dword_t)(raw & LDG_DANGCSA_IMM32_MASK);
}

static inline uint32_t ldg_dangcsa_decode(ldg_qword_t raw, ldg_dangcsa_decoded_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->conf = raw & LDG_DANGCSA_CONF_MASK;
    out->imm32 = (ldg_dword_t)(raw & LDG_DANGCSA_IMM32_MASK);
    out->opcode = (uint8_t)((raw & LDG_DANGCSA_OPCODE_MASK) >> LDG_DANGCSA_OPCODE_SHIFT);
    out->dst = (uint8_t)((raw & LDG_DANGCSA_DST_MASK) >> LDG_DANGCSA_DST_SHIFT);
    out->src = (uint8_t)((raw & LDG_DANGCSA_SRC_MASK) >> LDG_DANGCSA_SRC_SHIFT);
    out->f0 = (uint8_t)((raw & LDG_DANGCSA_F0_MASK) >> LDG_DANGCSA_F0_BIT);
    out->f1 = (uint8_t)((raw & LDG_DANGCSA_F1_MASK) >> LDG_DANGCSA_F1_BIT);
    out->f2 = (uint8_t)((raw & LDG_DANGCSA_F2_MASK) >> LDG_DANGCSA_F2_BIT);
    out->f3 = (uint8_t)((raw & LDG_DANGCSA_F3_MASK) >> LDG_DANGCSA_F3_BIT);
    out->cond = (uint8_t)((raw & LDG_DANGCSA_COND_MASK) >> LDG_DANGCSA_COND_SHIFT);
    out->width = (uint8_t)((raw & LDG_DANGCSA_WIDTH_MASK) >> LDG_DANGCSA_WIDTH_SHIFT);
    out->quarter = (uint8_t)((raw & LDG_DANGCSA_QUARTER_MASK) >> LDG_DANGCSA_QUARTER_SHIFT);
    out->shift_amt = (uint8_t)(raw & LDG_DANGCSA_SHIFT_AMT_MASK);
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

static inline uint8_t ldg_dangcsa_dst_writeback_is(uint8_t opcode)
{
    switch (opcode)
    {
        case LDG_DANGCSA_OP_ST:
        case LDG_DANGCSA_OP_JMP:
        case LDG_DANGCSA_OP_CALL:
        case LDG_DANGCSA_OP_RET:
            return LDG_TRUTH_FALSE;

        default:
            return LDG_TRUTH_TRUE;
    }
}

static inline uint32_t ldg_dangcsa_validate(const ldg_dangcsa_decoded_t *decoded)
{
    if (LDG_UNLIKELY(!decoded)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(decoded->opcode > LDG_DANGCSA_OP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->dst > LDG_DANGCSA_REG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->src > LDG_DANGCSA_REG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_dangcsa_dst_writeback_is(decoded->opcode) == LDG_TRUTH_TRUE && decoded->dst == LDG_DANGCSA_LOC)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((decoded->opcode == LDG_DANGCSA_OP_JMP || decoded->opcode == LDG_DANGCSA_OP_SETCC) && decoded->cond == LDG_DANGCSA_COND_RESERVED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((decoded->conf & LDG_DANGCSA_CONF_RESERVED_MASK) != LDG_DANGCSA_CONF_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static inline ldg_qword_t ldg_dangcsa_cpy_encode(uint8_t dst, uint8_t src)
{
    return ldg_dangcsa_encode(LDG_DANGCSA_OP_CPY, dst, src, LDG_DANGCSA_CONF_NONE);
}

static inline ldg_qword_t ldg_dangcsa_cpy_imm_encode(uint8_t dst, ldg_dword_t imm32)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_CPY_IMM | (ldg_qword_t)imm32;

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_CPY, dst, LDG_DANGCSA_SRC_UNUSED, conf);
}

static inline ldg_qword_t ldg_dangcsa_cpy_quarter_encode(uint8_t dst, uint8_t src, uint8_t quarter)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_CPY_PARTIAL | ((ldg_qword_t)(quarter & LDG_DANGCSA_QUARTER_FIELD_MASK) << LDG_DANGCSA_QUARTER_SHIFT);

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_CPY, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_cpy_quarter_imm_encode(uint8_t dst, uint8_t quarter, ldg_word_t imm16)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_CPY_IMM | LDG_DANGCSA_CONF_CPY_PARTIAL | ((ldg_qword_t)(quarter & LDG_DANGCSA_QUARTER_FIELD_MASK) << LDG_DANGCSA_QUARTER_SHIFT) | (ldg_qword_t)imm16;

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_CPY, dst, LDG_DANGCSA_SRC_UNUSED, conf);
}

static inline ldg_qword_t ldg_dangcsa_ld_encode(uint8_t dst, uint8_t base, uint8_t width, uint8_t signed_is, ldg_dword_t disp)
{
    ldg_qword_t conf = ((ldg_qword_t)(width & LDG_DANGCSA_WIDTH_FIELD_MASK) << LDG_DANGCSA_WIDTH_SHIFT) | (ldg_qword_t)disp;
    if (signed_is) { conf |= LDG_DANGCSA_CONF_LD_SIGNED; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_LD, dst, base, conf);
}

static inline ldg_qword_t ldg_dangcsa_st_encode(uint8_t base, uint8_t src, uint8_t width, ldg_dword_t disp)
{
    ldg_qword_t conf = ((ldg_qword_t)(width & LDG_DANGCSA_WIDTH_FIELD_MASK) << LDG_DANGCSA_WIDTH_SHIFT) | (ldg_qword_t)disp;

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_ST, base, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_add_encode(uint8_t dst, uint8_t src, uint8_t carry_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_NONE;
    if (carry_is) { conf |= LDG_DANGCSA_CONF_ADD_CARRY; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_ADD, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_add_imm_encode(uint8_t dst, ldg_dword_t imm32, uint8_t carry_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_ADD_IMM | (ldg_qword_t)imm32;
    if (carry_is) { conf |= LDG_DANGCSA_CONF_ADD_CARRY; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_ADD, dst, LDG_DANGCSA_SRC_UNUSED, conf);
}

static inline ldg_qword_t ldg_dangcsa_sub_encode(uint8_t dst, uint8_t src, uint8_t borrow_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_NONE;
    if (borrow_is) { conf |= LDG_DANGCSA_CONF_SUB_BORROW; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_SUB, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_sub_imm_encode(uint8_t dst, ldg_dword_t imm32, uint8_t borrow_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_SUB_IMM | (ldg_qword_t)imm32;
    if (borrow_is) { conf |= LDG_DANGCSA_CONF_SUB_BORROW; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_SUB, dst, LDG_DANGCSA_SRC_UNUSED, conf);
}

static inline ldg_qword_t ldg_dangcsa_and_encode(uint8_t dst, uint8_t src)
{
    return ldg_dangcsa_encode(LDG_DANGCSA_OP_AND, dst, src, LDG_DANGCSA_CONF_NONE);
}

static inline ldg_qword_t ldg_dangcsa_or_encode(uint8_t dst, uint8_t src)
{
    return ldg_dangcsa_encode(LDG_DANGCSA_OP_OR, dst, src, LDG_DANGCSA_CONF_NONE);
}

static inline ldg_qword_t ldg_dangcsa_xor_encode(uint8_t dst, uint8_t src)
{
    return ldg_dangcsa_encode(LDG_DANGCSA_OP_XOR, dst, src, LDG_DANGCSA_CONF_NONE);
}

static inline ldg_qword_t ldg_dangcsa_sh_encode(uint8_t dst, uint8_t src, uint8_t shr_is, uint8_t arith_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_NONE;
    if (shr_is) { conf |= LDG_DANGCSA_CONF_SH_SHR; }

    if (arith_is) { conf |= LDG_DANGCSA_CONF_SH_ARITH; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_SH, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_sh_imm_encode(uint8_t dst, uint8_t amt, uint8_t shr_is, uint8_t arith_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_SH_IMM | (ldg_qword_t)(amt & LDG_DANGCSA_SHIFT_AMT_MASK);
    if (shr_is) { conf |= LDG_DANGCSA_CONF_SH_SHR; }

    if (arith_is) { conf |= LDG_DANGCSA_CONF_SH_ARITH; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_SH, dst, LDG_DANGCSA_SRC_UNUSED, conf);
}

static inline ldg_qword_t ldg_dangcsa_mul_encode(uint8_t dst, uint8_t src, uint8_t high_is, uint8_t signed_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_NONE;
    if (high_is) { conf |= LDG_DANGCSA_CONF_MUL_HIGH; }

    if (signed_is) { conf |= LDG_DANGCSA_CONF_MUL_SIGNED; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_MUL, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_div_encode(uint8_t dst, uint8_t src, uint8_t signed_is, uint8_t remainder_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_NONE;
    if (signed_is) { conf |= LDG_DANGCSA_CONF_DIV_SIGNED; }

    if (remainder_is) { conf |= LDG_DANGCSA_CONF_DIV_REMAINDER; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_DIV, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_clz_encode(uint8_t dst, uint8_t src, uint8_t trailing_is)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_NONE;
    if (trailing_is) { conf |= LDG_DANGCSA_CONF_CLZ_TRAILING; }

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_CLZ, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_jmp_encode(uint8_t dst, uint8_t src, uint8_t cond)
{
    ldg_qword_t conf = (ldg_qword_t)(cond & LDG_DANGCSA_COND_FIELD_MASK) << LDG_DANGCSA_COND_SHIFT;

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_JMP, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_jmp_relative_encode(uint8_t lhs, uint8_t rhs, uint8_t cond, ldg_dword_t off)
{
    ldg_qword_t conf = LDG_DANGCSA_CONF_JMP_RELATIVE | ((ldg_qword_t)(cond & LDG_DANGCSA_COND_FIELD_MASK) << LDG_DANGCSA_COND_SHIFT) | (ldg_qword_t)off;

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_JMP, lhs, rhs, conf);
}

static inline ldg_qword_t ldg_dangcsa_setcc_encode(uint8_t dst, uint8_t src, uint8_t cond)
{
    ldg_qword_t conf = (ldg_qword_t)(cond & LDG_DANGCSA_COND_FIELD_MASK) << LDG_DANGCSA_COND_SHIFT;

    return ldg_dangcsa_encode(LDG_DANGCSA_OP_SETCC, dst, src, conf);
}

static inline ldg_qword_t ldg_dangcsa_call_encode(uint8_t dst)
{
    return ldg_dangcsa_encode(LDG_DANGCSA_OP_CALL, dst, LDG_DANGCSA_SRC_UNUSED, LDG_DANGCSA_CONF_NONE);
}

static inline ldg_qword_t ldg_dangcsa_ret_encode(void)
{
    return ldg_dangcsa_encode(LDG_DANGCSA_OP_RET, LDG_DANGCSA_DR0, LDG_DANGCSA_SRC_UNUSED, LDG_DANGCSA_CONF_NONE);
}

LDG_EXPORT uint32_t ldg_dangcsa_disasm(const ldg_dangcsa_decoded_t *decoded, char *buff, uint64_t buff_len);

#endif
