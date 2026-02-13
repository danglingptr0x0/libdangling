#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dangling/thread/sync.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SEM_MAX_CUNT 0x7FFFFFFF


// impl accessors
static inline CRITICAL_SECTION* mut_cs(ldg_mut_t *m)
{
    return (CRITICAL_SECTION *)m->impl;
}

static inline CONDITION_VARIABLE* cond_cv(ldg_cond_t *c)
{
    return (CONDITION_VARIABLE *)c->impl;
}

uint32_t ldg_mut_init(ldg_mut_t *m, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(CRITICAL_SECTION) <= LDG_MUT_IMPL_SIZE);

    if (LDG_UNLIKELY(!m)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(shared)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(memset(m, 0, sizeof(ldg_mut_t)) != m)) { return LDG_ERR_MEM_BAD; }

    InitializeCriticalSection(mut_cs(m));

    m->is_shared = 0;
    m->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_destroy(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    DeleteCriticalSection(mut_cs(m));

    m->is_init = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_lock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    EnterCriticalSection(mut_cs(m));

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_unlock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    LeaveCriticalSection(mut_cs(m));

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_trylock(ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (!TryEnterCriticalSection(mut_cs(m))) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_init(ldg_cond_t *c, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(CONDITION_VARIABLE) <= LDG_COND_IMPL_SIZE);

    if (LDG_UNLIKELY(!c)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(shared)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(memset(c, 0, sizeof(ldg_cond_t)) != c)) { return LDG_ERR_MEM_BAD; }

    InitializeConditionVariable(cond_cv(c));

    c->is_shared = 0;
    c->is_monotonic = 1;
    c->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_destroy(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(c->impl, 0, LDG_COND_IMPL_SIZE);

    c->is_init = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_wait(ldg_cond_t *c, ldg_mut_t *m)
{
    if (LDG_UNLIKELY(!c || !c->is_init || !m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!SleepConditionVariableCS(cond_cv(c), mut_cs(m), INFINITE))) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_timedwait(ldg_cond_t *c, ldg_mut_t *m, uint64_t timeout_ms)
{
    DWORD win_timeout = 0;

    if (LDG_UNLIKELY(!c || !c->is_init || !m || !m->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    win_timeout = (timeout_ms >= (uint64_t)(INFINITE)) ? (INFINITE - 1) : (DWORD)timeout_ms;

    if (!SleepConditionVariableCS(cond_cv(c), mut_cs(m), win_timeout))
    {
        if (GetLastError() == ERROR_TIMEOUT) { return LDG_ERR_TIMEOUT; }

        return LDG_ERR_FUNC_ARG_INVALID;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_sig(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    WakeConditionVariable(cond_cv(c));

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_bcast(ldg_cond_t *c)
{
    if (LDG_UNLIKELY(!c || !c->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    WakeAllConditionVariable(cond_cv(c));

    return LDG_ERR_AOK;
}

// sem (Win32 named semaphores)

uint32_t ldg_sem_init(ldg_sem_t *s, const char *name, uint32_t init_val)
{
    uint64_t name_len = 0;
    HANDLE h = 0x0;

    if (LDG_UNLIKELY(!s || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(s, 0, sizeof(ldg_sem_t)) != s)) { return LDG_ERR_MEM_BAD; }

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(init_val > (uint32_t)LDG_SEM_MAX_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    h = CreateSemaphoreA(0x0, (LONG)init_val, LDG_SEM_MAX_CUNT, name);
    if (LDG_UNLIKELY(!h)) { return LDG_ERR_FUNC_ARG_INVALID; }

    s->handle = (void *)h;

    if (LDG_UNLIKELY(memcpy(s->name, name, name_len) != s->name)) { CloseHandle(h); return LDG_ERR_MEM_BAD; }

    s->name[name_len] = LDG_STR_TERM;
    s->is_owner = 1;
    s->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_open(ldg_sem_t *s, const char *name)
{
    uint64_t name_len = 0;
    HANDLE h = 0x0;

    if (LDG_UNLIKELY(!s || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(s, 0, sizeof(ldg_sem_t)) != s)) { return LDG_ERR_MEM_BAD; }

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    h = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name);
    if (LDG_UNLIKELY(!h)) { return LDG_ERR_FUNC_ARG_INVALID; }

    s->handle = (void *)h;

    if (LDG_UNLIKELY(memcpy(s->name, name, name_len) != s->name)) { CloseHandle(h); return LDG_ERR_MEM_BAD; }

    s->name[name_len] = LDG_STR_TERM;
    s->is_owner = 0;
    s->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_destroy(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!CloseHandle((HANDLE)s->handle))) { s->is_init = 0; return LDG_ERR_FUNC_ARG_INVALID; }

    s->is_init = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_wait(ldg_sem_t *s)
{
    DWORD ret = 0;

    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = WaitForSingleObject((HANDLE)s->handle, INFINITE);
    if (LDG_UNLIKELY(ret != WAIT_OBJECT_0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_trywait(ldg_sem_t *s)
{
    DWORD ret = 0;

    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = WaitForSingleObject((HANDLE)s->handle, 0);
    if (LDG_LIKELY(ret == WAIT_OBJECT_0)) { return LDG_ERR_AOK; }

    if (ret == WAIT_TIMEOUT) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_sem_post(ldg_sem_t *s)
{
    if (LDG_UNLIKELY(!s || !s->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ReleaseSemaphore((HANDLE)s->handle, 1, 0x0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}
