#ifdef LDG_AUDIO_PIPEWIRE
#include <string.h>
#endif

#include <dangling/sys/window.h>
#include <dangling/sys/info.h>
#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_WINDOW_TERM_ENV "WINDOWID"
#define LDG_SYS_WINDOW_TERM_ID_STR_MAX 24
#define LDG_SYS_WINDOW_TERM_ID_NONE 0

#ifdef LDG_AUDIO_PIPEWIRE
#include <pipewire/pipewire.h>

#define LDG_SYS_WINDOW_CAPTURE_MEDIA_CLASS "Video/Source"
#define LDG_SYS_WINDOW_CAPTURE_SESH_ENV "WAYLAND_DISPLAY"
#define LDG_SYS_WINDOW_CAPTURE_SESH_NAME_MAX 108
#define LDG_SYS_WINDOW_CAPTURE_USER_SIZE_NONE 0
#define LDG_SYS_WINDOW_CAPTURE_SYNC_SEQ_FIRST 0

typedef struct sys_window_capture_scan
{
    struct pw_main_loop *loop;
    int32_t sync_seq;
    uint8_t found;
    uint8_t done;
    uint8_t pudding[2];
} sys_window_capture_scan_t;

static void sys_window_capture_glob_cb(void *data, __attribute__((unused)) uint32_t id, __attribute__((unused)) uint32_t perm, const char *type, __attribute__((unused)) uint32_t version, const struct spa_dict *props)
{
    sys_window_capture_scan_t *scan = (sys_window_capture_scan_t *)data;
    const char *media_class = 0x0;

    if (!props || strcmp(type, PW_TYPE_INTERFACE_Node) != 0) { return; }

    media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!media_class || strcmp(media_class, LDG_SYS_WINDOW_CAPTURE_MEDIA_CLASS) != 0) { return; }

    if (spa_dict_lookup(props, PW_KEY_DEVICE_ID)) { return; }

    scan->found = LDG_TRUTH_TRUE;
}

static void sys_window_capture_done_cb(void *data, uint32_t id, int32_t seq)
{
    sys_window_capture_scan_t *scan = (sys_window_capture_scan_t *)data;

    if (id != PW_ID_CORE || seq != scan->sync_seq) { return; }

    scan->done = LDG_TRUTH_TRUE;
    pw_main_loop_quit(scan->loop);
}

static void sys_window_capture_err_cb(void *data, __attribute__((unused)) uint32_t id, __attribute__((unused)) int32_t seq, __attribute__((unused)) int32_t res, __attribute__((unused)) const char *msg)
{
    sys_window_capture_scan_t *scan = (sys_window_capture_scan_t *)data;

    pw_main_loop_quit(scan->loop);
}

#endif

uint32_t ldg_sys_window_flash_conf_default_get(ldg_sys_window_flash_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->cunt = LDG_SYS_WINDOW_FLASH_CUNT_DEFAULT;
    out->period = LDG_SYS_WINDOW_FLASH_PERIOD_SYS;
    out->tgt = LDG_SYS_WINDOW_FLASH_TGT_ALL;
    out->until_focus = LDG_TRUTH_FALSE;
    out->pudding[0] = 0;
    out->pudding[1] = 0;
    out->pudding[2] = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_window_flash(void *native_window, const ldg_sys_window_flash_conf_t *conf)
{
    if (LDG_UNLIKELY(!native_window || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_window_flash_stop(void *native_window)
{
    if (LDG_UNLIKELY(!native_window)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_window_term_get(void **out)
{
    char id_str[LDG_SYS_WINDOW_TERM_ID_STR_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t id = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_sys_env_get(LDG_SYS_WINDOW_TERM_ENV, id_str, sizeof(id_str));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_uint_parse(id_str, LDG_BASE_DEC, sizeof(id_str), &id);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(id == LDG_SYS_WINDOW_TERM_ID_NONE)) { return LDG_ERR_NOT_FOUND; }

    *out = (void *)(uintptr_t)id;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_window_capture_exclude_set(void *native_window, uint8_t en)
{
    if (LDG_UNLIKELY(!native_window)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

#ifdef LDG_AUDIO_PIPEWIRE

uint8_t ldg_sys_window_capture_active_is(void)
{
    struct pw_registry_events registry_events = LDG_STRUCT_ZERO_INIT;
    struct pw_core_events core_events = LDG_STRUCT_ZERO_INIT;
    struct spa_hook registry_listener = LDG_STRUCT_ZERO_INIT;
    struct spa_hook core_listener = LDG_STRUCT_ZERO_INIT;
    sys_window_capture_scan_t scan = LDG_STRUCT_ZERO_INIT;
    char sesh_name[LDG_SYS_WINDOW_CAPTURE_SESH_NAME_MAX] = LDG_ARR_ZERO_INIT;
    struct pw_context *ctx = 0x0;
    struct pw_core *core = 0x0;
    struct pw_registry *registry = 0x0;

    if (ldg_sys_env_get(LDG_SYS_WINDOW_CAPTURE_SESH_ENV, sesh_name, sizeof(sesh_name)) == LDG_ERR_NOT_FOUND) { return LDG_TRUTH_INDETERMINATE; }

    registry_events.version = PW_VERSION_REGISTRY_EVENTS;
    registry_events.global = sys_window_capture_glob_cb;
    core_events.version = PW_VERSION_CORE_EVENTS;
    core_events.done = sys_window_capture_done_cb;
    core_events.error = sys_window_capture_err_cb;

    pw_init(0x0, 0x0);

    scan.loop = pw_main_loop_new(0x0);
    if (LDG_UNLIKELY(!scan.loop)) { return LDG_TRUTH_INDETERMINATE; }

    ctx = pw_context_new(pw_main_loop_get_loop(scan.loop), 0x0, LDG_SYS_WINDOW_CAPTURE_USER_SIZE_NONE);
    if (LDG_UNLIKELY(!ctx))
    {
        pw_main_loop_destroy(scan.loop);
        return LDG_TRUTH_INDETERMINATE;
    }

    core = pw_context_connect(ctx, 0x0, LDG_SYS_WINDOW_CAPTURE_USER_SIZE_NONE);
    if (LDG_UNLIKELY(!core))
    {
        pw_context_destroy(ctx);
        pw_main_loop_destroy(scan.loop);
        return LDG_TRUTH_INDETERMINATE;
    }

    pw_core_add_listener(core, &core_listener, &core_events, &scan);

    registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, LDG_SYS_WINDOW_CAPTURE_USER_SIZE_NONE);
    if (LDG_UNLIKELY(!registry))
    {
        spa_hook_remove(&core_listener);
        pw_core_disconnect(core);
        pw_context_destroy(ctx);
        pw_main_loop_destroy(scan.loop);
        return LDG_TRUTH_INDETERMINATE;
    }

    pw_registry_add_listener(registry, &registry_listener, &registry_events, &scan);

    scan.sync_seq = pw_core_sync(core, PW_ID_CORE, LDG_SYS_WINDOW_CAPTURE_SYNC_SEQ_FIRST);
    pw_main_loop_run(scan.loop);

    spa_hook_remove(&registry_listener);
    spa_hook_remove(&core_listener);
    pw_proxy_destroy((struct pw_proxy *)registry);
    pw_core_disconnect(core);
    pw_context_destroy(ctx);
    pw_main_loop_destroy(scan.loop);

    if (LDG_UNLIKELY(scan.done != LDG_TRUTH_TRUE)) { return LDG_TRUTH_INDETERMINATE; }

    return scan.found;
}

#else

uint8_t ldg_sys_window_capture_active_is(void)
{
    return LDG_TRUTH_INDETERMINATE;
}

#endif
