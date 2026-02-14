#include <dangling/parse/parse.h>
#include <dangling/core/err.h>
#include <dangling/str/str.h>

uint8_t ldg_parse_streq_is(const char *a, const char *b)
{
    if (LDG_UNLIKELY(!a || !b)) { return 0; }

    while (*a && *b)
    {
        if (*a != *b) { return 0; }

        a++;
        b++;
    }

    return (*a == *b);
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
        while (ldg_char_space_is((uint8_t)*input))
        {
            input++;
            pos++;
        }

        if (!*input) { break; }

        tok = &toks->toks[toks->cunt];
        tok->pos = pos;
        val_idx = 0;

        if (ldg_char_alpha_is((uint8_t)*input))
        {
            tok->type = LDG_TOK_WORD;
            while (ldg_char_alpha_is((uint8_t)*input) && val_idx < LDG_TOK_LEN_MAX - 1)
            {
                tok->val[val_idx++] = *input++;
                pos++;
            }
        }
        else if (ldg_char_digit_is((uint8_t)*input))
        {
            tok->type = LDG_TOK_NUM;
            while ((ldg_char_digit_is((uint8_t)*input) || ldg_char_hex_is((uint8_t)*input) || *input == 'x' || *input == 'X') && val_idx < LDG_TOK_LEN_MAX - 1)
            {
                tok->val[val_idx++] = *input++;
                pos++;
            }
        }
        else
        {
            tok->type = LDG_TOK_SYMBOL;
            tok->val[val_idx++] = *input++;
            pos++;
        }

        tok->val[val_idx] = '\0';
        tok->type = val_idx > 0 ? tok->type : LDG_TOK_END;
        toks->cunt++;
    }

    return LDG_ERR_AOK;
}
