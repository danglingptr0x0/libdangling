#ifdef LDG_AUDIO_PIPEWIRE

#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>
#include <dangling/thread/sync.h>

#include <pipewire/pipewire.h>
#include <spa/param/props.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define LDG_AUDIO_NODE_STREAM 1
#define LDG_AUDIO_NODE_SINK   2
#define LDG_AUDIO_NODE_SOURCE 3

#define LDG_AUDIO_DUCK_STACK_MAX 16

typedef struct ldg_audio_node
{
    uint32_t id;
    uint32_t type;
    uint32_t pid;
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
    uint32_t exclude_pid;
    uint32_t *stream_ids;
    double *orig_volumes;
    uint32_t stream_cunt;
} ldg_audio_duck_entry_t;

typedef struct ldg_audio_ctx
{
    struct pw_main_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_registry *registry;
    struct spa_hook registry_listener;
    struct spa_hook core_listener;
    ldg_audio_node_t *streams;
    ldg_audio_node_t *sinks;
    ldg_audio_node_t *sources;
    ldg_mut_t lock;
    pthread_t loop_thread;
    ldg_audio_duck_entry_t duck_stack[LDG_AUDIO_DUCK_STACK_MAX];
    uint32_t duck_cunt;
    uint32_t default_sink_id;
    uint32_t default_source_id;
    volatile int32_t is_init;
    volatile int32_t is_running;
    uint8_t pudding[4];
} LDG_ALIGNED ldg_audio_ctx_t;

static ldg_audio_ctx_t g_audio_ctx = { 0 };

static void audio_node_free(ldg_audio_node_t *node)
{
    if (!node) { return; }

    if (node->proxy)
    {
        spa_hook_remove(&node->listener);
        pw_proxy_destroy(node->proxy);
    }

    ldg_mem_dealloc(node);
}

static void audio_node_list_free(ldg_audio_node_t **head)
{
    ldg_audio_node_t *curr = NULL;
    ldg_audio_node_t *next = NULL;

    if (!head || !*head) { return; }

    curr = *head;
    while (curr)
    {
        next = curr->next;
        audio_node_free(curr);
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

            audio_node_free(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

static void node_event_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param)
{
    ldg_audio_node_t *node = (ldg_audio_node_t *)data;
    float volumes[SPA_AUDIO_MAX_CHANNELS] = { 0 };
    uint32_t n_volumes = 0;
    int32_t mute = 0;
    uint32_t i = 0;
    double sum = 0.0;

    (void)seq;
    (void)index;
    (void)next;

    if (!param || id != SPA_PARAM_Props) { return; }

    if (spa_pod_parse_object(param, SPA_TYPE_OBJECT_Props, NULL, SPA_PROP_channelVolumes, SPA_POD_Array(&n_volumes, SPA_TYPE_Float, SPA_AUDIO_MAX_CHANNELS, volumes), SPA_PROP_mute, SPA_POD_Bool(&mute)) < 0)
    {
        (void)spa_pod_parse_object(param, SPA_TYPE_OBJECT_Props, NULL, SPA_PROP_channelVolumes, SPA_POD_Array(&n_volumes, SPA_TYPE_Float, SPA_AUDIO_MAX_CHANNELS, volumes));
        (void)spa_pod_parse_object(param, SPA_TYPE_OBJECT_Props, NULL, SPA_PROP_mute, SPA_POD_Bool(&mute));
    }

    ldg_mut_lock(&g_audio_ctx.lock);

    if (n_volumes > 0)
    {
        node->channel_cunt = (int32_t)n_volumes;
        for (i = 0; i < n_volumes; i++) { sum += (double)volumes[i]; }
        node->volume = sum / (double)n_volumes;
        if (node->volume < 0.0) { node->volume = 0.0; }

        if (node->volume > 1.0) { node->volume = 1.0; }
    }

    node->muted = mute;

    ldg_mut_unlock(&g_audio_ctx.lock);
}

static const struct pw_node_events node_events = {
    PW_VERSION_NODE_EVENTS, .param = node_event_param, };

static void registry_event_global(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props)
{
    const char *media_class = NULL;
    const char *name = NULL;
    const char *desc = NULL;
    const char *pid_str = NULL;
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
    else{ return; }

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

        pid_str = spa_dict_lookup(props, PW_KEY_APP_PROCESS_ID);
        if (pid_str) { node->pid = (uint32_t)strtoul(pid_str, NULL, LDG_BASE_DECIMAL); }
    }

    node->proxy = (struct pw_proxy *)pw_registry_bind(g_audio_ctx.registry, id, type, PW_VERSION_NODE, 0);
    if (node->proxy)
    {
        pw_proxy_add_object_listener(node->proxy, &node->listener, &node_events, node);

        pw_node_subscribe_params((struct pw_node *)node->proxy, (uint32_t[]) { SPA_PARAM_Props }, 1);
    }

    ldg_mut_lock(&g_audio_ctx.lock);
    node->next = *list;
    *list = node;
    ldg_mut_unlock(&g_audio_ctx.lock);
}

static void registry_event_global_remove(void *data, uint32_t id)
{
    (void)data;

    ldg_mut_lock(&g_audio_ctx.lock);
    audio_node_remove(&g_audio_ctx.streams, id);
    audio_node_remove(&g_audio_ctx.sinks, id);
    audio_node_remove(&g_audio_ctx.sources, id);
    ldg_mut_unlock(&g_audio_ctx.lock);
}

static const struct pw_registry_events registry_events = {
    PW_VERSION_REGISTRY_EVENTS, .global = registry_event_global, .global_remove = registry_event_global_remove, };

static void core_event_done(void *data, uint32_t id, int seq)
{
    (void)data;
    (void)id;
    (void)seq;
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

static void* audio_loop_thread(void *arg)
{
    (void)arg;

    pw_main_loop_run(g_audio_ctx.loop);

    return NULL;
}

uint32_t ldg_audio_init(void)
{
    int ret = 0;

    if (g_audio_ctx.is_init) { return LDG_ERR_AOK; }

    (void)memset(&g_audio_ctx, 0, sizeof(ldg_audio_ctx_t));

    pw_init(NULL, NULL);

    g_audio_ctx.loop = pw_main_loop_new(NULL);
    if (LDG_UNLIKELY(!g_audio_ctx.loop)) { return LDG_ERR_AUDIO_INIT; }

    g_audio_ctx.context = pw_context_new(pw_main_loop_get_loop(g_audio_ctx.loop), NULL, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.context))
    {
        pw_main_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    g_audio_ctx.core = pw_context_connect(g_audio_ctx.context, NULL, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.core))
    {
        pw_context_destroy(g_audio_ctx.context);
        pw_main_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    pw_core_add_listener(g_audio_ctx.core, &g_audio_ctx.core_listener, &core_events, NULL);

    g_audio_ctx.registry = pw_core_get_registry(g_audio_ctx.core, PW_VERSION_REGISTRY, 0);
    if (LDG_UNLIKELY(!g_audio_ctx.registry))
    {
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.context);
        pw_main_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    pw_registry_add_listener(g_audio_ctx.registry, &g_audio_ctx.registry_listener, &registry_events, NULL);

    ret = ldg_mut_init(&g_audio_ctx.lock, 0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        pw_proxy_destroy((struct pw_proxy *)g_audio_ctx.registry);
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.context);
        pw_main_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    g_audio_ctx.is_running = 1;
    g_audio_ctx.is_init = 1;

    ret = pthread_create(&g_audio_ctx.loop_thread, NULL, audio_loop_thread, NULL);
    if (LDG_UNLIKELY(ret != 0))
    {
        g_audio_ctx.is_running = 0;
        g_audio_ctx.is_init = 0;
        ldg_mut_destroy(&g_audio_ctx.lock);
        pw_proxy_destroy((struct pw_proxy *)g_audio_ctx.registry);
        pw_core_disconnect(g_audio_ctx.core);
        pw_context_destroy(g_audio_ctx.context);
        pw_main_loop_destroy(g_audio_ctx.loop);
        return LDG_ERR_AUDIO_INIT;
    }

    return LDG_ERR_AOK;
}

void ldg_audio_shutdown(void)
{
    uint32_t i = 0;

    if (!g_audio_ctx.is_init) { return; }

    while (g_audio_ctx.duck_cunt > 0) { (void)ldg_audio_unduck(); }

    g_audio_ctx.is_running = 0;

    if (g_audio_ctx.loop) { pw_main_loop_quit(g_audio_ctx.loop); }

    (void)pthread_join(g_audio_ctx.loop_thread, NULL);

    ldg_mut_lock(&g_audio_ctx.lock);
    audio_node_list_free(&g_audio_ctx.streams);
    audio_node_list_free(&g_audio_ctx.sinks);
    audio_node_list_free(&g_audio_ctx.sources);
    ldg_mut_unlock(&g_audio_ctx.lock);

    for (i = 0; i < LDG_AUDIO_DUCK_STACK_MAX; i++)
    {
        if (g_audio_ctx.duck_stack[i].stream_ids) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].stream_ids); }

        if (g_audio_ctx.duck_stack[i].orig_volumes) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].orig_volumes); }
    }

    spa_hook_remove(&g_audio_ctx.registry_listener);
    spa_hook_remove(&g_audio_ctx.core_listener);

    if (g_audio_ctx.registry) { pw_proxy_destroy((struct pw_proxy *)g_audio_ctx.registry); }

    if (g_audio_ctx.core) { pw_core_disconnect(g_audio_ctx.core); }

    if (g_audio_ctx.context) { pw_context_destroy(g_audio_ctx.context); }

    if (g_audio_ctx.loop) { pw_main_loop_destroy(g_audio_ctx.loop); }

    ldg_mut_destroy(&g_audio_ctx.lock);

    pw_deinit();

    g_audio_ctx.is_init = 0;
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

    ldg_mut_lock(&g_audio_ctx.lock);
    sink = audio_default_sink_find();
    if (!sink)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    *vol = sink->volume;
    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_master_volume_set(double vol)
{
    ldg_audio_node_t *sink = NULL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    ldg_mut_lock(&g_audio_ctx.lock);
    sink = audio_default_sink_find();
    if (!sink)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    ret = audio_node_volume_set(sink, vol);
    ldg_mut_unlock(&g_audio_ctx.lock);

    return ret;
}

uint32_t ldg_audio_stream_volume_get(uint32_t stream_id, double *vol)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);
    node = audio_node_find(g_audio_ctx.streams, stream_id);
    if (!node)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    *vol = node->volume;
    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_volume_set(uint32_t stream_id, double vol)
{
    ldg_audio_node_t *node = NULL;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    ldg_mut_lock(&g_audio_ctx.lock);
    node = audio_node_find(g_audio_ctx.streams, stream_id);
    if (!node)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    ret = audio_node_volume_set(node, vol);
    ldg_mut_unlock(&g_audio_ctx.lock);

    return ret;
}

uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, int32_t *muted)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!muted)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    if (stream_id == 0) { node = audio_default_sink_find(); }
    else{ node = audio_node_find(g_audio_ctx.streams, stream_id); }

    if (!node)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return (stream_id == 0) ? LDG_ERR_AUDIO_NO_DEFAULT : LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    *muted = node->muted;
    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, int32_t muted)
{
    ldg_audio_node_t *node = NULL;
    uint8_t buff[256] = { 0 };
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buff, sizeof(buff));
    struct spa_pod *pod = NULL;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    if (stream_id == 0) { node = audio_default_sink_find(); }
    else{ node = audio_node_find(g_audio_ctx.streams, stream_id); }

    if (!node || !node->proxy)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return (stream_id == 0) ? LDG_ERR_AUDIO_NO_DEFAULT : LDG_ERR_AUDIO_STREAM_NOT_FOUND;
    }

    pod = (struct spa_pod *)spa_pod_builder_add_object(&b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_mute, SPA_POD_Bool(muted ? 1 : 0));

    pw_node_set_param((struct pw_node *)node->proxy, SPA_PARAM_Props, 0, pod);

    ldg_mut_unlock(&g_audio_ctx.lock);

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

    ldg_mut_lock(&g_audio_ctx.lock);

    for (node = g_audio_ctx.streams; node; node = node->next) { n++; }

    if (n == 0)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        *streams = NULL;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_stream_t *)ldg_mem_alloc(n * sizeof(ldg_audio_stream_t));
    if (LDG_UNLIKELY(!arr))
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_ALLOC_NULL;
    }

    (void)memset(arr, 0, n * sizeof(ldg_audio_stream_t));

    for (node = g_audio_ctx.streams; node && i < n; node = node->next, i++)
    {
        arr[i].id = node->id;
        arr[i].pid = node->pid;
        (void)ldg_strrbrcpy(arr[i].name, node->name, LDG_AUDIO_NAME_MAX);
        (void)ldg_strrbrcpy(arr[i].app_name, node->desc, LDG_AUDIO_NAME_MAX);
        arr[i].volume = node->volume;
        arr[i].muted = node->muted;
        arr[i].sink_id = node->target_id;
    }

    ldg_mut_unlock(&g_audio_ctx.lock);

    *streams = arr;
    *cunt = i;

    return LDG_ERR_AOK;
}

void ldg_audio_stream_free(ldg_audio_stream_t *streams)
{
    if (LDG_UNLIKELY(!streams)) { return; }

    ldg_mem_dealloc(streams);
}

uint32_t ldg_audio_stream_pid_get(uint32_t pid, ldg_audio_stream_t *stream)
{
    ldg_audio_node_t *node = NULL;
    uint32_t ret = LDG_ERR_AUDIO_STREAM_NOT_FOUND;

    if (LDG_UNLIKELY(!stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    for (node = g_audio_ctx.streams; node; node = node->next)
    {
        if (node->pid == pid)
        {
            (void)memset(stream, 0, sizeof(ldg_audio_stream_t));
            stream->id = node->id;
            stream->pid = node->pid;
            (void)ldg_strrbrcpy(stream->name, node->name, LDG_AUDIO_NAME_MAX);
            (void)ldg_strrbrcpy(stream->app_name, node->desc, LDG_AUDIO_NAME_MAX);
            stream->volume = node->volume;
            stream->muted = node->muted;
            stream->sink_id = node->target_id;
            ret = LDG_ERR_AOK;
            break;
        }
    }

    ldg_mut_unlock(&g_audio_ctx.lock);

    return ret;
}

uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream)
{
    return ldg_audio_stream_pid_get((uint32_t)getpid(), stream);
}

uint32_t ldg_audio_sink_list(ldg_audio_sink_t **sinks, uint32_t *cunt)
{
    ldg_audio_node_t *node = NULL;
    ldg_audio_sink_t *arr = NULL;
    uint32_t n = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sinks || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    for (node = g_audio_ctx.sinks; node; node = node->next) { n++; }

    if (n == 0)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        *sinks = NULL;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_sink_t *)ldg_mem_alloc(n * sizeof(ldg_audio_sink_t));
    if (LDG_UNLIKELY(!arr))
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
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

    ldg_mut_unlock(&g_audio_ctx.lock);

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

    ldg_mut_lock(&g_audio_ctx.lock);

    for (node = g_audio_ctx.sources; node; node = node->next) { n++; }

    if (n == 0)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        *sources = NULL;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_source_t *)ldg_mem_alloc(n * sizeof(ldg_audio_source_t));
    if (LDG_UNLIKELY(!arr))
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
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

    ldg_mut_unlock(&g_audio_ctx.lock);

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

    ldg_mut_lock(&g_audio_ctx.lock);

    node = audio_default_sink_find();
    if (!node)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    (void)memset(sink, 0, sizeof(ldg_audio_sink_t));
    sink->id = node->id;
    (void)ldg_strrbrcpy(sink->name, node->name, LDG_AUDIO_NAME_MAX);
    (void)ldg_strrbrcpy(sink->desc, node->desc, LDG_AUDIO_DESC_MAX);
    sink->volume = node->volume;
    sink->muted = node->muted;
    sink->is_default = 1;

    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_default_source_get(ldg_audio_source_t *source)
{
    ldg_audio_node_t *node = NULL;

    if (LDG_UNLIKELY(!source)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    if (g_audio_ctx.default_source_id != 0) { node = audio_node_find(g_audio_ctx.sources, g_audio_ctx.default_source_id); }

    if (!node) { node = g_audio_ctx.sources; }

    if (!node)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    (void)memset(source, 0, sizeof(ldg_audio_source_t));
    source->id = node->id;
    (void)ldg_strrbrcpy(source->name, node->name, LDG_AUDIO_NAME_MAX);
    (void)ldg_strrbrcpy(source->desc, node->desc, LDG_AUDIO_DESC_MAX);
    source->volume = node->volume;
    source->muted = node->muted;
    source->is_default = 1;

    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_duck(double factor, uint32_t exclude_pid)
{
    ldg_audio_duck_entry_t *entry = NULL;
    ldg_audio_node_t *node = NULL;
    uint32_t cunt = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(factor < 0.0 || factor > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    if (g_audio_ctx.duck_cunt >= LDG_AUDIO_DUCK_STACK_MAX)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
        return LDG_ERR_AUDIO_DUCK_FULL;
    }

    for (node = g_audio_ctx.streams; node; node = node->next) { if (node->pid != exclude_pid) { cunt++; } }

    entry = &g_audio_ctx.duck_stack[g_audio_ctx.duck_cunt];
    (void)memset(entry, 0, sizeof(ldg_audio_duck_entry_t));
    entry->factor = factor;
    entry->exclude_pid = exclude_pid;
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
            ldg_mut_unlock(&g_audio_ctx.lock);
            return LDG_ERR_ALLOC_NULL;
        }

        for (node = g_audio_ctx.streams; node && i < cunt; node = node->next) { if (node->pid != exclude_pid)
            {
                entry->stream_ids[i] = node->id;
                entry->orig_volumes[i] = node->volume;
                (void)audio_node_volume_set(node, node->volume * factor);
                i++;
            }
        }
    }

    g_audio_ctx.duck_cunt++;

    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_unduck(void)
{
    ldg_audio_duck_entry_t *entry = NULL;
    ldg_audio_node_t *node = NULL;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    ldg_mut_lock(&g_audio_ctx.lock);

    if (g_audio_ctx.duck_cunt == 0)
    {
        ldg_mut_unlock(&g_audio_ctx.lock);
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

    ldg_mut_unlock(&g_audio_ctx.lock);

    return LDG_ERR_AOK;
}

#endif
