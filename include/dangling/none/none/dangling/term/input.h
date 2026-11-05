#ifndef LDG_TERM_INPUT_H
#define LDG_TERM_INPUT_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/term/vt.h>

#define LDG_TERM_INPUT_EVENT_CAP_DEFAULT 256U
#define LDG_TERM_INPUT_EVENT_CAP_MAX 65536U
#define LDG_TERM_INPUT_CODEPOINT_TRACK_MAX 128U
#define LDG_TERM_INPUT_KITTY_FLAGS_DEFAULT (LDG_TERM_VT_KBD_DISAMBIGUATE | LDG_TERM_VT_KBD_EVENT_TYPE | LDG_TERM_VT_KBD_ALL_ESC | LDG_TERM_VT_KBD_TEXT)
#define LDG_TERM_INPUT_ESC_WAIT_DEFAULT 50U

typedef struct ldg_term_input_conf
{
    uint32_t event_cap;
    uint32_t kitty_flags;
    uint32_t esc_wait;
    uint8_t kitty_en;
    uint8_t pudding[3];
} ldg_term_input_conf_t;

typedef struct ldg_term_input ldg_term_input_t;

LDG_EXPORT uint32_t ldg_term_input_conf_default_get(ldg_term_input_conf_t *out);
LDG_EXPORT uint32_t ldg_term_input_create(const ldg_term_input_conf_t *conf, ldg_term_input_t **out);
LDG_EXPORT uint32_t ldg_term_input_destroy(ldg_term_input_t *input);
LDG_EXPORT uint32_t ldg_term_input_poll(ldg_term_input_t *input);
LDG_EXPORT uint32_t ldg_term_input_event_next(ldg_term_input_t *input, ldg_term_key_event_t *out);
LDG_EXPORT uint32_t ldg_term_input_dropped_cunt_get(const ldg_term_input_t *input, uint64_t *out);
LDG_EXPORT uint8_t ldg_term_input_key_down_is(const ldg_term_input_t *input, uint8_t key);
LDG_EXPORT uint8_t ldg_term_input_codepoint_down_is(const ldg_term_input_t *input, uint32_t codepoint);
LDG_EXPORT uint8_t ldg_term_input_release_report_is(const ldg_term_input_t *input);

#endif
