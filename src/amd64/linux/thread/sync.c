#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <dangling/thread/sync.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>


// impl accessors
static inline pthread_mutex_t* mut_mtx(ldg_mut_t *m)
{
    return (pthread_mutex_t *)m->impl;
}

static inline pthread_mutexattr_t* mut_attr(ldg_mut_t *m)
{
    return (pthread_mutexattr_t *)((uint8_t *)m->impl + sizeof(pthread_mutex_t));
}

static inline pthread_cond_t* cond_cv(ldg_cond_t *c)
{
    return (pthread_cond_t *)c->impl;
}

static inline pthread_condattr_t* cond_attr(ldg_cond_t *c)
{
    return (pthread_condattr_t *)((uint8_t *)c->impl + sizeof(pthread_cond_t));
}

uint32_t ldg_mut_init(ldg_mut_t *m, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(pthread_mutex_t) + sizeof(pthread_mutexattr_t) <= LDG_MUT_IMPL_SIZE);

    if (LDG_UNLIKELY(!m)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(m, 0, sizeof(ldg_mut_t)) != m)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(pthread_mutexattr_init(mut_attr(m)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (shared) { if (LDG_UNLIKELY(pthread_mutexattr_setpshared(mut_attr(m), PTHREAD_PROCESS_SHARED) != 0))
        {
            pthread_mutexattr_destroy(mut_attr(m));
            return LDG_ERR_FUNC_ARG_INVALID;
        }
    }

    if (LDG_UNLIKELY(pthread_mutex_init(mut_mtx(m), mut_attr(m)) != 0))
    {
        pthread_mutexattr_destroy(mut_attr(m));
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    m->is_shared = shared;
    m->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_destroy(ldg_mut_t *m)
{
    uint32_t first_err = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_mutex_destroy(mut_mtx(m)) != 0 && first_err == LDG_ERR_AOK)) { first_err = LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(pthread_mutexattr_destroy(mut_attr(m)) != 0 && first_err == LDG_ERR_AOK)) { first_err = LDG_ERR_FUNC_ARG_INVALID; }

    m->is_init = 0;

    return first_err;
}

uint32_t ldg_mut_lock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_mutex_lock(mut_mtx(m)) != 0)) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_unlock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_mutex_unlock(mut_mtx(m)) != 0)) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_trylock(ldg_mut_t *m)
{
    int32_t ret = 0;

    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = (int32_t)pthread_mutex_trylock(mut_mtx(m));
    if (LDG_LIKELY(ret == 0)) { return LDG_ERR_AOK; }

    if (ret == EBUSY) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_cond_init(ldg_cond_t *c, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(pthread_cond_t) + sizeof(pthread_condattr_t) <= LDG_COND_IMPL_SIZE);

    if (LDG_UNLIKELY(!c)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(c, 0, sizeof(ldg_cond_t)) != c)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(pthread_condattr_init(cond_attr(c)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pthread_condattr_setclock(cond_attr(c), CLOCK_MONOTONIC) != 0))
    {
        pthread_condattr_destroy(cond_attr(c));
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    if (shared) { if (LDG_UNLIKELY(pthread_condattr_setpshared(cond_attr(c), PTHREAD_PROCESS_SHARED) != 0))
        {
            pthread_condattr_destroy(cond_attr(c));
            return LDG_ERR_FUNC_ARG_INVALID;
        }
    }

    if (LDG_UNLIKELY(pthread_cond_init(cond_cv(c), cond_attr(c)) != 0))
    {
        pthread_condattr_destroy(cond_attr(c));
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    c->is_shared = shared;
    c->is_monotonic = 1;
    c->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_destroy(ldg_cond_t *c)
{
    uint32_t first_err = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!c || !c->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_destroy(cond_cv(c)) != 0 && first_err == LDG_ERR_AOK)) { first_err = LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(pthread_condattr_destroy(cond_attr(c)) != 0 && first_err == LDG_ERR_AOK)) { first_err = LDG_ERR_FUNC_ARG_INVALID; }

    c->is_init = 0;

    return first_err;
}

uint32_t ldg_cond_wait(ldg_cond_t *c, ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!c || !c->is_init || !m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_wait(cond_cv(c), mut_mtx(m)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_timedwait(ldg_cond_t *c, ldg_mut_t *m, uint64_t timeout_ms)
{
    struct timespec ts = { 0 };
    int32_t ret = 0;
    uint64_t max_add_ms = 0;

    if (LDG_UNLIKELY(!c || !c->is_init || !m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    max_add_ms = (uint64_t)(INT64_MAX - ts.tv_sec) * LDG_MS_PER_SEC;
    if (LDG_UNLIKELY(timeout_ms > max_add_ms)) { timeout_ms = max_add_ms; }

    ts.tv_sec += (time_t)(timeout_ms / LDG_MS_PER_SEC);
    ts.tv_nsec += (long)((timeout_ms % LDG_MS_PER_SEC) * LDG_NS_PER_MS);
    if (ts.tv_nsec >= (long)(LDG_NS_PER_SEC))
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= (long)(LDG_NS_PER_SEC);
    }

    ret = (int32_t)pthread_cond_timedwait(cond_cv(c), mut_mtx(m), &ts);
    if (LDG_LIKELY(ret == 0)) { return LDG_ERR_AOK; }

    if (ret == ETIMEDOUT) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_cond_sig(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_signal(cond_cv(c)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_bcast(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_broadcast(cond_cv(c)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_init(ldg_sem_t *s, const char *name, uint32_t init_val)
{
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(!s || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(s, 0, sizeof(ldg_sem_t)) != s)) { return LDG_ERR_MEM_BAD; }

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    s->handle = sem_open(name, O_CREAT | O_EXCL, 0600, init_val);
    if (LDG_UNLIKELY(s->handle == SEM_FAILED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memcpy(s->name, name, name_len) != s->name)) { sem_close(s->handle); sem_unlink(name); return LDG_ERR_MEM_BAD; }

    s->name[name_len] = LDG_STR_TERM;
    s->is_owner = 1;
    s->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_open(ldg_sem_t *s, const char *name)
{
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(!s || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(s, 0, sizeof(ldg_sem_t)) != s)) { return LDG_ERR_MEM_BAD; }

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    s->handle = sem_open(name, 0);
    if (LDG_UNLIKELY(s->handle == SEM_FAILED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memcpy(s->name, name, name_len) != s->name)) { sem_close(s->handle); return LDG_ERR_MEM_BAD; }

    s->name[name_len] = LDG_STR_TERM;
    s->is_owner = 0;
    s->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_destroy(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sem_close(s->handle) != 0)) { s->is_init = 0; return LDG_ERR_FUNC_ARG_INVALID; }

    if (s->is_owner) { sem_unlink(s->name); }

    s->is_init = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_wait(ldg_sem_t *s)
{
    uint32_t retries = 0;

    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_UNLIKELY(sem_wait(s->handle) != 0))
    {
        if (errno != EINTR || retries >= 1024) { return (errno == EINTR) ? LDG_ERR_INTERRUPTED : LDG_ERR_FUNC_ARG_INVALID; }

        retries++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_trywait(ldg_sem_t *s)
{
    int32_t ret = 0;

    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = (int32_t)sem_trywait(s->handle);
    if (LDG_LIKELY(ret == 0)) { return LDG_ERR_AOK; }

    if (errno == EAGAIN) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_sem_post(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sem_post(s->handle) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}
