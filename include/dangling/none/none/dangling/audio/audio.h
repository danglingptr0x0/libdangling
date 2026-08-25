#ifndef LDG_AUDIO_AUDIO_H
#define LDG_AUDIO_AUDIO_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_AUDIO_NAME_MAX 128
#define LDG_AUDIO_DESC_MAX 256
#define LDG_AUDIO_WAV_HDR_SIZE 44

typedef struct ldg_audio_wav_info
{
    uint64_t data_off;
    uint64_t data_size;
    uint64_t frame_cunt;
    uint32_t rate;
    uint16_t channel_cunt;
    uint16_t bit_depth;
} ldg_audio_wav_info_t;

typedef struct ldg_audio_stream
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char app_name[LDG_AUDIO_NAME_MAX];
    double vol;
    uint8_t muted;
    uint32_t sink_id;
    uint8_t pudding[4];
} ldg_audio_stream_t;

typedef struct ldg_audio_sink
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    double vol;
    uint8_t muted;
    uint8_t is_default;
} ldg_audio_sink_t;

typedef struct ldg_audio_src
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    double vol;
    uint8_t muted;
    uint8_t is_default;
} ldg_audio_src_t;

#define LDG_AUDIO_PLAY_PATH_MAX 512
#define LDG_AUDIO_PLAY_TRACK_MAX 64

typedef enum ldg_audio_play_state
{
    LDG_AUDIO_PLAY_STATE_IDLE = 0,
    LDG_AUDIO_PLAY_STATE_PLAYING,
    LDG_AUDIO_PLAY_STATE_PAUSED
} ldg_audio_play_state_t;

typedef struct ldg_audio_play_status
{
    char path[LDG_AUDIO_PLAY_PATH_MAX];
    uint64_t frame_pos;
    uint64_t frame_cunt;
    uint32_t rate;
    uint32_t track_idx;
    uint32_t track_cunt;
    uint16_t channel_cunt;
    uint16_t bit_depth;
    ldg_audio_play_state_t state;
    uint8_t pudding[4];
} ldg_audio_play_status_t;

LDG_EXPORT uint32_t ldg_audio_play_open(const char *path);
LDG_EXPORT uint32_t ldg_audio_play_mem_open(const ldg_byte_t *raw, uint64_t size);
LDG_EXPORT uint32_t ldg_audio_wav_wrap(const ldg_byte_t *pcm, uint64_t pcm_size, uint32_t rate, uint16_t channel_cunt, uint16_t bit_depth, ldg_byte_t *out, uint64_t out_cap, uint64_t *out_size);
LDG_EXPORT uint32_t ldg_audio_wav_unwrap(const ldg_byte_t *raw, uint64_t size, ldg_audio_wav_info_t *out);
LDG_EXPORT uint32_t ldg_audio_play_list_open(const char *const *path, uint32_t cunt);
LDG_EXPORT uint32_t ldg_audio_play_close(void);
LDG_EXPORT uint32_t ldg_audio_play_start(void);
LDG_EXPORT uint32_t ldg_audio_play_pause(void);
LDG_EXPORT uint32_t ldg_audio_play_stop(void);
LDG_EXPORT uint32_t ldg_audio_play_next(void);
LDG_EXPORT uint32_t ldg_audio_play_prev(void);
LDG_EXPORT uint32_t ldg_audio_play_track_set(uint32_t track_idx);
LDG_EXPORT uint32_t ldg_audio_play_loop_set(uint8_t loop);
LDG_EXPORT uint32_t ldg_audio_play_vol_set(double vol);
LDG_EXPORT uint32_t ldg_audio_play_vol_get(double *out);
LDG_EXPORT uint32_t ldg_audio_play_status_get(ldg_audio_play_status_t *out);

typedef uint32_t (*ldg_audio_pcm_pull_cb_t)(void *user, void *buff, uint32_t frame_cunt, uint32_t *out_cunt);

LDG_EXPORT uint32_t ldg_audio_play_fmt_get(uint32_t *out_rate, uint16_t *out_channel_cunt, uint16_t *out_bit_depth);
LDG_EXPORT uint32_t ldg_audio_play_frames_pull(void *buff, uint32_t frame_cunt, uint32_t *out_cunt);

LDG_EXPORT uint32_t ldg_audio_sink_pcm_open(uint32_t rate, uint16_t channel_cunt, uint16_t bit_depth);
LDG_EXPORT uint32_t ldg_audio_sink_pcm_close(void);
LDG_EXPORT uint32_t ldg_audio_sink_pcm_pause_set(uint8_t paused);
LDG_EXPORT uint32_t ldg_audio_sink_pcm_vol_set(double vol);
LDG_EXPORT uint32_t ldg_audio_sink_pcm_src_set(ldg_audio_pcm_pull_cb_t cb, void *user);

// init/shutdown
LDG_EXPORT uint32_t ldg_audio_init(void);
LDG_EXPORT void ldg_audio_shutdown(void);
LDG_EXPORT uint32_t ldg_audio_sync(void);

// vol control
LDG_EXPORT uint32_t ldg_audio_master_vol_get(double *vol);
LDG_EXPORT uint32_t ldg_audio_master_vol_set(double vol);
LDG_EXPORT uint32_t ldg_audio_stream_vol_get(uint32_t stream_id, double *vol);
LDG_EXPORT uint32_t ldg_audio_stream_vol_set(uint32_t stream_id, double vol);
LDG_EXPORT uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, uint8_t *muted);
LDG_EXPORT uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, uint8_t muted);

// stream enumeration
LDG_EXPORT uint32_t ldg_audio_stream_list(ldg_audio_stream_t **streams, uint32_t *cunt);
LDG_EXPORT void ldg_audio_stream_free(ldg_audio_stream_t *streams);
LDG_EXPORT uint32_t ldg_audio_stream_name_get(const char *name, ldg_audio_stream_t *stream);

// self stream
LDG_EXPORT void ldg_audio_self_reg(uint32_t stream_id);
LDG_EXPORT uint64_t ldg_audio_self_id_get(void);
LDG_EXPORT uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream);
LDG_EXPORT uint32_t ldg_audio_self_vol_get(double *vol);
LDG_EXPORT uint32_t ldg_audio_self_vol_set(double vol);

// sink/src enumeration
LDG_EXPORT uint32_t ldg_audio_sink_list(ldg_audio_sink_t **sinks, uint32_t *cunt);
LDG_EXPORT void ldg_audio_sink_free(ldg_audio_sink_t *sinks);
LDG_EXPORT uint32_t ldg_audio_src_list(ldg_audio_src_t **srcs, uint32_t *cunt);
LDG_EXPORT void ldg_audio_src_free(ldg_audio_src_t *srcs);
LDG_EXPORT uint32_t ldg_audio_default_sink_get(ldg_audio_sink_t *sink);
LDG_EXPORT uint32_t ldg_audio_default_src_get(ldg_audio_src_t *src);

// stacked ducking
LDG_EXPORT uint32_t ldg_audio_duck(double factor, uint32_t exclude_stream_id);
LDG_EXPORT uint32_t ldg_audio_unduck(void);

#endif
