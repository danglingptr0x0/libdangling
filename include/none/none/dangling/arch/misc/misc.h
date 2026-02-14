#ifndef LDG_ARCH_MISC_MISC_H
#define LDG_ARCH_MISC_MISC_H

#include <stdint.h>
#include <dangling/core/types.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>

// opcodes
#define LDG_MISC_OP_CPY 0x0
#define LDG_MISC_OP_LDD 0x1
#define LDG_MISC_OP_STD 0x2
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
#define LDG_MISC_OP_CUNT 16
#define LDG_MISC_OP_MAX 0xF

// registers
#define LDG_MISC_DR0 0
#define LDG_MISC_DR1 1
#define LDG_MISC_DR2 2
#define LDG_MISC_DR3 3
#define LDG_MISC_DR4 4
#define LDG_MISC_DR5 5
#define LDG_MISC_DR6 6
#define LDG_MISC_DR7 7
#define LDG_MISC_LOC 6
#define LDG_MISC_SP 7
#define LDG_MISC_REG_CUNT 8
#define LDG_MISC_REG_MAX 7

// field masks and shifts
#define LDG_MISC_OPCODE_SHIFT 28
#define LDG_MISC_OPCODE_MASK 0xF0000000
#define LDG_MISC_DST_SHIFT 25
#define LDG_MISC_DST_MASK 0x0E000000
#define LDG_MISC_SRC_SHIFT 22
#define LDG_MISC_SRC_MASK 0x01C00000
#define LDG_MISC_CONFIG_MASK 0x003FFFFF

// mode and arithmetic shift bits
#define LDG_MISC_MODE_BIT 21
#define LDG_MISC_MODE_MASK (1U << LDG_MISC_MODE_BIT)
#define LDG_MISC_ARITH_BIT 20
#define LDG_MISC_ARITH_MASK (1U << LDG_MISC_ARITH_BIT)

// immediate fields
#define LDG_MISC_IMM16_MASK 0x0000FFFF
#define LDG_MISC_SHIFT_AMT_MASK 0x0000003F

// architecture constants
#define LDG_MISC_WORD_SIZE 4
#define LDG_MISC_INSN_WIDTH 32
#define LDG_MISC_ALIGNMENT 4
#define LDG_MISC_RESET_VEC 0x00000000

// memory map
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

// disasm
#define LDG_MISC_DISASM_BUFF_MIN 32

typedef struct ldg_misc_decoded
{
    uint8_t opcode;
    uint8_t dst;
    uint8_t src;
    uint8_t mode;
    uint8_t arith;
    uint8_t pudding[3];
    ldg_dword_t config;
    ldg_word_t imm16;
    uint8_t shift_amt;
    uint8_t pudding2;
} ldg_misc_decoded_t;

// mnemonics

extern const char *const LDG_MISC_MNEMONICS[LDG_MISC_OP_CUNT];

// encode

static inline ldg_dword_t ldg_misc_encode(uint8_t opcode, uint8_t dst, uint8_t src, ldg_dword_t config)
{
    return ((ldg_dword_t)(opcode & 0xF) << LDG_MISC_OPCODE_SHIFT) |
           ((ldg_dword_t)(dst & 0x7) << LDG_MISC_DST_SHIFT) |
           ((ldg_dword_t)(src & 0x7) << LDG_MISC_SRC_SHIFT) |
           (config & LDG_MISC_CONFIG_MASK);
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
    out->config = 0;
    out->imm16 = 0;
    out->shift_amt = 0;

    out->opcode = (uint8_t)((raw & LDG_MISC_OPCODE_MASK) >> LDG_MISC_OPCODE_SHIFT);
    out->dst = (uint8_t)((raw & LDG_MISC_DST_MASK) >> LDG_MISC_DST_SHIFT);
    out->src = (uint8_t)((raw & LDG_MISC_SRC_MASK) >> LDG_MISC_SRC_SHIFT);
    out->config = raw & LDG_MISC_CONFIG_MASK;
    out->mode = (uint8_t)((raw >> LDG_MISC_MODE_BIT) & 1);
    out->arith = (uint8_t)((raw >> LDG_MISC_ARITH_BIT) & 1);
    out->imm16 = (ldg_word_t)(raw & LDG_MISC_IMM16_MASK);
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

    return LDG_ERR_AOK;
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

static inline ldg_dword_t ldg_misc_config_get(ldg_dword_t raw)
{
    return raw & LDG_MISC_CONFIG_MASK;
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
    return ldg_misc_encode(LDG_MISC_OP_SHL, dst, 0, LDG_MISC_MODE_MASK | (ldg_dword_t)(amt & 0x3F));
}

static inline ldg_dword_t ldg_misc_shr_imm_encode(uint8_t dst, uint8_t amt, uint8_t arith)
{
    ldg_dword_t flags = LDG_MISC_MODE_MASK | (ldg_dword_t)(amt & 0x3F);
    if (arith) { flags |= LDG_MISC_ARITH_MASK; }

    return ldg_misc_encode(LDG_MISC_OP_SHR, dst, 0, flags);
}

// disassembly

static inline uint32_t ldg_misc_disasm(const ldg_misc_decoded_t *decoded, char *buff, uint64_t buff_len)
{
    uint8_t op = 0;
    const char *mnem = 0x0;
    uint64_t pos = 0;
    uint64_t i = 0;
    ldg_word_t imm = 0;
    uint8_t amt = 0;

    if (LDG_UNLIKELY(!decoded)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_len < LDG_MISC_DISASM_BUFF_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[0] = '\0';

    op = decoded->opcode;
    if (LDG_UNLIKELY(op > LDG_MISC_OP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    mnem = LDG_MISC_MNEMONICS[op];

    for (i = 0; mnem[i] != '\0' && pos < buff_len - 1; i++) { buff[pos++] = mnem[i]; }

    switch (op)
    {
        case LDG_MISC_OP_RET:
            buff[pos] = '\0';
            return LDG_ERR_AOK;

        case LDG_MISC_OP_JMP:
        case LDG_MISC_OP_CALL:
            if (pos + 5 >= buff_len) { return LDG_ERR_STR_TRUNC; }

            buff[pos++] = ' ';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->dst);
            buff[pos] = '\0';
            return LDG_ERR_AOK;

        case LDG_MISC_OP_LDD:
            if (pos + 13 >= buff_len) { return LDG_ERR_STR_TRUNC; }

            buff[pos++] = ' ';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->dst);
            buff[pos++] = ',';
            buff[pos++] = ' ';
            buff[pos++] = '[';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->src);
            buff[pos++] = ']';
            buff[pos] = '\0';
            return LDG_ERR_AOK;

        case LDG_MISC_OP_STD:
            if (pos + 13 >= buff_len) { return LDG_ERR_STR_TRUNC; }

            buff[pos++] = ' ';
            buff[pos++] = '[';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->dst);
            buff[pos++] = ']';
            buff[pos++] = ',';
            buff[pos++] = ' ';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->src);
            buff[pos] = '\0';
            return LDG_ERR_AOK;

        case LDG_MISC_OP_CPH:
        case LDG_MISC_OP_CPL:
            if (decoded->mode)
            {
                if (pos + 14 >= buff_len) { return LDG_ERR_STR_TRUNC; }

                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->dst);
                buff[pos++] = ',';
                buff[pos++] = ' ';
                buff[pos++] = '0';
                buff[pos++] = 'x';
                imm = decoded->imm16;
                buff[pos++] = "0123456789ABCDEF"[(imm >> 12) & 0xF];
                buff[pos++] = "0123456789ABCDEF"[(imm >> 8) & 0xF];
                buff[pos++] = "0123456789ABCDEF"[(imm >> 4) & 0xF];
                buff[pos++] = "0123456789ABCDEF"[imm & 0xF];
                buff[pos] = '\0';
            }
            else
            {
                if (pos + 10 >= buff_len) { return LDG_ERR_STR_TRUNC; }

                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->dst);
                buff[pos++] = ',';
                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->src);
                buff[pos] = '\0';
            }

            return LDG_ERR_AOK;

        case LDG_MISC_OP_SHL:
            if (decoded->mode)
            {
                if (pos + 9 >= buff_len) { return LDG_ERR_STR_TRUNC; }

                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->dst);
                buff[pos++] = ',';
                buff[pos++] = ' ';
                amt = decoded->shift_amt;
                if (amt >= LDG_BASE_DEC) { buff[pos++] = (char)('0' + (amt / LDG_BASE_DEC)); }

                buff[pos++] = (char)('0' + (amt % LDG_BASE_DEC));
                buff[pos] = '\0';
            }
            else
            {
                if (pos + 10 >= buff_len) { return LDG_ERR_STR_TRUNC; }

                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->dst);
                buff[pos++] = ',';
                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->src);
                buff[pos] = '\0';
            }

            return LDG_ERR_AOK;

        case LDG_MISC_OP_SHR:
            if (decoded->mode)
            {
                if (pos + 9 >= buff_len) { return LDG_ERR_STR_TRUNC; }

                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->dst);
                buff[pos++] = ',';
                buff[pos++] = ' ';
                amt = decoded->shift_amt;
                if (amt >= LDG_BASE_DEC) { buff[pos++] = (char)('0' + (amt / LDG_BASE_DEC)); }

                buff[pos++] = (char)('0' + (amt % LDG_BASE_DEC));
                buff[pos] = '\0';
            }
            else
            {
                if (pos + 10 >= buff_len) { return LDG_ERR_STR_TRUNC; }

                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->dst);
                buff[pos++] = ',';
                buff[pos++] = ' ';
                buff[pos++] = 'd';
                buff[pos++] = 'r';
                buff[pos++] = (char)('0' + decoded->src);
                buff[pos] = '\0';
            }

            return LDG_ERR_AOK;

        default:
            if (pos + 10 >= buff_len) { return LDG_ERR_STR_TRUNC; }

            buff[pos++] = ' ';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->dst);
            buff[pos++] = ',';
            buff[pos++] = ' ';
            buff[pos++] = 'd';
            buff[pos++] = 'r';
            buff[pos++] = (char)('0' + decoded->src);
            buff[pos] = '\0';
            return LDG_ERR_AOK;
    }
}

#endif
