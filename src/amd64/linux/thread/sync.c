#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <dangling/thread/sync.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/core/macros.h>


// impl accessors
static inline pthread_mutex_t* mut_mtx(ldg_mut_t *mut)
{
    return (pthread_mutex_t *)mut->impl;
}

static inline pthread_mutexattr_t* mut_attr(ldg_mut_t *mut)
{
    return (pthread_mutexattr_t *)((uint8_t *)mut->impl + sizeof(pthread_mutex_t));
}

static inline pthread_cond_t* cond_cv(ldg_cond_t *cond)
{
    return (pthread_cond_t *)cond->impl;
}

static inline pthread_condattr_t* cond_attr(ldg_cond_t *cond)
{
    return (pthread_condattr_t *)((uint8_t *)cond->impl + sizeof(pthread_cond_t));
}

uint32_t ldg_mut_init(ldg_mut_t *mut, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(pthread_mutex_t) + sizeof(pthread_mutexattr_t) <= LDG_MUT_IMPL_SIZE);

    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(mut, 0, sizeof(ldg_mut_t)) != mut)) { return LDG_ERR_MEM_BAD; }

    mut->is_init = UINT8_MAX;
    mut->is_shared = UINT8_MAX;

    if (LDG_UNLIKELY(pthread_mutexattr_init(mut_attr(mut)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (shared == LDG_TRUTH_TRUE) { if (LDG_UNLIKELY(pthread_mutexattr_setpshared(mut_attr(mut), PTHREAD_PROCESS_SHARED) != 0))
        {
            pthread_mutexattr_destroy(mut_attr(mut));
            return LDG_ERR_FUNC_ARG_INVALID;
        }
    }

    if (LDG_UNLIKELY(pthread_mutex_init(mut_mtx(mut), mut_attr(mut)) != 0))
    {
        pthread_mutexattr_destroy(mut_attr(mut));
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    mut->is_shared = (shared == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    mut->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_destroy(ldg_mut_t *mut)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_mutex_destroy(mut_mtx(mut)) != 0 && ret == LDG_ERR_AOK)) { ret = LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(pthread_mutexattr_destroy(mut_attr(mut)) != 0 && ret == LDG_ERR_AOK)) { ret = LDG_ERR_FUNC_ARG_INVALID; }

    mut->is_init = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t ldg_mut_lock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_mutex_lock(mut_mtx(mut)) != 0)) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_unlock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_mutex_unlock(mut_mtx(mut)) != 0)) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_trylock(ldg_mut_t *mut)
{
    int32_t ret = 0;

    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = (int32_t)pthread_mutex_trylock(mut_mtx(mut));
    if (LDG_LIKELY(ret == 0)) { return LDG_ERR_AOK; }

    if (ret == EBUSY) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_cond_init(ldg_cond_t *cond, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(pthread_cond_t) + sizeof(pthread_condattr_t) <= LDG_COND_IMPL_SIZE);

    if (LDG_UNLIKELY(!cond)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(cond, 0, sizeof(ldg_cond_t)) != cond)) { return LDG_ERR_MEM_BAD; }

    cond->is_init = UINT8_MAX;
    cond->is_shared = UINT8_MAX;
    cond->is_monotonic = UINT8_MAX;

    if (LDG_UNLIKELY(pthread_condattr_init(cond_attr(cond)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pthread_condattr_setclock(cond_attr(cond), CLOCK_MONOTONIC) != 0))
    {
        pthread_condattr_destroy(cond_attr(cond));
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    if (shared == LDG_TRUTH_TRUE) { if (LDG_UNLIKELY(pthread_condattr_setpshared(cond_attr(cond), PTHREAD_PROCESS_SHARED) != 0))
        {
            pthread_condattr_destroy(cond_attr(cond));
            return LDG_ERR_FUNC_ARG_INVALID;
        }
    }

    if (LDG_UNLIKELY(pthread_cond_init(cond_cv(cond), cond_attr(cond)) != 0))
    {
        pthread_condattr_destroy(cond_attr(cond));
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    cond->is_shared = (shared == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    cond->is_monotonic = LDG_TRUTH_TRUE;
    cond->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_destroy(ldg_cond_t *cond)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_destroy(cond_cv(cond)) != 0 && ret == LDG_ERR_AOK)) { ret = LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(pthread_condattr_destroy(cond_attr(cond)) != 0 && ret == LDG_ERR_AOK)) { ret = LDG_ERR_FUNC_ARG_INVALID; }

    cond->is_init = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t ldg_cond_wait(ldg_cond_t *cond, ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE || !mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_wait(cond_cv(cond), mut_mtx(mut)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_timedwait(ldg_cond_t *cond, ldg_mut_t *mut, uint64_t timeout_ms)
{
    struct timespec ts = { 0 };
    int32_t ret = 0;
    uint64_t max_add_ms = 0;

    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE || !mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    max_add_ms = (uint64_t)(INT64_MAX - ts.tv_sec) * LDG_MS_PER_SEC;
    if (LDG_UNLIKELY(timeout_ms > max_add_ms)) { timeout_ms = max_add_ms; }

    ts.tv_sec += (time_t)(timeout_ms / LDG_MS_PER_SEC);
    ts.tv_nsec += (int64_t)((timeout_ms % LDG_MS_PER_SEC) * LDG_NS_PER_MS);
    if (ts.tv_nsec >= (int64_t)(LDG_NS_PER_SEC))
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= (int64_t)(LDG_NS_PER_SEC);
    }

    ret = (int32_t)pthread_cond_timedwait(cond_cv(cond), mut_mtx(mut), &ts);
    if (LDG_LIKELY(ret == 0)) { return LDG_ERR_AOK; }

    if (ret == ETIMEDOUT) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_cond_sig(ldg_cond_t *cond)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_signal(cond_cv(cond)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_bcast(ldg_cond_t *cond)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pthread_cond_broadcast(cond_cv(cond)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_init(ldg_sem_t *sem, const char *name, uint32_t init_val)
{
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(!sem || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(sem, 0, sizeof(ldg_sem_t)) != sem)) { return LDG_ERR_MEM_BAD; }

    sem->is_init = UINT8_MAX;
    sem->is_owner = UINT8_MAX;

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sem->handle = sem_open(name, O_CREAT | O_EXCL, 0600, init_val);
    if (LDG_UNLIKELY(sem->handle == SEM_FAILED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(sem->name, name, name_len) != LDG_ERR_AOK)) { sem_close(sem->handle); sem_unlink(name); return LDG_ERR_MEM_BAD; }

    sem->name[name_len] = LDG_STR_TERM;
    sem->is_owner = LDG_TRUTH_TRUE;
    sem->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_open(ldg_sem_t *sem, const char *name)
{
    uint64_t name_len = 0;

    if (LDG_UNLIKELY(!sem || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(sem, 0, sizeof(ldg_sem_t)) != sem)) { return LDG_ERR_MEM_BAD; }

    sem->is_init = UINT8_MAX;
    sem->is_owner = UINT8_MAX;

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sem->handle = sem_open(name, 0);
    if (LDG_UNLIKELY(sem->handle == SEM_FAILED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(sem->name, name, name_len) != LDG_ERR_AOK)) { sem_close(sem->handle); return LDG_ERR_MEM_BAD; }

    sem->name[name_len] = LDG_STR_TERM;
    sem->is_owner = LDG_TRUTH_FALSE;
    sem->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_destroy(ldg_sem_t *sem)
{
    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sem_close(sem->handle) != 0)) { sem->is_init = LDG_TRUTH_FALSE; return LDG_ERR_FUNC_ARG_INVALID; }

    if (sem->is_owner == LDG_TRUTH_TRUE) { sem_unlink(sem->name); }

    sem->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_wait(ldg_sem_t *sem)
{
    uint32_t retries = 0;

    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (LDG_UNLIKELY(sem_wait(sem->handle) != 0))
    {
        if (errno != EINTR || retries >= 1024) { return (errno == EINTR) ? LDG_ERR_INTERRUPTED : LDG_ERR_FUNC_ARG_INVALID; }

        retries++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_trywait(ldg_sem_t *sem)
{
    int32_t ret = 0;

    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = (int32_t)sem_trywait(sem->handle);
    if (LDG_LIKELY(ret == 0)) { return LDG_ERR_AOK; }

    if (errno == EAGAIN) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_sem_post(ldg_sem_t *sem)
{
    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(sem_post(sem->handle) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}
