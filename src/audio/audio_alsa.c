#ifdef LDG_AUDIO_ALSA

#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#include <alsa/asoundlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define ALSA_CARD_FIRST ((int32_t) ~0u)

typedef struct ldg_audio_ctx
{
    snd_mixer_t *mixer;
    snd_mixer_elem_t *master_elem;
    long vol_min;
    long vol_max;
    int32_t card_idx;
    uint8_t is_init;
    uint8_t pudding[3];
} ldg_audio_ctx_t;

static ldg_audio_ctx_t g_audio_ctx = { 0 };

static uint32_t audio_mixer_open(int32_t card_idx)
{
    char card_name[32] = { 0 };
    snd_mixer_selem_id_t *sid = 0x0;
    int ret = 0;

    if (LDG_UNLIKELY(snprintf(card_name, sizeof(card_name), "hw:%d", card_idx) < 0)) { return LDG_ERR_AUDIO_INIT; }

    ret = snd_mixer_open(&g_audio_ctx.mixer, 0);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_AUDIO_INIT; }

    ret = snd_mixer_attach(g_audio_ctx.mixer, card_name);
    if (LDG_UNLIKELY(ret < 0))
    {
        snd_mixer_close(g_audio_ctx.mixer);
        g_audio_ctx.mixer = 0x0;
        return LDG_ERR_AUDIO_INIT;
    }

    ret = snd_mixer_selem_register(g_audio_ctx.mixer, 0x0, 0x0);
    if (LDG_UNLIKELY(ret < 0))
    {
        snd_mixer_close(g_audio_ctx.mixer);
        g_audio_ctx.mixer = 0x0;
        return LDG_ERR_AUDIO_INIT;
    }

    ret = snd_mixer_load(g_audio_ctx.mixer);
    if (LDG_UNLIKELY(ret < 0))
    {
        snd_mixer_close(g_audio_ctx.mixer);
        g_audio_ctx.mixer = 0x0;
        return LDG_ERR_AUDIO_INIT;
    }

    snd_mixer_selem_id_alloca(&sid);

    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master");
    g_audio_ctx.master_elem = snd_mixer_find_selem(g_audio_ctx.mixer, sid);

    if (!g_audio_ctx.master_elem)
    {
        snd_mixer_selem_id_set_name(sid, "PCM");
        g_audio_ctx.master_elem = snd_mixer_find_selem(g_audio_ctx.mixer, sid);
    }

    if (!g_audio_ctx.master_elem)
    {
        snd_mixer_close(g_audio_ctx.mixer);
        g_audio_ctx.mixer = 0x0;
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    if (LDG_UNLIKELY(snd_mixer_selem_get_playback_volume_range(g_audio_ctx.master_elem, &g_audio_ctx.vol_min, &g_audio_ctx.vol_max) < 0)) { return LDG_ERR_AUDIO_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_init(void)
{
    char *card_env = 0x0;
    int32_t card_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(g_audio_ctx.is_init)) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(memset(&g_audio_ctx, 0, sizeof(ldg_audio_ctx_t)) != &g_audio_ctx)) { return LDG_ERR_MEM_BAD; }

    card_env = getenv("ALSA_CARD");
    if (card_env) { card_idx = (int32_t)strtol(card_env, 0x0, LDG_BASE_DECIMAL); }

    ret = audio_mixer_open(card_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_ctx.card_idx = card_idx;
    g_audio_ctx.is_init = 1;

    return LDG_ERR_AOK;
}

void ldg_audio_shutdown(void)
{
    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return; }

    if (g_audio_ctx.mixer)
    {
        snd_mixer_close(g_audio_ctx.mixer);
        g_audio_ctx.mixer = 0x0;
    }

    g_audio_ctx.master_elem = 0x0;
    g_audio_ctx.is_init = 0;
}

uint32_t ldg_audio_master_volume_get(double *vol)
{
    long val = 0;
    int ret = 0;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(!g_audio_ctx.master_elem)) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    ret = snd_mixer_selem_get_playback_volume(g_audio_ctx.master_elem, SND_MIXER_SCHN_FRONT_LEFT, &val);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_AUDIO_INIT; }

    if (g_audio_ctx.vol_max == g_audio_ctx.vol_min) { *vol = 0.0; }
    else{ *vol = (double)(val - g_audio_ctx.vol_min) / (double)(g_audio_ctx.vol_max - g_audio_ctx.vol_min); }

    if (*vol < 0.0) { *vol = 0.0; }

    if (*vol > 1.0) { *vol = 1.0; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_master_volume_set(double vol)
{
    long val = 0;
    int ret = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(!g_audio_ctx.master_elem)) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOLUME_RANGE; }

    val = g_audio_ctx.vol_min + (long)(vol * (double)(g_audio_ctx.vol_max - g_audio_ctx.vol_min));

    ret = snd_mixer_selem_set_playback_volume_all(g_audio_ctx.master_elem, val);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_AUDIO_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_volume_get(uint32_t stream_id, double *vol)
{
    (void)stream_id;
    (void)vol;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

uint32_t ldg_audio_stream_volume_set(uint32_t stream_id, double vol)
{
    (void)stream_id;
    (void)vol;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, uint8_t *muted)
{
    int val = 0;
    int ret = 0;

    if (stream_id != 0) { return LDG_ERR_AUDIO_NOT_AVAILABLE; }

    if (LDG_UNLIKELY(!muted)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(!g_audio_ctx.master_elem)) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (!snd_mixer_selem_has_playback_switch(g_audio_ctx.master_elem))
    {
        *muted = 0;
        return LDG_ERR_AOK;
    }

    ret = snd_mixer_selem_get_playback_switch(g_audio_ctx.master_elem, SND_MIXER_SCHN_FRONT_LEFT, &val);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_AUDIO_INIT; }

    *muted = (val == 0) ? 1 : 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, uint8_t muted)
{
    int ret = 0;

    if (stream_id != 0) { return LDG_ERR_AUDIO_NOT_AVAILABLE; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(!g_audio_ctx.master_elem)) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (!snd_mixer_selem_has_playback_switch(g_audio_ctx.master_elem)) { return LDG_ERR_AOK; }

    ret = snd_mixer_selem_set_playback_switch_all(g_audio_ctx.master_elem, muted ? 0 : 1);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_AUDIO_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_list(ldg_audio_stream_t **streams, uint32_t *cunt)
{
    (void)streams;
    (void)cunt;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

void ldg_audio_stream_free(ldg_audio_stream_t *streams)
{
    (void)streams;
}

uint32_t ldg_audio_stream_name_get(const char *name, ldg_audio_stream_t *stream)
{
    (void)name;
    (void)stream;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

void ldg_audio_self_register(uint32_t stream_id)
{
    (void)stream_id;
}

uint32_t ldg_audio_self_id_get(void)
{
    return 0;
}

uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream)
{
    (void)stream;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

uint32_t ldg_audio_self_volume_get(double *vol)
{
    (void)vol;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

uint32_t ldg_audio_self_volume_set(double vol)
{
    (void)vol;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

uint32_t ldg_audio_sync(void)
{
    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_sink_list(ldg_audio_sink_t **sinks, uint32_t *cunt)
{
    int32_t card = ALSA_CARD_FIRST;
    uint32_t n = 0;
    uint32_t i = 0;
    ldg_audio_sink_t *arr = 0x0;
    snd_ctl_t *ctl = 0x0;
    snd_ctl_card_info_t *info = 0x0;
    char card_name[32] = { 0 };

    if (LDG_UNLIKELY(!sinks || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    snd_ctl_card_info_alloca(&info);

    while (snd_card_next(&card) >= 0 && card >= 0) { n++; }

    if (n == 0)
    {
        *sinks = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_sink_t *)ldg_mem_alloc(n * sizeof(ldg_audio_sink_t));
    if (LDG_UNLIKELY(!arr)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(arr, 0, n * sizeof(ldg_audio_sink_t)) != arr)) { ldg_mem_dealloc(arr); return LDG_ERR_MEM_BAD; }

    card = ALSA_CARD_FIRST;
    while (snd_card_next(&card) >= 0 && card >= 0 && i < n)
    {
        if (LDG_UNLIKELY(snprintf(card_name, sizeof(card_name), "hw:%d", card) < 0)) { continue; }

        if (snd_ctl_open(&ctl, card_name, 0) >= 0)
        {
            if (snd_ctl_card_info(ctl, info) >= 0)
            {
                arr[i].id = (uint32_t)card;
                if (LDG_UNLIKELY(ldg_strrbrcpy(arr[i].name, snd_ctl_card_info_get_id(info), LDG_AUDIO_NAME_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); continue; }

                if (LDG_UNLIKELY(ldg_strrbrcpy(arr[i].desc, snd_ctl_card_info_get_name(info), LDG_AUDIO_DESC_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); continue; }

                arr[i].volume = 0.0;
                arr[i].muted = 0;
                arr[i].is_default = (card == g_audio_ctx.card_idx) ? 1 : 0;
                i++;
            }

            snd_ctl_close(ctl);
        }
    }

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
    int32_t card = ALSA_CARD_FIRST;
    uint32_t n = 0;
    uint32_t i = 0;
    ldg_audio_source_t *arr = 0x0;
    snd_ctl_t *ctl = 0x0;
    snd_ctl_card_info_t *info = 0x0;
    char card_name[32] = { 0 };

    if (LDG_UNLIKELY(!sources || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    snd_ctl_card_info_alloca(&info);

    while (snd_card_next(&card) >= 0 && card >= 0) { n++; }

    if (n == 0)
    {
        *sources = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    arr = (ldg_audio_source_t *)ldg_mem_alloc(n * sizeof(ldg_audio_source_t));
    if (LDG_UNLIKELY(!arr)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(arr, 0, n * sizeof(ldg_audio_source_t)) != arr)) { ldg_mem_dealloc(arr); return LDG_ERR_MEM_BAD; }

    card = ALSA_CARD_FIRST;
    while (snd_card_next(&card) >= 0 && card >= 0 && i < n)
    {
        if (LDG_UNLIKELY(snprintf(card_name, sizeof(card_name), "hw:%d", card) < 0)) { continue; }

        if (snd_ctl_open(&ctl, card_name, 0) >= 0)
        {
            if (snd_ctl_card_info(ctl, info) >= 0)
            {
                arr[i].id = (uint32_t)card;
                if (LDG_UNLIKELY(ldg_strrbrcpy(arr[i].name, snd_ctl_card_info_get_id(info), LDG_AUDIO_NAME_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); continue; }

                if (LDG_UNLIKELY(ldg_strrbrcpy(arr[i].desc, snd_ctl_card_info_get_name(info), LDG_AUDIO_DESC_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); continue; }

                arr[i].volume = 0.0;
                arr[i].muted = 0;
                arr[i].is_default = (card == g_audio_ctx.card_idx) ? 1 : 0;
                i++;
            }

            snd_ctl_close(ctl);
        }
    }

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
    snd_ctl_t *ctl = 0x0;
    snd_ctl_card_info_t *info = 0x0;
    char card_name[32] = { 0 };

    if (LDG_UNLIKELY(!sink)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    snd_ctl_card_info_alloca(&info);
    if (LDG_UNLIKELY(snprintf(card_name, sizeof(card_name), "hw:%d", g_audio_ctx.card_idx) < 0)) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (snd_ctl_open(&ctl, card_name, 0) < 0) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (snd_ctl_card_info(ctl, info) < 0)
    {
        snd_ctl_close(ctl);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    if (LDG_UNLIKELY(memset(sink, 0, sizeof(ldg_audio_sink_t)) != sink)) { snd_ctl_close(ctl); return LDG_ERR_MEM_BAD; }

    sink->id = (uint32_t)g_audio_ctx.card_idx;
    if (LDG_UNLIKELY(ldg_strrbrcpy(sink->name, snd_ctl_card_info_get_id(info), LDG_AUDIO_NAME_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(ldg_strrbrcpy(sink->desc, snd_ctl_card_info_get_name(info), LDG_AUDIO_DESC_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); return LDG_ERR_MEM_BAD; }

    sink->is_default = 1;

    if (LDG_UNLIKELY(ldg_audio_master_volume_get(&sink->volume) != LDG_ERR_AOK)) { sink->volume = 0.0; }

    if (LDG_UNLIKELY(ldg_audio_stream_mute_get(0, &sink->muted) != LDG_ERR_AOK)) { sink->muted = 0; }

    snd_ctl_close(ctl);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_default_source_get(ldg_audio_source_t *source)
{
    snd_ctl_t *ctl = 0x0;
    snd_ctl_card_info_t *info = 0x0;
    char card_name[32] = { 0 };

    if (LDG_UNLIKELY(!source)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    snd_ctl_card_info_alloca(&info);
    if (LDG_UNLIKELY(snprintf(card_name, sizeof(card_name), "hw:%d", g_audio_ctx.card_idx) < 0)) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (snd_ctl_open(&ctl, card_name, 0) < 0) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (snd_ctl_card_info(ctl, info) < 0)
    {
        snd_ctl_close(ctl);
        return LDG_ERR_AUDIO_NO_DEFAULT;
    }

    if (LDG_UNLIKELY(memset(source, 0, sizeof(ldg_audio_source_t)) != source)) { snd_ctl_close(ctl); return LDG_ERR_MEM_BAD; }

    source->id = (uint32_t)g_audio_ctx.card_idx;
    if (LDG_UNLIKELY(ldg_strrbrcpy(source->name, snd_ctl_card_info_get_id(info), LDG_AUDIO_NAME_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(ldg_strrbrcpy(source->desc, snd_ctl_card_info_get_name(info), LDG_AUDIO_DESC_MAX) > LDG_ERR_STR_OVERLAP)) { snd_ctl_close(ctl); return LDG_ERR_MEM_BAD; }

    source->is_default = 1;

    snd_ctl_close(ctl);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_duck(double factor, uint32_t exclude_stream_id)
{
    (void)factor;
    (void)exclude_stream_id;
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

uint32_t ldg_audio_unduck(void)
{
    return LDG_ERR_AUDIO_NOT_AVAILABLE;
}

#endif
