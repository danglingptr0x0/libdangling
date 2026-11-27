#define COBJMACROS

#include <string.h>
#include <windows.h>
#include <tlhelp32.h>
#include <appmodel.h>
#include <roapi.h>
#include <winstring.h>
#include <windows.media.control.h>

#define LDG_SYS_MB_NO_FLAG 0
#define LDG_SYS_MEDIA_SPIN_MAX 400
#define LDG_SYS_MEDIA_SPIN_WAIT 5
#define LDG_SYS_MEDIA_NONE 0
#define LDG_SYS_ASYNC_STARTED 0
#define LDG_SYS_ASYNC_COMPLETED 1
#define LDG_SYS_MEDIA_AUMID_CAP 256
#define LDG_SYS_MEDIA_EXE_EXT L".exe"
#define LDG_SYS_MEDIA_EXE_EXT_LEN 4
#define LDG_SYS_MEDIA_PATH_SEP L'\\'
#define LDG_SYS_MEDIA_PATH_SEP_ALT L'/'
#define LDG_SYS_MEDIA_ID_SEP L'.'
#define LDG_SYS_MEDIA_WSTR_TERM L'\0'
#define LDG_SYS_MEDIA_EPOCH_OFF 116444736000000000LL
#define LDG_SYS_MEDIA_TICK_SCALE 10000ULL
#define LDG_SYS_MEDIA_SNAPSHOT_ALL 0
#define LDG_SYS_MEDIA_STEP 1
#define LDG_SYS_MEDIA_ASCII_UPPER_MIN 'A'
#define LDG_SYS_MEDIA_ASCII_UPPER_MAX 'Z'
#define LDG_SYS_MEDIA_ASCII_CASE_OFF ('a' - 'A')

#include <dangling/sys/info.h>
#include <dangling/mem/cpy.h>
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
typedef __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionTimelineProperties ldg_gsmtc_timeline_t;
typedef __FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession ldg_gsmtc_sesh_view_t;
typedef __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager ldg_gsmtc_mngr_op_t;
typedef __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionMediaProperties ldg_gsmtc_props_op_t;
typedef __FIAsyncOperation_1_boolean ldg_gsmtc_bool_op_t;

typedef enum sys_media_op
{
    SYS_MEDIA_OP_PLAY = 0,
    SYS_MEDIA_OP_PAUSE,
    SYS_MEDIA_OP_NEXT,
    SYS_MEDIA_OP_PREV
} sys_media_op_t;

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

        Sleep(LDG_SYS_MEDIA_SPIN_WAIT);
    }

    IAsyncInfo_Release(info);

    if ((int32_t)status != LDG_SYS_ASYNC_COMPLETED) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_AOK;
}

static uint32_t sys_wstr_utf8_copy(const WCHAR *src, uint32_t len, char *out, uint64_t out_size)
{
    int32_t wrote = 0;

    out[0] = LDG_STR_TERM;
    if (!src || len == 0) { return LDG_ERR_AOK; }

    wrote = WideCharToMultiByte(CP_UTF8, LDG_SYS_MB_NO_FLAG, src, (int32_t)len, out, (int32_t)(out_size - LDG_STR_TERM_SIZE), 0x0, 0x0);
    if (LDG_UNLIKELY(wrote <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    out[wrote] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t sys_hstring_copy(HSTRING raw_str, char *out, uint64_t out_size)
{
    const WCHAR *raw = 0x0;
    UINT32 len = 0;

    out[0] = LDG_STR_TERM;
    if (!raw_str) { return LDG_ERR_AOK; }

    raw = WindowsGetStringRawBuffer(raw_str, &len);

    return sys_wstr_utf8_copy(raw, (uint32_t)len, out, out_size);
}

static uint8_t sys_wstr_ieq_is(const WCHAR *lhs, uint32_t lhs_len, const WCHAR *rhs, uint32_t rhs_len)
{
    if (CompareStringOrdinal(lhs, (int32_t)lhs_len, rhs, (int32_t)rhs_len, TRUE) == CSTR_EQUAL) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t sys_wstr_len_get(const WCHAR *src, uint32_t cap)
{
    uint32_t len = 0;

    while (len < cap && src[len] != LDG_SYS_MEDIA_WSTR_TERM) { len++; }

    return len;
}

static void sys_wstr_cpy(WCHAR *dst, const WCHAR *src, uint32_t cap)
{
    uint32_t idx = 0;

    while (idx + LDG_STR_TERM_SIZE < cap && src[idx] != LDG_SYS_MEDIA_WSTR_TERM)
    {
        dst[idx] = src[idx];
        idx++;
    }

    dst[idx] = LDG_SYS_MEDIA_WSTR_TERM;
}

static uint8_t sys_media_exe_ext_has(const WCHAR *name, uint32_t len)
{
    if (len <= LDG_SYS_MEDIA_EXE_EXT_LEN) { return LDG_TRUTH_FALSE; }

    return sys_wstr_ieq_is(name + len - LDG_SYS_MEDIA_EXE_EXT_LEN, LDG_SYS_MEDIA_EXE_EXT_LEN, LDG_SYS_MEDIA_EXE_EXT, LDG_SYS_MEDIA_EXE_EXT_LEN);
}

static uint8_t sys_media_stem_eq_is(const WCHAR *exe, const WCHAR *name, uint32_t name_len)
{
    uint32_t exe_len = 0;

    if (name_len == 0) { return LDG_TRUTH_FALSE; }

    exe_len = sys_wstr_len_get(exe, MAX_PATH);
    if (sys_media_exe_ext_has(exe, exe_len) == LDG_TRUTH_TRUE) { exe_len -= LDG_SYS_MEDIA_EXE_EXT_LEN; }

    return sys_wstr_ieq_is(exe, exe_len, name, name_len);
}

static uint8_t sys_media_proc_aumid_is(DWORD pid, const WCHAR *aumid, uint32_t aumid_len)
{
    WCHAR proc_aumid[LDG_SYS_MEDIA_AUMID_CAP] = LDG_ARR_ZERO_INIT;
    HANDLE proc = 0x0;
    UINT32 len = LDG_SYS_MEDIA_AUMID_CAP;
    LONG code = 0;
    uint8_t res = LDG_TRUTH_FALSE;

    proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) { return LDG_TRUTH_FALSE; }

    code = GetApplicationUserModelId(proc, &len, proc_aumid);
    if (code == ERROR_SUCCESS) { res = sys_wstr_ieq_is(proc_aumid, sys_wstr_len_get(proc_aumid, LDG_SYS_MEDIA_AUMID_CAP), aumid, aumid_len); }

    if (LDG_UNLIKELY(!CloseHandle(proc))) { return LDG_TRUTH_FALSE; }

    return res;
}

static uint32_t sys_media_player_resolve(const WCHAR *aumid, uint32_t aumid_len, char *out, uint64_t out_size)
{
    PROCESSENTRY32W entry = LDG_STRUCT_ZERO_INIT;
    WCHAR exe[MAX_PATH] = LDG_ARR_ZERO_INIT;
    HANDLE snap = INVALID_HANDLE_VALUE;
    uint32_t base_at = 0;
    uint32_t tail_at = 0;
    uint32_t idx = 0;
    uint8_t found = LDG_TRUTH_FALSE;
    uint8_t stem_found = LDG_TRUTH_FALSE;
    uint8_t more = LDG_TRUTH_FALSE;

    out[0] = LDG_STR_TERM;
    if (!aumid || aumid_len == 0) { return LDG_ERR_AOK; }

    for (idx = 0; idx < aumid_len; idx++)
    {
        if (aumid[idx] == LDG_SYS_MEDIA_PATH_SEP || aumid[idx] == LDG_SYS_MEDIA_PATH_SEP_ALT) { base_at = idx + LDG_SYS_MEDIA_STEP; }

        if (aumid[idx] == LDG_SYS_MEDIA_ID_SEP) { tail_at = idx + LDG_SYS_MEDIA_STEP; }
    }

    if (sys_media_exe_ext_has(aumid + base_at, aumid_len - base_at) == LDG_TRUTH_TRUE) { return sys_wstr_utf8_copy(aumid + base_at, aumid_len - base_at, out, out_size); }

    if (tail_at < base_at) { tail_at = base_at; }

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, LDG_SYS_MEDIA_SNAPSHOT_ALL);
    if (snap == INVALID_HANDLE_VALUE) { return LDG_ERR_AOK; }

    entry.dwSize = sizeof(entry);
    more = Process32FirstW(snap, &entry) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    while (more == LDG_TRUTH_TRUE && found == LDG_TRUTH_FALSE)
    {
        if (sys_media_proc_aumid_is(entry.th32ProcessID, aumid, aumid_len) == LDG_TRUTH_TRUE)
        {
            found = LDG_TRUTH_TRUE;
            sys_wstr_cpy(exe, entry.szExeFile, MAX_PATH);
        }
        else if (stem_found == LDG_TRUTH_FALSE && (sys_media_stem_eq_is(entry.szExeFile, aumid, aumid_len) == LDG_TRUTH_TRUE || sys_media_stem_eq_is(entry.szExeFile, aumid + tail_at, aumid_len - tail_at) == LDG_TRUTH_TRUE))
        {
            stem_found = LDG_TRUTH_TRUE;
            sys_wstr_cpy(exe, entry.szExeFile, MAX_PATH);
        }

        if (found == LDG_TRUTH_FALSE) { more = Process32NextW(snap, &entry) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
    }

    if (LDG_UNLIKELY(!CloseHandle(snap))) { return LDG_ERR_IO_CLOSE; }

    if (found == LDG_TRUTH_FALSE && stem_found == LDG_TRUTH_FALSE) { return LDG_ERR_AOK; }

    return sys_wstr_utf8_copy(exe, sys_wstr_len_get(exe, MAX_PATH), out, out_size);
}

static uint32_t sys_media_mngr_get(ldg_gsmtc_mngr_t **out)
{
    HSTRING cls = 0x0;
    ldg_gsmtc_statics_t *statics = 0x0;
    ldg_gsmtc_mngr_op_t *op = 0x0;
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
    if (SUCCEEDED(__FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_GetResults(op, out))) { ret = LDG_ERR_AOK; }

    __FIAsyncOperation_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSessionManager_Release(op);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!(*out))) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t sys_media_sesh_get(ldg_gsmtc_sesh_t **out)
{
    ldg_gsmtc_mngr_t *mngr = 0x0;
    uint32_t ret = 0;

    *out = 0x0;

    ret = sys_media_mngr_get(&mngr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

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
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { out->title[0] = LDG_STR_TERM; }
    }

    text = 0x0;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_get_Artist(props, &text)))
    {
        ret = sys_hstring_copy(text, out->artist, LDG_SYS_MEDIA_TEXT_MAX);
        WindowsDeleteString(text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { out->artist[0] = LDG_STR_TERM; }
    }

    text = 0x0;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_get_AlbumTitle(props, &text)))
    {
        ret = sys_hstring_copy(text, out->album, LDG_SYS_MEDIA_TEXT_MAX);
        WindowsDeleteString(text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { out->album[0] = LDG_STR_TERM; }
    }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionMediaProperties_Release(props);

    return LDG_ERR_AOK;
}

static uint32_t sys_media_status_rd(ldg_gsmtc_sesh_t *sesh, ldg_sys_media_status_t *out)
{
    ldg_gsmtc_playback_t *playback = 0x0;
    __x_ABI_CWindows_CMedia_CControl_CGlobalSystemMediaTransportControlsSessionPlaybackStatus status = LDG_SYS_MEDIA_NONE;

    if (FAILED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_GetPlaybackInfo(sesh, &playback)) || !playback) { return LDG_ERR_AOK; }

    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionPlaybackInfo_get_PlaybackStatus(playback, &status))) { *out = (ldg_sys_media_status_t)status; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionPlaybackInfo_Release(playback);

    return LDG_ERR_AOK;
}

static uint32_t sys_media_timeline_rd(ldg_gsmtc_sesh_t *sesh, uint64_t *out_change, uint64_t *out_len, uint64_t *out_pos)
{
    ldg_gsmtc_timeline_t *timeline = 0x0;
    __x_ABI_CWindows_CFoundation_CDateTime stamp = LDG_STRUCT_ZERO_INIT;
    __x_ABI_CWindows_CFoundation_CTimeSpan start = LDG_STRUCT_ZERO_INIT;
    __x_ABI_CWindows_CFoundation_CTimeSpan end = LDG_STRUCT_ZERO_INIT;
    __x_ABI_CWindows_CFoundation_CTimeSpan at = LDG_STRUCT_ZERO_INIT;
    uint8_t start_has = LDG_TRUTH_FALSE;

    *out_change = 0;
    *out_len = 0;
    *out_pos = 0;

    if (FAILED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_GetTimelineProperties(sesh, &timeline)) || !timeline) { return LDG_ERR_AOK; }

    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionTimelineProperties_get_LastUpdatedTime(timeline, &stamp)) && stamp.UniversalTime > LDG_SYS_MEDIA_EPOCH_OFF) { *out_change = (uint64_t)(stamp.UniversalTime - LDG_SYS_MEDIA_EPOCH_OFF) / LDG_SYS_MEDIA_TICK_SCALE; }

    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionTimelineProperties_get_StartTime(timeline, &start)) && start.Duration >= 0) { start_has = LDG_TRUTH_TRUE; }

    if (start_has == LDG_TRUTH_TRUE && SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionTimelineProperties_get_EndTime(timeline, &end)) && end.Duration > start.Duration) { *out_len = (uint64_t)(end.Duration - start.Duration) / LDG_SYS_MEDIA_TICK_SCALE; }

    if (start_has == LDG_TRUTH_TRUE && SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionTimelineProperties_get_Position(timeline, &at)) && at.Duration > start.Duration) { *out_pos = (uint64_t)(at.Duration - start.Duration) / LDG_SYS_MEDIA_TICK_SCALE; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionTimelineProperties_Release(timeline);

    return LDG_ERR_AOK;
}

static uint32_t sys_media_player_rd(ldg_gsmtc_sesh_t *sesh, HSTRING current_id, char *out, uint64_t out_size, uint8_t *out_current)
{
    HSTRING id = 0x0;
    const WCHAR *raw = 0x0;
    UINT32 len = 0;
    INT32 order = 0;
    uint32_t ret = 0;

    out[0] = LDG_STR_TERM;
    *out_current = LDG_TRUTH_FALSE;

    if (FAILED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_get_SourceAppUserModelId(sesh, &id)) || !id) { return LDG_ERR_AOK; }

    if (current_id && SUCCEEDED(WindowsCompareStringOrdinal(id, current_id, &order)) && order == 0) { *out_current = LDG_TRUTH_TRUE; }

    raw = WindowsGetStringRawBuffer(id, &len);
    ret = sys_media_player_resolve(raw, (uint32_t)len, out, out_size);

    WindowsDeleteString(id);

    return ret;
}

static uint32_t sys_media_sesh_rd(ldg_gsmtc_sesh_t *sesh, HSTRING current_id, ldg_sys_media_info_t *out)
{
    uint32_t ret = 0;

    ret = sys_media_props_rd(sesh, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_status_rd(sesh, &out->status);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_timeline_rd(sesh, &out->change, &out->len, &out->pos);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return sys_media_player_rd(sesh, current_id, out->player, LDG_SYS_MEDIA_TEXT_MAX, &out->current);
}

static uint32_t sys_media_list_sort(ldg_sys_media_info_t *list, uint32_t cunt)
{
    ldg_sys_media_info_t held = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t at = 0;
    uint32_t ret = 0;

    for (idx = LDG_SYS_MEDIA_STEP; idx < cunt; idx++)
    {
        ret = ldg_mem_cpy(&held, &list[idx], sizeof(held));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        at = idx;

        while (at > 0 && list[at - LDG_SYS_MEDIA_STEP].change < held.change)
        {
            ret = ldg_mem_cpy(&list[at], &list[at - LDG_SYS_MEDIA_STEP], sizeof(held));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            at--;
        }

        ret = ldg_mem_cpy(&list[at], &held, sizeof(held));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint8_t sys_media_player_eq_is(const char *lhs, const char *rhs)
{
    uint32_t idx = 0;
    char lhs_ch = LDG_STR_TERM;
    char rhs_ch = LDG_STR_TERM;

    for (idx = 0; idx < LDG_SYS_MEDIA_TEXT_MAX; idx++)
    {
        lhs_ch = lhs[idx];
        rhs_ch = rhs[idx];

        if (lhs_ch >= LDG_SYS_MEDIA_ASCII_UPPER_MIN && lhs_ch <= LDG_SYS_MEDIA_ASCII_UPPER_MAX) { lhs_ch = (char)(lhs_ch + LDG_SYS_MEDIA_ASCII_CASE_OFF); }

        if (rhs_ch >= LDG_SYS_MEDIA_ASCII_UPPER_MIN && rhs_ch <= LDG_SYS_MEDIA_ASCII_UPPER_MAX) { rhs_ch = (char)(rhs_ch + LDG_SYS_MEDIA_ASCII_CASE_OFF); }

        if (lhs_ch != rhs_ch) { return LDG_TRUTH_FALSE; }

        if (lhs_ch == LDG_STR_TERM) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t sys_media_player_sesh_get(const char *player, ldg_gsmtc_sesh_t **out)
{
    char name[LDG_SYS_MEDIA_TEXT_MAX] = LDG_ARR_ZERO_INIT;
    ldg_gsmtc_mngr_t *mngr = 0x0;
    ldg_gsmtc_sesh_view_t *view = 0x0;
    ldg_gsmtc_sesh_t *sesh = 0x0;
    ldg_gsmtc_sesh_t *best = 0x0;
    uint64_t change = 0;
    uint64_t best_change = 0;
    uint64_t len = 0;
    uint64_t pos = 0;
    UINT32 size = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t current = LDG_TRUTH_FALSE;

    *out = 0x0;

    ret = sys_media_mngr_get(&mngr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_GetSessions(mngr, &view)) && view) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_Release(mngr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (FAILED(__FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_get_Size(view, &size))) { size = 0; }

    for (idx = 0; idx < (uint32_t)size; idx++)
    {
        sesh = 0x0;
        if (FAILED(__FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_GetAt(view, idx, &sesh)) || !sesh) { continue; }

        if (sys_media_player_rd(sesh, 0x0, name, sizeof(name), &current) != LDG_ERR_AOK || sys_media_player_eq_is(name, player) != LDG_TRUTH_TRUE)
        {
            __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
            continue;
        }

        if (sys_media_timeline_rd(sesh, &change, &len, &pos) != LDG_ERR_AOK) { change = 0; }

        if (best && change <= best_change)
        {
            __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
            continue;
        }

        if (best) { __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(best); }

        best = sesh;
        best_change = change;
    }

    __FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_Release(view);

    if (!best) { return LDG_ERR_NOT_FOUND; }

    *out = best;

    return LDG_ERR_AOK;
}

static uint32_t sys_media_op_run(ldg_gsmtc_sesh_t *sesh, sys_media_op_t kind)
{
    ldg_gsmtc_bool_op_t *op = 0x0;
    HRESULT hr = E_FAIL;
    uint32_t ret = 0;

    if (kind == SYS_MEDIA_OP_PLAY) { hr = __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TryPlayAsync(sesh, &op); }
    else if (kind == SYS_MEDIA_OP_PAUSE) { hr = __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TryPauseAsync(sesh, &op); }
    else if (kind == SYS_MEDIA_OP_NEXT) { hr = __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TrySkipNextAsync(sesh, &op); }
    else { hr = __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_TrySkipPreviousAsync(sesh, &op); }

    if (LDG_UNLIKELY(FAILED(hr) || !op)) { return LDG_ERR_NET_RPC_FAIL; }

    ret = sys_media_await((IUnknown *)op);
    __FIAsyncOperation_1_boolean_Release(op);

    return ret;
}

static uint32_t sys_media_transport(const char *player, sys_media_op_t kind)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (player) { ret = sys_media_player_sesh_get(player, &sesh); }
    else { ret = sys_media_sesh_get(&sesh); }

    if (ret == LDG_ERR_AOK)
    {
        ret = sys_media_op_run(sesh, kind);
        __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    }

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return ret;
}

uint32_t ldg_sys_media_info_get(ldg_sys_media_info_t *out)
{
    ldg_gsmtc_sesh_t *sesh = 0x0;
    uint8_t owned = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_sys_media_info_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_sesh_get(&sesh);
    if (ret == LDG_ERR_AOK)
    {
        ret = sys_media_sesh_rd(sesh, 0x0, out);
        out->current = LDG_TRUTH_TRUE;
        __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    }

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    return ret;
}

uint32_t ldg_sys_media_list(ldg_sys_media_info_t *out, uint32_t cap, uint32_t *out_cunt)
{
    ldg_gsmtc_mngr_t *mngr = 0x0;
    ldg_gsmtc_sesh_view_t *view = 0x0;
    ldg_gsmtc_sesh_t *sesh = 0x0;
    HSTRING current_id = 0x0;
    UINT32 size = 0;
    uint32_t idx = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;
    uint8_t owned = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = sys_media_apartment_enter(&owned);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_media_mngr_get(&mngr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

        return ret;
    }

    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_GetCurrentSession(mngr, &sesh)) && sesh)
    {
        if (FAILED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_get_SourceAppUserModelId(sesh, &current_id))) { current_id = 0x0; }

        __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
    }

    ret = LDG_ERR_NET_RPC_FAIL;
    if (SUCCEEDED(__x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_GetSessions(mngr, &view)) && view) { ret = LDG_ERR_AOK; }

    __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSessionManager_Release(mngr);

    if (ret == LDG_ERR_AOK)
    {
        if (FAILED(__FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_get_Size(view, &size))) { size = 0; }

        for (idx = 0; idx < (uint32_t)size && cunt < cap; idx++)
        {
            sesh = 0x0;
            if (FAILED(__FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_GetAt(view, idx, &sesh)) || !sesh) { continue; }

            if (ldg_mem_secure_zero(&out[cunt], sizeof(ldg_sys_media_info_t)) == LDG_ERR_AOK && sys_media_sesh_rd(sesh, current_id, &out[cunt]) == LDG_ERR_AOK) { cunt++; }

            __x_ABI_CWindows_CMedia_CControl_CIGlobalSystemMediaTransportControlsSession_Release(sesh);
        }

        if (idx < (uint32_t)size) { ret = LDG_ERR_FULL; }

        __FIVectorView_1_Windows__CMedia__CControl__CGlobalSystemMediaTransportControlsSession_Release(view);
    }

    if (current_id) { WindowsDeleteString(current_id); }

    if (owned == LDG_TRUTH_TRUE) { RoUninitialize(); }

    if (ret != LDG_ERR_AOK && ret != LDG_ERR_FULL) { return ret; }

    *out_cunt = cunt;

    if (LDG_UNLIKELY(sys_media_list_sort(out, cunt) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    return ret;
}

uint32_t ldg_sys_media_play(void)
{
    return sys_media_transport(0x0, SYS_MEDIA_OP_PLAY);
}

uint32_t ldg_sys_media_pause(void)
{
    return sys_media_transport(0x0, SYS_MEDIA_OP_PAUSE);
}

uint32_t ldg_sys_media_next_skip(void)
{
    return sys_media_transport(0x0, SYS_MEDIA_OP_NEXT);
}

uint32_t ldg_sys_media_prev_skip(void)
{
    return sys_media_transport(0x0, SYS_MEDIA_OP_PREV);
}

uint32_t ldg_sys_media_player_play(const char *player)
{
    if (LDG_UNLIKELY(!player)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(player[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return sys_media_transport(player, SYS_MEDIA_OP_PLAY);
}

uint32_t ldg_sys_media_player_pause(const char *player)
{
    if (LDG_UNLIKELY(!player)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(player[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return sys_media_transport(player, SYS_MEDIA_OP_PAUSE);
}

uint32_t ldg_sys_media_player_next_skip(const char *player)
{
    if (LDG_UNLIKELY(!player)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(player[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return sys_media_transport(player, SYS_MEDIA_OP_NEXT);
}

uint32_t ldg_sys_media_player_prev_skip(const char *player)
{
    if (LDG_UNLIKELY(!player)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(player[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return sys_media_transport(player, SYS_MEDIA_OP_PREV);
}
