#include <stdint.h>

#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/thread/sync.h>

#define LDG_AUDIO_WAV_CHUNK_HDR 8
#define LDG_AUDIO_WAV_FMT_MIN 16
#define LDG_AUDIO_WAV_FMT_PCM 1
#define LDG_AUDIO_WAV_FMT_EXTENSIBLE 0xFFFE
#define LDG_AUDIO_WAV_FMT_EXT_MIN 40
#define LDG_AUDIO_WAV_FMT_CB_OFF 16
#define LDG_AUDIO_WAV_FMT_CB_MIN 22
#define LDG_AUDIO_WAV_FMT_VALID_DEPTH_OFF 18
#define LDG_AUDIO_WAV_FMT_SUBFORMAT_OFF 24
#define LDG_AUDIO_WAV_GUID_TAIL0_OFF 4
#define LDG_AUDIO_WAV_GUID_TAIL1_OFF 8
#define LDG_AUDIO_WAV_GUID_TAIL2_OFF 12
#define LDG_AUDIO_WAV_GUID_TAIL0 0x00100000
#define LDG_AUDIO_WAV_GUID_TAIL1 0xAA000080
#define LDG_AUDIO_WAV_GUID_TAIL2 0x719B3800
#define LDG_AUDIO_WAV_SCAN_MAX 1024
#define LDG_AUDIO_WAV_BIT_DEPTH_MIN 8
#define LDG_AUDIO_WAV_BIT_DEPTH_MAX 32
#define LDG_AUDIO_WAV_CHANNEL_MAX 8
#define LDG_AUDIO_WAV_RATE_MIN 1000
#define LDG_AUDIO_WAV_RATE_MAX 384000
#define LDG_AUDIO_WAV_TAG_SIZE 4
#define LDG_AUDIO_WAV_BODY_OFF 12
#define LDG_AUDIO_WAV_PAD_MASK 1
#define LDG_AUDIO_WAV_CHANNEL_OFF 2
#define LDG_AUDIO_WAV_RATE_OFF 4
#define LDG_AUDIO_WAV_DEPTH_OFF 14
#define LDG_AUDIO_PLAY_VOL_UNITY 1.0
#define LDG_AUDIO_PLAY_NONE 0
#define LDG_AUDIO_PLAY_NO_MODE 0
#define LDG_AUDIO_PLAY_STEP 1
#define LDG_AUDIO_PLAY_ONE_TRACK 1
#define LDG_AUDIO_WAV_RIFF_OFF 0
#define LDG_AUDIO_WAV_RIFF_SIZE_OFF 4
#define LDG_AUDIO_WAV_WAVE_OFF 8
#define LDG_AUDIO_WAV_FMT_TAG_OFF 12
#define LDG_AUDIO_WAV_FMT_SIZE_OFF 16
#define LDG_AUDIO_WAV_FMT_CODE_OFF 20
#define LDG_AUDIO_WAV_FMT_CH_OFF 22
#define LDG_AUDIO_WAV_FMT_RATE_OFF 24
#define LDG_AUDIO_WAV_FMT_BYTE_RATE_OFF 28
#define LDG_AUDIO_WAV_FMT_ALIGN_OFF 32
#define LDG_AUDIO_WAV_FMT_DEPTH_OFF 34
#define LDG_AUDIO_WAV_DATA_TAG_OFF 36
#define LDG_AUDIO_WAV_DATA_SIZE_OFF 40
#define LDG_AUDIO_WAV_RIFF_ADJ 36
#define LDG_AUDIO_WAV_HDR_SIZE 44

typedef struct ldg_audio_play_track
{
    char path[LDG_AUDIO_PLAY_PATH_MAX];
    const ldg_byte_t *raw;
    uint64_t data_off;
    uint64_t data_size;
    uint64_t frame_cunt;
    uint32_t rate;
    uint16_t channel_cunt;
    uint16_t bit_depth;
} ldg_audio_play_track_t;

typedef struct ldg_audio_play_ctx
{
    ldg_audio_play_track_t track[LDG_AUDIO_PLAY_TRACK_MAX];
    ldg_audio_play_track_t *cur;
    ldg_io_file_t *file;
    ldg_mut_t mut;
    uint64_t frame_pos;
    double vol;
    uint32_t track_cunt;
    uint32_t track_idx;
    ldg_audio_play_state_t state;
    uint8_t loop;
    uint8_t is_init;
    uint8_t pudding[6];
} ldg_audio_play_ctx_t;

static ldg_audio_play_ctx_t g_audio_play = LDG_STRUCT_ZERO_INIT;

static uint32_t wav_dword_get(const uint8_t *raw)
{
    return (uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16) | ((uint32_t)raw[3] << 24);
}

static uint16_t wav_word_get(const uint8_t *raw)
{
    return (uint16_t)((uint32_t)raw[0] | ((uint32_t)raw[1] << 8));
}

static void wav_dword_put(uint8_t *raw, uint32_t val)
{
    raw[0] = (uint8_t)(val & UINT8_MAX);
    raw[1] = (uint8_t)((val >> 8) & UINT8_MAX);
    raw[2] = (uint8_t)((val >> 16) & UINT8_MAX);
    raw[3] = (uint8_t)((val >> 24) & UINT8_MAX);
}

static void wav_word_put(uint8_t *raw, uint16_t val)
{
    raw[0] = (uint8_t)(val & UINT8_MAX);
    raw[1] = (uint8_t)(((uint32_t)val >> 8) & UINT8_MAX);
}

static void wav_tag_put(uint8_t *raw, const char *tag)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_AUDIO_WAV_TAG_SIZE; idx++) { raw[idx] = (uint8_t)tag[idx]; }
}

static uint8_t wav_tag_match_is(const uint8_t *raw, const char *tag)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_AUDIO_WAV_TAG_SIZE; idx++) { if (raw[idx] != (uint8_t)tag[idx]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint8_t wav_guid_tail_match_is(const uint8_t *guid)
{
    if (LDG_UNLIKELY(!guid)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (wav_dword_get(guid + LDG_AUDIO_WAV_GUID_TAIL0_OFF) != LDG_AUDIO_WAV_GUID_TAIL0) { return LDG_TRUTH_FALSE; }

    if (wav_dword_get(guid + LDG_AUDIO_WAV_GUID_TAIL1_OFF) != LDG_AUDIO_WAV_GUID_TAIL1) { return LDG_TRUTH_FALSE; }

    if (wav_dword_get(guid + LDG_AUDIO_WAV_GUID_TAIL2_OFF) != LDG_AUDIO_WAV_GUID_TAIL2) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t wav_fmt_parse(const uint8_t *raw, uint64_t size, ldg_audio_play_track_t *track)
{
    uint32_t code = 0;
    uint16_t valid_depth = 0;

    if (LDG_UNLIKELY(size < LDG_AUDIO_WAV_FMT_MIN)) { return LDG_ERR_IO_FMT; }

    code = (uint32_t)wav_word_get(raw);

    if (code == LDG_AUDIO_WAV_FMT_EXTENSIBLE)
    {
        if (LDG_UNLIKELY(size < LDG_AUDIO_WAV_FMT_EXT_MIN)) { return LDG_ERR_IO_FMT; }

        if (LDG_UNLIKELY(wav_word_get(raw + LDG_AUDIO_WAV_FMT_CB_OFF) < LDG_AUDIO_WAV_FMT_CB_MIN)) { return LDG_ERR_IO_FMT; }

        if (LDG_UNLIKELY(wav_guid_tail_match_is(raw + LDG_AUDIO_WAV_FMT_SUBFORMAT_OFF) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

        code = wav_dword_get(raw + LDG_AUDIO_WAV_FMT_SUBFORMAT_OFF);
        valid_depth = wav_word_get(raw + LDG_AUDIO_WAV_FMT_VALID_DEPTH_OFF);
    }

    if (LDG_UNLIKELY(code != LDG_AUDIO_WAV_FMT_PCM)) { return LDG_ERR_UNSUPPORTED; }

    track->channel_cunt = wav_word_get(raw + LDG_AUDIO_WAV_CHANNEL_OFF);
    track->rate = wav_dword_get(raw + LDG_AUDIO_WAV_RATE_OFF);
    track->bit_depth = wav_word_get(raw + LDG_AUDIO_WAV_DEPTH_OFF);

    if (LDG_UNLIKELY(track->channel_cunt == 0 || track->channel_cunt > LDG_AUDIO_WAV_CHANNEL_MAX)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(track->rate < LDG_AUDIO_WAV_RATE_MIN || track->rate > LDG_AUDIO_WAV_RATE_MAX)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(track->bit_depth < LDG_AUDIO_WAV_BIT_DEPTH_MIN || track->bit_depth > LDG_AUDIO_WAV_BIT_DEPTH_MAX)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY((track->bit_depth % LDG_AUDIO_WAV_BIT_DEPTH_MIN) != 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(valid_depth != 0 && valid_depth != track->bit_depth)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static uint32_t wav_track_parse(const char *path, ldg_audio_play_track_t *track)
{
    uint8_t hdr[LDG_AUDIO_WAV_CHUNK_HDR] = LDG_ARR_ZERO_INIT;
    uint8_t fmt[LDG_AUDIO_WAV_SCAN_MAX] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    ldg_io_info_t info = LDG_STRUCT_ZERO_INIT;
    uint64_t rd_cunt = 0;
    uint64_t off = 0;
    uint64_t chunk_size = 0;
    uint64_t frame_size = 0;
    uint32_t ret = 0;
    uint8_t fmt_seen = LDG_TRUTH_FALSE;

    track->raw = 0x0;

    ret = ldg_io_path_info_get(path, &info);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_open(path, LDG_IO_RD_ONLY, LDG_AUDIO_PLAY_NO_MODE, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_rd(file, hdr, LDG_AUDIO_WAV_CHUNK_HDR, &rd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || rd_cunt != LDG_AUDIO_WAV_CHUNK_HDR || wav_tag_match_is(hdr, "RIFF") != LDG_TRUTH_TRUE))
    {
        ldg_io_file_close(file);
        return LDG_ERR_IO_FMT;
    }

    ret = ldg_io_file_rd(file, hdr, LDG_AUDIO_WAV_TAG_SIZE, &rd_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || rd_cunt != LDG_AUDIO_WAV_TAG_SIZE || wav_tag_match_is(hdr, "WAVE") != LDG_TRUTH_TRUE))
    {
        ldg_io_file_close(file);
        return LDG_ERR_IO_FMT;
    }

    off = LDG_AUDIO_WAV_BODY_OFF;
    while (off + LDG_AUDIO_WAV_CHUNK_HDR <= info.size)
    {
        ret = ldg_io_file_seek(file, off, LDG_IO_SEEK_START);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

        ret = ldg_io_file_rd(file, hdr, LDG_AUDIO_WAV_CHUNK_HDR, &rd_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK || rd_cunt != LDG_AUDIO_WAV_CHUNK_HDR)) { break; }

        chunk_size = (uint64_t)wav_dword_get(hdr + LDG_AUDIO_WAV_TAG_SIZE);
        off = off + LDG_AUDIO_WAV_CHUNK_HDR;

        if (wav_tag_match_is(hdr, "fmt ") == LDG_TRUTH_TRUE)
        {
            if (chunk_size > LDG_AUDIO_WAV_SCAN_MAX) { chunk_size = LDG_AUDIO_WAV_SCAN_MAX; }

            ret = ldg_io_file_rd(file, fmt, chunk_size, &rd_cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

            ret = wav_fmt_parse(fmt, rd_cunt, track);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                ldg_io_file_close(file);
                return ret;
            }

            fmt_seen = LDG_TRUTH_TRUE;
        }
        else if (wav_tag_match_is(hdr, "data") == LDG_TRUTH_TRUE)
        {
            track->data_off = off;
            track->data_size = chunk_size;
            if (track->data_off + track->data_size > info.size) { track->data_size = info.size - track->data_off; }

            break;
        }

        off = off + chunk_size + (chunk_size & LDG_AUDIO_WAV_PAD_MASK);
    }

    ldg_io_file_close(file);

    if (LDG_UNLIKELY(fmt_seen != LDG_TRUTH_TRUE || track->data_size == 0)) { return LDG_ERR_IO_FMT; }

    frame_size = (uint64_t)track->channel_cunt * ((uint64_t)track->bit_depth / LDG_AUDIO_WAV_BIT_DEPTH_MIN);
    if (LDG_UNLIKELY(frame_size == 0)) { return LDG_ERR_IO_FMT; }

    track->frame_cunt = track->data_size / frame_size;
    if (LDG_UNLIKELY(track->frame_cunt == 0)) { return LDG_ERR_IO_FMT; }

    return ldg_str_rbrcpy(track->path, path, LDG_AUDIO_PLAY_PATH_MAX);
}

static uint32_t wav_mem_parse(const ldg_byte_t *raw, uint64_t size, ldg_audio_play_track_t *track)
{
    uint64_t off = 0;
    uint64_t chunk_size = 0;
    uint64_t frame_size = 0;
    uint32_t ret = 0;
    uint8_t fmt_seen = LDG_TRUTH_FALSE;

    track->raw = 0x0;
    track->path[0] = LDG_STR_TERM;
    track->data_off = 0;
    track->data_size = 0;

    if (LDG_UNLIKELY(size < LDG_AUDIO_WAV_BODY_OFF)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(wav_tag_match_is(raw, "RIFF") != LDG_TRUTH_TRUE)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(wav_tag_match_is(raw + LDG_AUDIO_WAV_CHUNK_HDR, "WAVE") != LDG_TRUTH_TRUE)) { return LDG_ERR_IO_FMT; }

    off = LDG_AUDIO_WAV_BODY_OFF;
    while (off + LDG_AUDIO_WAV_CHUNK_HDR <= size)
    {
        chunk_size = (uint64_t)wav_dword_get(raw + off + LDG_AUDIO_WAV_TAG_SIZE);

        if (wav_tag_match_is(raw + off, "fmt ") == LDG_TRUTH_TRUE)
        {
            if (LDG_UNLIKELY(off + LDG_AUDIO_WAV_CHUNK_HDR + chunk_size > size)) { return LDG_ERR_IO_FMT; }

            ret = wav_fmt_parse(raw + off + LDG_AUDIO_WAV_CHUNK_HDR, chunk_size, track);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            fmt_seen = LDG_TRUTH_TRUE;
        }
        else if (wav_tag_match_is(raw + off, "data") == LDG_TRUTH_TRUE)
        {
            track->data_off = off + LDG_AUDIO_WAV_CHUNK_HDR;
            track->data_size = chunk_size;
            if (track->data_off + track->data_size > size) { track->data_size = size - track->data_off; }

            break;
        }

        off = off + LDG_AUDIO_WAV_CHUNK_HDR + chunk_size + (chunk_size & LDG_AUDIO_WAV_PAD_MASK);
    }

    if (LDG_UNLIKELY(fmt_seen != LDG_TRUTH_TRUE || track->data_size == 0)) { return LDG_ERR_IO_FMT; }

    frame_size = (uint64_t)track->channel_cunt * ((uint64_t)track->bit_depth / LDG_AUDIO_WAV_BIT_DEPTH_MIN);
    if (LDG_UNLIKELY(frame_size == 0)) { return LDG_ERR_IO_FMT; }

    track->frame_cunt = track->data_size / frame_size;
    if (LDG_UNLIKELY(track->frame_cunt == 0)) { return LDG_ERR_IO_FMT; }

    track->raw = raw;

    return LDG_ERR_AOK;
}

static uint32_t play_track_ld(uint32_t track_idx)
{
    ldg_audio_play_track_t *track = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(track_idx >= g_audio_play.track_cunt)) { return LDG_ERR_BOUNDS; }

    if (g_audio_play.file != 0x0)
    {
        ldg_io_file_close(g_audio_play.file);
        g_audio_play.file = 0x0;
    }

    track = &g_audio_play.track[track_idx];

    if (track->raw != 0x0)
    {
        g_audio_play.cur = track;
        g_audio_play.track_idx = track_idx;
        g_audio_play.frame_pos = LDG_AUDIO_PLAY_NONE;

        return LDG_ERR_AOK;
    }

    ret = ldg_io_file_open(track->path, LDG_IO_RD_ONLY, LDG_AUDIO_PLAY_NO_MODE, &g_audio_play.file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_seek(g_audio_play.file, track->data_off, LDG_IO_SEEK_START);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_io_file_close(g_audio_play.file);
        g_audio_play.file = 0x0;
        return ret;
    }

    g_audio_play.cur = track;
    g_audio_play.track_idx = track_idx;
    g_audio_play.frame_pos = LDG_AUDIO_PLAY_NONE;

    return LDG_ERR_AOK;
}

static uint32_t play_ctx_arm(void)
{
    uint32_t ret = 0;

    if (g_audio_play.is_init == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_mut_init(&g_audio_play.mut, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_play.vol = LDG_AUDIO_PLAY_VOL_UNITY;
    g_audio_play.is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_list_open(const char *const *path, uint32_t cunt)
{
    uint32_t ret = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0 || cunt > LDG_AUDIO_PLAY_TRACK_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = play_ctx_arm();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_audio_play.file != 0x0)
    {
        ldg_io_file_close(g_audio_play.file);
        g_audio_play.file = 0x0;
    }

    g_audio_play.cur = 0x0;
    g_audio_play.track_cunt = LDG_AUDIO_PLAY_NONE;
    g_audio_play.track_idx = LDG_AUDIO_PLAY_NONE;
    g_audio_play.frame_pos = LDG_AUDIO_PLAY_NONE;
    g_audio_play.state = LDG_AUDIO_PLAY_STATE_IDLE;

    for (idx = 0; idx < cunt; idx++)
    {
        if (LDG_UNLIKELY(!path[idx]))
        {
            ldg_mut_unlock(&g_audio_play.mut);
            return LDG_ERR_FUNC_ARG_NULL;
        }

        ret = wav_track_parse(path[idx], &g_audio_play.track[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mut_unlock(&g_audio_play.mut);
            return ret;
        }

        g_audio_play.track_cunt = idx + LDG_AUDIO_PLAY_STEP;
    }

    ret = play_track_ld(LDG_AUDIO_PLAY_NONE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return ret;
    }

    return ldg_mut_unlock(&g_audio_play.mut);
}

uint32_t ldg_audio_play_open(const char *path)
{
    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_audio_play_list_open(&path, LDG_AUDIO_PLAY_ONE_TRACK);
}

uint32_t ldg_audio_wav_wrap(const ldg_byte_t *pcm, uint64_t pcm_size, uint32_t rate, uint16_t channel_cunt, uint16_t bit_depth, ldg_byte_t *out, uint64_t out_cap, uint64_t *out_size)
{
    uint64_t total = 0;
    uint64_t blk_align = 0;
    uint64_t byte_rate = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pcm || !out || !out_size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_size = 0;

    if (LDG_UNLIKELY(pcm_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pcm_size > UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(channel_cunt == 0 || channel_cunt > LDG_AUDIO_WAV_CHANNEL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(rate < LDG_AUDIO_WAV_RATE_MIN || rate > LDG_AUDIO_WAV_RATE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(bit_depth < LDG_AUDIO_WAV_BIT_DEPTH_MIN || bit_depth > LDG_AUDIO_WAV_BIT_DEPTH_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((bit_depth % LDG_AUDIO_WAV_BIT_DEPTH_MIN) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = LDG_AUDIO_WAV_HDR_SIZE + pcm_size;
    *out_size = total;

    if (LDG_UNLIKELY(out_cap < total)) { return LDG_ERR_OVERFLOW; }

    blk_align = (uint64_t)channel_cunt * ((uint64_t)bit_depth / LDG_AUDIO_WAV_BIT_DEPTH_MIN);
    byte_rate = (uint64_t)rate * blk_align;

    wav_tag_put(out + LDG_AUDIO_WAV_RIFF_OFF, "RIFF");
    wav_dword_put(out + LDG_AUDIO_WAV_RIFF_SIZE_OFF, (uint32_t)(LDG_AUDIO_WAV_RIFF_ADJ + pcm_size));
    wav_tag_put(out + LDG_AUDIO_WAV_WAVE_OFF, "WAVE");
    wav_tag_put(out + LDG_AUDIO_WAV_FMT_TAG_OFF, "fmt ");
    wav_dword_put(out + LDG_AUDIO_WAV_FMT_SIZE_OFF, LDG_AUDIO_WAV_FMT_MIN);
    wav_word_put(out + LDG_AUDIO_WAV_FMT_CODE_OFF, LDG_AUDIO_WAV_FMT_PCM);
    wav_word_put(out + LDG_AUDIO_WAV_FMT_CH_OFF, channel_cunt);
    wav_dword_put(out + LDG_AUDIO_WAV_FMT_RATE_OFF, rate);
    wav_dword_put(out + LDG_AUDIO_WAV_FMT_BYTE_RATE_OFF, (uint32_t)byte_rate);
    wav_word_put(out + LDG_AUDIO_WAV_FMT_ALIGN_OFF, (uint16_t)blk_align);
    wav_word_put(out + LDG_AUDIO_WAV_FMT_DEPTH_OFF, bit_depth);
    wav_tag_put(out + LDG_AUDIO_WAV_DATA_TAG_OFF, "data");
    wav_dword_put(out + LDG_AUDIO_WAV_DATA_SIZE_OFF, (uint32_t)pcm_size);

    ret = ldg_mem_secure_cpy(out + LDG_AUDIO_WAV_HDR_SIZE, pcm, pcm_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_wav_unwrap(const ldg_byte_t *raw, uint64_t size, ldg_audio_wav_info_t *out)
{
    ldg_audio_play_track_t track = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!raw || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_audio_wav_info_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = wav_mem_parse(raw, size, &track);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->data_off = track.data_off;
    out->data_size = track.data_size;
    out->frame_cunt = track.frame_cunt;
    out->rate = track.rate;
    out->channel_cunt = track.channel_cunt;
    out->bit_depth = track.bit_depth;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_mem_open(const ldg_byte_t *raw, uint64_t size)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!raw)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = play_ctx_arm();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_audio_play.file != 0x0)
    {
        ldg_io_file_close(g_audio_play.file);
        g_audio_play.file = 0x0;
    }

    g_audio_play.cur = 0x0;
    g_audio_play.track_cunt = LDG_AUDIO_PLAY_NONE;
    g_audio_play.track_idx = LDG_AUDIO_PLAY_NONE;
    g_audio_play.frame_pos = LDG_AUDIO_PLAY_NONE;
    g_audio_play.state = LDG_AUDIO_PLAY_STATE_IDLE;

    ret = wav_mem_parse(raw, size, &g_audio_play.track[LDG_AUDIO_PLAY_NONE]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return ret;
    }

    g_audio_play.track_cunt = LDG_AUDIO_PLAY_ONE_TRACK;

    ret = play_track_ld(LDG_AUDIO_PLAY_NONE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return ret;
    }

    return ldg_mut_unlock(&g_audio_play.mut);
}

uint32_t ldg_audio_play_fmt_get(uint32_t *out_rate, uint16_t *out_channel_cunt, uint16_t *out_bit_depth)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out_rate || !out_channel_cunt || !out_bit_depth)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rate = LDG_AUDIO_PLAY_NONE;
    *out_channel_cunt = LDG_AUDIO_PLAY_NONE;
    *out_bit_depth = LDG_AUDIO_PLAY_NONE;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!g_audio_play.cur))
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return LDG_ERR_EMPTY;
    }

    *out_rate = g_audio_play.cur->rate;
    *out_channel_cunt = g_audio_play.cur->channel_cunt;
    *out_bit_depth = g_audio_play.cur->bit_depth;

    return ldg_mut_unlock(&g_audio_play.mut);
}

uint32_t ldg_audio_play_frames_pull(void *buff, uint32_t frame_cunt, uint32_t *out_cunt)
{
    uint64_t frame_size = 0;
    uint64_t want = 0;
    uint64_t remain = 0;
    uint64_t rd_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!buff || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = LDG_AUDIO_PLAY_NONE;
    if (LDG_UNLIKELY(frame_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_audio_play.state != LDG_AUDIO_PLAY_STATE_PLAYING || !g_audio_play.cur)
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return LDG_ERR_AGAIN;
    }

    if (g_audio_play.cur->raw == 0x0 && !g_audio_play.file)
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return LDG_ERR_AGAIN;
    }

    frame_size = (uint64_t)g_audio_play.cur->channel_cunt * ((uint64_t)g_audio_play.cur->bit_depth / LDG_AUDIO_WAV_BIT_DEPTH_MIN);
    remain = g_audio_play.cur->frame_cunt - g_audio_play.frame_pos;
    want = frame_cunt;
    if (want > remain) { want = remain; }

    if (want == 0)
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return LDG_ERR_EOF;
    }

    if (g_audio_play.cur->raw != 0x0)
    {
        rd_cunt = want * frame_size;

        ret = ldg_mem_secure_cpy(buff, g_audio_play.cur->raw + g_audio_play.cur->data_off + (g_audio_play.frame_pos * frame_size), rd_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mut_unlock(&g_audio_play.mut);
            return ret;
        }
    }
    else
    {
        ret = ldg_io_file_rd(g_audio_play.file, buff, want * frame_size, &rd_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mut_unlock(&g_audio_play.mut);
            return ret;
        }
    }

    *out_cunt = (uint32_t)(rd_cunt / frame_size);
    g_audio_play.frame_pos = g_audio_play.frame_pos + *out_cunt;

    return ldg_mut_unlock(&g_audio_play.mut);
}

uint32_t ldg_audio_play_start(void)
{
    uint32_t rate = 0;
    uint32_t ret = 0;
    uint16_t channel_cunt = 0;
    uint16_t bit_depth = 0;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!g_audio_play.cur)) { return LDG_ERR_EMPTY; }

    if (g_audio_play.state == LDG_AUDIO_PLAY_STATE_PAUSED)
    {
        ret = ldg_audio_sink_pcm_pause_set(LDG_TRUTH_FALSE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        g_audio_play.state = LDG_AUDIO_PLAY_STATE_PLAYING;
        return LDG_ERR_AOK;
    }

    if (g_audio_play.state == LDG_AUDIO_PLAY_STATE_PLAYING) { return LDG_ERR_AOK; }

    ret = ldg_audio_play_fmt_get(&rate, &channel_cunt, &bit_depth);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_play.state = LDG_AUDIO_PLAY_STATE_PLAYING;

    ret = ldg_audio_sink_pcm_open(rate, channel_cunt, bit_depth);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        g_audio_play.state = LDG_AUDIO_PLAY_STATE_IDLE;
        return ret;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_pause(void)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (g_audio_play.state != LDG_AUDIO_PLAY_STATE_PLAYING) { return LDG_ERR_AOK; }

    ret = ldg_audio_sink_pcm_pause_set(LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_play.state = LDG_AUDIO_PLAY_STATE_PAUSED;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_stop(void)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (g_audio_play.state == LDG_AUDIO_PLAY_STATE_IDLE) { return LDG_ERR_AOK; }

    ret = ldg_audio_sink_pcm_close();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_play.state = LDG_AUDIO_PLAY_STATE_IDLE;
    ret = play_track_ld(g_audio_play.track_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return ret;
    }

    return ldg_mut_unlock(&g_audio_play.mut);
}

uint32_t ldg_audio_play_track_set(uint32_t track_idx)
{
    ldg_audio_play_state_t was = LDG_AUDIO_PLAY_STATE_IDLE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(track_idx >= g_audio_play.track_cunt)) { return LDG_ERR_BOUNDS; }

    was = g_audio_play.state;
    if (was != LDG_AUDIO_PLAY_STATE_IDLE)
    {
        ret = ldg_audio_sink_pcm_close();
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_play.state = LDG_AUDIO_PLAY_STATE_IDLE;
    ret = play_track_ld(track_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_audio_play.mut);
        return ret;
    }

    ret = ldg_mut_unlock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (was == LDG_AUDIO_PLAY_STATE_IDLE) { return LDG_ERR_AOK; }

    return ldg_audio_play_start();
}

uint32_t ldg_audio_play_next(void)
{
    uint32_t tgt = 0;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(g_audio_play.track_cunt == 0)) { return LDG_ERR_EMPTY; }

    tgt = g_audio_play.track_idx + LDG_AUDIO_PLAY_STEP;
    if (tgt >= g_audio_play.track_cunt)
    {
        if (g_audio_play.loop != LDG_TRUTH_TRUE) { return LDG_ERR_EOF; }

        tgt = LDG_AUDIO_PLAY_NONE;
    }

    return ldg_audio_play_track_set(tgt);
}

uint32_t ldg_audio_play_prev(void)
{
    uint32_t tgt = 0;

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(g_audio_play.track_cunt == 0)) { return LDG_ERR_EMPTY; }

    if (g_audio_play.track_idx == 0)
    {
        if (g_audio_play.loop != LDG_TRUTH_TRUE) { return LDG_ERR_BOUNDS; }

        tgt = g_audio_play.track_cunt - LDG_AUDIO_PLAY_STEP;
    }
    else { tgt = g_audio_play.track_idx - LDG_AUDIO_PLAY_STEP; }

    return ldg_audio_play_track_set(tgt);
}

uint32_t ldg_audio_play_loop_set(uint8_t loop)
{
    if (LDG_UNLIKELY(loop > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    g_audio_play.loop = loop;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_vol_set(double vol)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(vol < 0.0 || vol > LDG_AUDIO_PLAY_VOL_UNITY)) { return LDG_ERR_FUNC_ARG_INVALID; }

    g_audio_play.vol = vol;

    if (g_audio_play.state == LDG_AUDIO_PLAY_STATE_IDLE) { return LDG_ERR_AOK; }

    ret = ldg_audio_sink_pcm_vol_set(vol);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_vol_get(double *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = g_audio_play.vol;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_play_status_get(ldg_audio_play_status_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_audio_play_status_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(g_audio_play.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->state = g_audio_play.state;
    out->track_idx = g_audio_play.track_idx;
    out->track_cunt = g_audio_play.track_cunt;
    out->frame_pos = g_audio_play.frame_pos;

    if (g_audio_play.cur != 0x0)
    {
        out->frame_cunt = g_audio_play.cur->frame_cunt;
        out->rate = g_audio_play.cur->rate;
        out->channel_cunt = g_audio_play.cur->channel_cunt;
        out->bit_depth = g_audio_play.cur->bit_depth;
        ret = ldg_str_rbrcpy(out->path, g_audio_play.cur->path, LDG_AUDIO_PLAY_PATH_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mut_unlock(&g_audio_play.mut);
            return ret;
        }
    }

    return ldg_mut_unlock(&g_audio_play.mut);
}

uint32_t ldg_audio_play_close(void)
{
    uint32_t ret = 0;

    if (g_audio_play.is_init != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (g_audio_play.state != LDG_AUDIO_PLAY_STATE_IDLE)
    {
        ret = ldg_audio_sink_pcm_close();
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mut_lock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_audio_play.file != 0x0)
    {
        ldg_io_file_close(g_audio_play.file);
        g_audio_play.file = 0x0;
    }

    g_audio_play.cur = 0x0;
    g_audio_play.track_cunt = LDG_AUDIO_PLAY_NONE;
    g_audio_play.track_idx = LDG_AUDIO_PLAY_NONE;
    g_audio_play.frame_pos = LDG_AUDIO_PLAY_NONE;
    g_audio_play.state = LDG_AUDIO_PLAY_STATE_IDLE;

    ret = ldg_mut_unlock(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mut_destroy(&g_audio_play.mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    g_audio_play.is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
