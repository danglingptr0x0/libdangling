#ifdef LDG_AUDIO_WASAPI

#define COBJMACROS
#define INITGUID

#include <string.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys_devpkey.h>

#include <dangling/audio/audio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/str/str.h>

#define LDG_AUDIO_DUCK_STACK_MAX 16
#define LDG_AUDIO_MAX_SESSIONS 256
#define LDG_AUDIO_MAX_ENDPOINTS 32

typedef struct ldg_audio_session
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char app_name[LDG_AUDIO_NAME_MAX];
    uint8_t pudding0[4];
    double vol;
    uint8_t muted;
    uint8_t pudding1[7];
    ISimpleAudioVolume *vol_ctrl;
    IAudioSessionControl2 *sess_ctrl;
} ldg_audio_session_t;

typedef struct ldg_audio_endpoint
{
    uint32_t id;
    char name[LDG_AUDIO_NAME_MAX];
    char desc[LDG_AUDIO_DESC_MAX];
    uint8_t pudding0[4];
    double vol;
    uint8_t muted;
    uint8_t is_default;
    uint8_t pudding1[6];
    IMMDevice *device;
    IAudioEndpointVolume *ep_vol;
} ldg_audio_endpoint_t;

typedef struct ldg_audio_duck_entry
{
    double factor;
    uint32_t exclude_stream_id;
    uint8_t pudding0[4];
    uint32_t *stream_ids;
    double *orig_vols;
    uint32_t stream_cunt;
    uint8_t pudding1[4];
} ldg_audio_duck_entry_t;

typedef struct ldg_audio_ctx
{
    IMMDeviceEnumerator *enumerator;
    ldg_audio_endpoint_t sinks[LDG_AUDIO_MAX_ENDPOINTS];
    ldg_audio_endpoint_t srcs[LDG_AUDIO_MAX_ENDPOINTS];
    ldg_audio_session_t sessions[LDG_AUDIO_MAX_SESSIONS];
    uint32_t sink_cunt;
    uint32_t src_cunt;
    uint32_t session_cunt;
    uint32_t self_stream_id;
    uint32_t next_id;
    uint8_t pudding0[4];
    ldg_audio_duck_entry_t duck_stack[LDG_AUDIO_DUCK_STACK_MAX];
    uint32_t duck_cunt;
    uint8_t is_init;
    uint8_t pudding1[3];
} ldg_audio_ctx_t;

// singleton audio backend; file-scope static required for COM callbacks
static ldg_audio_ctx_t g_audio_ctx = { 0 };

static void audio_wstr_to_utf8(const WCHAR *src, char *dst, uint64_t dst_size)
{
    int32_t src_len = 0;

    if (LDG_UNLIKELY(!src || !dst || dst_size == 0)) { return; }

    src_len = (int32_t)lstrlenW(src) + 1;
    WideCharToMultiByte(CP_UTF8, 0, src, (int)src_len, dst, (int)(dst_size - LDG_STR_TERM_SIZE), 0x0, 0x0);
    dst[dst_size - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
}

static void audio_endpoint_name_get(IMMDevice *device, char *name, uint64_t name_size, char *desc, uint64_t desc_size)
{
    IPropertyStore *props = 0x0;
    PROPVARIANT pv = LDG_STRUCT_ZERO_INIT;
    HRESULT hr = 0;

    PropVariantInit(&pv);

    hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &props);
    if (LDG_UNLIKELY(FAILED(hr))) { return; }

    hr = IPropertyStore_GetValue(props, &PKEY_Device_FriendlyName, &pv);
    if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR && pv.pwszVal)
    {
        if (desc) { audio_wstr_to_utf8(pv.pwszVal, desc, desc_size); }

        if (name) { audio_wstr_to_utf8(pv.pwszVal, name, name_size); }
    }

    PropVariantClear(&pv);

    hr = IPropertyStore_GetValue(props, &PKEY_DeviceInterface_FriendlyName, &pv);
    if (SUCCEEDED(hr) && pv.vt == VT_LPWSTR && pv.pwszVal) { if (name) { audio_wstr_to_utf8(pv.pwszVal, name, name_size); } }

    PropVariantClear(&pv);

    IPropertyStore_Release(props);
}

static void audio_endpoints_enumerate(EDataFlow flow, ldg_audio_endpoint_t *eps, uint32_t *cunt, IMMDevice *default_dev)
{
    IMMDeviceCollection *coll = 0x0;
    IMMDevice *dev = 0x0;
    UINT dev_cunt = 0;
    UINT i = 0;
    HRESULT hr = 0;
    LPWSTR default_id = 0x0;
    LPWSTR dev_id = 0x0;

    *cunt = 0;

    if (default_dev) { IMMDevice_GetId(default_dev, &default_id); }

    hr = IMMDeviceEnumerator_EnumAudioEndpoints(g_audio_ctx.enumerator, flow, DEVICE_STATE_ACTIVE, &coll);
    if (LDG_UNLIKELY(FAILED(hr)))
    {
        if (default_id) { CoTaskMemFree(default_id); }

        return;
    }

    IMMDeviceCollection_GetCount(coll, &dev_cunt);

    for (i = 0; i < dev_cunt && *cunt < LDG_AUDIO_MAX_ENDPOINTS; i++)
    {
        hr = IMMDeviceCollection_Item(coll, i, &dev);
        if (LDG_UNLIKELY(FAILED(hr))) { continue; }

        eps[*cunt].id = g_audio_ctx.next_id++;
        eps[*cunt].device = dev;
        eps[*cunt].is_default = 0;

        audio_endpoint_name_get(dev, eps[*cunt].name, LDG_AUDIO_NAME_MAX, eps[*cunt].desc, LDG_AUDIO_DESC_MAX);

        hr = IMMDevice_GetId(dev, &dev_id);
        if (SUCCEEDED(hr) && default_id && dev_id)
        {
            if (lstrcmpW(dev_id, default_id) == 0) { eps[*cunt].is_default = 1; }

            CoTaskMemFree(dev_id);
        }

        hr = IMMDevice_Activate(dev, &IID_IAudioEndpointVolume, CLSCTX_ALL, 0x0, (void **)&eps[*cunt].ep_vol);
        if (SUCCEEDED(hr) && eps[*cunt].ep_vol)
        {
            float com_vol = 0.0f;
            BOOL muted = FALSE;
            IAudioEndpointVolume_GetMasterVolumeLevelScalar(eps[*cunt].ep_vol, &com_vol);
            IAudioEndpointVolume_GetMute(eps[*cunt].ep_vol, &muted);
            eps[*cunt].vol = (double)com_vol;
            eps[*cunt].muted = muted ? 1 : 0;
        }

        (*cunt)++;
    }

    if (default_id) { CoTaskMemFree(default_id); }

    IMMDeviceCollection_Release(coll);
}

static void audio_sessions_enumerate(void)
{
    IAudioSessionManager2 *mgr = 0x0;
    IAudioSessionEnumerator *sess_enum = 0x0;
    IAudioSessionControl *ctrl = 0x0;
    IAudioSessionControl2 *ctrl2 = 0x0;
    ISimpleAudioVolume *vol = 0x0;
    int32_t sess_cunt = 0;
    int32_t i = 0;
    HRESULT hr = 0;
    LPWSTR disp_name = 0x0;
    uint32_t idx = 0;

    g_audio_ctx.session_cunt = 0;

    if (g_audio_ctx.sink_cunt == 0) { return; }

    idx = 0;
    while (idx < g_audio_ctx.sink_cunt)
    {
        if (g_audio_ctx.sinks[idx].is_default) { break; }

        idx++;
    }
    if (idx >= g_audio_ctx.sink_cunt) { idx = 0; }

    hr = IMMDevice_Activate(g_audio_ctx.sinks[idx].device, &IID_IAudioSessionManager2, CLSCTX_ALL, 0x0, (void **)&mgr);
    if (LDG_UNLIKELY(FAILED(hr))) { return; }

    hr = IAudioSessionManager2_GetSessionEnumerator(mgr, &sess_enum);
    if (LDG_UNLIKELY(FAILED(hr))) { IAudioSessionManager2_Release(mgr); return; }

    {
        int com_sess_cunt = 0;

        IAudioSessionEnumerator_GetCount(sess_enum, &com_sess_cunt);
        sess_cunt = (int32_t)com_sess_cunt;
    }

    for (i = 0; i < sess_cunt && g_audio_ctx.session_cunt < LDG_AUDIO_MAX_SESSIONS; i++)
    {
        AudioSessionState state = AudioSessionStateInactive;
        ldg_audio_session_t *s = 0x0;
        DWORD pid = 0;
        HANDLE proc = 0x0;
        char path[MAX_PATH] = { 0 };
        DWORD path_size = MAX_PATH;
        char *slash = 0x0;
        float fvol = 0.0f;
        BOOL fmuted = FALSE;

        hr = IAudioSessionEnumerator_GetSession(sess_enum, (int)i, &ctrl);
        if (LDG_UNLIKELY(FAILED(hr))) { continue; }

        IAudioSessionControl_GetState(ctrl, &state);
        if (state == AudioSessionStateExpired) { IAudioSessionControl_Release(ctrl); continue; }

        hr = IAudioSessionControl_QueryInterface(ctrl, &IID_IAudioSessionControl2, (void **)&ctrl2);
        if (LDG_UNLIKELY(FAILED(hr))) { IAudioSessionControl_Release(ctrl); continue; }

        hr = IAudioSessionControl_QueryInterface(ctrl, &IID_ISimpleAudioVolume, (void **)&vol);
        if (LDG_UNLIKELY(FAILED(hr))) { IAudioSessionControl2_Release(ctrl2); IAudioSessionControl_Release(ctrl); continue; }

        s = &g_audio_ctx.sessions[g_audio_ctx.session_cunt];
        if (LDG_UNLIKELY(memset(s, 0, sizeof(ldg_audio_session_t)) != s)) { continue; }

        s->id = g_audio_ctx.next_id++;
        s->sess_ctrl = ctrl2;
        s->vol_ctrl = vol;

        hr = IAudioSessionControl_GetDisplayName(ctrl, &disp_name);
        if (SUCCEEDED(hr) && disp_name)
        {
            audio_wstr_to_utf8(disp_name, s->name, LDG_AUDIO_NAME_MAX);
            audio_wstr_to_utf8(disp_name, s->app_name, LDG_AUDIO_NAME_MAX);
            CoTaskMemFree(disp_name);
        }

        if (s->name[0] == LDG_STR_TERM)
        {
            pid = 0;
            IAudioSessionControl2_GetProcessId(ctrl2, &pid);
            proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (proc)
            {
                if (LDG_UNLIKELY(memset(path, 0, MAX_PATH) != path)) { CloseHandle(proc); continue; }

                path_size = MAX_PATH;
                if (QueryFullProcessImageNameA(proc, 0, path, &path_size))
                {
                    slash = strrchr(path, '\\');
                    if (slash) { ldg_strrbrcpy(s->name, slash + 1, LDG_AUDIO_NAME_MAX); ldg_strrbrcpy(s->app_name, slash + 1, LDG_AUDIO_NAME_MAX); }
                    else { ldg_strrbrcpy(s->name, path, LDG_AUDIO_NAME_MAX); ldg_strrbrcpy(s->app_name, path, LDG_AUDIO_NAME_MAX); }
                }

                CloseHandle(proc);
            }
        }

        fvol = 0.0f;
        fmuted = FALSE;
        ISimpleAudioVolume_GetMasterVolume(vol, &fvol);
        ISimpleAudioVolume_GetMute(vol, &fmuted);
        s->vol = (double)fvol;
        s->muted = fmuted ? 1 : 0;

        IAudioSessionControl_Release(ctrl);
        g_audio_ctx.session_cunt++;
    }

    IAudioSessionEnumerator_Release(sess_enum);
    IAudioSessionManager2_Release(mgr);
}

static uint32_t audio_session_find(uint32_t id, ldg_audio_session_t **out)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (i = 0; i < g_audio_ctx.session_cunt; i++) { if (g_audio_ctx.sessions[i].id == id) { *out = &g_audio_ctx.sessions[i]; return LDG_ERR_AOK; } }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t audio_default_sink_find(ldg_audio_endpoint_t **out)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (i = 0; i < g_audio_ctx.sink_cunt; i++) { if (g_audio_ctx.sinks[i].is_default) { *out = &g_audio_ctx.sinks[i]; return LDG_ERR_AOK; } }

    if (g_audio_ctx.sink_cunt > 0) { *out = &g_audio_ctx.sinks[0]; return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t audio_default_src_find(ldg_audio_endpoint_t **out)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    for (i = 0; i < g_audio_ctx.src_cunt; i++) { if (g_audio_ctx.srcs[i].is_default) { *out = &g_audio_ctx.srcs[i]; return LDG_ERR_AOK; } }

    if (g_audio_ctx.src_cunt > 0) { *out = &g_audio_ctx.srcs[0]; return LDG_ERR_AOK; }

    return LDG_ERR_NOT_FOUND;
}

static void audio_sessions_release(void)
{
    uint32_t i = 0;

    for (i = 0; i < g_audio_ctx.session_cunt; i++)
    {
        if (g_audio_ctx.sessions[i].vol_ctrl) { ISimpleAudioVolume_Release(g_audio_ctx.sessions[i].vol_ctrl); }

        if (g_audio_ctx.sessions[i].sess_ctrl) { IAudioSessionControl2_Release(g_audio_ctx.sessions[i].sess_ctrl); }
    }

    g_audio_ctx.session_cunt = 0;
}

static void audio_endpoints_release(ldg_audio_endpoint_t *eps, uint32_t cunt)
{
    uint32_t i = 0;

    for (i = 0; i < cunt; i++)
    {
        if (eps[i].ep_vol) { IAudioEndpointVolume_Release(eps[i].ep_vol); eps[i].ep_vol = 0x0; }

        if (eps[i].device) { IMMDevice_Release(eps[i].device); eps[i].device = 0x0; }
    }
}

uint32_t ldg_audio_init(void)
{
    HRESULT hr = 0;
    IMMDevice *default_render = 0x0;
    IMMDevice *default_capture = 0x0;

    if (g_audio_ctx.is_init) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(memset(&g_audio_ctx, 0, sizeof(ldg_audio_ctx_t)) != &g_audio_ctx)) { return LDG_ERR_MEM_BAD; }

    g_audio_ctx.next_id = 1;

    hr = CoInitializeEx(0x0, COINIT_MULTITHREADED);
    if (LDG_UNLIKELY(FAILED(hr) && hr != RPC_E_CHANGED_MODE)) { return LDG_ERR_AUDIO_INIT; }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0x0, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **)&g_audio_ctx.enumerator);
    if (LDG_UNLIKELY(FAILED(hr))) { return LDG_ERR_AUDIO_INIT; }

    IMMDeviceEnumerator_GetDefaultAudioEndpoint(g_audio_ctx.enumerator, eRender, eConsole, &default_render);
    IMMDeviceEnumerator_GetDefaultAudioEndpoint(g_audio_ctx.enumerator, eCapture, eConsole, &default_capture);

    audio_endpoints_enumerate(eRender, g_audio_ctx.sinks, &g_audio_ctx.sink_cunt, default_render);
    audio_endpoints_enumerate(eCapture, g_audio_ctx.srcs, &g_audio_ctx.src_cunt, default_capture);

    if (default_render) { IMMDevice_Release(default_render); }

    if (default_capture) { IMMDevice_Release(default_capture); }

    audio_sessions_enumerate();

    g_audio_ctx.is_init = 1;

    return LDG_ERR_AOK;
}

void ldg_audio_shutdown(void)
{
    uint32_t i = 0;

    if (!g_audio_ctx.is_init) { return; }

    g_audio_ctx.is_init = 0;

    for (i = 0; i < LDG_AUDIO_DUCK_STACK_MAX; i++)
    {
        if (g_audio_ctx.duck_stack[i].stream_ids) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].stream_ids); }

        if (g_audio_ctx.duck_stack[i].orig_vols) { ldg_mem_dealloc(g_audio_ctx.duck_stack[i].orig_vols); }
    }

    audio_sessions_release();
    audio_endpoints_release(g_audio_ctx.sinks, g_audio_ctx.sink_cunt);
    audio_endpoints_release(g_audio_ctx.srcs, g_audio_ctx.src_cunt);

    if (g_audio_ctx.enumerator) { IMMDeviceEnumerator_Release(g_audio_ctx.enumerator); }

    CoUninitialize();
}

uint32_t ldg_audio_sync(void)
{
    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    audio_sessions_release();
    audio_sessions_enumerate();

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_master_vol_get(double *vol)
{
    ldg_audio_endpoint_t *sink = 0x0;
    float fvol = 0.0f;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (audio_default_sink_find(&sink) != LDG_ERR_AOK || !sink->ep_vol) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    IAudioEndpointVolume_GetMasterVolumeLevelScalar(sink->ep_vol, &fvol);
    *vol = (double)fvol;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_master_vol_set(double vol)
{
    ldg_audio_endpoint_t *sink = 0x0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOL_RANGE; }

    if (audio_default_sink_find(&sink) != LDG_ERR_AOK || !sink->ep_vol) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    IAudioEndpointVolume_SetMasterVolumeLevelScalar(sink->ep_vol, (float)vol, 0x0);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_vol_get(uint32_t stream_id, double *vol)
{
    ldg_audio_session_t *s = 0x0;
    float fvol = 0.0f;

    if (LDG_UNLIKELY(!vol)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (audio_session_find(stream_id, &s) != LDG_ERR_AOK || !s->vol_ctrl) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    ISimpleAudioVolume_GetMasterVolume(s->vol_ctrl, &fvol);
    *vol = (double)fvol;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_vol_set(uint32_t stream_id, double vol)
{
    ldg_audio_session_t *s = 0x0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(vol < 0.0 || vol > 1.0)) { return LDG_ERR_AUDIO_VOL_RANGE; }

    if (audio_session_find(stream_id, &s) != LDG_ERR_AOK || !s->vol_ctrl) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    ISimpleAudioVolume_SetMasterVolume(s->vol_ctrl, (float)vol, 0x0);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_mute_get(uint32_t stream_id, uint8_t *muted)
{
    ldg_audio_session_t *s = 0x0;
    ldg_audio_endpoint_t *sink = 0x0;
    BOOL m = FALSE;

    if (LDG_UNLIKELY(!muted)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (stream_id == 0)
    {
        if (audio_default_sink_find(&sink) != LDG_ERR_AOK || !sink->ep_vol) { return LDG_ERR_AUDIO_NO_DEFAULT; }

        IAudioEndpointVolume_GetMute(sink->ep_vol, &m);
        *muted = m ? 1 : 0;
        return LDG_ERR_AOK;
    }

    if (audio_session_find(stream_id, &s) != LDG_ERR_AOK || !s->vol_ctrl) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    ISimpleAudioVolume_GetMute(s->vol_ctrl, &m);
    *muted = m ? 1 : 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_mute_set(uint32_t stream_id, uint8_t muted)
{
    ldg_audio_session_t *s = 0x0;
    ldg_audio_endpoint_t *sink = 0x0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (stream_id == 0)
    {
        if (audio_default_sink_find(&sink) != LDG_ERR_AOK || !sink->ep_vol) { return LDG_ERR_AUDIO_NO_DEFAULT; }

        IAudioEndpointVolume_SetMute(sink->ep_vol, muted ? TRUE : FALSE, 0x0);
        return LDG_ERR_AOK;
    }

    if (audio_session_find(stream_id, &s) != LDG_ERR_AOK || !s->vol_ctrl) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    ISimpleAudioVolume_SetMute(s->vol_ctrl, muted ? TRUE : FALSE, 0x0);

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_stream_list(ldg_audio_stream_t **streams, uint32_t *cunt)
{
    ldg_audio_stream_t *arr = 0x0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!streams || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (g_audio_ctx.session_cunt == 0)
    {
        *streams = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    if (LDG_UNLIKELY(ldg_mem_alloc(g_audio_ctx.session_cunt * sizeof(ldg_audio_stream_t), (void **)&arr) != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(arr, 0, g_audio_ctx.session_cunt * sizeof(ldg_audio_stream_t)) != arr)) { return LDG_ERR_MEM_BAD; }

    for (i = 0; i < g_audio_ctx.session_cunt; i++)
    {
        arr[i].id = g_audio_ctx.sessions[i].id;
        ldg_strrbrcpy(arr[i].name, g_audio_ctx.sessions[i].name, LDG_AUDIO_NAME_MAX);
        ldg_strrbrcpy(arr[i].app_name, g_audio_ctx.sessions[i].app_name, LDG_AUDIO_NAME_MAX);
        arr[i].vol = g_audio_ctx.sessions[i].vol;
        arr[i].muted = g_audio_ctx.sessions[i].muted;
        arr[i].sink_id = 0;
    }

    *streams = arr;
    *cunt = g_audio_ctx.session_cunt;

    return LDG_ERR_AOK;
}

void ldg_audio_stream_free(ldg_audio_stream_t *streams)
{
    if (LDG_UNLIKELY(!streams)) { return; }

    ldg_mem_dealloc(streams);
}

uint32_t ldg_audio_stream_name_get(const char *name, ldg_audio_stream_t *stream)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!name || !stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    for (i = 0; i < g_audio_ctx.session_cunt; i++)
    {
        if (strcmp(g_audio_ctx.sessions[i].name, name) == 0)
        {
            if (LDG_UNLIKELY(memset(stream, 0, sizeof(ldg_audio_stream_t)) != stream)) { return LDG_ERR_MEM_BAD; }

            stream->id = g_audio_ctx.sessions[i].id;
            ldg_strrbrcpy(stream->name, g_audio_ctx.sessions[i].name, LDG_AUDIO_NAME_MAX);
            ldg_strrbrcpy(stream->app_name, g_audio_ctx.sessions[i].app_name, LDG_AUDIO_NAME_MAX);
            stream->vol = g_audio_ctx.sessions[i].vol;
            stream->muted = g_audio_ctx.sessions[i].muted;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_AUDIO_STREAM_NOT_FOUND;
}

void ldg_audio_self_register(uint32_t stream_id)
{
    g_audio_ctx.self_stream_id = stream_id;
}

uint64_t ldg_audio_self_id_get(void)
{
    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return UINT64_MAX; }

    return (uint64_t)g_audio_ctx.self_stream_id;
}

uint32_t ldg_audio_stream_self_get(ldg_audio_stream_t *stream)
{
    ldg_audio_session_t *s = 0x0;

    if (LDG_UNLIKELY(!stream)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (LDG_UNLIKELY(g_audio_ctx.self_stream_id == 0)) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    if (audio_session_find(g_audio_ctx.self_stream_id, &s) != LDG_ERR_AOK) { return LDG_ERR_AUDIO_STREAM_NOT_FOUND; }

    if (LDG_UNLIKELY(memset(stream, 0, sizeof(ldg_audio_stream_t)) != stream)) { return LDG_ERR_MEM_BAD; }

    stream->id = s->id;
    ldg_strrbrcpy(stream->name, s->name, LDG_AUDIO_NAME_MAX);
    ldg_strrbrcpy(stream->app_name, s->app_name, LDG_AUDIO_NAME_MAX);
    stream->vol = s->vol;
    stream->muted = s->muted;

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
    ldg_audio_sink_t *arr = 0x0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sinks || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (g_audio_ctx.sink_cunt == 0)
    {
        *sinks = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    if (LDG_UNLIKELY(ldg_mem_alloc(g_audio_ctx.sink_cunt * sizeof(ldg_audio_sink_t), (void **)&arr) != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(arr, 0, g_audio_ctx.sink_cunt * sizeof(ldg_audio_sink_t)) != arr)) { return LDG_ERR_MEM_BAD; }

    for (i = 0; i < g_audio_ctx.sink_cunt; i++)
    {
        arr[i].id = g_audio_ctx.sinks[i].id;
        ldg_strrbrcpy(arr[i].name, g_audio_ctx.sinks[i].name, LDG_AUDIO_NAME_MAX);
        ldg_strrbrcpy(arr[i].desc, g_audio_ctx.sinks[i].desc, LDG_AUDIO_DESC_MAX);
        arr[i].vol = g_audio_ctx.sinks[i].vol;
        arr[i].muted = g_audio_ctx.sinks[i].muted;
        arr[i].is_default = g_audio_ctx.sinks[i].is_default;
    }

    *sinks = arr;
    *cunt = g_audio_ctx.sink_cunt;

    return LDG_ERR_AOK;
}

void ldg_audio_sink_free(ldg_audio_sink_t *sinks)
{
    if (LDG_UNLIKELY(!sinks)) { return; }

    ldg_mem_dealloc(sinks);
}

uint32_t ldg_audio_src_list(ldg_audio_src_t **srcs, uint32_t *cunt)
{
    ldg_audio_src_t *arr = 0x0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!srcs || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (g_audio_ctx.src_cunt == 0)
    {
        *srcs = 0x0;
        *cunt = 0;
        return LDG_ERR_EMPTY;
    }

    if (LDG_UNLIKELY(ldg_mem_alloc(g_audio_ctx.src_cunt * sizeof(ldg_audio_src_t), (void **)&arr) != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    if (LDG_UNLIKELY(memset(arr, 0, g_audio_ctx.src_cunt * sizeof(ldg_audio_src_t)) != arr)) { return LDG_ERR_MEM_BAD; }

    for (i = 0; i < g_audio_ctx.src_cunt; i++)
    {
        arr[i].id = g_audio_ctx.srcs[i].id;
        ldg_strrbrcpy(arr[i].name, g_audio_ctx.srcs[i].name, LDG_AUDIO_NAME_MAX);
        ldg_strrbrcpy(arr[i].desc, g_audio_ctx.srcs[i].desc, LDG_AUDIO_DESC_MAX);
        arr[i].vol = g_audio_ctx.srcs[i].vol;
        arr[i].muted = g_audio_ctx.srcs[i].muted;
        arr[i].is_default = g_audio_ctx.srcs[i].is_default;
    }

    *srcs = arr;
    *cunt = g_audio_ctx.src_cunt;

    return LDG_ERR_AOK;
}

void ldg_audio_src_free(ldg_audio_src_t *srcs)
{
    if (LDG_UNLIKELY(!srcs)) { return; }

    ldg_mem_dealloc(srcs);
}

uint32_t ldg_audio_default_sink_get(ldg_audio_sink_t *sink)
{
    ldg_audio_endpoint_t *ep = 0x0;

    if (LDG_UNLIKELY(!sink)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (audio_default_sink_find(&ep) != LDG_ERR_AOK) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (LDG_UNLIKELY(memset(sink, 0, sizeof(ldg_audio_sink_t)) != sink)) { return LDG_ERR_MEM_BAD; }

    sink->id = ep->id;
    ldg_strrbrcpy(sink->name, ep->name, LDG_AUDIO_NAME_MAX);
    ldg_strrbrcpy(sink->desc, ep->desc, LDG_AUDIO_DESC_MAX);
    sink->vol = ep->vol;
    sink->muted = ep->muted;
    sink->is_default = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_default_src_get(ldg_audio_src_t *src)
{
    ldg_audio_endpoint_t *ep = 0x0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (audio_default_src_find(&ep) != LDG_ERR_AOK) { return LDG_ERR_AUDIO_NO_DEFAULT; }

    if (LDG_UNLIKELY(memset(src, 0, sizeof(ldg_audio_src_t)) != src)) { return LDG_ERR_MEM_BAD; }

    src->id = ep->id;
    ldg_strrbrcpy(src->name, ep->name, LDG_AUDIO_NAME_MAX);
    ldg_strrbrcpy(src->desc, ep->desc, LDG_AUDIO_DESC_MAX);
    src->vol = ep->vol;
    src->muted = ep->muted;
    src->is_default = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_duck(double factor, uint32_t exclude_stream_id)
{
    ldg_audio_duck_entry_t *entry = 0x0;
    uint32_t cunt = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    if (LDG_UNLIKELY(factor < 0.0 || factor > 1.0)) { return LDG_ERR_AUDIO_VOL_RANGE; }

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (g_audio_ctx.duck_cunt >= LDG_AUDIO_DUCK_STACK_MAX) { return LDG_ERR_AUDIO_DUCK_FULL; }

    for (i = 0; i < g_audio_ctx.session_cunt; i++) { if (g_audio_ctx.sessions[i].id != exclude_stream_id) { cunt++; } }

    entry = &g_audio_ctx.duck_stack[g_audio_ctx.duck_cunt];
    if (LDG_UNLIKELY(memset(entry, 0, sizeof(ldg_audio_duck_entry_t)) != entry)) { return LDG_ERR_MEM_BAD; }

    entry->factor = factor;
    entry->exclude_stream_id = exclude_stream_id;
    entry->stream_cunt = cunt;

    if (cunt > 0)
    {
        ldg_mem_alloc(cunt * sizeof(uint32_t), (void **)&entry->stream_ids);
        ldg_mem_alloc(cunt * sizeof(double), (void **)&entry->orig_vols);

        if (!entry->stream_ids || !entry->orig_vols)
        {
            if (entry->stream_ids) { ldg_mem_dealloc(entry->stream_ids); }

            if (entry->orig_vols) { ldg_mem_dealloc(entry->orig_vols); }

            if (LDG_UNLIKELY(memset(entry, 0, sizeof(ldg_audio_duck_entry_t)) != entry)) { return LDG_ERR_MEM_BAD; }

            return LDG_ERR_ALLOC_NULL;
        }

        j = 0;
        for (i = 0; i < g_audio_ctx.session_cunt && j < cunt; i++) { if (g_audio_ctx.sessions[i].id != exclude_stream_id)
            {
                entry->stream_ids[j] = g_audio_ctx.sessions[i].id;
                entry->orig_vols[j] = g_audio_ctx.sessions[i].vol;
                if (g_audio_ctx.sessions[i].vol_ctrl) { ISimpleAudioVolume_SetMasterVolume(g_audio_ctx.sessions[i].vol_ctrl, (float)(g_audio_ctx.sessions[i].vol * factor), 0x0); }

                j++;
            }
        }
    }

    g_audio_ctx.duck_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_audio_unduck(void)
{
    ldg_audio_duck_entry_t *entry = 0x0;
    ldg_audio_session_t *s = 0x0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!g_audio_ctx.is_init)) { return LDG_ERR_AUDIO_NOT_INIT; }

    if (g_audio_ctx.duck_cunt == 0) { return LDG_ERR_AUDIO_DUCK_EMPTY; }

    g_audio_ctx.duck_cunt--;
    entry = &g_audio_ctx.duck_stack[g_audio_ctx.duck_cunt];

    for (i = 0; i < entry->stream_cunt; i++) { if (audio_session_find(entry->stream_ids[i], &s) == LDG_ERR_AOK && s->vol_ctrl) { ISimpleAudioVolume_SetMasterVolume(s->vol_ctrl, (float)entry->orig_vols[i], 0x0); } }

    if (entry->stream_ids) { ldg_mem_dealloc(entry->stream_ids); }

    if (entry->orig_vols) { ldg_mem_dealloc(entry->orig_vols); }

    if (LDG_UNLIKELY(memset(entry, 0, sizeof(ldg_audio_duck_entry_t)) != entry)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

#endif
