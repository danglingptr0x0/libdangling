#include <dangling/parse/parse.h>
#include <dangling/str/str.h>

int ldg_parse_streq_is(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (*a != *b) { return 0; }

        a++;
        b++;
    }

    return (*a == *b);
}

void ldg_parse_tokenize(const char *input, ldg_tok_arr_t *toks)
{
    size_t pos = 0;
    ldg_tok_t *tok = 0x0;
    size_t val_idx = 0;

    toks->cunt = 0;

    while (*input && toks->cunt < LDG_TOK_MAX)
    {
        while (ldg_char_space_is(*input))
        {
            input++;
            pos++;
        }

        if (!*input) { break; }

        tok = &toks->toks[toks->cunt];
        tok->pos = pos;
        val_idx = 0;

        if (ldg_char_alpha_is(*input))
        {
            tok->type = LDG_TOK_WORD;
            while (ldg_char_alpha_is(*input) && val_idx < LDG_TOK_LEN_MAX - 1)
            {
                tok->val[val_idx++] = *input++;
                pos++;
            }
        }
        else if (ldg_char_digit_is(*input))
        {
            tok->type = LDG_TOK_NUM;
            while ((ldg_char_digit_is(*input) || ldg_char_hex_is(*input) || *input == 'x' || *input == 'X') && val_idx < LDG_TOK_LEN_MAX - 1)
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
}
