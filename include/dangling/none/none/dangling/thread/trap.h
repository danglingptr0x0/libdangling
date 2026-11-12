#ifndef LDG_THREAD_TRAP_H
#define LDG_THREAD_TRAP_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/thread/ref.h>

#define LDG_THREAD_TRAP_SLOT_MAX 4
#define LDG_THREAD_TRAP_SLOT_NONE UINT32_MAX
#define LDG_THREAD_TRAP_RFLAGS_STEP 0x100ULL
#define LDG_THREAD_TRAP_RFLAGS_RESUME 0x10000ULL

typedef enum ldg_thread_trap_kind
{
    LDG_THREAD_TRAP_BREAK = 0,
    LDG_THREAD_TRAP_STEP,
    LDG_THREAD_TRAP_ILLEGAL,
    LDG_THREAD_TRAP_KIND_CUNT
} ldg_thread_trap_kind_t;

typedef uint32_t (*ldg_thread_trap_cb_t)(ldg_thread_trap_kind_t kind, uint64_t thread_id, uint32_t slot, ldg_thread_regs_t *regs, void *user);

LDG_EXPORT uint32_t ldg_thread_trap_init(ldg_thread_trap_cb_t cb, void *user);
LDG_EXPORT uint32_t ldg_thread_trap_shutdown(void);
LDG_EXPORT uint32_t ldg_thread_trap_break_arm(ldg_thread_ref_t *thread, uint32_t slot, uint64_t addr);
LDG_EXPORT uint32_t ldg_thread_trap_break_disarm(ldg_thread_ref_t *thread, uint32_t slot);

#endif
