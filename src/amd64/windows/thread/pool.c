#include <string.h>
#include <windows.h>

#define LDG_THREAD_POOL_CORE_BIT 1
#define LDG_THREAD_POOL_CPU_FALLBACK 4
#define LDG_THREAD_POOL_PIN_NONE 0
#define LDG_THREAD_POOL_THREAD_OK 0

#include <dangling/thread/pool.h>
#include <dangling/thread/mpmc.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/macros.h>
#include <dangling/sys/info.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

static uint64_t thread_pool_pin_mask_get(const ldg_thread_pool_t *pool)
{
    uint64_t bit = LDG_THREAD_POOL_CORE_BIT;
    uint64_t tid = 0;
    uint32_t cpu_cunt = 0;

    if (pool->affinity == LDG_THREAD_POOL_AFFINITY_MASK) { return pool->affinity_mask; }

    if (pool->affinity != LDG_THREAD_POOL_AFFINITY_SPREAD) { return LDG_THREAD_POOL_PIN_NONE; }

    if (ldg_sys_cpu_cunt_get(&cpu_cunt) != LDG_ERR_AOK || cpu_cunt == 0) { cpu_cunt = LDG_THREAD_POOL_CPU_FALLBACK; }

    tid = (uint64_t)GetCurrentThreadId();

    return bit << (tid % (uint64_t)cpu_cunt);
}

static DWORD WINAPI ldg_thread_pool_worker_enter(LPVOID arg)
{
    ldg_thread_pool_worker_t *worker = (ldg_thread_pool_worker_t *)arg;
    ldg_thread_pool_t *pool = 0x0;
    ldg_thread_pool_task_t task = LDG_STRUCT_ZERO_INIT;
    uint64_t pin_mask = 0;
    uint64_t pin_prev = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!worker)) { return LDG_THREAD_POOL_THREAD_OK; }

    pool = (ldg_thread_pool_t *)worker->pool;

    pin_mask = thread_pool_pin_mask_get(pool);

    if (pin_mask != 0)
    {
        pin_prev = (uint64_t)SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)pin_mask);
        if (LDG_UNLIKELY(pin_prev == 0)) { LDG_WR_ONCE(worker->core_id, UINT32_MAX); }
    }

    LDG_WR_ONCE(worker->state, LDG_THREAD_POOL_WORKER_RUNNING);

    while (!LDG_RD_ONCE(worker->should_stop))
    {
        if (worker->func) { worker->func(worker->id, worker->func_arg); }
        else if (LDG_RD_ONCE(pool->task_queue))
        {
            ret = ldg_mpmc_wait(pool->task_queue, &task, LDG_THREAD_POOL_WAIT_TIMEOUT_MS);
            if (ret == LDG_ERR_AOK && task.func) { task.func(worker->id, task.arg); }
        }

        LDG_PAUSE;
    }

    LDG_WR_ONCE(worker->state, LDG_THREAD_POOL_WORKER_STOPPED);

    return 0;
}

// caller shall pair with shutdown(); handles are not reclaimed otherwise
uint32_t ldg_thread_pool_init(ldg_thread_pool_t *pool, uint32_t worker_cunt)
{
    uint32_t idx = 0;
    uint32_t ret = 0;
    void *queue_tmp = 0x0;

    LDG_BOOL_ASSERT(sizeof(HANDLE) <= sizeof(uint64_t));

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(worker_cunt == 0 || worker_cunt > LDG_THREAD_POOL_MAX_WORKERS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(pool, 0, sizeof(ldg_thread_pool_t)) != pool)) { return LDG_ERR_MEM_BAD; }

    pool->is_init = UINT8_MAX;
    pool->is_running = UINT8_MAX;

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_mpmc_queue_t), &queue_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pool->task_queue = (ldg_mpmc_queue_t *)queue_tmp;

    ret = ldg_mpmc_init(pool->task_queue, sizeof(ldg_thread_pool_task_t), LDG_THREAD_POOL_TASK_QUEUE_CAP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(pool->task_queue);
        pool->task_queue = 0x0;
        return ret;
    }

    pool->worker_cunt = worker_cunt;

    for (idx = 0; idx < worker_cunt; idx++)
    {
        pool->workers[idx].id = idx;
        pool->workers[idx].core_id = UINT32_MAX;
        pool->workers[idx].pool = pool;
        LDG_WR_ONCE(pool->workers[idx].state, LDG_THREAD_POOL_WORKER_IDLE);
        LDG_WR_ONCE(pool->workers[idx].should_stop, 0);
    }

    pool->is_running = LDG_TRUTH_FALSE;
    pool->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_pool_shutdown(ldg_thread_pool_t *pool)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_thread_pool_stop(pool);

    if (pool->task_queue)
    {
        if (LDG_UNLIKELY(ldg_mpmc_shutdown(pool->task_queue) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_BUSY; }

        if (LDG_UNLIKELY(ldg_mem_dealloc(pool->task_queue) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_MEM_BAD; }

        pool->task_queue = 0x0;
    }

    pool->is_init = LDG_TRUTH_FALSE;

    return ret;
}

// start() and submit() are mutually exclusive; concurrent use is undefined
uint32_t ldg_thread_pool_start(ldg_thread_pool_t *pool, ldg_thread_pool_worker_func_t func, void *arg)
{
    uint32_t idx = 0;
    uint32_t started = 0;
    HANDLE h = 0x0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !func)) { return LDG_ERR_FUNC_ARG_NULL; }

    {
        uint8_t expected = LDG_TRUTH_FALSE;
        if (LDG_UNLIKELY(!LDG_CAS(&pool->is_running, &expected, LDG_TRUTH_TRUE))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }

    for (idx = 0; idx < pool->worker_cunt; idx++)
    {
        pool->workers[idx].func = func;
        pool->workers[idx].func_arg = arg;

        LDG_WR_ONCE(pool->workers[idx].state, LDG_THREAD_POOL_WORKER_STARTING);
        LDG_WR_ONCE(pool->workers[idx].should_stop, 0);

        h = CreateThread(0x0, 0, ldg_thread_pool_worker_enter, &pool->workers[idx], 0, 0x0);
        pool->workers[idx].handle = (h != 0x0) ? (uint64_t)h : 0;

        if (LDG_UNLIKELY(h == 0x0))
        {
            LDG_WR_ONCE(pool->workers[idx].state, LDG_THREAD_POOL_WORKER_STOPPED);
            continue;
        }

        started++;
    }

    if (LDG_UNLIKELY(started == 0)) { LDG_WR_ONCE(pool->is_running, LDG_TRUTH_FALSE); return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_pool_stop(ldg_thread_pool_t *pool)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(LDG_RD_ONCE(pool->is_running) != LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    for (idx = 0; idx < pool->worker_cunt; idx++) { LDG_WR_ONCE(pool->workers[idx].should_stop, 1); }

    LDG_SMP_MB();

    // single bcast; workers re-check should_stop every WAIT_TIMEOUT_MS on timedwait expiry
    if (pool->task_queue) { if (LDG_UNLIKELY(ldg_cond_bcast(&pool->task_queue->wait_cond) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_BUSY; } }

    for (idx = 0; idx < pool->worker_cunt; idx++) { if (pool->workers[idx].handle != 0)
        {
            if (LDG_UNLIKELY(WaitForSingleObject((HANDLE)pool->workers[idx].handle, INFINITE) != WAIT_OBJECT_0 && ret == LDG_ERR_AOK)) { ret = LDG_ERR_FUNC_ARG_INVALID; }

            CloseHandle((HANDLE)pool->workers[idx].handle);
            pool->workers[idx].handle = 0;
        }
    }

    LDG_WR_ONCE(pool->is_running, LDG_TRUTH_FALSE);

    return ret;
}

uint64_t ldg_thread_pool_worker_cunt_get(ldg_thread_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return (uint64_t)pool->worker_cunt;
}

uint32_t ldg_thread_pool_affinity_set(ldg_thread_pool_t *pool, ldg_thread_pool_affinity_t policy, uint64_t mask)
{
    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(LDG_RD_ONCE(pool->is_running) == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(policy > LDG_THREAD_POOL_AFFINITY_MASK)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(policy == LDG_THREAD_POOL_AFFINITY_MASK && mask == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    pool->affinity = (uint32_t)policy;
    pool->affinity_mask = mask;

    return LDG_ERR_AOK;
}

static uint32_t thread_pool_submit_workers_start(ldg_thread_pool_t *pool)
{
    uint32_t idx = 0;
    uint32_t started = 0;
    HANDLE h = 0x0;

    for (idx = 0; idx < pool->worker_cunt; idx++)
    {
        pool->workers[idx].func = 0x0;
        pool->workers[idx].func_arg = 0x0;

        LDG_WR_ONCE(pool->workers[idx].state, LDG_THREAD_POOL_WORKER_STARTING);
        LDG_WR_ONCE(pool->workers[idx].should_stop, 0);

        h = CreateThread(0x0, 0, ldg_thread_pool_worker_enter, &pool->workers[idx], 0, 0x0);
        pool->workers[idx].handle = (h != 0x0) ? (uint64_t)h : 0;

        if (LDG_UNLIKELY(h == 0x0))
        {
            LDG_WR_ONCE(pool->workers[idx].state, LDG_THREAD_POOL_WORKER_STOPPED);
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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !func)) { return LDG_ERR_FUNC_ARG_NULL; }

    {
        uint8_t expected = LDG_TRUTH_FALSE;
        if (LDG_CAS(&pool->is_running, &expected, LDG_TRUTH_TRUE)) { if (LDG_UNLIKELY(thread_pool_submit_workers_start(pool) != LDG_ERR_AOK)) { LDG_WR_ONCE(pool->is_running, LDG_TRUTH_FALSE); return LDG_ERR_FUNC_ARG_INVALID; } }
    }

    task.func = func;
    task.arg = arg;

    return ldg_mpmc_push(pool->task_queue, &task);
}
