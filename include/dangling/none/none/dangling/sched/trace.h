#ifndef LDG_SCHED_TRACE_H
#define LDG_SCHED_TRACE_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_SCHED_TRACE_RING_SIZE (4 * LDG_MIB)
#define LDG_SCHED_TRACE_LOCK_SPIN_MAX 65536
#define LDG_SCHED_TRACE_LOCK_FREE 0
#define LDG_SCHED_TRACE_LOCK_HELD 1
#define LDG_SCHED_TRACE_STEP 1

typedef enum ldg_sched_trace_type
{
    LDG_SCHED_TRACE_JOB_SUBMIT = 0,
    LDG_SCHED_TRACE_JOB_START,
    LDG_SCHED_TRACE_JOB_YIELD,
    LDG_SCHED_TRACE_JOB_END,
    LDG_SCHED_TRACE_EVENT_EMIT,
    LDG_SCHED_TRACE_ASTAR_PLAN,
    LDG_SCHED_TRACE_ASTAR_REPLAN,
    LDG_SCHED_TRACE_SVM_OBSERVE,
    LDG_SCHED_TRACE_PROC_CRASH,
    LDG_SCHED_TRACE_PROC_RESTART,
    LDG_SCHED_TRACE_MAX
} ldg_sched_trace_type_t;

typedef struct ldg_sched_trace_entry
{
    uint64_t tsc;
    uint32_t type;
    uint32_t data0;
    uint64_t data1;
} ldg_sched_trace_entry_t;

#define LDG_SCHED_TRACE_ENTRY_CUNT (LDG_SCHED_TRACE_RING_SIZE / sizeof(ldg_sched_trace_entry_t))

typedef struct ldg_sched_trace_ring
{
    ldg_sched_trace_entry_t *entries;
    volatile uint64_t hd;
    uint64_t cap;
    volatile uint32_t lock;
    uint8_t is_init;
    uint8_t pudding[35];
} LDG_ALIGNED ldg_sched_trace_ring_t;

LDG_EXPORT uint32_t ldg_sched_trace_init(ldg_sched_trace_ring_t *ring);
LDG_EXPORT uint32_t ldg_sched_trace_shutdown(ldg_sched_trace_ring_t *ring);
LDG_EXPORT uint32_t ldg_sched_trace_push(ldg_sched_trace_ring_t *ring, ldg_sched_trace_type_t type, uint32_t data0, uint64_t data1);
LDG_EXPORT uint32_t ldg_sched_trace_pop(ldg_sched_trace_ring_t *ring, ldg_sched_trace_entry_t *entry);
LDG_EXPORT uint64_t ldg_sched_trace_cunt_get(ldg_sched_trace_ring_t *ring);
LDG_EXPORT uint32_t ldg_sched_trace_clear(ldg_sched_trace_ring_t *ring);

#endif
