#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <dangling/thread/sync.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/core/macros.h>

#define LDG_SEM_MAX_CUNT 0x7FFFFFFF


// impl accessors
static inline CRITICAL_SECTION* mut_cs(ldg_mut_t *mut)
{
    return (CRITICAL_SECTION *)mut->impl;
}

static inline CONDITION_VARIABLE* cond_cv(ldg_cond_t *cond)
{
    return (CONDITION_VARIABLE *)cond->impl;
}

uint32_t ldg_mut_init(ldg_mut_t *mut, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(CRITICAL_SECTION) <= LDG_MUT_IMPL_SIZE);

    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(shared == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(memset(mut, 0, sizeof(ldg_mut_t)) != mut)) { return LDG_ERR_MEM_BAD; }

    InitializeCriticalSection(mut_cs(mut));

    mut->is_shared = 0;
    mut->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_destroy(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    DeleteCriticalSection(mut_cs(mut));

    mut->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_lock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    EnterCriticalSection(mut_cs(mut));

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_unlock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    LeaveCriticalSection(mut_cs(mut));

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_trylock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (!TryEnterCriticalSection(mut_cs(mut))) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_init(ldg_cond_t *cond, uint8_t shared)
{
    LDG_BOOL_ASSERT(sizeof(CONDITION_VARIABLE) <= LDG_COND_IMPL_SIZE);

    if (LDG_UNLIKELY(!cond)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(shared == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(memset(cond, 0, sizeof(ldg_cond_t)) != cond)) { return LDG_ERR_MEM_BAD; }

    InitializeConditionVariable(cond_cv(cond));

    cond->is_shared = 0;
    cond->is_monotonic = 1;
    cond->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_destroy(ldg_cond_t *cond)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(cond->impl, 0, LDG_COND_IMPL_SIZE);

    cond->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_wait(ldg_cond_t *cond, ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE || !mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!SleepConditionVariableCS(cond_cv(cond), mut_cs(mut), INFINITE))) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_timedwait(ldg_cond_t *cond, ldg_mut_t *mut, uint64_t timeout_ms)
{
    DWORD win_timeout = 0;

    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE || !mut || mut->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    win_timeout = (timeout_ms >= (uint64_t)(INFINITE)) ? (INFINITE - 1) : (DWORD)timeout_ms;

    if (!SleepConditionVariableCS(cond_cv(cond), mut_cs(mut), win_timeout))
    {
        if (GetLastError() == ERROR_TIMEOUT) { return LDG_ERR_TIMEOUT; }

        return LDG_ERR_FUNC_ARG_INVALID;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_sig(ldg_cond_t *cond)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    WakeConditionVariable(cond_cv(cond));

    return LDG_ERR_AOK;
}

uint32_t ldg_cond_bcast(ldg_cond_t *cond)
{
    if (LDG_UNLIKELY(!cond || cond->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    WakeAllConditionVariable(cond_cv(cond));

    return LDG_ERR_AOK;
}

// sem (Win32 named semaphores)

uint32_t ldg_sem_init(ldg_sem_t *sem, const char *name, uint32_t init_val)
{
    uint64_t name_len = 0;
    HANDLE handle = 0x0;

    if (LDG_UNLIKELY(!sem || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(sem, 0, sizeof(ldg_sem_t)) != sem)) { return LDG_ERR_MEM_BAD; }

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(init_val > (uint32_t)LDG_SEM_MAX_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    handle = CreateSemaphoreA(0x0, (LONG)init_val, LDG_SEM_MAX_CUNT, name);
    if (LDG_UNLIKELY(!handle)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sem->handle = (void *)handle;

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(sem->name, name, name_len) != LDG_ERR_AOK)) { CloseHandle(handle); return LDG_ERR_MEM_BAD; }

    sem->name[name_len] = LDG_STR_TERM;
    sem->is_owner = 1;
    sem->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_open(ldg_sem_t *sem, const char *name)
{
    uint64_t name_len = 0;
    HANDLE handle = 0x0;

    if (LDG_UNLIKELY(!sem || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(sem, 0, sizeof(ldg_sem_t)) != sem)) { return LDG_ERR_MEM_BAD; }

    name_len = strlen(name);
    if (LDG_UNLIKELY(name_len == 0 || name_len >= LDG_SEM_NAME_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    handle = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, name);
    if (LDG_UNLIKELY(!handle)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sem->handle = (void *)handle;

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(sem->name, name, name_len) != LDG_ERR_AOK)) { CloseHandle(handle); return LDG_ERR_MEM_BAD; }

    sem->name[name_len] = LDG_STR_TERM;
    sem->is_owner = 0;
    sem->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_destroy(ldg_sem_t *sem)
{
    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!CloseHandle((HANDLE)sem->handle))) { sem->is_init = LDG_TRUTH_FALSE; return LDG_ERR_FUNC_ARG_INVALID; }

    sem->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_wait(ldg_sem_t *sem)
{
    DWORD ret = 0;

    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = WaitForSingleObject((HANDLE)sem->handle, INFINITE);
    if (LDG_UNLIKELY(ret != WAIT_OBJECT_0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sem_trywait(ldg_sem_t *sem)
{
    DWORD ret = 0;

    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = WaitForSingleObject((HANDLE)sem->handle, 0);
    if (LDG_LIKELY(ret == WAIT_OBJECT_0)) { return LDG_ERR_AOK; }

    if (ret == WAIT_TIMEOUT) { return LDG_ERR_BUSY; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_sem_post(ldg_sem_t *sem)
{
    if (LDG_UNLIKELY(!sem || sem->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ReleaseSemaphore((HANDLE)sem->handle, 1, 0x0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}
