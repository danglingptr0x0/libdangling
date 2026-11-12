#ifndef LDG_ARCH_AMD64_CALL_H
#define LDG_ARCH_AMD64_CALL_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_CALL_ARG_MAX 8

typedef struct ldg_call_frame
{
    uint64_t addr;
    uint64_t arg[LDG_CALL_ARG_MAX];
    uint64_t ret;
    uint64_t fp_ret;
    uint32_t arg_cunt;
    uint32_t fp_mask;
} ldg_call_frame_t;

LDG_EXPORT uint32_t ldg_call_frame_run(ldg_call_frame_t *frame);

#endif
