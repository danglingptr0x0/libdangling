#ifndef LDG_THREAD_SYNC_H
#define LDG_THREAD_SYNC_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MUT_IMPL_SIZE 48
#define LDG_COND_IMPL_SIZE 56

typedef struct ldg_mut
{
    uint64_t impl[LDG_MUT_IMPL_SIZE / 8];
    uint8_t is_shared;
    uint8_t is_init;
    uint8_t pudding[6];
} ldg_mut_t;

LDG_EXPORT uint32_t ldg_mut_init(ldg_mut_t *m, uint8_t shared);
LDG_EXPORT uint32_t ldg_mut_destroy(ldg_mut_t *m);
LDG_EXPORT uint32_t ldg_mut_lock(ldg_mut_t *m);
LDG_EXPORT uint32_t ldg_mut_unlock(ldg_mut_t *m);
LDG_EXPORT uint32_t ldg_mut_trylock(ldg_mut_t *m);

typedef struct ldg_cond
{
    uint64_t impl[LDG_COND_IMPL_SIZE / 8];
    uint8_t is_shared;
    uint8_t is_init;
    uint8_t is_monotonic;
    uint8_t pudding[5];
} ldg_cond_t;

LDG_EXPORT uint32_t ldg_cond_init(ldg_cond_t *c, uint8_t shared);
LDG_EXPORT uint32_t ldg_cond_destroy(ldg_cond_t *c);
LDG_EXPORT uint32_t ldg_cond_wait(ldg_cond_t *c, ldg_mut_t *m);
LDG_EXPORT uint32_t ldg_cond_timedwait(ldg_cond_t *c, ldg_mut_t *m, uint64_t timeout_ms);
LDG_EXPORT uint32_t ldg_cond_sig(ldg_cond_t *c);
LDG_EXPORT uint32_t ldg_cond_bcast(ldg_cond_t *c);

#define LDG_SEM_NAME_MAX 32

typedef struct ldg_sem
{
    void *handle;
    char name[LDG_SEM_NAME_MAX];
    uint8_t is_owner;
    uint8_t is_init;
    uint8_t pudding[6];
} ldg_sem_t;

LDG_EXPORT uint32_t ldg_sem_init(ldg_sem_t *s, const char *name, uint32_t init_val);
LDG_EXPORT uint32_t ldg_sem_open(ldg_sem_t *s, const char *name);
LDG_EXPORT uint32_t ldg_sem_destroy(ldg_sem_t *s);
LDG_EXPORT uint32_t ldg_sem_wait(ldg_sem_t *s);
LDG_EXPORT uint32_t ldg_sem_trywait(ldg_sem_t *s);
LDG_EXPORT uint32_t ldg_sem_post(ldg_sem_t *s);

#endif
