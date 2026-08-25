#ifndef LDG_ARCH_MISC_MISC_H
#define LDG_ARCH_MISC_MISC_H

#include <stdint.h>
#include <dangling/core/types.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>

// opcodes
#define LDG_MISC_OP_CPY 0x0
#define LDG_MISC_OP_LD 0x1
#define LDG_MISC_OP_ST 0x2
#define LDG_MISC_OP_LDD LDG_MISC_OP_LD
#define LDG_MISC_OP_STD LDG_MISC_OP_ST
#define LDG_MISC_OP_CPH 0x3
#define LDG_MISC_OP_ADD 0x4
#define LDG_MISC_OP_SUB 0x5
#define LDG_MISC_OP_AND 0x6
#define LDG_MISC_OP_OR 0x7
#define LDG_MISC_OP_XOR 0x8
#define LDG_MISC_OP_SHL 0x9
#define LDG_MISC_OP_SHR 0xA
#define LDG_MISC_OP_JMP 0xB
#define LDG_MISC_OP_JNZ 0xC
#define LDG_MISC_OP_CALL 0xD
#define LDG_MISC_OP_RET 0xE
#define LDG_MISC_OP_CPL 0xF
#define LDG_MISC_OP_MUL 0x10
#define LDG_MISC_OP_DIV 0x11
#define LDG_MISC_OP_BCC 0x12
#define LDG_MISC_OP_SETCC 0x13
#define LDG_MISC_OP_MULH 0x14
#define LDG_MISC_OP_CLZ 0x15
#define LDG_MISC_OP_ADDI 0x16
#define LDG_MISC_OP_CUNT 23
#define LDG_MISC_OP_MAX 0x16
#define LDG_MISC_OP_CEILING 24
#define LDG_MISC_OP_SPACE 32

// registers
#define LDG_MISC_DR0 0
#define LDG_MISC_DR1 1
#define LDG_MISC_DR2 2
#define LDG_MISC_DR3 3
#define LDG_MISC_DR4 4
#define LDG_MISC_DR5 5
#define LDG_MISC_DR6 6
#define LDG_MISC_DR7 7
#define LDG_MISC_DR8 8
#define LDG_MISC_DR9 9
#define LDG_MISC_DR10 10
#define LDG_MISC_DR11 11
#define LDG_MISC_DR12 12
#define LDG_MISC_DR13 13
#define LDG_MISC_DR14 14
#define LDG_MISC_DR15 15
#define LDG_MISC_LOC 14
#define LDG_MISC_SP 15
#define LDG_MISC_REG_CUNT 16
#define LDG_MISC_REG_MAX 15

// field masks and shifts
#define LDG_MISC_OPCODE_SHIFT 27
#define LDG_MISC_OPCODE_MASK 0xF8000000
#define LDG_MISC_OPCODE_FIELD_MASK 0x1F
#define LDG_MISC_DST_SHIFT 23
#define LDG_MISC_DST_MASK 0x07800000
#define LDG_MISC_SRC_SHIFT 19
#define LDG_MISC_SRC_MASK 0x00780000
#define LDG_MISC_REG_FIELD_MASK 0xF
#define LDG_MISC_CONF_MASK 0x0007FFFF
#define LDG_MISC_SRC_UNUSED 0
#define LDG_MISC_CONF_NONE 0

// mode and arithmetic shift bits
#define LDG_MISC_MODE_BIT 18
#define LDG_MISC_MODE_MASK (1U << LDG_MISC_MODE_BIT)
#define LDG_MISC_ARITH_BIT 17
#define LDG_MISC_ARITH_MASK (1U << LDG_MISC_ARITH_BIT)
#define LDG_MISC_CARRY_BIT 18
#define LDG_MISC_CARRY_MASK (1U << LDG_MISC_CARRY_BIT)
#define LDG_MISC_SIGNED_BIT 18
#define LDG_MISC_SIGNED_MASK (1U << LDG_MISC_SIGNED_BIT)
#define LDG_MISC_REMAINDER_BIT 17
#define LDG_MISC_REMAINDER_MASK (1U << LDG_MISC_REMAINDER_BIT)

#define LDG_MISC_SIZE_SHIFT 16
#define LDG_MISC_SIZE_MASK 0x00070000
#define LDG_MISC_SIZE_DWORD 0x0
#define LDG_MISC_SIZE_BYTE_S 0x1
#define LDG_MISC_SIZE_BYTE_U 0x2
#define LDG_MISC_SIZE_WORD_S 0x3
#define LDG_MISC_SIZE_WORD_U 0x4
#define LDG_MISC_SIZE_ST_DWORD 0x0
#define LDG_MISC_SIZE_ST_BYTE 0x1
#define LDG_MISC_SIZE_ST_WORD 0x2

#define LDG_MISC_COND_SHIFT 16
#define LDG_MISC_COND_MASK 0x00070000
#define LDG_MISC_COND_EQ 0x0
#define LDG_MISC_COND_NE 0x1
#define LDG_MISC_COND_LT 0x2
#define LDG_MISC_COND_GE 0x3
#define LDG_MISC_COND_LTU 0x4
#define LDG_MISC_COND_GEU 0x5
#define LDG_MISC_COND_MAX 0x5
#define LDG_MISC_COND_CUNT 6
#define LDG_MISC_SIZE_CUNT 5
#define LDG_MISC_SIZE_ST_CUNT 3
#define LDG_MISC_REG_NAME_WIDTH 4

// immediate fields
#define LDG_MISC_IMM16_MASK 0x0000FFFF
#define LDG_MISC_SHIFT_AMT_MASK 0x0000001F
#define LDG_MISC_SHIFT_AMT_MAX 31
#define LDG_MISC_DISP_MASK 0x0000FFFF
#define LDG_MISC_DISP_MIN (-32768)
#define LDG_MISC_DISP_MAX 32767
#define LDG_MISC_BRANCH_OFF_MIN (-32768)
#define LDG_MISC_BRANCH_OFF_MAX 32767

// architecture constants
#define LDG_MISC_WORD_SIZE 4
#define LDG_MISC_INSN_WIDTH 32
#define LDG_MISC_ALIGNMENT 4
#define LDG_MISC_RST_VEC 0x00000000

// memory map
#define LDG_MISC_RAM_BASE 0x00000000
#define LDG_MISC_RAM_END 0x7FFFFFFF
#define LDG_MISC_CPU_CTRL_BASE 0x80000000
#define LDG_MISC_CPU_CTRL_END 0x8FFFFFFF
#define LDG_MISC_IO_BASE 0x90000000
#define LDG_MISC_IO_END 0xFFFFFFFF
#define LDG_MISC_EXCEPTION_HANDLER 0x80000000
#define LDG_MISC_EXCEPTION_CAUSE 0x80000004

// exception codes
#define LDG_MISC_EXC_NONE 0x00000000
#define LDG_MISC_EXC_INVALID_OPCODE 0x00000001
#define LDG_MISC_EXC_UNALIGNED 0x00000002
#define LDG_MISC_EXC_STACK_OVERFLOW 0x00000003
#define LDG_MISC_EXC_STACK_UNDERFLOW 0x00000004
#define LDG_MISC_EXC_DIV_BY_ZERO 0x00000005
#define LDG_MISC_EXC_RESERVED_REG_WR 0x00000006

// disasm
#define LDG_MISC_DISASM_BUFF_MIN 48
#define LDG_MISC_IMM16_BITS 16
#define LDG_MISC_IMM16_SPAN 0x10000
#define LDG_MISC_SIGN16_MASK 0x8000
#define LDG_MISC_DEC_DIGITS_MAX 12
#define LDG_MISC_HEX_DIGIT_SET "0123456789ABCDEF"
#define LDG_MISC_ASCII_DIGIT_BASE '0'
#define LDG_MISC_NEG_SIGN '-'

typedef struct ldg_misc_decoded
{
    uint8_t opcode;
    uint8_t dst;
    uint8_t src;
    uint8_t mode;
    uint8_t arith;
    uint8_t carry;
    uint8_t sign;
    uint8_t remainder;
    uint8_t size;
    uint8_t cond;
    uint8_t shift_amt;
    uint8_t pudding;
    ldg_dword_t conf;
    ldg_word_t imm16;
    ldg_word_t disp;
} ldg_misc_decoded_t;

// mnemonics

LDG_EXPORT extern const char *const LDG_MISC_MNEMONICS[LDG_MISC_OP_CUNT];
LDG_EXPORT extern const char *const LDG_MISC_LD_MNEMONICS[LDG_MISC_SIZE_CUNT];
LDG_EXPORT extern const char *const LDG_MISC_ST_MNEMONICS[LDG_MISC_SIZE_ST_CUNT];
LDG_EXPORT extern const char *const LDG_MISC_COND_MNEMONICS[LDG_MISC_COND_CUNT];

// encode

static inline ldg_dword_t ldg_misc_encode(uint8_t opcode, uint8_t dst, uint8_t src, ldg_dword_t conf)
{
    return ((ldg_dword_t)(opcode & LDG_MISC_OPCODE_FIELD_MASK) << LDG_MISC_OPCODE_SHIFT) |
           ((ldg_dword_t)(dst & LDG_MISC_REG_FIELD_MASK) << LDG_MISC_DST_SHIFT) |
           ((ldg_dword_t)(src & LDG_MISC_REG_FIELD_MASK) << LDG_MISC_SRC_SHIFT) |
           (conf & LDG_MISC_CONF_MASK);
}

// decode

static inline uint32_t ldg_misc_decode(ldg_dword_t raw, ldg_misc_decoded_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->opcode = 0;
    out->dst = 0;
    out->src = 0;
    out->mode = 0;
    out->arith = 0;
    out->carry = 0;
    out->sign = 0;
    out->remainder = 0;
    out->size = 0;
    out->cond = 0;
    out->shift_amt = 0;
    out->pudding = 0;
    out->conf = 0;
    out->imm16 = 0;
    out->disp = 0;

    out->opcode = (uint8_t)((raw & LDG_MISC_OPCODE_MASK) >> LDG_MISC_OPCODE_SHIFT);
    out->dst = (uint8_t)((raw & LDG_MISC_DST_MASK) >> LDG_MISC_DST_SHIFT);
    out->src = (uint8_t)((raw & LDG_MISC_SRC_MASK) >> LDG_MISC_SRC_SHIFT);
    out->conf = raw & LDG_MISC_CONF_MASK;
    out->mode = (uint8_t)((raw & LDG_MISC_MODE_MASK) >> LDG_MISC_MODE_BIT);
    out->arith = (uint8_t)((raw & LDG_MISC_ARITH_MASK) >> LDG_MISC_ARITH_BIT);
    out->carry = (uint8_t)((raw & LDG_MISC_CARRY_MASK) >> LDG_MISC_CARRY_BIT);
    out->sign = (uint8_t)((raw & LDG_MISC_SIGNED_MASK) >> LDG_MISC_SIGNED_BIT);
    out->remainder = (uint8_t)((raw & LDG_MISC_REMAINDER_MASK) >> LDG_MISC_REMAINDER_BIT);
    out->size = (uint8_t)((raw & LDG_MISC_SIZE_MASK) >> LDG_MISC_SIZE_SHIFT);
    out->cond = (uint8_t)((raw & LDG_MISC_COND_MASK) >> LDG_MISC_COND_SHIFT);
    out->imm16 = (ldg_word_t)(raw & LDG_MISC_IMM16_MASK);
    out->disp = (ldg_word_t)(raw & LDG_MISC_DISP_MASK);
    out->shift_amt = (uint8_t)(raw & LDG_MISC_SHIFT_AMT_MASK);

    return LDG_ERR_AOK;
}

// validate

static inline uint32_t ldg_misc_validate(const ldg_misc_decoded_t *decoded)
{
    if (LDG_UNLIKELY(!decoded)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(decoded->opcode > LDG_MISC_OP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->dst > LDG_MISC_REG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->src > LDG_MISC_REG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->opcode == LDG_MISC_OP_BCC && decoded->cond > LDG_MISC_COND_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->opcode == LDG_MISC_OP_SETCC && decoded->cond > LDG_MISC_COND_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->opcode == LDG_MISC_OP_LD && decoded->size > LDG_MISC_SIZE_WORD_U)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(decoded->opcode == LDG_MISC_OP_ST && decoded->size > LDG_MISC_SIZE_ST_WORD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static inline uint8_t ldg_misc_dst_writeback_is(uint8_t opcode)
{
    switch (opcode)
    {
        case LDG_MISC_OP_ST:
        case LDG_MISC_OP_JMP:
        case LDG_MISC_OP_JNZ:
        case LDG_MISC_OP_CALL:
        case LDG_MISC_OP_RET:
        case LDG_MISC_OP_BCC:
            return LDG_TRUTH_FALSE;

        default:
            return LDG_TRUTH_TRUE;
    }
}

// field extractors

static inline uint8_t ldg_misc_opcode_get(ldg_dword_t raw)
{
    return (uint8_t)((raw & LDG_MISC_OPCODE_MASK) >> LDG_MISC_OPCODE_SHIFT);
}

static inline uint8_t ldg_misc_dst_get(ldg_dword_t raw)
{
    return (uint8_t)((raw & LDG_MISC_DST_MASK) >> LDG_MISC_DST_SHIFT);
}

static inline uint8_t ldg_misc_src_get(ldg_dword_t raw)
{
    return (uint8_t)((raw & LDG_MISC_SRC_MASK) >> LDG_MISC_SRC_SHIFT);
}

static inline ldg_dword_t ldg_misc_conf_get(ldg_dword_t raw)
{
    return raw & LDG_MISC_CONF_MASK;
}

// convenience encoders

static inline ldg_dword_t ldg_misc_cph_encode(uint8_t dst, ldg_word_t imm16)
{
    return ldg_misc_encode(LDG_MISC_OP_CPH, dst, 0, LDG_MISC_MODE_MASK | (ldg_dword_t)imm16);
}

static inline ldg_dword_t ldg_misc_cpl_encode(uint8_t dst, ldg_word_t imm16)
{
    return ldg_misc_encode(LDG_MISC_OP_CPL, dst, 0, LDG_MISC_MODE_MASK | (ldg_dword_t)imm16);
}

static inline ldg_dword_t ldg_misc_shl_imm_encode(uint8_t dst, uint8_t amt)
{
    return ldg_misc_encode(LDG_MISC_OP_SHL, dst, LDG_MISC_SRC_UNUSED, LDG_MISC_MODE_MASK | (ldg_dword_t)(amt & LDG_MISC_SHIFT_AMT_MASK));
}

static inline ldg_dword_t ldg_misc_shr_imm_encode(uint8_t dst, uint8_t amt, uint8_t arith)
{
    ldg_dword_t conf = LDG_MISC_MODE_MASK | (ldg_dword_t)(amt & LDG_MISC_SHIFT_AMT_MASK);
    if (arith) { conf |= LDG_MISC_ARITH_MASK; }

    return ldg_misc_encode(LDG_MISC_OP_SHR, dst, LDG_MISC_SRC_UNUSED, conf);
}

static inline ldg_dword_t ldg_misc_ld_encode(uint8_t dst, uint8_t base, uint8_t size, ldg_word_t disp)
{
    ldg_dword_t conf = ((ldg_dword_t)size << LDG_MISC_SIZE_SHIFT) | (ldg_dword_t)disp;

    return ldg_misc_encode(LDG_MISC_OP_LD, dst, base, conf);
}

static inline ldg_dword_t ldg_misc_st_encode(uint8_t base, uint8_t src, uint8_t size, ldg_word_t disp)
{
    ldg_dword_t conf = ((ldg_dword_t)size << LDG_MISC_SIZE_SHIFT) | (ldg_dword_t)disp;

    return ldg_misc_encode(LDG_MISC_OP_ST, base, src, conf);
}

static inline ldg_dword_t ldg_misc_add_encode(uint8_t dst, uint8_t src, uint8_t use_carry)
{
    ldg_dword_t conf = 0;
    if (use_carry) { conf |= LDG_MISC_CARRY_MASK; }

    return ldg_misc_encode(LDG_MISC_OP_ADD, dst, src, conf);
}

static inline ldg_dword_t ldg_misc_sub_encode(uint8_t dst, uint8_t src, uint8_t use_carry)
{
    ldg_dword_t conf = 0;
    if (use_carry) { conf |= LDG_MISC_CARRY_MASK; }

    return ldg_misc_encode(LDG_MISC_OP_SUB, dst, src, conf);
}

static inline ldg_dword_t ldg_misc_div_encode(uint8_t dst, uint8_t src, uint8_t is_signed, uint8_t remainder)
{
    ldg_dword_t conf = 0;
    if (is_signed) { conf |= LDG_MISC_SIGNED_MASK; }

    if (remainder) { conf |= LDG_MISC_REMAINDER_MASK; }

    return ldg_misc_encode(LDG_MISC_OP_DIV, dst, src, conf);
}

static inline ldg_dword_t ldg_misc_bcc_encode(uint8_t lhs, uint8_t rhs, uint8_t cond, ldg_word_t word_off)
{
    ldg_dword_t conf = ((ldg_dword_t)cond << LDG_MISC_COND_SHIFT) | (ldg_dword_t)word_off;

    return ldg_misc_encode(LDG_MISC_OP_BCC, lhs, rhs, conf);
}

static inline ldg_dword_t ldg_misc_setcc_encode(uint8_t dst, uint8_t src, uint8_t cond)
{
    return ldg_misc_encode(LDG_MISC_OP_SETCC, dst, src, (ldg_dword_t)cond << LDG_MISC_COND_SHIFT);
}

static inline ldg_dword_t ldg_misc_mul_encode(uint8_t dst, uint8_t src)
{
    return ldg_misc_encode(LDG_MISC_OP_MUL, dst, src, LDG_MISC_CONF_NONE);
}

static inline ldg_dword_t ldg_misc_mulh_encode(uint8_t dst, uint8_t src, uint8_t signed_is)
{
    return ldg_misc_encode(LDG_MISC_OP_MULH, dst, src, signed_is ? LDG_MISC_MODE_MASK : LDG_MISC_CONF_NONE);
}

static inline ldg_dword_t ldg_misc_clz_encode(uint8_t dst, uint8_t src, uint8_t trailing_is)
{
    return ldg_misc_encode(LDG_MISC_OP_CLZ, dst, src, trailing_is ? LDG_MISC_MODE_MASK : LDG_MISC_CONF_NONE);
}

static inline ldg_dword_t ldg_misc_addi_encode(uint8_t dst, ldg_word_t imm16)
{
    return ldg_misc_encode(LDG_MISC_OP_ADDI, dst, LDG_MISC_DR0, (ldg_dword_t)imm16 & LDG_MISC_IMM16_MASK);
}

// disassembly

static inline uint64_t ldg_misc_str_append(char *buff, uint64_t pos, uint64_t buff_len, const char *text)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

    while (text[idx] != LDG_STR_TERM)
    {
        if (LDG_UNLIKELY(pos + LDG_STR_TERM_SIZE >= buff_len)) { return UINT64_MAX; }

        buff[pos++] = text[idx++];
    }

    return pos;
}

static inline uint64_t ldg_misc_char_append(char *buff, uint64_t pos, uint64_t buff_len, char ch)
{
    if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(pos + LDG_STR_TERM_SIZE >= buff_len)) { return UINT64_MAX; }

    buff[pos++] = ch;

    return pos;
}

static inline uint64_t ldg_misc_reg_append(char *buff, uint64_t pos, uint64_t buff_len, uint8_t reg)
{
    pos = ldg_misc_str_append(buff, pos, buff_len, "dr");
    if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

    if (reg >= LDG_BASE_DEC) { pos = ldg_misc_char_append(buff, pos, buff_len, (char)(LDG_MISC_ASCII_DIGIT_BASE + (reg / LDG_BASE_DEC))); }

    if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

    return ldg_misc_char_append(buff, pos, buff_len, (char)(LDG_MISC_ASCII_DIGIT_BASE + (reg % LDG_BASE_DEC)));
}

static inline uint64_t ldg_misc_hex16_append(char *buff, uint64_t pos, uint64_t buff_len, ldg_word_t val)
{
    uint32_t shift = LDG_MISC_IMM16_BITS;

    pos = ldg_misc_str_append(buff, pos, buff_len, "0x");

    while (shift >= LDG_NIBBLE_BITS)
    {
        if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

        shift -= LDG_NIBBLE_BITS;
        pos = ldg_misc_char_append(buff, pos, buff_len, LDG_MISC_HEX_DIGIT_SET[(val >> shift) & LDG_NIBBLE_MASK]);
    }

    return pos;
}

static inline uint64_t ldg_misc_dec_append(char *buff, uint64_t pos, uint64_t buff_len, uint32_t val)
{
    char tmp[LDG_MISC_DEC_DIGITS_MAX];
    uint32_t len = 0;

    if (val == 0) { return ldg_misc_char_append(buff, pos, buff_len, LDG_MISC_ASCII_DIGIT_BASE); }

    while (val > 0)
    {
        tmp[len++] = (char)(LDG_MISC_ASCII_DIGIT_BASE + (val % LDG_BASE_DEC));
        val /= LDG_BASE_DEC;
    }

    while (len > 0)
    {
        if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

        len--;
        pos = ldg_misc_char_append(buff, pos, buff_len, tmp[len]);
    }

    return pos;
}

static inline uint64_t ldg_misc_signed16_append(char *buff, uint64_t pos, uint64_t buff_len, ldg_word_t val)
{
    uint32_t mag = (uint32_t)val;

    if (val & LDG_MISC_SIGN16_MASK)
    {
        pos = ldg_misc_char_append(buff, pos, buff_len, LDG_MISC_NEG_SIGN);
        if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

        mag = LDG_MISC_IMM16_SPAN - (uint32_t)val;
    }

    return ldg_misc_dec_append(buff, pos, buff_len, mag);
}

static inline const char* ldg_misc_mnemonic_get(const ldg_misc_decoded_t *decoded)
{
    switch (decoded->opcode)
    {
        case LDG_MISC_OP_LD:
            return LDG_MISC_LD_MNEMONICS[decoded->size];

        case LDG_MISC_OP_ST:
            return LDG_MISC_ST_MNEMONICS[decoded->size];

        case LDG_MISC_OP_ADD:
            return decoded->carry ? "ADDC" : "ADD";

        case LDG_MISC_OP_SUB:
            return decoded->carry ? "SUBC" : "SUB";

        case LDG_MISC_OP_SHR:
            return decoded->arith ? "SHRA" : "SHR";

        case LDG_MISC_OP_DIV:
            if (decoded->sign) { return decoded->remainder ? "SREM" : "SDIV"; }

            return decoded->remainder ? "UREM" : "UDIV";

        case LDG_MISC_OP_MULH:
            return decoded->mode ? "MULHS" : "MULHU";

        case LDG_MISC_OP_CLZ:
            return decoded->mode ? "CTZ" : "CLZ";

        case LDG_MISC_OP_BCC:
            return "B";

        case LDG_MISC_OP_SETCC:
            return "SET";

        default:
            return LDG_MISC_MNEMONICS[decoded->opcode];
    }
}

static inline uint32_t ldg_misc_disasm(const ldg_misc_decoded_t *decoded, char *buff, uint64_t buff_len)
{
    uint8_t op = 0;
    const char *mnem = 0x0;
    uint64_t pos = 0;

    if (LDG_UNLIKELY(!decoded)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_len < LDG_MISC_DISASM_BUFF_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[0] = LDG_STR_TERM;

    op = decoded->opcode;
    if (LDG_UNLIKELY(op > LDG_MISC_OP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_misc_validate(decoded) != LDG_ERR_AOK)) { return LDG_ERR_FUNC_ARG_INVALID; }

    mnem = ldg_misc_mnemonic_get(decoded);
    pos = ldg_misc_str_append(buff, pos, buff_len, mnem);

    if (op == LDG_MISC_OP_BCC || op == LDG_MISC_OP_SETCC) { pos = ldg_misc_str_append(buff, pos, buff_len, LDG_MISC_COND_MNEMONICS[decoded->cond]); }

    if (op == LDG_MISC_OP_RET)
    {
        if (LDG_UNLIKELY(pos == UINT64_MAX)) { return LDG_ERR_STR_TRUNC; }

        buff[pos] = LDG_STR_TERM;

        return LDG_ERR_AOK;
    }

    pos = ldg_misc_char_append(buff, pos, buff_len, ' ');

    switch (op)
    {
        case LDG_MISC_OP_JMP:
        case LDG_MISC_OP_CALL:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            break;

        case LDG_MISC_OP_LD:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", [");
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->src);
            pos = ldg_misc_str_append(buff, pos, buff_len, " + ");
            pos = ldg_misc_signed16_append(buff, pos, buff_len, decoded->disp);
            pos = ldg_misc_char_append(buff, pos, buff_len, ']');
            break;

        case LDG_MISC_OP_ST:
            pos = ldg_misc_char_append(buff, pos, buff_len, '[');
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, " + ");
            pos = ldg_misc_signed16_append(buff, pos, buff_len, decoded->disp);
            pos = ldg_misc_str_append(buff, pos, buff_len, "], ");
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->src);
            break;

        case LDG_MISC_OP_CPH:
        case LDG_MISC_OP_CPL:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", ");
            if (decoded->mode) { pos = ldg_misc_hex16_append(buff, pos, buff_len, decoded->imm16); }
            else { pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->src); }

            break;

        case LDG_MISC_OP_SHL:
        case LDG_MISC_OP_SHR:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", ");
            if (decoded->mode) { pos = ldg_misc_dec_append(buff, pos, buff_len, decoded->shift_amt); }
            else { pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->src); }

            break;

        case LDG_MISC_OP_BCC:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", ");
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->src);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", ");
            pos = ldg_misc_signed16_append(buff, pos, buff_len, decoded->disp);
            break;

        case LDG_MISC_OP_ADDI:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", ");
            pos = ldg_misc_signed16_append(buff, pos, buff_len, decoded->imm16);
            break;

        default:
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->dst);
            pos = ldg_misc_str_append(buff, pos, buff_len, ", ");
            pos = ldg_misc_reg_append(buff, pos, buff_len, decoded->src);
            break;
    }

    if (LDG_UNLIKELY(pos == UINT64_MAX)) { return LDG_ERR_STR_TRUNC; }

    buff[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

#endif
