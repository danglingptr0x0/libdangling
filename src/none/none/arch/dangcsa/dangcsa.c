#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/arch/dangcsa/dangcsa.h>

#define DANGCSA_DEC_DIGITS_MAX 12
#define DANGCSA_HEX16_DIGIT_CUNT 4
#define DANGCSA_HEX_DIGIT_SET "0123456789abcdef"
#define DANGCSA_ASCII_DIGIT_BASE '0'
#define DANGCSA_NEG_SIGN '-'
#define DANGCSA_SIGN32_MASK 0x80000000U
#define DANGCSA_LD_SIGNED_QWORD 7
#define DANGCSA_SH_WORD_SHL 0
#define DANGCSA_SH_WORD_SHR 1
#define DANGCSA_SH_WORD_SHRA 2
#define DANGCSA_MUL_WORD_HU 0
#define DANGCSA_MUL_WORD_HS 1
#define DANGCSA_FLAG_BIT_SHIFT 1
#define DANGCSA_WORD_SEP " "
#define DANGCSA_OP_SEP ", "
#define DANGCSA_MEM_OPEN "["
#define DANGCSA_MEM_DISP " + "
#define DANGCSA_MEM_CLOSE "]"
#define DANGCSA_HEX_PREFIX "0x"
#define DANGCSA_WORD_I "i"
#define DANGCSA_WORD_C "c"
#define DANGCSA_WORD_T "t"
#define DANGCSA_WORD_Q "q"
#define DANGCSA_WORD_REL "rel"

const char LDG_DANGCSA_MNEMONIC[LDG_DANGCSA_OP_CUNT][LDG_DANGCSA_MNEMONIC_CAP] = {
    "cpy", "ld", "st", "add", "sub", "and", "or", "xor", "sh", "mul", "div", "clz", "jmp", "setcc", "call", "ret"
};

const char LDG_DANGCSA_REG_NAME[LDG_DANGCSA_REG_CUNT][LDG_DANGCSA_REG_NAME_CAP] = {
    "dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7", "dr8", "dr9", "dr10", "dr11", "dr12", "dr13", "dr14", "dr15", "tdr0", "tdr1", "tdr2", "tdr3", "tdr4", "tdr5", "loc", "sp"
};

const char LDG_DANGCSA_LD_WORD[LDG_DANGCSA_LD_WORD_CUNT][LDG_DANGCSA_LD_WORD_CAP] = {
    "bu", "bs", "wu", "ws", "du", "ds", "q", ""
};

const char LDG_DANGCSA_ST_WORD[LDG_DANGCSA_ST_WORD_CUNT][LDG_DANGCSA_ST_WORD_CAP] = {
    "b", "w", "d", "q"
};

const char LDG_DANGCSA_DIV_WORD[LDG_DANGCSA_DIV_WORD_CUNT][LDG_DANGCSA_DIV_WORD_CAP] = {
    "u", "ur", "s", "sr"
};

const char LDG_DANGCSA_SH_WORD[LDG_DANGCSA_SH_WORD_CUNT][LDG_DANGCSA_SH_WORD_CAP] = {
    "l", "r", "ra"
};

const char LDG_DANGCSA_MUL_WORD[LDG_DANGCSA_MUL_WORD_CUNT][LDG_DANGCSA_MUL_WORD_CAP] = {
    "hu", "hs"
};

const char LDG_DANGCSA_COND_WORD[LDG_DANGCSA_COND_WORD_CUNT][LDG_DANGCSA_COND_WORD_CAP] = {
    "eq", "ne", "lt", "ge", "ltu", "geu", "al", ""
};

static uint64_t disasm_char_append(char *buff, uint64_t pos, uint64_t buff_len, char ch)
{
    if (LDG_UNLIKELY(pos == UINT64_MAX)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(pos + LDG_STR_TERM_SIZE >= buff_len)) { return UINT64_MAX; }

    buff[pos] = ch;

    return pos + LDG_STR_TERM_SIZE;
}

static uint64_t disasm_str_append(char *buff, uint64_t pos, uint64_t buff_len, const char *text)
{
    uint64_t idx = 0;

    while (pos != UINT64_MAX && text[idx] != LDG_STR_TERM)
    {
        pos = disasm_char_append(buff, pos, buff_len, text[idx]);
        idx++;
    }

    return pos;
}

static uint64_t disasm_dec_append(char *buff, uint64_t pos, uint64_t buff_len, uint32_t val)
{
    char tmp[DANGCSA_DEC_DIGITS_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0;

    if (val == 0) { return disasm_char_append(buff, pos, buff_len, DANGCSA_ASCII_DIGIT_BASE); }

    while (val > 0)
    {
        tmp[len] = (char)(DANGCSA_ASCII_DIGIT_BASE + (val % LDG_BASE_DEC));
        val /= LDG_BASE_DEC;
        len++;
    }

    while (len > 0)
    {
        len--;
        pos = disasm_char_append(buff, pos, buff_len, tmp[len]);
    }

    return pos;
}

static uint64_t disasm_signed_append(char *buff, uint64_t pos, uint64_t buff_len, uint32_t raw)
{
    uint32_t mag = raw;

    if ((raw & DANGCSA_SIGN32_MASK) != LDG_TRUTH_FALSE)
    {
        pos = disasm_char_append(buff, pos, buff_len, DANGCSA_NEG_SIGN);
        mag = ~raw + (uint32_t)LDG_TRUTH_TRUE;
    }

    return disasm_dec_append(buff, pos, buff_len, mag);
}

static uint64_t disasm_hex16_append(char *buff, uint64_t pos, uint64_t buff_len, uint32_t val)
{
    const char *digit = DANGCSA_HEX_DIGIT_SET;
    uint32_t shift = DANGCSA_HEX16_DIGIT_CUNT * LDG_NIBBLE_BITS;

    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_HEX_PREFIX);

    while (shift >= LDG_NIBBLE_BITS)
    {
        shift -= LDG_NIBBLE_BITS;
        pos = disasm_char_append(buff, pos, buff_len, digit[(val >> shift) & LDG_NIBBLE_MASK]);
    }

    return pos;
}

static uint64_t disasm_word_append(char *buff, uint64_t pos, uint64_t buff_len, const char *word)
{
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);

    return disasm_str_append(buff, pos, buff_len, word);
}

static uint64_t disasm_pair_append(char *buff, uint64_t pos, uint64_t buff_len, uint8_t dst, uint8_t src)
{
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[dst]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

    return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[src]);
}

static uint64_t disasm_mem_append(char *buff, uint64_t pos, uint64_t buff_len, uint8_t base, uint32_t disp)
{
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_MEM_OPEN);
    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[base]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_MEM_DISP);
    pos = disasm_signed_append(buff, pos, buff_len, disp);

    return disasm_str_append(buff, pos, buff_len, DANGCSA_MEM_CLOSE);
}

static uint64_t disasm_cpy_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    if (decoded->f1 == LDG_TRUTH_TRUE)
    {
        pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_Q);
        pos = disasm_char_append(buff, pos, buff_len, (char)(DANGCSA_ASCII_DIGIT_BASE + decoded->quarter));
    }

    if (decoded->f0 == LDG_TRUTH_TRUE) { pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_I); }

    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->dst]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

    if (decoded->f0 != LDG_TRUTH_TRUE) { return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->src]); }

    if (decoded->f1 == LDG_TRUTH_TRUE) { return disasm_hex16_append(buff, pos, buff_len, decoded->imm32 & LDG_DANGCSA_QUARTER_VAL_MASK); }

    return disasm_signed_append(buff, pos, buff_len, decoded->imm32);
}

static uint64_t disasm_ld_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    uint32_t word = ((uint32_t)decoded->width << DANGCSA_FLAG_BIT_SHIFT) | decoded->f2;

    if (word == DANGCSA_LD_SIGNED_QWORD) { return UINT64_MAX; }

    pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_LD_WORD[word]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->dst]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

    return disasm_mem_append(buff, pos, buff_len, decoded->src, decoded->imm32);
}

static uint64_t disasm_st_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_ST_WORD[decoded->width]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
    pos = disasm_mem_append(buff, pos, buff_len, decoded->dst, decoded->imm32);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

    return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->src]);
}

static uint64_t disasm_add_sub_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    if (decoded->f0 == LDG_TRUTH_TRUE) { pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_C); }

    if (decoded->f1 == LDG_TRUTH_TRUE) { pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_I); }

    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->dst]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

    if (decoded->f1 == LDG_TRUTH_TRUE) { return disasm_signed_append(buff, pos, buff_len, decoded->imm32); }

    return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->src]);
}

static uint64_t disasm_sh_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    uint32_t word = DANGCSA_SH_WORD_SHL;

    if (decoded->f0 == LDG_TRUTH_TRUE) { word = (decoded->f1 == LDG_TRUTH_TRUE) ? DANGCSA_SH_WORD_SHRA : DANGCSA_SH_WORD_SHR; }

    pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_SH_WORD[word]);

    if (decoded->f2 == LDG_TRUTH_TRUE) { pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_I); }

    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->dst]);
    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

    if (decoded->f2 == LDG_TRUTH_TRUE) { return disasm_dec_append(buff, pos, buff_len, decoded->shift_amt); }

    return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->src]);
}

static uint64_t disasm_mul_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    if (decoded->f0 == LDG_TRUTH_TRUE) { pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_MUL_WORD[(decoded->f1 == LDG_TRUTH_TRUE) ? DANGCSA_MUL_WORD_HS : DANGCSA_MUL_WORD_HU]); }

    return disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);
}

static uint64_t disasm_div_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    uint32_t word = ((uint32_t)decoded->f0 << DANGCSA_FLAG_BIT_SHIFT) | decoded->f1;

    pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_DIV_WORD[word]);

    return disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);
}

static uint64_t disasm_clz_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    if (decoded->f0 == LDG_TRUTH_TRUE) { pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_T); }

    return disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);
}

static uint64_t disasm_jmp_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    if (decoded->cond == LDG_DANGCSA_COND_RESERVED) { return UINT64_MAX; }

    if (decoded->f3 != LDG_TRUTH_TRUE)
    {
        if (decoded->cond == LDG_DANGCSA_COND_ALWAYS)
        {
            pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);

            return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->dst]);
        }

        pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_COND_WORD[decoded->cond]);

        return disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);
    }

    pos = disasm_word_append(buff, pos, buff_len, DANGCSA_WORD_REL);
    pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_COND_WORD[decoded->cond]);

    if (decoded->cond != LDG_DANGCSA_COND_ALWAYS)
    {
        pos = disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);
        pos = disasm_str_append(buff, pos, buff_len, DANGCSA_OP_SEP);

        return disasm_signed_append(buff, pos, buff_len, decoded->imm32);
    }

    pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);

    return disasm_signed_append(buff, pos, buff_len, decoded->imm32);
}

static uint64_t disasm_setcc_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    if (decoded->cond >= LDG_DANGCSA_COND_ALWAYS) { return UINT64_MAX; }

    pos = disasm_word_append(buff, pos, buff_len, LDG_DANGCSA_COND_WORD[decoded->cond]);

    return disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);
}

static uint64_t disasm_op_append(char *buff, uint64_t pos, uint64_t buff_len, const ldg_dangcsa_decoded_t *decoded)
{
    switch (decoded->opcode)
    {
        case LDG_DANGCSA_OP_CPY:
            return disasm_cpy_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_LD:
            return disasm_ld_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_ST:
            return disasm_st_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_ADD:
        case LDG_DANGCSA_OP_SUB:
            return disasm_add_sub_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_AND:
        case LDG_DANGCSA_OP_OR:
        case LDG_DANGCSA_OP_XOR:
            return disasm_pair_append(buff, pos, buff_len, decoded->dst, decoded->src);

        case LDG_DANGCSA_OP_SH:
            return disasm_sh_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_MUL:
            return disasm_mul_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_DIV:
            return disasm_div_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_CLZ:
            return disasm_clz_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_JMP:
            return disasm_jmp_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_SETCC:
            return disasm_setcc_append(buff, pos, buff_len, decoded);

        case LDG_DANGCSA_OP_CALL:
            pos = disasm_str_append(buff, pos, buff_len, DANGCSA_WORD_SEP);
            return disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_REG_NAME[decoded->dst]);

        default:
            return pos;
    }
}

uint32_t ldg_dangcsa_disasm(const ldg_dangcsa_decoded_t *decoded, char *buff, uint64_t buff_len)
{
    uint64_t pos = 0;

    if (LDG_UNLIKELY(!decoded || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_len < LDG_DANGCSA_DISASM_BUFF_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[pos] = LDG_STR_TERM;

    if (LDG_UNLIKELY(ldg_dangcsa_validate(decoded) != LDG_ERR_AOK)) { return LDG_ERR_FUNC_ARG_INVALID; }

    pos = disasm_str_append(buff, pos, buff_len, LDG_DANGCSA_MNEMONIC[decoded->opcode]);
    pos = disasm_op_append(buff, pos, buff_len, decoded);

    if (LDG_UNLIKELY(pos == UINT64_MAX))
    {
        buff[0] = LDG_STR_TERM;
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    buff[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}
