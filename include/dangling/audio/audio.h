#ifndef LDG_AUDIO_AUDIO_H
#define LDG_AUDIO_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>

#define LDG_AUDIO_NAME_MAX 128
#define LDG_AUDIO_DESC_MAX 256

typedef struct ldg_audio_stream
{
    uint32_t id;
    uint32_t pid;
    char name[LDG_AUDIO_NAME_MAX];
    char app_name[LDG_AUDIO_NAME_MAX];
    double volume;
    int32_t muted;
    uint32_t sink_id;
    uint8_t pudding[4];
} ldg_audio_stream_t;

typedef struct ldg_audio_sink
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    double volume;
    int32_t muted;
    int32_t is_default;
} ldg_audio_sink_t;

typedef struct ldg_audio_source
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    double volume;
    int32_t muted;
    int32_t is_default;
} ldg_audio_source_t;

// init/shutdown
LDG_EXPORT uint32_t ldg_audio_init(void);
LDG_EXPORT void ldg_audio_shutdown(void);

// volume control
LDG_EXPORT uint32_t ldg_audio_master_volume_get(double *vol);
LDG_EXPORT uint32_t ldg_audio_master_volume_set(double vol);
LDG_EXPORT uint32_t ldg_audio_stream_volume_get(uint32_t stream_id, double *vol);
LDG_EXPORT uint32_t ldg_audio_stream_volume_set(uint32_t stream_id, double vol);
LDG_EXPORT uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, int32_t *muted);
LDG_EXPORT uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, int32_t muted);

// stream enumeration
LDG_EXPORT uint32_t ldg_audio_stream_list(ldg_audio_stream_t **streams, uint32_t *cunt);
LDG_EXPORT void ldg_audio_stream_free(ldg_audio_stream_t *streams);
LDG_EXPORT uint32_t ldg_audio_stream_pid_get(uint32_t pid, ldg_audio_stream_t *stream);
LDG_EXPORT uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream);

// sink/source enumeration
LDG_EXPORT uint32_t ldg_audio_sink_list(ldg_audio_sink_t **sinks, uint32_t *cunt);
LDG_EXPORT void ldg_audio_sink_free(ldg_audio_sink_t *sinks);
LDG_EXPORT uint32_t ldg_audio_source_list(ldg_audio_source_t **sources, uint32_t *cunt);
LDG_EXPORT void ldg_audio_source_free(ldg_audio_source_t *sources);
LDG_EXPORT uint32_t ldg_audio_default_sink_get(ldg_audio_sink_t *sink);
LDG_EXPORT uint32_t ldg_audio_default_source_get(ldg_audio_source_t *source);

// stacked ducking
LDG_EXPORT uint32_t ldg_audio_duck(double factor, uint32_t exclude_pid);
LDG_EXPORT uint32_t ldg_audio_unduck(void);

#endif
