#include <dangling/parse/parse.h>
#include <dangling/core/err.h>
#include <dangling/str/str.h>

uint8_t ldg_parse_streq_is(const char *lhs, const char *rhs)
{
    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (*lhs && *rhs)
    {
        if (*lhs != *rhs) { return LDG_TRUTH_FALSE; }

        lhs++;
        rhs++;
    }

    if (*lhs != *rhs) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_parse_tokenize(const char *input, ldg_tok_arr_t *toks)
{
    uint64_t pos = 0;
    ldg_tok_t *tok = 0x0;
    uint64_t val_idx = 0;

    if (LDG_UNLIKELY(!input || !toks)) { return LDG_ERR_FUNC_ARG_NULL; }

    toks->cunt = 0;

    while (*input && toks->cunt < LDG_TOK_MAX)
    {
        while (ldg_char_space_is((uint8_t)*input) == LDG_TRUTH_TRUE)
        {
            input++;
            pos++;
        }

        if (!*input) { break; }

        tok = &toks->toks[toks->cunt];
        tok->pos = pos;
        val_idx = 0;

        if (ldg_char_alpha_is((uint8_t)*input) == LDG_TRUTH_TRUE)
        {
            tok->type = LDG_TOK_WORD;
            while (ldg_char_alpha_is((uint8_t)*input) == LDG_TRUTH_TRUE && val_idx < LDG_TOK_LEN_MAX - 1)
            {
                tok->val[val_idx++] = *input++;
                pos++;
            }
        }
        else if (ldg_char_digit_is((uint8_t)*input) == LDG_TRUTH_TRUE)
        {
            tok->type = LDG_TOK_NUM;
            while ((ldg_char_digit_is((uint8_t)*input) == LDG_TRUTH_TRUE || ldg_char_hex_is((uint8_t)*input) == LDG_TRUTH_TRUE || *input == 'x' || *input == 'X') && val_idx < LDG_TOK_LEN_MAX - 1)
            {
                tok->val[val_idx++] = *input++;
                pos++;
            }
        }
        else
        {
            tok->type = LDG_TOK_SYM;
            tok->val[val_idx++] = *input++;
            pos++;
        }

        tok->val[val_idx] = '\0';
        tok->type = val_idx > 0 ? tok->type : LDG_TOK_END;
        toks->cunt++;
    }

    return LDG_ERR_AOK;
}
