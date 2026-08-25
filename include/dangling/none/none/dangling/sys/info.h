#ifndef LDG_SYS_INFO_H
#define LDG_SYS_INFO_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_sys_hostname_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_sys_cpu_cunt_get(uint32_t *cunt);
LDG_EXPORT uint32_t ldg_sys_page_size_get(uint64_t *size);
LDG_EXPORT uint32_t ldg_sys_mem_total_get(uint64_t *out);
LDG_EXPORT uint32_t ldg_sys_mem_avail_get(uint64_t *out);

#define LDG_SYS_CPU_CACHE_L1 1
#define LDG_SYS_CPU_CACHE_L2 2
#define LDG_SYS_CPU_CACHE_L3 3

LDG_EXPORT uint32_t ldg_sys_cpu_cache_width_get(uint32_t cpu_idx, uint32_t level, uint64_t *out);
LDG_EXPORT uint32_t ldg_sys_cpu_class_get(uint32_t cpu_idx, uint32_t *out);
LDG_EXPORT uint32_t ldg_sys_env_get(const char *name, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_sys_pid_get(uint64_t *pid);
LDG_EXPORT uint32_t ldg_sys_url_open(const char *url);
LDG_EXPORT uint32_t ldg_sys_url_scheme_reg(const char *scheme, const char *exec_path, const char *app_name);
LDG_EXPORT uint32_t ldg_sys_secret_set(const char *app, const char *key, const char *secret);
LDG_EXPORT uint32_t ldg_sys_secret_get(const char *app, const char *key, char *out, uint64_t out_size);

#define LDG_SYS_MEDIA_TEXT_MAX 256

typedef enum ldg_sys_media_status
{
    LDG_SYS_MEDIA_STATUS_CLOSED = 0,
    LDG_SYS_MEDIA_STATUS_OPENED,
    LDG_SYS_MEDIA_STATUS_CHANGING,
    LDG_SYS_MEDIA_STATUS_STOPPED,
    LDG_SYS_MEDIA_STATUS_PLAYING,
    LDG_SYS_MEDIA_STATUS_PAUSED
} ldg_sys_media_status_t;

typedef struct ldg_sys_media_info
{
    char title[LDG_SYS_MEDIA_TEXT_MAX];
    char artist[LDG_SYS_MEDIA_TEXT_MAX];
    char album[LDG_SYS_MEDIA_TEXT_MAX];
    ldg_sys_media_status_t status;
} ldg_sys_media_info_t;

LDG_EXPORT uint32_t ldg_sys_media_info_get(ldg_sys_media_info_t *out);
LDG_EXPORT uint32_t ldg_sys_media_play(void);
LDG_EXPORT uint32_t ldg_sys_media_pause(void);
LDG_EXPORT uint32_t ldg_sys_media_next_skip(void);
LDG_EXPORT uint32_t ldg_sys_media_prev_skip(void);

typedef enum ldg_sys_notify_level
{
    LDG_SYS_NOTIFY_LEVEL_LOW = 0,
    LDG_SYS_NOTIFY_LEVEL_NORMAL,
    LDG_SYS_NOTIFY_LEVEL_CRIT
} ldg_sys_notify_level_t;

LDG_EXPORT uint32_t ldg_sys_notify(const char *title, const char *body, ldg_sys_notify_level_t level);
LDG_EXPORT uint32_t ldg_sys_notify_close(void);
LDG_EXPORT uint32_t ldg_sys_notify_icon_set(const char *path);
LDG_EXPORT uint32_t ldg_sys_notify_tray_icon_set(const char *path);
LDG_EXPORT uint32_t ldg_sys_notify_tip_set(const char *tip);
LDG_EXPORT uint8_t ldg_sys_focus_is(void);

#endif
