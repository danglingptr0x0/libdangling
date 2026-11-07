#ifdef LDG_AUDIO_PIPEWIRE

#include <stdlib.h>
#include <string.h>

#include <dangling/audio/audio.h>
#include <dangling/arch/amd64/fence.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/str.h>
#include <pipewire/pipewire.h>
#include <spa/param/props.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <spa/pod/iter.h>
#include <pipewire/extensions/metadata.h>

#define LDG_AUDIO_NODE_STREAM 1
#define LDG_AUDIO_NODE_SINK 2
#define LDG_AUDIO_NODE_SRC 3

#define LDG_AUDIO_DUCK_STACK_MAX 16

#define LDG_AUDIO_META_NAME_DEFAULT "default"
#define LDG_AUDIO_META_KEY_NAME "metadata.name"
#define LDG_AUDIO_META_TYPE_JSON "Spa:String:JSON"
#define LDG_AUDIO_META_KEY_SINK "default.audio.sink"
#define LDG_AUDIO_META_KEY_SRC "default.audio.source"
#define LDG_AUDIO_META_KEY_TGT_OBJECT "target.object"
#define LDG_AUDIO_META_KEY_TGT_NODE "target.node"
#define LDG_AUDIO_KEY_OBJECT_SERIAL "object.serial"
#define LDG_AUDIO_META_JSON_NAME "\"name\""
#define LDG_AUDIO_META_JSON_SEP ':'
#define LDG_AUDIO_META_JSON_QUOTE '"'
#define LDG_AUDIO_META_CB_OK 0
#define LDG_AUDIO_NODE_ID_NONE 0
#define LDG_AUDIO_DEC_DIGIT_BASE '0'
#define LDG_AUDIO_BIND_USER_SIZE_NONE 0
#define LDG_AUDIO_CHANNEL_CUNT_DEFAULT 2
#define LDG_AUDIO_PARAM_FLAGS_NONE 0
#define LDG_AUDIO_VOL_MIN 0.0
#define LDG_AUDIO_VOL_MAX 1.0

#define LDG_AUDIO_PCM_DEPTH_U8 8
#define LDG_AUDIO_PCM_DEPTH_S16 16
#define LDG_AUDIO_PCM_DEPTH_S24 24
#define LDG_AUDIO_PCM_DEPTH_S32 32
#define LDG_AUDIO_PCM_BITS_PER_BYTE 8
#define LDG_AUDIO_PCM_POD_MAX 1024
#define LDG_AUDIO_PCM_PARAM_CUNT 1
#define LDG_AUDIO_PCM_CTRL_CUNT 1
#define LDG_AUDIO_PCM_NONE 0
#define LDG_AUDIO_PCM_SILENCE 0

typedef struct ldg_audio_node
{
    uint32_t id;
    uint32_t type;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    double vol;
    uint32_t channel_cunt;
    uint32_t tgt_id;
    uint64_t serial;
    struct pw_proxy *proxy;
    struct spa_hook listener;
    struct ldg_audio_node *next;
    uint8_t muted;
    uint8_t pudding[7];
} ldg_audio_node_t;

typedef struct ldg_audio_duck_entry
{
    double factor;
    uint32_t *stream_ids;
    double *orig_vols;
    uint32_t exclude_stream_id;
    uint32_t stream_cunt;
} ldg_audio_duck_entry_t;

typedef struct ldg_audio_ctx
{
    struct pw_thread_loop *loop;
    struct pw_context *ctx;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;
    struct spa_hook core_listener;
    struct pw_proxy *metadata;
    struct spa_hook metadata_listener;
    char default_sink_name[LDG_AUDIO_NAME_MAX];
    char default_src_name[LDG_AUDIO_NAME_MAX];
    ldg_audio_node_t *streams;
    ldg_audio_node_t *sinks;
    ldg_audio_node_t *srcs;
    ldg_audio_duck_entry_t duck_stack[LDG_AUDIO_DUCK_STACK_MAX];
    uint32_t duck_cunt;
    uint32_t self_stream_id;
    int32_t sync_seq;
    volatile uint8_t is_init;
    uint8_t pudding[35];
} LDG_ALIGNED ldg_audio_ctx_t;

// singleton audio backend; file-scope static required for PipeWire callbacks
static ldg_audio_ctx_t g_audio_ctx = { .is_init = LDG_TRUTH_FALSE };

static void audio_node_free(ldg_audio_node_t *node, uint8_t destroy_proxy)
{
    if (!node) { return; }

    if (node->proxy)
    {
        spa_hook_remove(&node->listener);
        if (destroy_proxy) { pw_proxy_destroy(node->proxy); }
    }

    ldg_mem_dealloc(node);
}

static void audio_node_list_free(ldg_audio_node_t **hd, uint8_t destroy_proxy)
{
    ldg_audio_node_t *curr = 0x0;
    ldg_audio_node_t *next = 0x0;

    if (!hd || !*hd) { return; }

    curr = *hd;
    while (curr)
    {
        next = curr->next;
        audio_node_free(curr, destroy_proxy);
        curr = next;
    }

    *hd = 0x0;
}

static ldg_audio_node_t* audio_node_find(ldg_audio_node_t *hd, uint32_t id)
{
    ldg_audio_node_t *curr = hd;

    while (curr)
    {
        if (curr->id == id) { return curr; }

        curr = curr->next;
    }

    return 0x0;
}

static void audio_node_remove(ldg_audio_node_t **hd, uint32_t id)
{
    ldg_audio_node_t *curr = 0x0;
    ldg_audio_node_t *prev = 0x0;

    if (!hd || !*hd) { return; }

    curr = *hd;
    while (curr)
    {
        if (curr->id == id)
        {
            if (prev) { prev->next = curr->next; }
            else { *hd = curr->next; }

            audio_node_free(curr, 1);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

static void audio_name_fold_cpy(char *dst, const char *src, uint64_t cap)
{
    uint64_t idx = 0;
    uint8_t ch = 0;

    if (!dst || cap == 0) { return; }

    dst[0] = LDG_STR_TERM;

    if (!src) { return; }

    while (idx + LDG_STR_TERM_SIZE < cap && src[idx] != LDG_STR_TERM)
    {
        ch = (uint8_t)src[idx];
        dst[idx] = (ch < LDG_ASCII_PRINTABLE_MIN || ch > LDG_ASCII_PRINTABLE_MAX) ? LDG_ASCII_NONPRINTABLE_CHAR : (char)ch;
        idx++;
    }

    dst[idx] = LDG_STR_TERM;
}

static uint8_t audio_dec_parse_is(const char *text, uint64_t *out)
{
    uint64_t val = 0;
    uint64_t digit = 0;
    uint64_t idx = 0;

    *out = 0;

    if (!text || text[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    for (idx = 0; text[idx] != LDG_STR_TERM; idx++)
    {
        if (ldg_char_digit_is((uint8_t)text[idx]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        digit = (uint64_t)((uint8_t)text[idx] - (uint8_t)LDG_AUDIO_DEC_DIGIT_BASE);

        if (val > (UINT64_MAX - digit) / LDG_BASE_DEC) { return LDG_TRUTH_FALSE; }

        val = (val * LDG_BASE_DEC) + digit;
    }

    *out = val;

    return LDG_TRUTH_TRUE;
}

static ldg_audio_node_t* audio_node_name_find(ldg_audio_node_t *hd, const char *name)
{
    ldg_audio_node_t *curr = hd;

    if (!name || name[0] == LDG_STR_TERM) { return 0x0; }

    while (curr)
    {
        if (strcmp(curr->name, name) == 0) { return curr; }

        curr = curr->next;
    }

    return 0x0;
}

static uint32_t audio_tgt_resolve(const char *text)
{
    ldg_audio_node_t *curr = 0x0;
    char name[LDG_AUDIO_NAME_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t serial = 0;

    if (!text) { return LDG_AUDIO_NODE_ID_NONE; }

    if (audio_dec_parse_is(text, &serial) == LDG_TRUTH_TRUE)
    {
        for (curr = g_audio_ctx.sinks; curr; curr = curr->next) { if (curr->serial == serial) { return curr->id; } }

        return LDG_AUDIO_NODE_ID_NONE;
    }

    audio_name_fold_cpy(name, text, LDG_AUDIO_NAME_MAX);
    curr = audio_node_name_find(g_audio_ctx.sinks, name);

    return curr ? curr->id : LDG_AUDIO_NODE_ID_NONE;
}

static void audio_meta_name_take(char *dst, const char *json)
{
    const char *at = 0x0;
    uint64_t len = 0;

    dst[0] = LDG_STR_TERM;

    if (!json) { return; }

    at = strstr(json, LDG_AUDIO_META_JSON_NAME);
    if (!at) { return; }

    at = strchr(at + strlen(LDG_AUDIO_META_JSON_NAME), LDG_AUDIO_META_JSON_SEP);
    if (!at) { return; }

    at = strchr(at, LDG_AUDIO_META_JSON_QUOTE);
    if (!at) { return; }

    at++;

    while (at[len] != LDG_STR_TERM && at[len] != LDG_AUDIO_META_JSON_QUOTE && len + LDG_STR_TERM_SIZE < LDG_AUDIO_NAME_MAX) { len++; }

    if (at[len] != LDG_AUDIO_META_JSON_QUOTE) { return; }

    audio_name_fold_cpy(dst, at, len + LDG_STR_TERM_SIZE);
}

static int32_t audio_meta_property(void *data, uint32_t subject, const char *key, const char *type, const char *text)
{
    ldg_audio_ctx_t *ctx = (ldg_audio_ctx_t *)data;
    ldg_audio_node_t *node = 0x0;
    uint8_t json_is = LDG_TRUTH_FALSE;

    if (!ctx || !key) { return LDG_AUDIO_META_CB_OK; }

    if (type && strcmp(type, LDG_AUDIO_META_TYPE_JSON) == 0) { json_is = LDG_TRUTH_TRUE; }

    if (strcmp(key, LDG_AUDIO_META_KEY_SINK) == 0)
    {
        if (json_is == LDG_TRUTH_TRUE || !text) { audio_meta_name_take(ctx->default_sink_name, text); }

        return LDG_AUDIO_META_CB_OK;
    }

    if (strcmp(key, LDG_AUDIO_META_KEY_SRC) == 0)
    {
        if (json_is == LDG_TRUTH_TRUE || !text) { audio_meta_name_take(ctx->default_src_name, text); }

        return LDG_AUDIO_META_CB_OK;
    }

    if (strcmp(key, LDG_AUDIO_META_KEY_TGT_OBJECT) != 0 && strcmp(key, LDG_AUDIO_META_KEY_TGT_NODE) != 0) { return LDG_AUDIO_META_CB_OK; }

    node = audio_node_find(ctx->streams, subject);
    if (node) { node->tgt_id = audio_tgt_resolve(text); }

    return LDG_AUDIO_META_CB_OK;
}

static const struct pw_metadata_events audio_meta_events = {
    PW_VERSION_METADATA_EVENTS, .property = audio_meta_property, };

static void node_event_param(void *data, __attribute__((unused)) int seq, uint32_t id, __attribute__((unused)) uint32_t idx, __attribute__((unused)) uint32_t next, const struct spa_pod *param)
{
    ldg_audio_node_t *node = (ldg_audio_node_t *)data;
    const struct spa_pod_prop *prop = 0x0;
    const float *vols = 0x0;
    uint32_t n_vols = 0;
    uint32_t val_size = 0;
    uint32_t val_type = 0;
    uint32_t mute_val = 0;
    uint32_t ch_idx = 0;
    double sum = 0.0;

    if (!param || id != SPA_PARAM_Props) { return; }

    if (!spa_pod_is_object(param)) { return; }

    prop = spa_pod_find_prop(param, 0x0, SPA_PROP_channelVolumes);

    if (prop) { vols = (const float *)spa_pod_get_array_full(&prop->value, &n_vols, &val_size, &val_type); }

    if (vols && val_type == SPA_TYPE_Float && val_size == (uint32_t)sizeof(*vols) && n_vols > 0 && n_vols <= SPA_AUDIO_MAX_CHANNELS)
    {
        node->channel_cunt = n_vols;

        for (ch_idx = 0; ch_idx < n_vols; ch_idx++) { sum += (double)vols[ch_idx]; }

        node->vol = sum / (double)n_vols;

        if (node->vol < LDG_AUDIO_VOL_MIN) { node->vol = LDG_AUDIO_VOL_MIN; }

        if (node->vol > LDG_AUDIO_VOL_MAX) { node->vol = LDG_AUDIO_VOL_MAX; }
    }

    prop = spa_pod_find_prop(param, 0x0, SPA_PROP_mute);

    if (!prop || !spa_pod_is_bool(&prop->value)) { return; }

    if (ldg_mem_cpy(&mute_val, SPA_POD_BODY_CONST(&prop->value), sizeof(mute_val)) != LDG_ERR_AOK) { return; }

    node->muted = (mute_val != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static const struct pw_node_events node_events = {
    PW_VERSION_NODE_EVENTS, .param = node_event_param, };

static void registry_event_glob(__attribute__((unused)) void *data, uint32_t id, __attribute__((unused)) uint32_t permissions, const char *type, __attribute__((unused)) uint32_t version, const struct spa_dict *props)
{
    const char *media_class = 0x0;
    const char *meta_name = 0x0;
    const char *name = 0x0;
    const char *desc = 0x0;
    const char *serial_text = 0x0;
    ldg_audio_node_t *node = 0x0;
    ldg_audio_node_t **list = 0x0;
    uint32_t node_type = 0;

    if (!props) { return; }

    if (strcmp(type, PW_TYPE_INTERFACE_Metadata) == 0)
    {
        meta_name = spa_dict_lookup(props, LDG_AUDIO_META_KEY_NAME);

        if (!meta_name || strcmp(meta_name, LDG_AUDIO_META_NAME_DEFAULT) != 0 || g_audio_ctx.metadata) { return; }

        g_audio_ctx.metadata = (struct pw_proxy *)pw_registry_bind(g_audio_ctx.registry, id, type, PW_VERSION_METADATA, LDG_AUDIO_BIND_USER_SIZE_NONE);
        if (g_audio_ctx.metadata) { pw_metadata_add_listener((struct pw_metadata *)g_audio_ctx.metadata, &g_audio_ctx.metadata_listener, &audio_meta_events, &g_audio_ctx); }

        return;
    }

    if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0) { return; }

    media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    if (!media_class) { return; }

    if (strcmp(media_class, "Stream/Output/Audio") == 0)
    {
        node_type = LDG_AUDIO_NODE_STREAM;
        list = &g_audio_ctx.streams;
    }
    else if (strcmp(media_class, "Audio/Sink") == 0)
    {
        node_type = LDG_AUDIO_NODE_SINK;
        list = &g_audio_ctx.sinks;
    }
    else if (strcmp(media_class, "Audio/Source") == 0)
    {
        node_type = LDG_AUDIO_NODE_SRC;
        list = &g_audio_ctx.srcs;
    }
    else { return; }

    if (LDG_UNLIKELY(ldg_mem_alloc(sizeof(ldg_audio_node_t), (void **)&node) != LDG_ERR_AOK)) { return; }

    if (LDG_UNLIKELY(memset(node, 0, sizeof(ldg_audio_node_t)) != node)) { ldg_mem_dealloc(node); return; }

    node->id = id;
    node->type = node_type;

    name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    if (name) { audio_name_fold_cpy(node->name, name, LDG_AUDIO_NAME_MAX); }

    desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    if (desc) { audio_name_fold_cpy(node->desc, desc, LDG_AUDIO_DESC_MAX); }
    else if (name) { audio_name_fold_cpy(node->desc, name, LDG_AUDIO_DESC_MAX); }

    serial_text = spa_dict_lookup(props, LDG_AUDIO_KEY_OBJECT_SERIAL);
    if (audio_dec_parse_is(serial_text, &node->serial) != LDG_TRUTH_TRUE) { node->serial = UINT64_MAX; }

    if (node_type == LDG_AUDIO_NODE_STREAM)
    {
        name = spa_dict_lookup(props, PW_KEY_APP_NAME);
        if (name) { audio_name_fold_cpy(node->name, name, LDG_AUDIO_NAME_MAX); }

        node->tgt_id = audio_tgt_resolve(spa_dict_lookup(props, LDG_AUDIO_META_KEY_TGT_OBJECT));
    }

    node->proxy = (struct pw_proxy *)pw_registry_bind(g_audio_ctx.registry, id, type, PW_VERSION_NODE, 0);
    if (node->proxy)
    {
        pw_proxy_add_object_listener(node->proxy, &node->listener, &node_events, node);
        pw_node_subscribe_params((struct pw_node *)node->proxy, (uint32_t[]) { SPA_PARAM_Props }, 1);
    }

    node->next = *list;
    *list = node;
}

static void registry_event_glob_remove(__attribute__((unused)) void *data, uint32_t id)
{
    audio_node_remove(&g_audio_ctx.streams, id);
    audio_node_remove(&g_audio_ctx.sinks, id);
    audio_node_remove(&g_audio_ctx.srcs, id);
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS, .global = registry_event_glob, .global_remove = registry_event_glob_remove, };

static void core_event_done(__attribute__((unused)) void *data, __attribute__((unused)) uint32_t id, int seq)
{
    if (g_audio_ctx.sync_seq == seq) { pw_thread_loop_signal(g_audio_ctx.loop, 0); }
}

static void core_event_err(__attribute__((unused)) void *data, __attribute__((unused)) uint32_t id, __attribute__((unused)) int seq, __attribute__((unused)) int res, __attribute__((unused)) const char *msg)
{
}

static const struct pw_core_events core_events = {
    PW_VERSION_CORE_EVENTS, .done = core_event_done, .error = core_event_err, };

uint32_t ldg_audio_init(void)
{
    if (g_audio_ctx.is_init == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ldg_mem_zero(&g_audio_ctx, sizeof(ldg_audio_ctx_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    pw_init(0x0, 0x0);

    g_audio_ctx.loop = pw_thread_loop_new("ldg_audio", 0x0);
    if (LDG_UNLIKELY(!g_audio_ctx.loop)) { return LDG_ERR_AUDIO_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    g_audio_ctx.ctx = pw_context_new(pw_thread_loop_get_loop(g_audio_ctx.loop), 0x0, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.ctx))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    g_audio_ctx.core = pw_context_connect(g_audio_ctx.ctx, 0x0, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.core))
    {
        pw_context_destroy(g_audio_ctx.ctx);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    pw_core_add_listener(g_audio_ctx.core, &g_audio_ctx.core_listener, &core_events, 0x0);

    g_audio_ctx.registry = pw_core_get_registry(g_audio_ctx.core, PW_VERSION_REGISTRY, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.registry))
    {
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.ctx);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    pw_registry_add_listener(g_audio_ctx.registry, &g_audio_ctx.registry_listener, &registry_events, 0x0);

    g_audio_ctx.is_init = LDG_TRUTH_TRUE;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    if (pw_thread_loop_start(g_audio_ctx.loop) < 0)
    {
        g_audio_ctx.is_init = LDG_TRUTH_FALSE;
        pw_thread_loop_lock(g_audio_ctx.loop);
        pw_proxy_destroy((struct pw_proxy *)g_audio_ctx.registry);
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.ctx);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    return LDG_ERR_AOK;
}

void ldg_audio_shutdown(void)
{
    uint32_t i = 0;

    if (g_audio_ctx.is_init != LDG_TRUTH_TRUE) { return; }

    g_audio_ctx.is_init = LDG_TRUTH_FALSE;

    pw_thread_loop_stop(g_audio_ctx.loop);

    audio_node_list_free(&g_audio_ctx.streams, 0);
    audio_node_list_free(&g_audio_ctx.sinks, 0);
    audio_node_list_free(&g_audio_ctx.srcs, 0);

    for (i = 0; i < LDG_AUDIO_DUCK_STACK_MAX; i++)
    {
        if (g_audio_ctx.duck_stack[i].stream_ids) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].stream_ids); }

        if (g_audio_ctx.duck_stack[i].orig_vols) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].orig_vols); }
    }

    if (g_audio_ctx.metadata)
    {
        spa_hook_remove(&g_audio_ctx.metadata_listener);
        pw_proxy_destroy(g_audio_ctx.metadata);
        g_audio_ctx.metadata = 0x0;
    }

    spa_hook_remove(&g_audio_ctx.registry_listener);
    spa_hook_remove(&g_audio_ctx.core_listener);

    pw_core_disconnect(g_audio_ctx.core);
    pw_context_destroy(g_audio_ctx.ctx);
    pw_thread_loop_destroy(g_audio_ctx.loop);

    pw_deinit();
}

uint32_t ldg_audio_sync(void)
{
    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    g_audio_ctx.sync_seq = pw_core_sync(g_audio_ctx.core, PW_ID_CORE, 0);
    pw_thread_loop_wait(g_audio_ctx.loop);

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

static uint32_t audio_node_vol_set(ldg_audio_node_t *node, double vol)
{
    uint8_t buff[LDG_AUDIO_PCM_POD_MAX] = LDG_ARR_ZERO_INIT;
    struct spa_pod_builder pod_wr = LDG_STRUCT_ZERO_INIT;
    struct spa_pod_frame props = LDG_STRUCT_ZERO_INIT;
    struct spa_pod_frame vols = LDG_STRUCT_ZERO_INIT;
    struct spa_pod *pod = 0x0;
    uint32_t idx = 0;
    uint32_t cunt = 0;

    if (!node || !node->proxy) { return LDG_ERR_FUNC_ARG_NULL; }

    spa_pod_builder_init(&pod_wr, buff, sizeof(buff));

    cunt = (node->channel_cunt > 0) ? node->channel_cunt : LDG_AUDIO_CHANNEL_CUNT_DEFAULT;

    spa_pod_builder_push_object(&pod_wr, &props, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
    spa_pod_builder_prop(&pod_wr, SPA_PROP_channelVolumes, LDG_AUDIO_PARAM_FLAGS_NONE);
    spa_pod_builder_push_array(&pod_wr, &vols);
    for (idx = 0; idx < cunt; idx++) { spa_pod_builder_float(&pod_wr, (float)vol); }
    spa_pod_builder_pop(&pod_wr, &vols);

    pod = (struct spa_pod *)spa_pod_builder_pop(&pod_wr, &props);
    if (LDG_UNLIKELY(!pod)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(pw_node_set_param((struct pw_node *)node->proxy, SPA_PARAM_Props, LDG_AUDIO_PARAM_FLAGS_NONE, pod) < 0)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

static ldg_audio_node_t* audio_default_sink_find(void)
{
    ldg_audio_node_t *node = 0x0;

    node = audio_node_name_find(g_audio_ctx.sinks, g_audio_ctx.default_sink_name);
    if (node) { return node; }

    return g_audio_ctx.sinks;
}

static ldg_audio_node_t* audio_default_src_find(void)
{
    ldg_audio_node_t *node = 0x0;

    node = audio_node_name_find(g_audio_ctx.srcs, g_audio_ctx.default_src_name);
    if (node) { return node; }

    return g_audio_ctx.srcs;
}

uint32_t ldg_audio_master_vol_get(double *vol)
{
    ldg_audio_node_t *sink = 0x0;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    sink = audio_default_sink_find();
    if (!sink)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    *vol = sink->vol;
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_master_vol_set(double vol)
{
    ldg_audio_node_t *sink = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOL_RANGE; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    sink = audio_default_sink_find();
    if (!sink)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    ret = audio_node_vol_set(sink, vol);
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

uint32_t ldg_audio_stream_vol_get(uint32_t stream_id, double *vol)
{
    ldg_audio_node_t *node = 0x0;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    node = audio_node_find(g_audio_ctx.streams, stream_id);
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    *vol = node->vol;
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_vol_set(uint32_t stream_id, double vol)
{
    ldg_audio_node_t *node = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOL_RANGE; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    node = audio_node_find(g_audio_ctx.streams, stream_id);
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    ret = audio_node_vol_set(node, vol);
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, uint8_t *muted)
{
    ldg_audio_node_t *node = 0x0;

    if (LDG_UNLIKELY(!muted)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (stream_id == 0) { node = audio_default_sink_find(); }
    else { node = audio_node_find(g_audio_ctx.streams, stream_id); }

    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return (stream_id == 0) ? LDG_ERR_AUDIO_NO_DEFAULT : LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    *muted = node->muted;
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, uint8_t muted)
{
    ldg_audio_node_t *node = 0x0;
    uint8_t buff[LDG_AUDIO_PCM_POD_MAX] = LDG_ARR_ZERO_INIT;
    struct spa_pod_builder pod_wr = LDG_STRUCT_ZERO_INIT;
    struct spa_pod_frame props = LDG_STRUCT_ZERO_INIT;
    struct spa_pod *pod = 0x0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (stream_id == 0) { node = audio_default_sink_find(); }
    else { node = audio_node_find(g_audio_ctx.streams, stream_id); }

    if (!node || !node->proxy)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return (stream_id == 0) ? LDG_ERR_AUDIO_NO_DEFAULT : LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    spa_pod_builder_init(&pod_wr, buff, sizeof(buff));
    spa_pod_builder_push_object(&pod_wr, &props, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
    spa_pod_builder_prop(&pod_wr, SPA_PROP_mute, LDG_AUDIO_PARAM_FLAGS_NONE);
    spa_pod_builder_bool(&pod_wr, muted == LDG_TRUTH_TRUE);
    pod = (struct spa_pod *)spa_pod_builder_pop(&pod_wr, &props);

    if (LDG_UNLIKELY(!pod)) { ret = LDG_ERR_OVERFLOW; }
    else if (LDG_UNLIKELY(pw_node_set_param((struct pw_node *)node->proxy, SPA_PARAM_Props, LDG_AUDIO_PARAM_FLAGS_NONE, pod) < 0)) { ret = LDG_ERR_INVALID; }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

uint32_t ldg_audio_stream_list(ldg_audio_stream_t **streams, uint32_t *cunt)
{
    ldg_audio_node_t *node = 0x0;
    ldg_audio_stream_t *arr = 0x0;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!streams || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.streams; node; node = node->next) { n++; }

    if (n == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        *streams = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    if (LDG_UNLIKELY(ldg_mem_alloc(n * sizeof(ldg_audio_stream_t), (void **)&arr) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(ldg_mem_zero(arr, n * sizeof(ldg_audio_stream_t)) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(arr);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    for (node = g_audio_ctx.streams; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        audio_name_fold_cpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);

        audio_name_fold_cpy(arr[i].app_name, node->desc, LDG_AUDIO_NAME_MAX);

        arr[i].vol = node->vol;
        arr[i].muted = node->muted;
        arr[i].sink_id = node->tgt_id;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    *streams = arr;
    *cunt = i;

    return LDG_ERR_AOK;
}

void ldg_audio_stream_free(ldg_audio_stream_t *streams)
{
    if (LDG_UNLIKELY(!streams)) { return; }

    ldg_mem_dealloc(streams);
}

uint32_t ldg_audio_stream_name_get(const char *name, ldg_audio_stream_t *stream)
{
    ldg_audio_node_t *node = 0x0;
    uint32_t ret = LDG_ERR_AUDIO_STREAM_NOT_FOUND;

    if (LDG_UNLIKELY(!name || !stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.streams; node; node = node->next)
    {
        if (strcmp(node->name, name) == 0)
        {
            if (LDG_UNLIKELY(ldg_mem_zero(stream, sizeof(ldg_audio_stream_t)) != LDG_ERR_AOK))
            {
                pw_thread_loop_unlock(g_audio_ctx.loop);
                return LDG_ERR_MEM_BAD;
            }

            stream->id = node->id;
            audio_name_fold_cpy(stream->name, node->name, LDG_AUDIO_NAME_MAX);

            audio_name_fold_cpy(stream->app_name, node->desc, LDG_AUDIO_NAME_MAX);

            stream->vol = node->vol;
            stream->muted = node->muted;
            stream->sink_id = node->tgt_id;
            ret = LDG_ERR_AOK;
            break;
        }
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

void ldg_audio_self_reg(uint32_t stream_id)
{
    g_audio_ctx.self_stream_id = stream_id;
}

uint64_t ldg_audio_self_id_get(void)
{
    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return (uint64_t)g_audio_ctx.self_stream_id;
}

uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream)
{
    ldg_audio_node_t *node = 0x0;

    if (LDG_UNLIKELY(!stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    node = audio_node_find(g_audio_ctx.streams, g_audio_ctx.self_stream_id);
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    if (LDG_UNLIKELY(ldg_mem_zero(stream, sizeof(ldg_audio_stream_t)) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    stream->id = node->id;
    audio_name_fold_cpy(stream->name, node->name, LDG_AUDIO_NAME_MAX);

    audio_name_fold_cpy(stream->app_name, node->desc, LDG_AUDIO_NAME_MAX);

    stream->vol = node->vol;
    stream->muted = node->muted;
    stream->sink_id = node->tgt_id;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_self_vol_get(double *vol)
{
    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    return ldg_audio_stream_vol_get(g_audio_ctx.self_stream_id, vol);
}

uint32_t ldg_audio_self_vol_set(double vol)
{
    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    return ldg_audio_stream_vol_set(g_audio_ctx.self_stream_id, vol);
}

uint32_t ldg_audio_sink_list(ldg_audio_sink_t **sinks, uint32_t *cunt)
{
    ldg_audio_node_t *node = 0x0;
    ldg_audio_sink_t *arr = 0x0;
    ldg_audio_node_t *dflt = 0x0;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sinks || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.sinks; node; node = node->next) { n++; }

    if (n == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        *sinks = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    if (LDG_UNLIKELY(ldg_mem_alloc(n * sizeof(ldg_audio_sink_t), (void **)&arr) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(ldg_mem_zero(arr, n * sizeof(ldg_audio_sink_t)) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(arr);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    dflt = audio_default_sink_find();

    for (node = g_audio_ctx.sinks; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        audio_name_fold_cpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);

        audio_name_fold_cpy(arr[i].desc, node->desc, LDG_AUDIO_DESC_MAX);

        arr[i].vol = node->vol;
        arr[i].muted = node->muted;
        arr[i].is_default = (node == dflt) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    *sinks = arr;
    *cunt = i;

    return LDG_ERR_AOK;
}

void ldg_audio_sink_free(ldg_audio_sink_t *sinks)
{
    if (LDG_UNLIKELY(!sinks)) { return; }

    ldg_mem_dealloc(sinks);
}

uint32_t ldg_audio_src_list(ldg_audio_src_t **srcs, uint32_t *cunt)
{
    ldg_audio_node_t *node = 0x0;
    ldg_audio_src_t *arr = 0x0;
    ldg_audio_node_t *dflt = 0x0;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!srcs || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.srcs; node; node = node->next) { n++; }

    if (n == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        *srcs = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    if (LDG_UNLIKELY(ldg_mem_alloc(n * sizeof(ldg_audio_src_t), (void **)&arr) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(ldg_mem_zero(arr, n * sizeof(ldg_audio_src_t)) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(arr);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    dflt = audio_default_src_find();

    for (node = g_audio_ctx.srcs; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        audio_name_fold_cpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);

        audio_name_fold_cpy(arr[i].desc, node->desc, LDG_AUDIO_DESC_MAX);

        arr[i].vol = node->vol;
        arr[i].muted = node->muted;
        arr[i].is_default = (node == dflt) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    *srcs = arr;
    *cunt = i;

    return LDG_ERR_AOK;
}

void ldg_audio_src_free(ldg_audio_src_t *srcs)
{
    if (LDG_UNLIKELY(!srcs)) { return; }

    ldg_mem_dealloc(srcs);
}

uint32_t ldg_audio_default_sink_get(ldg_audio_sink_t *sink)
{
    ldg_audio_node_t *node = 0x0;

    if (LDG_UNLIKELY(!sink)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    node = audio_default_sink_find();
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    if (LDG_UNLIKELY(ldg_mem_zero(sink, sizeof(ldg_audio_sink_t)) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    sink->id = node->id;
    audio_name_fold_cpy(sink->name, node->name, LDG_AUDIO_NAME_MAX);

    audio_name_fold_cpy(sink->desc, node->desc, LDG_AUDIO_DESC_MAX);

    sink->vol = node->vol;
    sink->muted = node->muted;
    sink->is_default = LDG_TRUTH_TRUE;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_default_src_get(ldg_audio_src_t *src)
{
    ldg_audio_node_t *node = 0x0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    node = audio_default_src_find();

    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    if (LDG_UNLIKELY(ldg_mem_zero(src, sizeof(ldg_audio_src_t)) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    src->id = node->id;
    audio_name_fold_cpy(src->name, node->name, LDG_AUDIO_NAME_MAX);

    audio_name_fold_cpy(src->desc, node->desc, LDG_AUDIO_DESC_MAX);

    src->vol = node->vol;
    src->muted = node->muted;
    src->is_default = LDG_TRUTH_TRUE;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_duck(double factor, uint32_t exclude_stream_id)
{
    ldg_audio_duck_entry_t *entry = 0x0;
    ldg_audio_node_t *node = 0x0;
    uint32_t ret = 0;
    uint32_t cunt = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(factor < 0.0 || factor > 1.0)) { return LDG_ERR_AUDIO_VOL_RANGE; }

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (g_audio_ctx.duck_cunt >= LDG_AUDIO_DUCK_STACK_MAX)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_DUCK_FULL;
    }

    for (node = g_audio_ctx.streams; node; node = node->next) { if (node->id != exclude_stream_id) { cunt++; } }

    entry = &g_audio_ctx.duck_stack[g_audio_ctx.duck_cunt];
    if (LDG_UNLIKELY(ldg_mem_zero(entry, sizeof(ldg_audio_duck_entry_t)) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    entry->factor = factor;
    entry->exclude_stream_id = exclude_stream_id;

    if (cunt > 0)
    {
        ret = ldg_mem_alloc(cunt * sizeof(uint32_t), (void **)&entry->stream_ids);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("audio: duck stream id alloc failed"); }

        ret = ldg_mem_alloc(cunt * sizeof(double), (void **)&entry->orig_vols);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("audio: duck orig vol alloc failed"); }

        if (!entry->stream_ids || !entry->orig_vols)
        {
            if (entry->stream_ids) { ldg_mem_dealloc(entry->stream_ids); }

            if (entry->orig_vols) { ldg_mem_dealloc(entry->orig_vols); }

            if (LDG_UNLIKELY(ldg_mem_zero(entry, sizeof(ldg_audio_duck_entry_t)) != LDG_ERR_AOK))
            {
                pw_thread_loop_unlock(g_audio_ctx.loop);
                return LDG_ERR_MEM_BAD;
            }

            pw_thread_loop_unlock(g_audio_ctx.loop);
            return LDG_ERR_ALLOC_NULL;
        }

        for (node = g_audio_ctx.streams; node && i < cunt; node = node->next) { if (node->id != exclude_stream_id)
            {
                entry->stream_ids[i] = node->id;
                entry->orig_vols[i] = node->vol;
                if (LDG_UNLIKELY(audio_node_vol_set(node, node->vol * factor) != LDG_ERR_AOK)) { continue; }

                i++;
            }
        }
    }

    entry->stream_cunt = i;
    g_audio_ctx.duck_cunt++;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_unduck(void)
{
    ldg_audio_duck_entry_t *entry = 0x0;
    ldg_audio_node_t *node = 0x0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (g_audio_ctx.duck_cunt == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_DUCK_EMPTY;
    }

    g_audio_ctx.duck_cunt--;
    entry = &g_audio_ctx.duck_stack[g_audio_ctx.duck_cunt];

    for (i = 0; i < entry->stream_cunt; i++)
    {
        node = audio_node_find(g_audio_ctx.streams, entry->stream_ids[i]);
        if (node) { if (LDG_UNLIKELY(audio_node_vol_set(node, entry->orig_vols[i]) != LDG_ERR_AOK)) { continue; } }
    }

    if (entry->stream_ids) { ldg_mem_dealloc(entry->stream_ids); }

    if (entry->orig_vols) { ldg_mem_dealloc(entry->orig_vols); }

    if (LDG_UNLIKELY(ldg_mem_zero(entry, sizeof(ldg_audio_duck_entry_t)) != LDG_ERR_AOK))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_MEM_BAD;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

typedef struct ldg_audio_pcm
{
    struct pw_stream *stream;
    struct spa_hook listener;
    ldg_audio_pcm_pull_cb_t src_cb;
    void *src_user;
    uint32_t frame_size;
    uint8_t is_open;
    uint8_t pudding[3];
} ldg_audio_pcm_t;

static ldg_audio_pcm_t g_audio_pcm = { .is_open = LDG_TRUTH_FALSE };

static uint32_t audio_pcm_fmt_get(uint16_t bit_depth)
{
    if (bit_depth == LDG_AUDIO_PCM_DEPTH_U8) { return SPA_AUDIO_FORMAT_U8; }

    if (bit_depth == LDG_AUDIO_PCM_DEPTH_S16) { return SPA_AUDIO_FORMAT_S16_LE; }

    if (bit_depth == LDG_AUDIO_PCM_DEPTH_S24) { return SPA_AUDIO_FORMAT_S24_LE; }

    if (bit_depth == LDG_AUDIO_PCM_DEPTH_S32) { return SPA_AUDIO_FORMAT_S32_LE; }

    return SPA_AUDIO_FORMAT_UNKNOWN;
}

static void audio_pcm_process(__attribute__((unused)) void *data)
{
    ldg_audio_pcm_pull_cb_t src_cb = 0x0;
    struct pw_buffer *pw_buff = 0x0;
    struct spa_buffer *spa_buff = 0x0;
    uint8_t *tail = 0x0;
    uint32_t frame_cunt = 0;
    uint32_t got = 0;

    if (!g_audio_pcm.stream) { return; }

    pw_buff = pw_stream_dequeue_buffer(g_audio_pcm.stream);
    if (!pw_buff) { return; }

    spa_buff = pw_buff->buffer;
    if (!spa_buff->datas[0].data)
    {
        pw_stream_queue_buffer(g_audio_pcm.stream, pw_buff);
        return;
    }

    frame_cunt = spa_buff->datas[0].maxsize / g_audio_pcm.frame_size;

    src_cb = g_audio_pcm.src_cb;
    LDG_BARRIER;

    if (src_cb != 0x0) { if (src_cb(g_audio_pcm.src_user, spa_buff->datas[0].data, frame_cunt, &got) != LDG_ERR_AOK) { got = LDG_AUDIO_PCM_NONE; } }
    else if (ldg_audio_play_frames_pull(spa_buff->datas[0].data, (uint64_t)frame_cunt * g_audio_pcm.frame_size, g_audio_pcm.frame_size, &got) != LDG_ERR_AOK) { got = LDG_AUDIO_PCM_NONE; }

    if (got < frame_cunt)
    {
        tail = (uint8_t *)spa_buff->datas[0].data + (got * g_audio_pcm.frame_size);
        if (LDG_UNLIKELY(memset(tail, LDG_AUDIO_PCM_SILENCE, (frame_cunt - got) * g_audio_pcm.frame_size) != tail)) { LDG_ERRLOG_WARN("audio: pcm silence fill failed"); }

        got = frame_cunt;
    }

    spa_buff->datas[0].chunk->offset = LDG_AUDIO_PCM_NONE;
    spa_buff->datas[0].chunk->stride = (int32_t)g_audio_pcm.frame_size;
    spa_buff->datas[0].chunk->size = got * g_audio_pcm.frame_size;

    pw_stream_queue_buffer(g_audio_pcm.stream, pw_buff);
}

static const struct pw_stream_events audio_pcm_events = {
    PW_VERSION_STREAM_EVENTS, .process = audio_pcm_process
};

uint32_t ldg_audio_sink_pcm_open(uint32_t rate, uint16_t channel_cunt, uint16_t bit_depth)
{
    uint8_t pod_buff[LDG_AUDIO_PCM_POD_MAX] = LDG_ARR_ZERO_INIT;
    const struct spa_pod *params[LDG_AUDIO_PCM_PARAM_CUNT] = LDG_ARR_ZERO_INIT;
    struct spa_pod_builder builder = LDG_STRUCT_ZERO_INIT;
    struct spa_audio_info_raw info = { .format = SPA_AUDIO_FORMAT_UNKNOWN };
    struct pw_properties *props = 0x0;
    enum pw_stream_flags stream_flags = PW_STREAM_FLAG_NONE;
    uint32_t stream_flag_bits = PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS;
    uint32_t fmt = 0;

    if (LDG_UNLIKELY(g_audio_ctx.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (g_audio_pcm.is_open == LDG_TRUTH_TRUE) { return LDG_ERR_BUSY; }

    fmt = audio_pcm_fmt_get(bit_depth);
    if (LDG_UNLIKELY(fmt == SPA_AUDIO_FORMAT_UNKNOWN)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(ldg_mem_cpy(&stream_flags, &stream_flag_bits, sizeof(stream_flags)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback", PW_KEY_MEDIA_ROLE, "Music", (const char *)0x0);
    if (LDG_UNLIKELY(!props)) { return LDG_ERR_ALLOC_NULL; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    g_audio_pcm.stream = pw_stream_new(g_audio_ctx.core, "ldg-play", props);
    if (LDG_UNLIKELY(!g_audio_pcm.stream))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    pw_stream_add_listener(g_audio_pcm.stream, &g_audio_pcm.listener, &audio_pcm_events, 0x0);

    info.format = (enum spa_audio_format)fmt;
    info.rate = rate;
    info.channels = channel_cunt;

    builder = SPA_POD_BUILDER_INIT(pod_buff, sizeof(pod_buff));
    params[0] = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info);

    g_audio_pcm.frame_size = (uint32_t)channel_cunt * ((uint32_t)bit_depth / LDG_AUDIO_PCM_BITS_PER_BYTE);

    if (LDG_UNLIKELY(pw_stream_connect(g_audio_pcm.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY, stream_flags, params, LDG_AUDIO_PCM_PARAM_CUNT) < 0))
    {
        pw_stream_destroy(g_audio_pcm.stream);
        g_audio_pcm.stream = 0x0;
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NOT_AVAILABLE;
    }

    g_audio_pcm.is_open = LDG_TRUTH_TRUE;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_sink_pcm_close(void)
{
    if (g_audio_pcm.is_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (g_audio_pcm.stream != 0x0)
    {
        pw_stream_destroy(g_audio_pcm.stream);
        g_audio_pcm.stream = 0x0;
    }

    g_audio_pcm.src_cb = 0x0;
    LDG_BARRIER;
    g_audio_pcm.src_user = 0x0;

    g_audio_pcm.is_open = LDG_TRUTH_FALSE;
    g_audio_pcm.frame_size = LDG_AUDIO_PCM_NONE;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_sink_pcm_src_set(ldg_audio_pcm_pull_cb_t cb, void *user)
{
    if (cb == 0x0)
    {
        g_audio_pcm.src_cb = 0x0;
        LDG_BARRIER;
        g_audio_pcm.src_user = 0x0;

        return LDG_ERR_AOK;
    }

    g_audio_pcm.src_user = user;
    LDG_BARRIER;
    g_audio_pcm.src_cb = cb;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_sink_pcm_pause_set(uint8_t paused)
{
    if (LDG_UNLIKELY(paused > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(g_audio_pcm.is_open != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (LDG_UNLIKELY(pw_stream_set_active(g_audio_pcm.stream, (paused == LDG_TRUTH_TRUE) ? false : true) < 0))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NOT_AVAILABLE;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_sink_pcm_vol_set(double vol)
{
    float level = 0.0;

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(g_audio_pcm.is_open != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    level = (float)vol;

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (LDG_UNLIKELY(pw_stream_set_control(g_audio_pcm.stream, SPA_PROP_volume, LDG_AUDIO_PCM_CTRL_CUNT, &level, 0x0) < 0))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NOT_AVAILABLE;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

#endif
