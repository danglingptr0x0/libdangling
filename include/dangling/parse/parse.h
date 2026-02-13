#ifndef LDG_PARSE_PARSE_H
#define LDG_PARSE_PARSE_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_TOK_MAX 16
#define LDG_TOK_LEN_MAX 32

typedef enum ldg_tok_type
{
    LDG_TOK_WORD,
    LDG_TOK_NUM,
    LDG_TOK_SYMBOL,
    LDG_TOK_END
} ldg_tok_type_t;

typedef struct ldg_tok
{
    ldg_tok_type_t type;
    char val[LDG_TOK_LEN_MAX];
    uint64_t pos;
} ldg_tok_t;

typedef struct ldg_tok_arr
{
    ldg_tok_t toks[LDG_TOK_MAX];
    uint64_t cunt;
} ldg_tok_arr_t;

typedef void (*ldg_cmd_handler_t)(ldg_tok_arr_t *toks);

typedef struct ldg_cmd_entry
{
    const char *name;
    ldg_cmd_handler_t handler;
    const char *help;
} ldg_cmd_entry_t;

LDG_EXPORT void ldg_parse_tokenize(const char *input, ldg_tok_arr_t *toks);
LDG_EXPORT uint8_t ldg_parse_streq_is(const char *a, const char *b);

#endif
