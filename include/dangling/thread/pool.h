#ifndef LDG_THREAD_POOL_H
#define LDG_THREAD_POOL_H

#include <stdint.h>
#include <pthread.h>
#include <dangling/core/macros.h>

#define LDG_THREAD_POOL_MAX_WORKERS 64

typedef void (*ldg_thread_pool_worker_func_t)(void *arg);

typedef enum ldg_thread_pool_worker_state
{
    LDG_THREAD_POOL_WORKER_IDLE = 0,
    LDG_THREAD_POOL_WORKER_STARTING,
    LDG_THREAD_POOL_WORKER_RUNNING,
    LDG_THREAD_POOL_WORKER_STOPPING,
    LDG_THREAD_POOL_WORKER_STOPPED
} ldg_thread_pool_worker_state_t;

typedef struct ldg_thread_pool_worker
{
    pthread_t handle;
    uint32_t id;
    uint32_t core_id;
    volatile int32_t state;
    volatile int32_t should_stop;
    ldg_thread_pool_worker_func_t func;
    void *func_arg;
    void *pool;
    uint8_t pudding[16];
} LDG_ALIGNED ldg_thread_pool_worker_t;

typedef struct ldg_thread_pool
{
    ldg_thread_pool_worker_t workers[LDG_THREAD_POOL_MAX_WORKERS];
    uint32_t worker_cunt;
    volatile int32_t is_running;
    uint8_t is_init;
    uint8_t pudding[55];
} LDG_ALIGNED ldg_thread_pool_t;

LDG_EXPORT int32_t ldg_thread_pool_init(ldg_thread_pool_t *pool, uint32_t worker_cunt);
LDG_EXPORT void ldg_thread_pool_shutdown(ldg_thread_pool_t *pool);
LDG_EXPORT int32_t ldg_thread_pool_start(ldg_thread_pool_t *pool, ldg_thread_pool_worker_func_t func, void *arg);
LDG_EXPORT int32_t ldg_thread_pool_stop(ldg_thread_pool_t *pool);
LDG_EXPORT uint32_t ldg_thread_pool_worker_cunt_get(ldg_thread_pool_t *pool);

#endif
