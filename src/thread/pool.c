#include <stdlib.h>
#include <string.h>
#include <sched.h>

#include <dangling/thread/pool.h>
#include <dangling/thread/mpmc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/arch/x86_64/atomic.h>
#include <dangling/arch/x86_64/fence.h>

static void* ldg_thread_pool_worker_entry(void *arg)
{
    ldg_thread_pool_worker_t *worker = (ldg_thread_pool_worker_t *)arg;
    ldg_thread_pool_t *pool = NULL;
    ldg_thread_pool_task_t task = { 0 };
    int32_t ret = 0;

    if (LDG_UNLIKELY(!worker)) { return NULL; }

    pool = (ldg_thread_pool_t *)worker->pool;

    LDG_WRITE_ONCE(worker->state, LDG_THREAD_POOL_WORKER_RUNNING);

    while (!LDG_READ_ONCE(worker->should_stop))
    {
        if (pool->task_queue)
        {
            ret = ldg_mpmc_wait(pool->task_queue, &task, 100);
            if (ret == LDG_ERR_AOK && task.func) { task.func(task.arg); }
        }
        else if (worker->func) { worker->func(worker->func_arg); }

        LDG_PAUSE;
    }

    LDG_WRITE_ONCE(worker->state, LDG_THREAD_POOL_WORKER_STOPPED);

    return NULL;
}

int32_t ldg_thread_pool_init(ldg_thread_pool_t *pool, uint32_t worker_cunt)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(worker_cunt == 0 || worker_cunt > LDG_THREAD_POOL_MAX_WORKERS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    (void)memset(pool, 0, sizeof(ldg_thread_pool_t));

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

void ldg_thread_pool_shutdown(ldg_thread_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return; }

    (void)ldg_thread_pool_stop(pool);

    if (pool->task_queue)
    {
        ldg_mpmc_shutdown(pool->task_queue);
        free(pool->task_queue);
        pool->task_queue = NULL;
    }

    pool->submit_mode = 0;
    pool->is_init = 0;
}

int32_t ldg_thread_pool_start(ldg_thread_pool_t *pool, ldg_thread_pool_worker_func_t func, void *arg)
{
    uint32_t i = 0;
    int ret = 0;
    pthread_attr_t attr;

    if (LDG_UNLIKELY(!pool || !pool->is_init || !func)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(LDG_READ_ONCE(pool->is_running))) { return LDG_ERR_FUNC_ARG_INVALID; }

    LDG_WRITE_ONCE(pool->is_running, 1);

    for (i = 0; i < pool->worker_cunt; i++)
    {
        pool->workers[i].func = func;
        pool->workers[i].func_arg = arg;

        LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STARTING);
        LDG_WRITE_ONCE(pool->workers[i].should_stop, 0);

        (void)pthread_attr_init(&attr);

        ret = pthread_create(&pool->workers[i].handle, &attr, ldg_thread_pool_worker_entry, &pool->workers[i]);

        (void)pthread_attr_destroy(&attr);

        if (LDG_UNLIKELY(ret != 0))
        {
            LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STOPPED);
            continue;
        }
    }

    return LDG_ERR_AOK;
}

int32_t ldg_thread_pool_stop(ldg_thread_pool_t *pool)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!LDG_READ_ONCE(pool->is_running))) { return LDG_ERR_AOK; }

    for (i = 0; i < pool->worker_cunt; i++) { LDG_WRITE_ONCE(pool->workers[i].should_stop, 1); }

    LDG_SMP_MB();

    for (i = 0; i < pool->worker_cunt; i++) { if (pool->workers[i].handle != 0)
        {
            (void)pthread_join(pool->workers[i].handle, NULL);
            pool->workers[i].handle = 0;
        }
    }

    LDG_WRITE_ONCE(pool->is_running, 0);

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_pool_worker_cunt_get(ldg_thread_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return 0; }

    return pool->worker_cunt;
}

static int32_t thread_pool_submit_workers_start(ldg_thread_pool_t *pool)
{
    uint32_t i = 0;
    int ret = 0;
    pthread_attr_t attr;

    LDG_WRITE_ONCE(pool->is_running, 1);

    for (i = 0; i < pool->worker_cunt; i++)
    {
        pool->workers[i].func = NULL;
        pool->workers[i].func_arg = NULL;

        LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STARTING);
        LDG_WRITE_ONCE(pool->workers[i].should_stop, 0);

        (void)pthread_attr_init(&attr);

        ret = pthread_create(&pool->workers[i].handle, &attr, ldg_thread_pool_worker_entry, &pool->workers[i]);

        (void)pthread_attr_destroy(&attr);

        if (LDG_UNLIKELY(ret != 0))
        {
            LDG_WRITE_ONCE(pool->workers[i].state, LDG_THREAD_POOL_WORKER_STOPPED);
            continue;
        }
    }

    return LDG_ERR_AOK;
}

int32_t ldg_thread_pool_submit(ldg_thread_pool_t *pool, ldg_thread_pool_worker_func_t func, void *arg)
{
    ldg_thread_pool_task_t task = { 0 };
    int32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !pool->is_init || !func)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (!pool->task_queue)
    {
        pool->task_queue = (ldg_mpmc_queue_t *)malloc(sizeof(ldg_mpmc_queue_t));
        if (LDG_UNLIKELY(!pool->task_queue)) { return LDG_ERR_ALLOC_NULL; }

        ret = ldg_mpmc_init(pool->task_queue, sizeof(ldg_thread_pool_task_t), LDG_THREAD_POOL_TASK_QUEUE_CAPACITY);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            free(pool->task_queue);
            pool->task_queue = NULL;
            return ret;
        }

        LDG_WRITE_ONCE(pool->submit_mode, 1);
    }

    if (!LDG_READ_ONCE(pool->is_running)) { (void)thread_pool_submit_workers_start(pool); }

    task.func = func;
    task.arg = arg;

    return ldg_mpmc_push(pool->task_queue, &task);
}
