#define COBJMACROS

#include <string.h>
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.media.control.h>

#define LDG_SYS_MB_NO_FLAG 0
#define LDG_SYS_MEDIA_SPIN_MAX 400
#define LDG_SYS_MEDIA_SPIN_MS 5
#define LDG_SYS_MEDIA_NONE 0
#define LDG_SYS_ASYNC_STARTED 0
#define LDG_SYS_ASYNC_COMPLETED 1

#include <dangling/sys/info.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

const IID IID_IAsyncInfo = { 0x00000036, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
const IID IID___x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManagerStatics = { 0x2050C4EE, 0x11A0, 0x57DE, { 0xAE, 0xD7, 0xC9, 0x7C, 0x70, 0x33, 0x82, 0x45 } };

typedef __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManagerStatics ldg_gsmtc_statics_t;
typedef __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager ldg_gsmtc_mngr_t;
typedef __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession ldg_gsmtc_sesh_t;
typedef __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties ldg_gsmtc_props_t;
typedef __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionPlaybackInfo ldg_gsmtc_playback_t;
typedef __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager ldg_gsmtc_mngr_op_t;
typedef __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionMediaProperties ldg_gsmtc_props_op_t;
typedef __FIAsyncOperation_1_boolean ldg_gsmtc_bool_op_t;

static uint32_t sys_media_apartment_enter(uint8_t *out_owned)
{
    HRESULT hr = 0;

    *out_owned = LDG_TRUTH_FALSE;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    if (hr == RPC_E_CHANGED_MODE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(FAILED(hr))) { return LDG_ERR_NOT_INIT; }

    *out_owned = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t sys_media_await(IUnknown *op)
{
    IAsyncInfo *info = 0x0;
    AsyncStatus status = LDG_SYS_ASYNC_STARTED;
    uint32_t spin = 0;

    if (LDG_UNLIKELY(FAILED(IUnknown_QueryInterface(op, &IID_IAsyncInfo, (void **)&info)))) { return LDG_ERR_NOT_INIT; }

    for (spin = 0; spin < LDG_SYS_MEDIA_SPIN_MAX; spin++)
    {
        if (LDG_UNLIKELY(FAILED(IAsyncInfo_get_Status(info, &status)))) { break; }

        if ((int32_t)status != LDG_SYS_ASYNC_STARTED) { break; }

        Sleep(LDG_SYS_MEDIA_SPIN_MS);
    }

    IAsyncInfo_Release(info);

    if ((int32_t)status != LDG_SYS_ASYNC_COMPLETED) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_AOK;
}

static uint32_t sys_hstring_copy(HSTRING raw_str, char *out, uint64_t out_size)
{
    const WCHAR *raw = 0x0;
    UINT32 len = 0;

    out[0] = LDG_STR_TERM;
    if (!raw_str) { return LDG_ERR_AOK; }

    raw = WindowsGetStringRawBuffer(raw_str, &len);
    if (!raw || len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(WideCharToMultiByte(CP_UTF8, LDG_SYS_MB_NO_FLAG, raw, (int32_t)len, out, (int32_t)(out_size - LDG_STR_TERM_SIZE), 0x0, 0x0) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    return LDG_ERR_AOK;
}

static uint32_t sys_media_sesh_get(ldg_gsmtc_sesh_t **out)
{
    HSTRING cls = 0x0;
    ldg_gsmtc_statics_t *statics = 0x0;
    ldg_gsmtc_mngr_op_t *op = 0x0;
    ldg_gsmtc_mngr_t *mngr = 0x0;
    uint32_t ret = 0;

    *out = 0x0;

    if (LDG_UNLIKELY(FAILED(WindowsCreateString(RuntimeClass_Windows_Media_Control_GlobalSystemMediaTransportControlsSessionManager, (UINT32)wcslen(RuntimeClass_Windows_Media_Control_GlobalSystemMediaTransportControlsSessionManager), &cls)))) { return LDG_ERR_NOT_INIT; }

    ret = LDG_ERR_UNSUPPORTED;
    if (SUCCEEDED(RoGetActivationFactory(cls, &IID___x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManagerStatics, (void **)&statics))) { ret = LDG_ERR_AOK; }

    WindowsDeleteString(cls);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManagerStatics_RequestAsync(statics, &op))) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManagerStatics_Release(statics);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_await((IUnknown *)op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_Release(op);
        return ret;
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_GetResults(op, &mngr))) { ret = LDG_ERR_AOK; }

    __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_Release(op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!mngr)) { return LDG_ERR_NOT_FOUND; }

    ret = LDG_ERR_NOT_FOUND;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_GetCurrentSession(mngr, out))) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_Release(mngr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!(*out))) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t sys_media_props_rd(ldg_gsmtc_sesh_t *sesh, ldg_sys_media_info_t *out)
{
    ldg_gsmtc_props_op_t *op = 0x0;
    ldg_gsmtc_props_t *props = 0x0;
    HSTRING text = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(FAILED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TryGetMediaPropertiesAsync(sesh, &op)))) { return LDG_ERR_NET_RPC_FAIL; }

    ret = sys_media_await((IUnknown *)op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionMediaProperties_Release(op);
        return ret;
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionMediaProperties_GetResults(op, &props))) { ret = LDG_ERR_AOK; }

    __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionMediaProperties_Release(op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!props)) { return LDG_ERR_NOT_FOUND; }

    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_get_Title(props, &text)))
    {
        ret = sys_hstring_copy(text, out->title, LDG_SYS_MEDIA_TEXT_MAX);
        WindowsDeleteString(text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("sys: media title truncated"); }
    }

    text = 0x0;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_get_Artist(props, &text)))
    {
        ret = sys_hstring_copy(text, out->artist, LDG_SYS_MEDIA_TEXT_MAX);
        WindowsDeleteString(text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("sys: media artist truncated"); }
    }

    text = 0x0;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_get_AlbumTitle(props, &text)))
    {
        ret = sys_hstring_copy(text, out->album, LDG_SYS_MEDIA_TEXT_MAX);
        WindowsDeleteString(text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("sys: media album truncated"); }
    }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_Release(props);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_media_info_get(ldg_sys_media_info_t *out)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    ldg_gsmtc_playback_t *playback = 0x0;
    __x_ABI_CWindows_CMedia_CControl_CGlobalSystemMediaTransportControlsSessionPlaybackStatus status = LDG_SYS_MEDIA_NONE;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_sys_media_info_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_sesh_get(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = sys_media_props_rd(sesh, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_GetPlaybackInfo(sesh, &playback))) { if (playback != 0x0)
        {
            if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionPlaybackInfo_get_PlaybackStatus(playback, &status))) { out->status = (ldg_sys_media_status_t)status; }

            __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionPlaybackInfo_Release(playback);
        }
    }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_media_play(void)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    ldg_gsmtc_bool_op_t *op = 0x0;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_sesh_get(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TryPlayAsync(sesh, &op))) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = sys_media_await((IUnknown *)op);
    __FIAsyncOperation_1_boolean_Release(op);

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return ret;
}

uint32_t ldg_sys_media_pause(void)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    ldg_gsmtc_bool_op_t *op = 0x0;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_sesh_get(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TryPauseAsync(sesh, &op))) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = sys_media_await((IUnknown *)op);
    __FIAsyncOperation_1_boolean_Release(op);

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return ret;
}

uint32_t ldg_sys_media_next_skip(void)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    ldg_gsmtc_bool_op_t *op = 0x0;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_sesh_get(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TrySkipNextAsync(sesh, &op))) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = sys_media_await((IUnknown *)op);
    __FIAsyncOperation_1_boolean_Release(op);

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return ret;
}

uint32_t ldg_sys_media_prev_skip(void)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    ldg_gsmtc_bool_op_t *op = 0x0;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_sesh_get(&sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TrySkipPreviousAsync(sesh, &op))) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    ret = sys_media_await((IUnknown *)op);
    __FIAsyncOperation_1_boolean_Release(op);

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return ret;
}
