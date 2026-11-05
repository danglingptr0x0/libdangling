#include <stdint.h>

#include <dangling/audio/mix.h>
#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>
#include <dangling/thread/spsc.h>

#define LDG_AUDIO_MIX_STEP 1
#define LDG_AUDIO_MIX_POOL_ITEM_SIZE_VAR 0
#define LDG_AUDIO_MIX_POOL_ALLOC_CUNT 8
#define LDG_AUDIO_MIX_QUEUE_CAP 256
#define LDG_AUDIO_MIX_DEPTH 16
#define LDG_AUDIO_MIX_CHANNEL_MAX 2
#define LDG_AUDIO_MIX_PITCH_MAX 8000U
#define LDG_AUDIO_MIX_VOL_MAX 4000U
#define LDG_AUDIO_MIX_STREAM_RATIO_MAX 4U
#define LDG_AUDIO_MIX_STREAM_SLACK 2U
#define LDG_AUDIO_MIX_PHASE_SHIFT 32
#define LDG_AUDIO_MIX_PHASE_MASK 0xFFFFFFFFULL
#define LDG_AUDIO_MIX_FRAC_REDUCE 16
#define LDG_AUDIO_MIX_FRAC_ONE 65536LL
#define LDG_AUDIO_MIX_SAMPLE_MAX 32767
#define LDG_AUDIO_MIX_SAMPLE_MIN (-LDG_AUDIO_MIX_SAMPLE_MAX - LDG_AUDIO_MIX_STEP)
#define LDG_AUDIO_MIX_ID_NONE 0U
#define LDG_AUDIO_MIX_NONE UINT32_MAX

typedef enum mix_cmd_kind
{
    MIX_CMD_KIND_PLAY = 0,
    MIX_CMD_KIND_STOP,
    MIX_CMD_KIND_STREAM_SET,
    MIX_CMD_KIND_STREAM_CLEAR,
    MIX_CMD_KIND_BUS_VOL
} mix_cmd_kind_t;

typedef struct mix_clip
{
    uint64_t off;
    uint64_t frame_cunt;
    uint32_t rate;
    uint8_t used;
    uint8_t pudding[3];
} mix_clip_t;

typedef struct mix_voice
{
    uint64_t id;
    uint64_t phase;
    uint64_t step;
    uint64_t seq;
    uint32_t clip;
    uint32_t vol;
    uint8_t active;
    uint8_t pudding[7];
} mix_voice_t;

typedef struct mix_cmd
{
    ldg_audio_mix_stream_cb_t cb;
    void *user;
    uint64_t id;
    uint32_t kind;
    uint32_t clip;
    uint32_t pitch;
    uint32_t vol;
    uint32_t rate;
    uint32_t bus;
} mix_cmd_t;

struct ldg_audio_mix
{
    ldg_spsc_queue_t queue;
    ldg_mem_pool_t *pool;
    int16_t *clip_pcm;
    mix_clip_t *clip;
    mix_voice_t *voice;
    int32_t *acc;
    int16_t *music;
    int16_t *stream_buff;
    ldg_audio_mix_stream_cb_t stream_cb;
    void *stream_user;
    uint64_t clip_frame_cap;
    uint64_t clip_frame_used;
    uint64_t id_next;
    uint64_t seq_next;
    uint64_t stream_phase;
    uint64_t stream_step;
    uint32_t voice_cap;
    uint32_t clip_cap;
    uint32_t clip_cunt;
    uint32_t chunk_cap;
    uint32_t rate;
    uint32_t stream_buff_cap;
    uint32_t bus_vol[LDG_AUDIO_MIX_BUS_CUNT];
    uint32_t bus_vol_mix[LDG_AUDIO_MIX_BUS_CUNT];
    uint16_t channel_cunt;
    int16_t stream_last;
    uint8_t player_en;
    uint8_t attached;
    uint8_t stream_en;
    uint8_t pudding[17];
};

static int32_t mix_lerp_get(int16_t lo, int16_t hi, uint64_t phase)
{
    int64_t frac = (int64_t)((phase & LDG_AUDIO_MIX_PHASE_MASK) >> LDG_AUDIO_MIX_FRAC_REDUCE);

    return (int32_t)((int64_t)lo + ((((int64_t)hi - (int64_t)lo) * frac) / LDG_AUDIO_MIX_FRAC_ONE));
}

static int32_t mix_gain_apply(int32_t sample, uint32_t vol, uint32_t bus)
{
    return (int32_t)(((int64_t)sample * (int64_t)vol * (int64_t)bus) / ((int64_t)LDG_AUDIO_MIX_VOL_UNITY * (int64_t)LDG_AUDIO_MIX_VOL_UNITY));
}

static void mix_voice_start(ldg_audio_mix_t *mix, const mix_cmd_t *cmd)
{
    mix_voice_t *voice = 0x0;
    const mix_clip_t *clip = &mix->clip[cmd->clip];
    uint32_t idx = 0;
    uint32_t pick = LDG_AUDIO_MIX_NONE;

    for (idx = 0; idx < mix->voice_cap; idx++)
    {
        if (mix->voice[idx].active != LDG_TRUTH_TRUE)
        {
            pick = idx;
            break;
        }

        if (pick == LDG_AUDIO_MIX_NONE || mix->voice[idx].seq < mix->voice[pick].seq) { pick = idx; }
    }

    voice = &mix->voice[pick];
    voice->id = cmd->id;
    voice->clip = cmd->clip;
    voice->vol = cmd->vol;
    voice->phase = 0;
    voice->step = (((uint64_t)cmd->pitch * clip->rate) << LDG_AUDIO_MIX_PHASE_SHIFT) / ((uint64_t)LDG_AUDIO_MIX_PITCH_UNITY * mix->rate);
    voice->seq = mix->seq_next;
    voice->active = LDG_TRUTH_TRUE;
    mix->seq_next++;
}

static void mix_cmd_apply(ldg_audio_mix_t *mix, const mix_cmd_t *cmd)
{
    uint32_t idx = 0;

    if (cmd->kind == MIX_CMD_KIND_PLAY)
    {
        mix_voice_start(mix, cmd);
        return;
    }

    if (cmd->kind == MIX_CMD_KIND_STOP)
    {
        for (idx = 0; idx < mix->voice_cap; idx++) { if (mix->voice[idx].active == LDG_TRUTH_TRUE && mix->voice[idx].id == cmd->id) { mix->voice[idx].active = LDG_TRUTH_FALSE; } }

        return;
    }

    if (cmd->kind == MIX_CMD_KIND_STREAM_SET)
    {
        mix->stream_cb = cmd->cb;
        mix->stream_user = cmd->user;
        mix->stream_step = ((uint64_t)cmd->rate << LDG_AUDIO_MIX_PHASE_SHIFT) / mix->rate;
        mix->stream_phase = 0;
        mix->stream_last = 0;
        mix->stream_en = LDG_TRUTH_TRUE;
        return;
    }

    if (cmd->kind == MIX_CMD_KIND_STREAM_CLEAR)
    {
        mix->stream_en = LDG_TRUTH_FALSE;
        mix->stream_cb = 0x0;
        mix->stream_user = 0x0;
        return;
    }

    if (cmd->kind == MIX_CMD_KIND_BUS_VOL && cmd->bus < LDG_AUDIO_MIX_BUS_CUNT) { mix->bus_vol_mix[cmd->bus] = cmd->vol; }
}

static void mix_music_add(ldg_audio_mix_t *mix, uint32_t frame_cunt)
{
    uint64_t idx = 0;
    uint32_t rate = 0;
    uint32_t got = 0;
    uint16_t channel_cunt = 0;
    uint16_t bit_depth = 0;

    if (ldg_audio_play_fmt_get(&rate, &channel_cunt, &bit_depth) != LDG_ERR_AOK) { return; }

    if (rate != mix->rate || channel_cunt != mix->channel_cunt || bit_depth != LDG_AUDIO_MIX_DEPTH) { return; }

    if (ldg_audio_play_frames_pull(mix->music, (uint64_t)frame_cunt * mix->channel_cunt * (uint64_t)sizeof(int16_t), mix->channel_cunt * (uint32_t)sizeof(int16_t), &got) != LDG_ERR_AOK) { return; }

    for (idx = 0; idx < (uint64_t)got * mix->channel_cunt; idx++) { mix->acc[idx] += mix_gain_apply(mix->music[idx], LDG_AUDIO_MIX_VOL_UNITY, mix->bus_vol_mix[LDG_AUDIO_MIX_BUS_MUSIC]); }
}

static void mix_voice_add(ldg_audio_mix_t *mix, mix_voice_t *voice, uint32_t frame_cunt)
{
    const mix_clip_t *clip = &mix->clip[voice->clip];
    const int16_t *pcm = mix->clip_pcm + clip->off;
    uint64_t pos = 0;
    int32_t val = 0;
    uint32_t frame = 0;
    uint32_t channel = 0;
    int16_t next = 0;

    for (frame = 0; frame < frame_cunt; frame++)
    {
        pos = voice->phase >> LDG_AUDIO_MIX_PHASE_SHIFT;

        if (pos >= clip->frame_cunt)
        {
            voice->active = LDG_TRUTH_FALSE;
            return;
        }

        next = (pos + LDG_AUDIO_MIX_STEP < clip->frame_cunt) ? pcm[pos + LDG_AUDIO_MIX_STEP] : pcm[pos];
        val = mix_gain_apply(mix_lerp_get(pcm[pos], next, voice->phase), voice->vol, mix->bus_vol_mix[LDG_AUDIO_MIX_BUS_FX]);

        for (channel = 0; channel < mix->channel_cunt; channel++) { mix->acc[((uint64_t)frame * mix->channel_cunt) + channel] += val; }

        voice->phase += voice->step;
    }
}

static void mix_stream_add(ldg_audio_mix_t *mix, uint32_t frame_cunt)
{
    uint64_t need = 0;
    uint64_t idx = 0;
    uint64_t consumed = 0;
    int32_t val = 0;
    uint32_t got = 0;
    uint32_t frame = 0;
    uint32_t channel = 0;

    need = ((mix->stream_phase + ((uint64_t)frame_cunt * mix->stream_step)) >> LDG_AUDIO_MIX_PHASE_SHIFT) + LDG_AUDIO_MIX_STEP;

    if (need > (uint64_t)mix->stream_buff_cap - LDG_AUDIO_MIX_STEP) { need = (uint64_t)mix->stream_buff_cap - LDG_AUDIO_MIX_STEP; }

    mix->stream_buff[0] = mix->stream_last;

    if (mix->stream_cb(mix->stream_user, mix->stream_buff + LDG_AUDIO_MIX_STEP, (uint32_t)need, &got) != LDG_ERR_AOK || got > need) { got = 0; }

    for (idx = (uint64_t)got + LDG_AUDIO_MIX_STEP; idx <= need; idx++) { mix->stream_buff[idx] = 0; }

    for (frame = 0; frame < frame_cunt; frame++)
    {
        idx = mix->stream_phase >> LDG_AUDIO_MIX_PHASE_SHIFT;

        if (idx + LDG_AUDIO_MIX_STEP > need) { break; }

        val = mix_gain_apply(mix_lerp_get(mix->stream_buff[idx], mix->stream_buff[idx + LDG_AUDIO_MIX_STEP], mix->stream_phase), LDG_AUDIO_MIX_VOL_UNITY, mix->bus_vol_mix[LDG_AUDIO_MIX_BUS_STREAM]);

        for (channel = 0; channel < mix->channel_cunt; channel++) { mix->acc[((uint64_t)frame * mix->channel_cunt) + channel] += val; }

        mix->stream_phase += mix->stream_step;
    }

    consumed = mix->stream_phase >> LDG_AUDIO_MIX_PHASE_SHIFT;

    if (consumed > need) { consumed = need; }

    mix->stream_last = mix->stream_buff[consumed];
    mix->stream_phase &= LDG_AUDIO_MIX_PHASE_MASK;
}

static void mix_chunk(ldg_audio_mix_t *mix, int16_t *dst, uint32_t frame_cunt)
{
    uint64_t total = (uint64_t)frame_cunt * mix->channel_cunt;
    uint64_t idx = 0;
    uint32_t voice = 0;
    int32_t val = 0;

    for (idx = 0; idx < total; idx++) { mix->acc[idx] = 0; }

    if (mix->player_en == LDG_TRUTH_TRUE) { mix_music_add(mix, frame_cunt); }

    for (voice = 0; voice < mix->voice_cap; voice++) { if (mix->voice[voice].active == LDG_TRUTH_TRUE) { mix_voice_add(mix, &mix->voice[voice], frame_cunt); } }

    if (mix->stream_en == LDG_TRUTH_TRUE && mix->stream_cb) { mix_stream_add(mix, frame_cunt); }

    for (idx = 0; idx < total; idx++)
    {
        val = mix->acc[idx];

        if (val > LDG_AUDIO_MIX_SAMPLE_MAX) { val = LDG_AUDIO_MIX_SAMPLE_MAX; }

        if (val < LDG_AUDIO_MIX_SAMPLE_MIN) { val = LDG_AUDIO_MIX_SAMPLE_MIN; }

        dst[idx] = (int16_t)val;
    }
}

static uint32_t mix_pull_cb(void *user, void *buff, uint32_t frame_cunt, uint32_t *out_cunt)
{
    ldg_audio_mix_t *mix = (ldg_audio_mix_t *)user;
    mix_cmd_t cmd = LDG_STRUCT_ZERO_INIT;
    int16_t *dst = (int16_t *)buff;
    uint32_t done = 0;
    uint32_t span = 0;

    if (LDG_UNLIKELY(!mix || !buff || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    while (ldg_spsc_pop(&mix->queue, &cmd) == LDG_ERR_AOK) { mix_cmd_apply(mix, &cmd); }

    while (done < frame_cunt)
    {
        span = frame_cunt - done;

        if (span > mix->chunk_cap) { span = mix->chunk_cap; }

        mix_chunk(mix, dst + ((uint64_t)done * mix->channel_cunt), span);
        done += span;
    }

    *out_cunt = frame_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_conf_default_get(ldg_audio_mix_conf_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->clip_frame_cap = LDG_AUDIO_MIX_CLIP_FRAME_CAP_DEFAULT;
    out->voice_cap = LDG_AUDIO_MIX_VOICE_CAP_DEFAULT;
    out->clip_cap = LDG_AUDIO_MIX_CLIP_CAP_DEFAULT;
    out->chunk_cap = LDG_AUDIO_MIX_CHUNK_CAP_DEFAULT;
    out->rate = LDG_AUDIO_MIX_RATE_DEFAULT;
    out->channel_cunt = LDG_AUDIO_MIX_CHANNEL_CUNT_DEFAULT;
    out->player_en = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t mix_buff_take(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(pool, size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static uint32_t mix_storage_take(ldg_audio_mix_t *mix)
{
    void *mem = 0x0;
    uint64_t chunk_size = (uint64_t)mix->chunk_cap * mix->channel_cunt;
    uint32_t ret = 0;

    ret = mix_buff_take(mix->pool, mix->clip_frame_cap * (uint64_t)sizeof(int16_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->clip_pcm = (int16_t *)mem;

    ret = mix_buff_take(mix->pool, (uint64_t)mix->clip_cap * (uint64_t)sizeof(mix_clip_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->clip = (mix_clip_t *)mem;

    ret = mix_buff_take(mix->pool, (uint64_t)mix->voice_cap * (uint64_t)sizeof(mix_voice_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->voice = (mix_voice_t *)mem;

    ret = mix_buff_take(mix->pool, chunk_size * (uint64_t)sizeof(int32_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->acc = (int32_t *)mem;

    ret = mix_buff_take(mix->pool, chunk_size * (uint64_t)sizeof(int16_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->music = (int16_t *)mem;

    ret = mix_buff_take(mix->pool, (uint64_t)mix->stream_buff_cap * (uint64_t)sizeof(int16_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->stream_buff = (int16_t *)mem;

    return ldg_spsc_init(&mix->queue, (uint64_t)sizeof(mix_cmd_t), LDG_AUDIO_MIX_QUEUE_CAP);
}

uint32_t ldg_audio_mix_create(const ldg_audio_mix_conf_t *conf, ldg_audio_mix_t **out)
{
    ldg_mem_pool_t *pool = 0x0;
    ldg_audio_mix_t *mix = 0x0;
    void *mem = 0x0;
    uint64_t chunk_size = 0;
    uint64_t stream_cap = 0;
    uint64_t pool_cap = 0;
    uint32_t bus = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(conf->rate == 0 || conf->channel_cunt == 0 || conf->channel_cunt > LDG_AUDIO_MIX_CHANNEL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->voice_cap == 0 || conf->clip_cap == 0 || conf->chunk_cap == 0 || conf->clip_frame_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    chunk_size = (uint64_t)conf->chunk_cap * conf->channel_cunt;
    stream_cap = ((uint64_t)conf->chunk_cap * LDG_AUDIO_MIX_STREAM_RATIO_MAX) + LDG_AUDIO_MIX_STREAM_SLACK;

    if (LDG_UNLIKELY(stream_cap > UINT32_MAX)) { return LDG_ERR_RANGE; }

    pool_cap = (uint64_t)sizeof(struct ldg_audio_mix) + LDG_AMD64_CACHE_LINE_WIDTH;
    pool_cap += conf->clip_frame_cap * (uint64_t)sizeof(int16_t);
    pool_cap += (uint64_t)conf->clip_cap * (uint64_t)sizeof(mix_clip_t);
    pool_cap += (uint64_t)conf->voice_cap * (uint64_t)sizeof(mix_voice_t);
    pool_cap += chunk_size * ((uint64_t)sizeof(int32_t) + (uint64_t)sizeof(int16_t));
    pool_cap += stream_cap * (uint64_t)sizeof(int16_t);
    pool_cap += (uint64_t)LDG_AUDIO_MIX_POOL_ALLOC_CUNT * LDG_MEM_POOL_VAR_ALIGN;

    ret = ldg_mem_pool_create(LDG_AUDIO_MIX_POOL_ITEM_SIZE_VAR, pool_cap, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = mix_buff_take(pool, (uint64_t)sizeof(struct ldg_audio_mix) + LDG_AMD64_CACHE_LINE_WIDTH, &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    mix = (ldg_audio_mix_t *)(uintptr_t)LDG_ALIGNED_UP((uintptr_t)mem);
    mix->pool = pool;
    mix->clip_frame_cap = conf->clip_frame_cap;
    mix->voice_cap = conf->voice_cap;
    mix->clip_cap = conf->clip_cap;
    mix->chunk_cap = conf->chunk_cap;
    mix->rate = conf->rate;
    mix->channel_cunt = conf->channel_cunt;
    mix->stream_buff_cap = (uint32_t)stream_cap;
    mix->player_en = (conf->player_en == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    for (bus = 0; bus < LDG_AUDIO_MIX_BUS_CUNT; bus++)
    {
        mix->bus_vol[bus] = LDG_AUDIO_MIX_VOL_UNITY;
        mix->bus_vol_mix[bus] = LDG_AUDIO_MIX_VOL_UNITY;
    }

    ret = mix_storage_take(mix);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    *out = mix;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_attach(ldg_audio_mix_t *mix)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mix)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mix->attached == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    ret = ldg_audio_sink_pcm_open(mix->rate, mix->channel_cunt, LDG_AUDIO_MIX_DEPTH);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_audio_sink_pcm_src_set(mix_pull_cb, mix);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_audio_sink_pcm_close() != LDG_ERR_AOK)) { return LDG_ERR_AUDIO_NOT_AVAILABLE; }

        return ret;
    }

    mix->attached = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_detach(ldg_audio_mix_t *mix)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mix)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (mix->attached != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_audio_sink_pcm_src_set(0x0, 0x0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_audio_sink_pcm_close();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->attached = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_destroy(ldg_audio_mix_t *mix)
{
    ldg_mem_pool_t *pool = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mix)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_audio_mix_detach(mix);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_spsc_shutdown(&mix->queue);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pool = mix->pool;

    return ldg_mem_pool_destroy(&pool);
}

uint32_t ldg_audio_mix_clip_ld(ldg_audio_mix_t *mix, const int16_t *pcm, uint64_t frame_cunt, uint32_t rate, uint32_t *out)
{
    mix_clip_t *clip = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mix || !pcm || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_AUDIO_MIX_NONE;

    if (LDG_UNLIKELY(frame_cunt == 0 || rate == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(mix->clip_cunt >= mix->clip_cap || frame_cunt > mix->clip_frame_cap - mix->clip_frame_used)) { return LDG_ERR_FULL; }

    ret = ldg_mem_cpy(mix->clip_pcm + mix->clip_frame_used, pcm, frame_cunt * (uint64_t)sizeof(int16_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    clip = &mix->clip[mix->clip_cunt];
    clip->off = mix->clip_frame_used;
    clip->frame_cunt = frame_cunt;
    clip->rate = rate;
    clip->used = LDG_TRUTH_TRUE;

    mix->clip_frame_used += frame_cunt;
    *out = mix->clip_cunt;
    mix->clip_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t mix_cmd_push(ldg_audio_mix_t *mix, const mix_cmd_t *cmd)
{
    if (LDG_UNLIKELY(ldg_spsc_push(&mix->queue, cmd) != LDG_ERR_AOK)) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_clip_play(ldg_audio_mix_t *mix, uint32_t clip, uint32_t pitch, uint32_t vol, uint64_t *out)
{
    mix_cmd_t cmd = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mix || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_AUDIO_MIX_ID_NONE;

    if (LDG_UNLIKELY(clip >= mix->clip_cunt)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(pitch == 0 || pitch > LDG_AUDIO_MIX_PITCH_MAX || vol > LDG_AUDIO_MIX_VOL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    mix->id_next++;
    cmd.kind = MIX_CMD_KIND_PLAY;
    cmd.id = mix->id_next;
    cmd.clip = clip;
    cmd.pitch = pitch;
    cmd.vol = vol;

    ret = mix_cmd_push(mix, &cmd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = cmd.id;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_voice_stop(ldg_audio_mix_t *mix, uint64_t voice)
{
    mix_cmd_t cmd = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!mix)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(voice == LDG_AUDIO_MIX_ID_NONE || voice > mix->id_next)) { return LDG_ERR_NOT_FOUND; }

    cmd.kind = MIX_CMD_KIND_STOP;
    cmd.id = voice;

    return mix_cmd_push(mix, &cmd);
}

uint32_t ldg_audio_mix_stream_set(ldg_audio_mix_t *mix, ldg_audio_mix_stream_cb_t cb, void *user, uint32_t rate)
{
    mix_cmd_t cmd = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!mix || !cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rate == 0 || (uint64_t)rate > (uint64_t)mix->rate * LDG_AUDIO_MIX_STREAM_RATIO_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cmd.kind = MIX_CMD_KIND_STREAM_SET;
    cmd.cb = cb;
    cmd.user = user;
    cmd.rate = rate;

    return mix_cmd_push(mix, &cmd);
}

uint32_t ldg_audio_mix_stream_clear(ldg_audio_mix_t *mix)
{
    mix_cmd_t cmd = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!mix)) { return LDG_ERR_FUNC_ARG_NULL; }

    cmd.kind = MIX_CMD_KIND_STREAM_CLEAR;

    return mix_cmd_push(mix, &cmd);
}

uint32_t ldg_audio_mix_bus_vol_set(ldg_audio_mix_t *mix, uint32_t bus, uint32_t vol)
{
    mix_cmd_t cmd = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mix)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bus >= LDG_AUDIO_MIX_BUS_CUNT || vol > LDG_AUDIO_MIX_VOL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cmd.kind = MIX_CMD_KIND_BUS_VOL;
    cmd.bus = bus;
    cmd.vol = vol;

    ret = mix_cmd_push(mix, &cmd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mix->bus_vol[bus] = vol;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_mix_bus_vol_get(const ldg_audio_mix_t *mix, uint32_t bus, uint32_t *out)
{
    if (LDG_UNLIKELY(!mix || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(bus >= LDG_AUDIO_MIX_BUS_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = mix->bus_vol[bus];

    return LDG_ERR_AOK;
}
