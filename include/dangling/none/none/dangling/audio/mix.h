#ifndef LDG_AUDIO_MIX_H
#define LDG_AUDIO_MIX_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_AUDIO_MIX_VOL_UNITY 1000U
#define LDG_AUDIO_MIX_PITCH_UNITY 1000U
#define LDG_AUDIO_MIX_RATE_DEFAULT 44100U
#define LDG_AUDIO_MIX_CHANNEL_CUNT_DEFAULT 2U
#define LDG_AUDIO_MIX_VOICE_CAP_DEFAULT 32U
#define LDG_AUDIO_MIX_CLIP_CAP_DEFAULT 64U
#define LDG_AUDIO_MIX_CLIP_FRAME_CAP_DEFAULT 1048576ULL
#define LDG_AUDIO_MIX_CHUNK_CAP_DEFAULT 4096U

typedef enum ldg_audio_mix_bus
{
    LDG_AUDIO_MIX_BUS_MUSIC = 0,
    LDG_AUDIO_MIX_BUS_FX,
    LDG_AUDIO_MIX_BUS_STREAM,
    LDG_AUDIO_MIX_BUS_CUNT
} ldg_audio_mix_bus_t;

typedef uint32_t (*ldg_audio_mix_stream_cb_t)(void *user, int16_t *dst, uint32_t frame_cunt, uint32_t *out_cunt);

typedef struct ldg_audio_mix_conf
{
    uint64_t clip_frame_cap;
    uint32_t voice_cap;
    uint32_t clip_cap;
    uint32_t chunk_cap;
    uint32_t rate;
    uint16_t channel_cunt;
    uint8_t player_en;
    uint8_t pudding[5];
} ldg_audio_mix_conf_t;

typedef struct ldg_audio_mix ldg_audio_mix_t;

LDG_EXPORT uint32_t ldg_audio_mix_conf_default_get(ldg_audio_mix_conf_t *out);
LDG_EXPORT uint32_t ldg_audio_mix_create(const ldg_audio_mix_conf_t *conf, ldg_audio_mix_t **out);
LDG_EXPORT uint32_t ldg_audio_mix_destroy(ldg_audio_mix_t *mix);
LDG_EXPORT uint32_t ldg_audio_mix_attach(ldg_audio_mix_t *mix);
LDG_EXPORT uint32_t ldg_audio_mix_detach(ldg_audio_mix_t *mix);
LDG_EXPORT uint32_t ldg_audio_mix_clip_ld(ldg_audio_mix_t *mix, const int16_t *pcm, uint64_t frame_cunt, uint32_t rate, uint32_t *out);
LDG_EXPORT uint32_t ldg_audio_mix_clip_play(ldg_audio_mix_t *mix, uint32_t clip, uint32_t pitch, uint32_t vol, uint64_t *out);
LDG_EXPORT uint32_t ldg_audio_mix_voice_stop(ldg_audio_mix_t *mix, uint64_t voice);
LDG_EXPORT uint32_t ldg_audio_mix_stream_set(ldg_audio_mix_t *mix, ldg_audio_mix_stream_cb_t cb, void *user, uint32_t rate);
LDG_EXPORT uint32_t ldg_audio_mix_stream_clear(ldg_audio_mix_t *mix);
LDG_EXPORT uint32_t ldg_audio_mix_bus_vol_set(ldg_audio_mix_t *mix, uint32_t bus, uint32_t vol);
LDG_EXPORT uint32_t ldg_audio_mix_bus_vol_get(const ldg_audio_mix_t *mix, uint32_t bus, uint32_t *out);

#endif
