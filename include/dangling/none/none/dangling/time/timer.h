#ifndef LDG_TIME_TIMER_H
#define LDG_TIME_TIMER_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_TIME_TIMER_WHEEL_SIZE 1024
#define LDG_TIME_TIMER_MAX 4096
#define LDG_TIME_TIMER_ID_INVALID 0

typedef uint32_t ldg_time_timer_id_t;

typedef void (*ldg_time_timer_cb_t)(ldg_time_timer_id_t id, void *arg);

typedef struct ldg_time_timer
{
    uint64_t interval_ns;
    uint64_t next_fire_ns;
    ldg_time_timer_cb_t func;
    void *arg;
    ldg_time_timer_id_t id;
    uint32_t bkt_idx;
    uint32_t next_in_bkt;
    uint8_t repeat;
    uint8_t is_active;
    uint8_t pudding[18];
} LDG_ALIGNED ldg_time_timer_t;

typedef struct ldg_time_timer_bkt
{
    uint32_t hd;
    uint32_t cunt;
    uint8_t pudding[56];
} LDG_ALIGNED ldg_time_timer_bkt_t;

typedef struct ldg_time_timer_wheel
{
    ldg_time_timer_t timers[LDG_TIME_TIMER_MAX];
    ldg_time_timer_bkt_t bkts[LDG_TIME_TIMER_WHEEL_SIZE];
    uint32_t free_list[LDG_TIME_TIMER_MAX];
    volatile uint64_t free_hd;
    uint64_t current_tick;
    uint64_t current_time_ns;
    uint64_t tick_interval_ns;
    uint32_t timer_cunt;
    uint8_t is_init;
    uint8_t pudding[27];
} LDG_ALIGNED ldg_time_timer_wheel_t;

LDG_EXPORT uint32_t ldg_time_timer_wheel_init(ldg_time_timer_wheel_t *wheel, uint64_t tick_interval_ns);
LDG_EXPORT uint32_t ldg_time_timer_wheel_shutdown(ldg_time_timer_wheel_t *wheel);
LDG_EXPORT uint32_t ldg_time_timer_create(ldg_time_timer_wheel_t *wheel, uint64_t interval_ns, uint8_t repeat, ldg_time_timer_cb_t func, void *arg, ldg_time_timer_id_t *out_id);
LDG_EXPORT uint32_t ldg_time_timer_destroy(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id);
LDG_EXPORT uint8_t ldg_time_timer_active_is(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id);
LDG_EXPORT uint32_t ldg_time_timer_rst(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id);
LDG_EXPORT uint32_t ldg_time_timer_interval_set(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id, uint64_t interval_ns);
LDG_EXPORT uint32_t ldg_time_timer_wheel_tick(ldg_time_timer_wheel_t *wheel, uint64_t current_time_ns);
LDG_EXPORT uint64_t ldg_time_timer_remaining_get(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id);
LDG_EXPORT uint64_t ldg_time_timer_cunt_get(ldg_time_timer_wheel_t *wheel);

#endif
