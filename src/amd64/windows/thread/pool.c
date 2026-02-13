#include <string.h>
#include <windows.h>

#include <dangling/thread/pool.h>
#include <dangling/thread/mpmc.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/macros.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

static DWORD WINAPI ldg_thread_pool_worker_enter(LPVOID arg)
{
    ldg_thread_pool_worker_t *worker = (ldg_thread_pool_worker_t *)arg;
    ldg_thread_pool_t *pool = 0x0;
    ldg_thread_pool_task_t task = { 0 };
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!worker)) { return 0; }

    pool = (ldg_thread_pool_t *)worker->pool;

    LDG_WRITE_ONCE(worker->state, LDG_THREAD_POOL_WORKER_RUNNING);

    while (!LDG_READ_ONCE(worker->should_stop))
    {
        if (LDG_READ_ONCE(pool->task_queue))
        {
            ret = ldg_mpmc_wait(pool->task_queue, &task, LDG_THREAD_POOL_WAIT_TIMEOUT_MS);
            if (ret == LDG_ERR_AOK && task.func) { task.func(task.arg); }
        }
        // amd64 TSO; CreateThread implies full barrier before worker entry
        else if (worker->func) { worker->func(worker->func_arg); }

        LDG_PAUSE;
    }

    LDG_WRITE_ONCE(worker->state, LDG_THREAD_POOL_WORKER_STOPPED);

    return 0;
}

// caller shall pair with shutdown(); handles are not reclaimed otherwise
uint32_t ldg_thread_pool_init(ldg_thread_pool_t *pool, uint32_t worker_cunt)
{
    uint32_t i = 0;
    uint32_t ret = 0;
    void *queue_tmp = 0x0;

    LDG_BOOL_ASSERT(sizeof(HANDLE) <= sizeof(uint64_t));

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(worker_cunt == 0 || worker_cunt > LDG_THREAD_POOL_MAX_WORKERS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(pool, 0, sizeof(ldg_thread_pool_t)) != pool)) { return LDG_ERR_MEM_BAD; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_mpmc_queue_t), &queue_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pool->task_queue = (ldg_mpmc_queue_t *)queue_tmp;

    ret = ldg_mpmc_init(pool->task_queue, sizeof(ldg_thread_pool_task_t), LDG_THREAD_POOL_TASK_QUEUE_CAPACITY);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(pool->task_queue);
        pool->task_queue = 0x0;
        return ret;
    }

    pool->worker_cunt = worker_cunt;

    for (i = 0; i < worker_cunt; i++)
    {
        pool->workers[i].id = i;
        pool->workers[i].core_id = i;
        pool->workers[i].pool = pool;
        LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_IDLE);
        LDG_WRITE_ONCE(pool->workers[i].should_stop, 0);
    }

    pool->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_pool_shutdown(ldg_thread_pool_t *pool)
{
    uint32_t first_err = LDG_ERR_AOK;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_thread_pool_stop(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK)) { first_err = ret; }

    if (pool->task_queue)
    {
        ret = ldg_mpmc_shutdown(pool->task_queue);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK)) { first_err = ret; }

        ldg_mem_dealloc(pool->task_queue);
        pool->task_queue = 0x0;
    }

    pool->submit_mode = 0;
    pool->is_init = 0;

    return first_err;
}

// start() and submit() are mutually exclusive; concurrent use is undefined
uint32_t ldg_thread_pool_start(ldg_thread_pool_t *pool, ldg_thread_pool_worker_func_t func, void *arg)
{
    uint32_t i = 0;
    uint32_t started = 0;
    HANDLE h = 0x0;

    if (LDG_UNLIKELY(!pool || !pool->is_init || !func)) { return LDG_ERR_FUNC_ARG_NULL; }

    {
        uint8_t expected = 0;
        if (LDG_UNLIKELY(!LDG_CAS(&pool->is_running, &expected, 1))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }

    for (i = 0; i < pool->worker_cunt; i++)
    {
        pool->workers[i].func = func;
        pool->workers[i].func_arg = arg;

        LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STARTING);
        LDG_WRITE_ONCE(pool->workers[i].should_stop, 0);

        h = CreateThread(0x0, 0, ldg_thread_pool_worker_enter, &pool->workers[i], 0, 0x0);
        pool->workers[i].handle = (h != 0x0) ? (uint64_t)h : 0;

        if (LDG_UNLIKELY(h == 0x0))
        {
            LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STOPPED);
            continue;
        }

        started++;
    }

    if (LDG_UNLIKELY(started == 0)) { LDG_WRITE_ONCE(pool->is_running, 0); return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_pool_stop(ldg_thread_pool_t *pool)
{
    uint32_t i = 0;
    uint32_t first_err = LDG_ERR_AOK;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!LDG_READ_ONCE(pool->is_running))) { return LDG_ERR_AOK; }

    for (i = 0; i < pool->worker_cunt; i++) { LDG_WRITE_ONCE(pool->workers[i].should_stop, 1); }

    LDG_SMP_MB();

    // single bcast; workers re-check should_stop every WAIT_TIMEOUT_MS on timedwait expiry
    if (pool->task_queue)
    {
        ret = ldg_cond_bcast(&pool->task_queue->wait_cond);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK)) { first_err = ret; }
    }

    for (i = 0; i < pool->worker_cunt; i++) { if (pool->workers[i].handle != 0)
        {
            if (LDG_UNLIKELY(WaitForSingleObject((HANDLE)pool->workers[i].handle, INFINITE) != WAIT_OBJECT_0 && first_err == LDG_ERR_AOK)) { first_err = LDG_ERR_FUNC_ARG_INVALID; }

            CloseHandle((HANDLE)pool->workers[i].handle);
            pool->workers[i].handle = 0;
        }
    }

    LDG_WRITE_ONCE(pool->is_running, 0);

    return first_err;
}

uint64_t ldg_thread_pool_worker_cunt_get(ldg_thread_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return UINT64_MAX; }

    return (uint64_t)pool->worker_cunt;
}

static uint32_t thread_pool_submit_workers_start(ldg_thread_pool_t *pool)
{
    uint32_t i = 0;
    uint32_t started = 0;
    HANDLE h = 0x0;

    for (i = 0; i < pool->worker_cunt; i++)
    {
        pool->workers[i].func = 0x0;
        pool->workers[i].func_arg = 0x0;

        LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STARTING);
        LDG_WRITE_ONCE(pool->workers[i].should_stop, 0);

        h = CreateThread(0x0, 0, ldg_thread_pool_worker_enter, &pool->workers[i], 0, 0x0);
        pool->workers[i].handle = (h != 0x0) ? (uint64_t)h : 0;

        if (LDG_UNLIKELY(h == 0x0))
        {
            LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STOPPED);
            continue;
        }

        started++;
    }

    if (LDG_UNLIKELY(started == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

// mutually exclusive with start(); tasks queue before workers reach mpmc_wait
uint32_t ldg_thread_pool_submit(ldg_thread_pool_t *pool, ldg_thread_pool_worker_func_t func, void *arg)
{
    ldg_thread_pool_task_t task = { 0 };

    if (LDG_UNLIKELY(!pool || !pool->is_init || !func)) { return LDG_ERR_FUNC_ARG_NULL; }

    {
        uint8_t sm_expected = 0;
        LDG_CAS(&pool->submit_mode, &sm_expected, 1);
    }

    {
        uint8_t expected = 0;
        if (LDG_CAS(&pool->is_running, &expected, 1)) { if (LDG_UNLIKELY(thread_pool_submit_workers_start(pool) != LDG_ERR_AOK)) { LDG_WRITE_ONCE(pool->is_running, 0); return LDG_ERR_FUNC_ARG_INVALID; } }
    }

    task.func = func;
    task.arg = arg;

    return ldg_mpmc_push(pool->task_queue, &task);
}
