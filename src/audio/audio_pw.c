#ifdef LDG_AUDIO_PIPEWIRE

#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>
#include <pipewire/pipewire.h>
#include <spa/param/props.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <spa/pod/iter.h>

#include <string.h>
#include <stdlib.h>

#define LDG_AUDIO_NODE_STREAM 1
#define LDG_AUDIO_NODE_SINK   2
#define LDG_AUDIO_NODE_SOURCE 3

#define LDG_AUDIO_DUCK_STACK_MAX 16

typedef struct ldg_audio_node
{
    uint32_t id;
    uint32_t type;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    double volume;
    int32_t muted;
    int32_t channel_cunt;
    uint32_t target_id;
    struct pw_proxy *proxy;
    struct spa_hook listener;
    struct ldg_audio_node *next;
} ldg_audio_node_t;

typedef struct ldg_audio_duck_entry
{
    double factor;
    uint32_t exclude_stream_id;
    uint32_t *stream_ids;
    double *orig_volumes;
    uint32_t stream_cunt;
} ldg_audio_duck_entry_t;

typedef struct ldg_audio_ctx
{
    struct pw_thread_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;
    struct spa_hook core_listener;
    ldg_audio_node_t *streams;
    ldg_audio_node_t *sinks;
    ldg_audio_node_t *sources;
    ldg_audio_duck_entry_t duck_stack[LDG_AUDIO_DUCK_STACK_MAX];
    uint32_t duck_cunt;
    uint32_t default_sink_id;
    uint32_t default_source_id;
    uint32_t self_stream_id;
    int32_t sync_seq;
    volatile int32_t is_init;
    uint8_t pudding[4];
} LDG_ALIGNED ldg_audio_ctx_t;

static ldg_audio_ctx_t g_audio_ctx = { 0 };

static void audio_node_free(ldg_audio_node_t *node, int32_t destroy_proxy)
{
    if (!node) { return; }

    if (node->proxy)
    {
        spa_hook_remove(&node->listener);
        if (destroy_proxy) { pw_proxy_destroy(node->proxy); }
    }

    ldg_mem_dealloc(node);
}

static void audio_node_list_free(ldg_audio_node_t **head, int32_t destroy_proxy)
{
    ldg_audio_node_t *curr = NULL;
    ldg_audio_node_t *next = NULL;

    if (!head || !*head) { return; }

    curr = *head;
    while (curr)
    {
        next = curr->next;
        audio_node_free(curr, destroy_proxy);
        curr = next;
    }

    *head = NULL;
}

static ldg_audio_node_t* audio_node_find(ldg_audio_node_t *head, uint32_t id)
{
    ldg_audio_node_t *curr = head;

    while (curr)
    {
        if (curr->id == id) { return curr; }

        curr = curr->next;
    }

    return NULL;
}

static void audio_node_remove(ldg_audio_node_t **head, uint32_t id)
{
    ldg_audio_node_t *curr = NULL;
    ldg_audio_node_t *prev = NULL;

    if (!head || !*head) { return; }

    curr = *head;
    while (curr)
    {
        if (curr->id == id)
        {
            if (prev) { prev->next = curr->next; }
            else { *head = curr->next; }

            audio_node_free(curr, 1);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

static void node_event_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param)
{
    ldg_audio_node_t *node = (ldg_audio_node_t *)data;
    const struct spa_pod_prop *prop = NULL;
    const struct spa_pod_object *obj = NULL;
    uint32_t n_volumes = 0;
    uint32_t i = 0;
    double sum = 0.0;

    (void)seq;
    (void)index;
    (void)next;

    if (!param || id != SPA_PARAM_Props) { return; }

    if (!spa_pod_is_object(param)) { return; }

    obj = (const struct spa_pod_object *)param;

    SPA_POD_OBJECT_FOREACH(obj, prop)
    {
        if (prop->key == SPA_PROP_channelVolumes)
        {
            if (spa_pod_is_array(&prop->value))
            {
                n_volumes = SPA_POD_ARRAY_N_VALUES(&prop->value);
                const float *vols = (const float *)SPA_POD_ARRAY_VALUES(&prop->value);

                if (n_volumes > 0 && n_volumes <= SPA_AUDIO_MAX_CHANNELS)
                {
                    node->channel_cunt = (int32_t)n_volumes;
                    sum = 0.0;
                    for (i = 0; i < n_volumes; i++) { sum += (double)vols[i]; }
                    node->volume = sum / (double)n_volumes;
                    if (node->volume < 0.0) { node->volume = 0.0; }

                    if (node->volume > 1.0) { node->volume = 1.0; }
                }
            }
        }
        else if (prop->key == SPA_PROP_mute) { if (spa_pod_is_bool(&prop->value)) { node->muted = SPA_POD_VALUE(struct spa_pod_bool, &prop->value); } }
    }
}

static const struct pw_node_events node_events = {
    PW_VERSION_NODE_EVENTS, .param = node_event_param, };

static void registry_event_global(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props)
{
    const char *media_class = NULL;
    const char *name = NULL;
    const char *desc = NULL;
    ldg_audio_node_t *node = NULL;
    ldg_audio_node_t **list = NULL;
    uint32_t node_type = 0;

    (void)data;
    (void)permissions;
    (void)version;

    if (!props) { return; }

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
        node_type = LDG_AUDIO_NODE_SOURCE;
        list = &g_audio_ctx.sources;
    }
    else { return; }

    node = (ldg_audio_node_t *)ldg_mem_alloc(sizeof(ldg_audio_node_t));
    if (LDG_UNLIKELY(!node)) { return; }

    (void)memset(node, 0, sizeof(ldg_audio_node_t));
    node->id = id;
    node->type = node_type;

    name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    if (name) { (void)ldg_strrbrcpy(node->name, name, LDG_AUDIO_NAME_MAX); }

    desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    if (desc) { (void)ldg_strrbrcpy(node->desc, desc, LDG_AUDIO_DESC_MAX); }
    else if (name) { (void)ldg_strrbrcpy(node->desc, name, LDG_AUDIO_DESC_MAX); }

    if (node_type == LDG_AUDIO_NODE_STREAM)
    {
        const char *app_name = spa_dict_lookup(props, PW_KEY_APP_NAME);
        if (app_name) { (void)ldg_strrbrcpy(node->name, app_name, LDG_AUDIO_NAME_MAX); }
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

static void registry_event_global_remove(void *data, uint32_t id)
{
    (void)data;

    audio_node_remove(&g_audio_ctx.streams, id);
    audio_node_remove(&g_audio_ctx.sinks, id);
    audio_node_remove(&g_audio_ctx.sources, id);
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS, .global = registry_event_global, .global_remove = registry_event_global_remove, };

static void core_event_done(void *data, uint32_t id, int seq)
{
    (void)data;
    (void)id;

    if (g_audio_ctx.sync_seq == seq) { pw_thread_loop_signal(g_audio_ctx.loop, 0); }
}

static void core_event_error(void *data, uint32_t id, int seq, int res, const char *message)
{
    (void)data;
    (void)id;
    (void)seq;
    (void)res;
    (void)message;
}

static const struct pw_core_events core_events = {
    PW_VERSION_CORE_EVENTS, .done = core_event_done, .error = core_event_error, };

uint32_t ldg_audio_init(void)
{
    if (g_audio_ctx.is_init) { return LDG_ERR_AOK; }

    (void)memset(&g_audio_ctx, 0, sizeof(ldg_audio_ctx_t));

    pw_init(NULL, NULL);

    g_audio_ctx.loop = pw_thread_loop_new("ldg_audio", NULL);
    if (LDG_UNLIKELY(!g_audio_ctx.loop)) { return LDG_ERR_AUDIO_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    g_audio_ctx.context = pw_context_new(pw_thread_loop_get_loop(g_audio_ctx.loop), NULL, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.context))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    g_audio_ctx.core = pw_context_connect(g_audio_ctx.context, NULL, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.core))
    {
        pw_context_destroy(g_audio_ctx.context);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    pw_core_add_listener(g_audio_ctx.core, &g_audio_ctx.core_listener, &core_events, NULL);

    g_audio_ctx.registry = pw_core_get_registry(g_audio_ctx.core, PW_VERSION_REGISTRY, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.registry))
    {
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.context);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    pw_registry_add_listener(g_audio_ctx.registry, &g_audio_ctx.registry_listener, &registry_events, NULL);

    g_audio_ctx.is_init = 1;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    if (pw_thread_loop_start(g_audio_ctx.loop) < 0)
    {
        g_audio_ctx.is_init = 0;
        pw_thread_loop_lock(g_audio_ctx.loop);
        pw_proxy_destroy((struct pw_proxy *)g_audio_ctx.registry);
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.context);
        pw_thread_loop_unlock(g_audio_ctx.loop);
        pw_thread_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    return LDG_ERR_AOK;
}

void ldg_audio_shutdown(void)
{
    uint32_t i = 0;

    if (!g_audio_ctx.is_init) { return; }

    g_audio_ctx.is_init = 0;

    pw_thread_loop_stop(g_audio_ctx.loop);

    audio_node_list_free(&g_audio_ctx.streams, 0);
    audio_node_list_free(&g_audio_ctx.sinks, 0);
    audio_node_list_free(&g_audio_ctx.sources, 0);

    for (i = 0; i < LDG_AUDIO_DUCK_STACK_MAX; i++)
    {
        if (g_audio_ctx.duck_stack[i].stream_ids) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].stream_ids); }

        if (g_audio_ctx.duck_stack[i].orig_volumes) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].orig_volumes); }
    }

    spa_hook_remove(&g_audio_ctx.registry_listener);
    spa_hook_remove(&g_audio_ctx.core_listener);

    pw_core_disconnect(g_audio_ctx.core);
    pw_context_destroy(g_audio_ctx.context);
    pw_thread_loop_destroy(g_audio_ctx.loop);

    pw_deinit();
}

uint32_t ldg_audio_sync(void)
{
    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    g_audio_ctx.sync_seq = pw_core_sync(g_audio_ctx.core, PW_ID_CORE, 0);
    pw_thread_loop_wait(g_audio_ctx.loop);

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

static uint32_t audio_node_volume_set(ldg_audio_node_t *node, double vol)
{
    uint8_t buff[1024] = { 0 };
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buff, sizeof(buff));
    struct spa_pod_frame f = { 0 };
    float volumes[SPA_AUDIO_MAX_CHANNELS] = { 0 };
    int32_t i = 0;
    int32_t cunt = 0;

    if (!node || !node->proxy) { return LDG_ERR_FUNC_ARG_NULL; }

    cunt = (node->channel_cunt > 0) ? node->channel_cunt : 2;
    for (i = 0; i < cunt; i++) { volumes[i] = (float)vol; }

    spa_pod_builder_push_object(&b, &f, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
    spa_pod_builder_prop(&b, SPA_PROP_channelVolumes, 0);
    spa_pod_builder_array(&b, sizeof(float), SPA_TYPE_Float, cunt, volumes);

    pw_node_set_param((struct pw_node *)node->proxy, SPA_PARAM_Props, 0, (struct spa_pod *)spa_pod_builder_pop(&b, &f));

    return LDG_ERR_AOK;
}

static ldg_audio_node_t* audio_default_sink_find(void)
{
    ldg_audio_node_t *node = g_audio_ctx.sinks;

    if (g_audio_ctx.default_sink_id != 0)
    {
        node = audio_node_find(g_audio_ctx.sinks, g_audio_ctx.default_sink_id);
        if (node) { return node; }
    }

    return g_audio_ctx.sinks;
}

uint32_t ldg_audio_master_volume_get(double *vol)
{
    ldg_audio_node_t *sink = NULL;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    sink = audio_default_sink_find();
    if (!sink)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    *vol = sink->volume;
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_master_volume_set(double vol)
{
    ldg_audio_node_t *sink = NULL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    sink = audio_default_sink_find();
    if (!sink)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    ret = audio_node_volume_set(sink, vol);
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

uint32_t ldg_audio_stream_volume_get(uint32_t stream_id, double *vol)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    node = audio_node_find(g_audio_ctx.streams, stream_id);
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    *vol = node->volume;
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_volume_set(uint32_t stream_id, double vol)
{
    ldg_audio_node_t *node = NULL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    pw_thread_loop_lock(g_audio_ctx.loop);
    node = audio_node_find(g_audio_ctx.streams, stream_id);
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    ret = audio_node_volume_set(node, vol);
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, int32_t *muted)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!muted)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (stream_id == 0) { node = audio_default_sink_find(); }
    else{ node = audio_node_find(g_audio_ctx.streams, stream_id); }

    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return (stream_id == 0) ? LDG_ERR_AUDIO_NO_DEFAULT : LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    *muted = node->muted;
    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, int32_t muted)
{
    ldg_audio_node_t *node = NULL;
    uint8_t buff[256] = { 0 };
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buff, sizeof(buff));
    struct spa_pod *pod = NULL;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (stream_id == 0) { node = audio_default_sink_find(); }
    else{ node = audio_node_find(g_audio_ctx.streams, stream_id); }

    if (!node || !node->proxy)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return (stream_id == 0) ? LDG_ERR_AUDIO_NO_DEFAULT : LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    pod = (struct spa_pod *)spa_pod_builder_add_object(&b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_mute, SPA_POD_Bool(muted ? 1 : 0));

    pw_node_set_param((struct pw_node *)node->proxy, SPA_PARAM_Props, 0, pod);

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_list(ldg_audio_stream_t **streams, uint32_t *cunt)
{
    ldg_audio_node_t *node = NULL;
    ldg_audio_stream_t *arr = NULL;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!streams || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.streams; node; node = node->next) { n++; }

    if (n == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        *streams = NULL;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_stream_t *)ldg_mem_alloc(n * sizeof(ldg_audio_stream_t));
    if (LDG_UNLIKELY(!arr))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    (void)memset(arr, 0, n * sizeof(ldg_audio_stream_t));

    for (node = g_audio_ctx.streams; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        (void)ldg_strrbrcpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);
        (void)ldg_strrbrcpy(arr[i].app_name, node->desc, LDG_AUDIO_NAME_MAX);
        arr[i].volume = node->volume;
        arr[i].muted = node->muted;
        arr[i].sink_id = node->target_id;
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
    ldg_audio_node_t *node = NULL;
    uint32_t ret = LDG_ERR_AUDIO_STREAM_NOT_FOUND;

    if (LDG_UNLIKELY(!name || !stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.streams; node; node = node->next)
    {
        if (strcmp(node->name, name) == 0)
        {
            (void)memset(stream, 0, sizeof(ldg_audio_stream_t));
            stream->id = node->id;
            (void)ldg_strrbrcpy(stream->name, node->name, LDG_AUDIO_NAME_MAX);
            (void)ldg_strrbrcpy(stream->app_name, node->desc, LDG_AUDIO_NAME_MAX);
            stream->volume = node->volume;
            stream->muted = node->muted;
            stream->sink_id = node->target_id;
            ret = LDG_ERR_AOK;
            break;
        }
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return ret;
}

void ldg_audio_self_register(uint32_t stream_id)
{
    g_audio_ctx.self_stream_id = stream_id;
}

uint32_t ldg_audio_self_id_get(void)
{
    return g_audio_ctx.self_stream_id;
}

uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    node = audio_node_find(g_audio_ctx.streams, g_audio_ctx.self_stream_id);
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    (void)memset(stream, 0, sizeof(ldg_audio_stream_t));
    stream->id = node->id;
    (void)ldg_strrbrcpy(stream->name, node->name, LDG_AUDIO_NAME_MAX);
    (void)ldg_strrbrcpy(stream->app_name, node->desc, LDG_AUDIO_NAME_MAX);
    stream->volume = node->volume;
    stream->muted = node->muted;
    stream->sink_id = node->target_id;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_self_volume_get(double *vol)
{
    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    return ldg_audio_stream_volume_get(g_audio_ctx.self_stream_id, vol);
}

uint32_t ldg_audio_self_volume_set(double vol)
{
    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    return ldg_audio_stream_volume_set(g_audio_ctx.self_stream_id, vol);
}

uint32_t ldg_audio_sink_list(ldg_audio_sink_t **sinks, uint32_t *cunt)
{
    ldg_audio_node_t *node = NULL;
    ldg_audio_sink_t *arr = NULL;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sinks || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.sinks; node; node = node->next) { n++; }

    if (n == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        *sinks = NULL;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_sink_t *)ldg_mem_alloc(n * sizeof(ldg_audio_sink_t));
    if (LDG_UNLIKELY(!arr))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    (void)memset(arr, 0, n * sizeof(ldg_audio_sink_t));

    for (node = g_audio_ctx.sinks; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        (void)ldg_strrbrcpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);
        (void)ldg_strrbrcpy(arr[i].desc, node->desc, LDG_AUDIO_DESC_MAX);
        arr[i].volume = node->volume;
        arr[i].muted = node->muted;
        arr[i].is_default = (node->id == g_audio_ctx.default_sink_id || (g_audio_ctx.default_sink_id == 0 && node == g_audio_ctx.sinks)) ? 1 : 0;
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

uint32_t ldg_audio_source_list(ldg_audio_source_t **sources, uint32_t *cunt)
{
    ldg_audio_node_t *node = NULL;
    ldg_audio_source_t *arr = NULL;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sources || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    for (node = g_audio_ctx.sources; node; node = node->next) { n++; }

    if (n == 0)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        *sources = NULL;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_source_t *)ldg_mem_alloc(n * sizeof(ldg_audio_source_t));
    if (LDG_UNLIKELY(!arr))
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_ALLOC_NULL;
    }

    (void)memset(arr, 0, n * sizeof(ldg_audio_source_t));

    for (node = g_audio_ctx.sources; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        (void)ldg_strrbrcpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);
        (void)ldg_strrbrcpy(arr[i].desc, node->desc, LDG_AUDIO_DESC_MAX);
        arr[i].volume = node->volume;
        arr[i].muted = node->muted;
        arr[i].is_default = (node->id == g_audio_ctx.default_source_id || (g_audio_ctx.default_source_id == 0 && node == g_audio_ctx.sources)) ? 1 : 0;
    }

    pw_thread_loop_unlock(g_audio_ctx.loop);

    *sources = arr;
    *cunt = i;

    return LDG_ERR_AOK;
}

void ldg_audio_source_free(ldg_audio_source_t *sources)
{
    if (LDG_UNLIKELY(!sources)) { return; }

    ldg_mem_dealloc(sources);
}

uint32_t ldg_audio_default_sink_get(ldg_audio_sink_t *sink)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!sink)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    node = audio_default_sink_find();
    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    (void)memset(sink, 0, sizeof(ldg_audio_sink_t));
    sink->id = node->id;
    (void)ldg_strrbrcpy(sink->name, node->name, LDG_AUDIO_NAME_MAX);
    (void)ldg_strrbrcpy(sink->desc, node->desc, LDG_AUDIO_DESC_MAX);
    sink->volume = node->volume;
    sink->muted = node->muted;
    sink->is_default = 1;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_default_source_get(ldg_audio_source_t *source)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!source)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (g_audio_ctx.default_source_id != 0) { node = audio_node_find(g_audio_ctx.sources, g_audio_ctx.default_source_id); }

    if (!node) { node = g_audio_ctx.sources; }

    if (!node)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    (void)memset(source, 0, sizeof(ldg_audio_source_t));
    source->id = node->id;
    (void)ldg_strrbrcpy(source->name, node->name, LDG_AUDIO_NAME_MAX);
    (void)ldg_strrbrcpy(source->desc, node->desc, LDG_AUDIO_DESC_MAX);
    source->volume = node->volume;
    source->muted = node->muted;
    source->is_default = 1;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_duck(double factor, uint32_t exclude_stream_id)
{
    ldg_audio_duck_entry_t *entry = NULL;
    ldg_audio_node_t *node = NULL;
    uint32_t cunt = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(factor < 0.0 || factor > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    pw_thread_loop_lock(g_audio_ctx.loop);

    if (g_audio_ctx.duck_cunt >= LDG_AUDIO_DUCK_STACK_MAX)
    {
        pw_thread_loop_unlock(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_DUCK_FULL;
    }

    for (node = g_audio_ctx.streams; node; node = node->next) { if (node->id != exclude_stream_id) { cunt++; } }

    entry = &g_audio_ctx.duck_stack[g_audio_ctx.duck_cunt];
    (void)memset(entry, 0, sizeof(ldg_audio_duck_entry_t));
    entry->factor = factor;
    entry->exclude_stream_id = exclude_stream_id;
    entry->stream_cunt = cunt;

    if (cunt > 0)
    {
        entry->stream_ids = (uint32_t *)ldg_mem_alloc(cunt * sizeof(uint32_t));
        entry->orig_volumes = (double *)ldg_mem_alloc(cunt * sizeof(double));

        if (!entry->stream_ids || !entry->orig_volumes)
        {
            if (entry->stream_ids) { ldg_mem_dealloc(entry->stream_ids); }

            if (entry->orig_volumes) { ldg_mem_dealloc(entry->orig_volumes); }

            (void)memset(entry, 0, sizeof(ldg_audio_duck_entry_t));
            pw_thread_loop_unlock(g_audio_ctx.loop);
            return LDG_ERR_ALLOC_NULL;
        }

        for (node = g_audio_ctx.streams; node && i < cunt; node = node->next) { if (node->id != exclude_stream_id)
            {
                entry->stream_ids[i] = node->id;
                entry->orig_volumes[i] = node->volume;
                (void)audio_node_volume_set(node, node->volume * factor);
                i++;
            }
        }
    }

    g_audio_ctx.duck_cunt++;

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_unduck(void)
{
    ldg_audio_duck_entry_t *entry = NULL;
    ldg_audio_node_t *node = NULL;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

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
        if (node) { (void)audio_node_volume_set(node, entry->orig_volumes[i]); }
    }

    if (entry->stream_ids) { ldg_mem_dealloc(entry->stream_ids); }

    if (entry->orig_volumes) { ldg_mem_dealloc(entry->orig_volumes); }

    (void)memset(entry, 0, sizeof(ldg_audio_duck_entry_t));

    pw_thread_loop_unlock(g_audio_ctx.loop);

    return LDG_ERR_AOK;
}

#endif
