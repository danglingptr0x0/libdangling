#ifndef LDG_SYS_PROGRESS_H
#define LDG_SYS_PROGRESS_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_SYS_PROGRESS_MODE_BAR 0
#define LDG_SYS_PROGRESS_MODE_SPINNER 1

#define LDG_SYS_PROGRESS_UNIT_RAW 0
#define LDG_SYS_PROGRESS_UNIT_SI 1
#define LDG_SYS_PROGRESS_UNIT_IEC 2

#define LDG_SYS_PROGRESS_FLAG_PCT 0x1U
#define LDG_SYS_PROGRESS_FLAG_CUNT 0x2U
#define LDG_SYS_PROGRESS_FLAG_RATE 0x4U
#define LDG_SYS_PROGRESS_FLAG_ETA 0x8U
#define LDG_SYS_PROGRESS_FLAG_COLOR 0x10U
#define LDG_SYS_PROGRESS_FLAG_ASCII 0x20U

#define LDG_SYS_PROGRESS_PCT_SCALE 10000

typedef struct ldg_sys_progress_conf_t
{
    const char *label;
    uint64_t total;
    uint64_t time_freq;
    uint64_t rate_tau;
    uint32_t label_len;
    uint32_t width;
    uint32_t mode;
    uint32_t unit;
    uint32_t flags;
    uint8_t pudding[4];
} ldg_sys_progress_conf_t;

typedef struct ldg_sys_progress_t
{
    ldg_sys_progress_conf_t conf;
    uint64_t cur;
    uint64_t start_time;
    uint64_t last_time;
    uint64_t last_cur;
    double rate;
    uint32_t spinner_phase;
    uint8_t rate_valid;
    uint8_t done;
    uint8_t is_init;
    uint8_t pudding[1];
} ldg_sys_progress_t;

LDG_EXPORT uint32_t ldg_sys_progress_conf_default_get(ldg_sys_progress_conf_t *out);
LDG_EXPORT uint32_t ldg_sys_progress_init(ldg_sys_progress_t *bar, const ldg_sys_progress_conf_t *conf, uint64_t now);
LDG_EXPORT uint32_t ldg_sys_progress_rst(ldg_sys_progress_t *bar, uint64_t now);
LDG_EXPORT uint32_t ldg_sys_progress_advance(ldg_sys_progress_t *bar, uint64_t delta, uint64_t now);
LDG_EXPORT uint32_t ldg_sys_progress_cur_set(ldg_sys_progress_t *bar, uint64_t cur, uint64_t now);
LDG_EXPORT uint32_t ldg_sys_progress_total_set(ldg_sys_progress_t *bar, uint64_t total);
LDG_EXPORT uint32_t ldg_sys_progress_finish(ldg_sys_progress_t *bar, uint64_t now);
LDG_EXPORT uint32_t ldg_sys_progress_render(ldg_sys_progress_t *bar, uint8_t *out, uint64_t cap, uint64_t *len);
LDG_EXPORT uint64_t ldg_sys_progress_rate_get(const ldg_sys_progress_t *bar);
LDG_EXPORT uint64_t ldg_sys_progress_eta_get(const ldg_sys_progress_t *bar);
LDG_EXPORT uint32_t ldg_sys_progress_pct_get(const ldg_sys_progress_t *bar);
LDG_EXPORT uint64_t ldg_sys_progress_cur_get(const ldg_sys_progress_t *bar);
LDG_EXPORT uint8_t ldg_sys_progress_done_is(const ldg_sys_progress_t *bar);

#endif
