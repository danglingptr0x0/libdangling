#ifndef LDG_TERM_KEY_H
#define LDG_TERM_KEY_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/term/vt.h>

LDG_EXPORT uint32_t ldg_term_key_decode(const ldg_byte_t *src, uint64_t len, uint8_t final_is, ldg_term_key_event_t *out, uint64_t *out_used);

#endif
