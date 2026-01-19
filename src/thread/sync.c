#include <dangling/thread/sync.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

int32_t ldg_mut_init(ldg_mut_t *m, uint8_t shared)
{
    int ret = 0;

    if (LDG_UNLIKELY(!m)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(m, 0, sizeof(ldg_mut_t));

    ret = pthread_mutexattr_init(&m->attr);
    if (LDG_UNLIKELY(ret != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (shared)
    {
        ret = pthread_mutexattr_setpshared(&m->attr, PTHREAD_PROCESS_SHARED);
        if (LDG_UNLIKELY(ret != 0))
        {
            (void)pthread_mutexattr_destroy(&m->attr);
            return LDG_ERR_FUNC_ARG_INVALID;
        }
    }

    ret = pthread_mutex_init(&m->mtx, &m->attr);
    if (LDG_UNLIKELY(ret != 0))
    {
        (void)pthread_mutexattr_destroy(&m->attr);
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    m->is_shared = shared;
    m->is_init = 1;

    return LDG_ERR_OK;
}

void ldg_mut_destroy(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return; }

    (void)pthread_mutex_destroy(&m->mtx);
    (void)pthread_mutexattr_destroy(&m->attr);

    m->is_init = 0;
}

void ldg_mut_lock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return; }

    (void)pthread_mutex_lock(&m->mtx);
}

void ldg_mut_unlock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return; }

    (void)pthread_mutex_unlock(&m->mtx);
}

int32_t ldg_mut_trylock(ldg_mut_t *m)
{
    int ret = 0;

    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = pthread_mutex_trylock(&m->mtx);
    if (ret == 0) { return LDG_ERR_OK; }

    if (ret == EBUSY) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

int32_t ldg_cond_init(ldg_cond_t *c, uint8_t shared)
{
    int ret = 0;

    if (LDG_UNLIKELY(!c)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(c, 0, sizeof(ldg_cond_t));

    ret = pthread_condattr_init(&c->attr);
    if (LDG_UNLIKELY(ret != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (shared)
    {
        ret = pthread_condattr_setpshared(&c->attr, PTHREAD_PROCESS_SHARED);
        if (LDG_UNLIKELY(ret != 0))
        {
            (void)pthread_condattr_destroy(&c->attr);
            return LDG_ERR_FUNC_ARG_INVALID;
        }
    }

    ret = pthread_cond_init(&c->cond, &c->attr);
    if (LDG_UNLIKELY(ret != 0))
    {
        (void)pthread_condattr_destroy(&c->attr);
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    c->is_shared = shared;
    c->is_init = 1;

    return LDG_ERR_OK;
}

void ldg_cond_destroy(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return; }

    (void)pthread_cond_destroy(&c->cond);
    (void)pthread_condattr_destroy(&c->attr);

    c->is_init = 0;
}

void ldg_cond_wait(ldg_cond_t *c, ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!c || !c->is_init || !m || !m->is_init)) { return; }

    (void)pthread_cond_wait(&c->cond, &m->mtx);
}

int32_t ldg_cond_timedwait(ldg_cond_t *c, ldg_mut_t *m, uint64_t timeout_ms)
{
    struct timespec ts = { 0 };
    int ret = 0;

    if (LDG_UNLIKELY(!c || !c->is_init || !m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += (time_t)(timeout_ms / LDG_MS_PER_SEC);
    ts.tv_nsec += (long)((timeout_ms % LDG_MS_PER_SEC) * LDG_NS_PER_MS);
    if (ts.tv_nsec >= (long)(LDG_MS_PER_SEC * LDG_NS_PER_MS))
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= (long)(LDG_MS_PER_SEC * LDG_NS_PER_MS);
    }

    ret = pthread_cond_timedwait(&c->cond, &m->mtx, &ts);
    if (ret == 0) { return LDG_ERR_OK; }

    if (ret == ETIMEDOUT) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

void ldg_cond_signal(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return; }

    (void)pthread_cond_signal(&c->cond);
}

void ldg_cond_broadcast(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return; }

    (void)pthread_cond_broadcast(&c->cond);
}

int32_t ldg_sem_init(ldg_sem_t *s, const char *name, uint32_t init_val)
{
    size_t name_len = 0;

    if (LDG_UNLIKELY(!s || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(s, 0, sizeof(ldg_sem_t));

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    (void)sem_unlink(name);

    s->sem = sem_open(name, O_CREAT | O_EXCL, 0600, init_val);
    if (LDG_UNLIKELY(s->sem == SEM_FAILED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    (void)memcpy(s->name, name, name_len);
    s->name[name_len] = LDG_STR_TERM;
    s->is_owner = 1;
    s->is_init = 1;

    return LDG_ERR_OK;
}

int32_t ldg_sem_open(ldg_sem_t *s, const char *name)
{
    size_t name_len = 0;

    if (LDG_UNLIKELY(!s || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(s, 0, sizeof(ldg_sem_t));

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    s->sem = sem_open(name, 0);
    if (LDG_UNLIKELY(s->sem == SEM_FAILED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    (void)memcpy(s->name, name, name_len);
    s->name[name_len] = LDG_STR_TERM;
    s->is_owner = 0;
    s->is_init = 1;

    return LDG_ERR_OK;
}

void ldg_sem_destroy(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return; }

    (void)sem_close(s->sem);

    if (s->is_owner) { (void)sem_unlink(s->name); }

    s->is_init = 0;
}

void ldg_sem_wait(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return; }

    (void)sem_wait(s->sem);
}

int32_t ldg_sem_trywait(ldg_sem_t *s)
{
    int ret = 0;

    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = sem_trywait(s->sem);
    if (ret == 0) { return LDG_ERR_OK; }

    if (errno == EAGAIN) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

void ldg_sem_post(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return; }

    (void)sem_post(s->sem);
}
