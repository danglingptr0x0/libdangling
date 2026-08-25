#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include <wincred.h>

#include <dangling/io/path.h>
#include <dangling/mem/secure.h>

#define LDG_SYS_URL_SCHEME_HTTP "http:"
#define LDG_SYS_URL_SCHEME_HTTPS "https:"
#define LDG_SYS_URL_WIDE_MAX 2048
#define LDG_SYS_MB_NO_FLAG 0
#define LDG_SYS_MB_NULL_TERM (-1)
#define LDG_SYS_SHELL_EXEC_OK_MIN 32
#define LDG_SYS_URL_SCHEME_MAX 32
#define LDG_SYS_APP_NAME_MAX 128
#define LDG_SYS_PATH_BUFF_MAX 512
#define LDG_SYS_EXEC_BUFF_MAX 1024
#define LDG_SYS_KEY_BUFF_MAX 2048
#define LDG_SYS_REG_NO_OPT 0
#define LDG_SYS_CLASSES_KEY "Software\\Classes\\"
#define LDG_SYS_CMD_SUBKEY "\\shell\\open\\command"
#define LDG_SYS_URL_PROTO_VAL "URL Protocol"
#define LDG_SYS_URL_DESC_PREFIX "URL:"
#define LDG_SYS_CMD_ARG_TOK "\"%\x31\""
#define LDG_SYS_CRED_NO_FLAG 0
#define LDG_SYS_CPU_CLASS_BASE 0
#define LDG_SYS_CPU_GROUP_BITS 64
#define LDG_SYS_CPU_INFO_BUFF_MAX 8192
#define LDG_SYS_CPU_BIT_MASK 0x1ULL
#define LDG_SYS_SECRET_APP_MAX 128
#define LDG_SYS_SECRET_KEY_MAX 128
#define LDG_SYS_NOTIFY_TITLE_MAX 64
#define LDG_SYS_NOTIFY_BODY_MAX 256
#define LDG_SYS_NOTIFY_TIP_MAX 128
#define LDG_SYS_NOTIFY_ICON_ID 1
#define LDG_SYS_NOTIFY_CLASS L"ldg_notify_wnd"
#define LDG_SYS_NOTIFY_NONE 0
#define LDG_SYS_NOTIFY_ICON_APP 32512

#include <dangling/sys/info.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

uint32_t ldg_sys_hostname_get(char *buff, uint64_t buff_size)
{
    DWORD size = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    size = (DWORD)(buff_size - LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(!GetComputerNameExA(ComputerNameDnsHostname, buff, &size))) { return LDG_ERR_IO_RD; }

    buff[buff_size - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_cpu_cunt_get(uint32_t *cunt)
{
    SYSTEM_INFO si = { 0 };

    if (LDG_UNLIKELY(!cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cunt = 0;

    GetSystemInfo(&si);
    if (LDG_UNLIKELY(si.dwNumberOfProcessors == 0)) { return LDG_ERR_UNSUPPORTED; }

    *cunt = (uint32_t)si.dwNumberOfProcessors;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_page_size_get(uint64_t *size)
{
    SYSTEM_INFO si = { 0 };

    if (LDG_UNLIKELY(!size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *size = 0;

    GetSystemInfo(&si);
    if (LDG_UNLIKELY(si.dwPageSize == 0)) { return LDG_ERR_UNSUPPORTED; }

    *size = (uint64_t)si.dwPageSize;

    return LDG_ERR_AOK;
}

static uint8_t sys_group_holds_is(const GROUP_AFFINITY *affinity, uint32_t cpu_idx)
{
    uint32_t group = 0;
    uint32_t bit = 0;

    if (LDG_UNLIKELY(!affinity)) { return LDG_ERR_FUNC_ARG_NULL; }

    group = cpu_idx / LDG_SYS_CPU_GROUP_BITS;
    bit = cpu_idx % LDG_SYS_CPU_GROUP_BITS;

    if ((uint32_t)affinity->Group != group) { return LDG_TRUTH_FALSE; }

    if ((((uint64_t)affinity->Mask >> bit) & LDG_SYS_CPU_BIT_MASK) == LDG_SYS_CPU_BIT_MASK) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_sys_cpu_cache_width_get(uint32_t cpu_idx, uint32_t level, uint64_t *out)
{
    uint8_t raw[LDG_SYS_CPU_INFO_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *entry = 0x0;
    DWORD len = 0;
    uint64_t off = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(level == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    len = (DWORD)LDG_SYS_CPU_INFO_BUFF_MAX;

    if (LDG_UNLIKELY(!GetLogicalProcessorInformationEx(RelationCache, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(void *)raw, &len))) { return LDG_ERR_UNSUPPORTED; }

    while (off < (uint64_t)len)
    {
        entry = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(void *)(raw + off);

        if (LDG_UNLIKELY(entry->Size == 0)) { return LDG_ERR_MEM_CORRUPTION; }

        if (entry->Relationship == RelationCache && (uint32_t)entry->Cache.Level == level) { if (sys_group_holds_is(&entry->Cache.GroupMask, cpu_idx) == LDG_TRUTH_TRUE)
            {
                *out = (uint64_t)entry->Cache.CacheSize;
                return LDG_ERR_AOK;
            }
        }

        off += (uint64_t)entry->Size;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_sys_cpu_class_get(uint32_t cpu_idx, uint32_t *out)
{
    uint8_t raw[LDG_SYS_CPU_INFO_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *entry = 0x0;
    DWORD len = 0;
    uint64_t off = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_SYS_CPU_CLASS_BASE;

    len = (DWORD)LDG_SYS_CPU_INFO_BUFF_MAX;

    if (LDG_UNLIKELY(!GetLogicalProcessorInformationEx(RelationProcessorCore, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(void *)raw, &len))) { return LDG_ERR_UNSUPPORTED; }

    while (off < (uint64_t)len)
    {
        entry = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)(void *)(raw + off);

        if (LDG_UNLIKELY(entry->Size == 0)) { return LDG_ERR_MEM_CORRUPTION; }

        if (entry->Relationship == RelationProcessorCore) { if (sys_group_holds_is(&entry->Processor.GroupMask[0], cpu_idx) == LDG_TRUTH_TRUE)
            {
                *out = (uint32_t)entry->Processor.EfficiencyClass;
                return LDG_ERR_AOK;
            }
        }

        off += (uint64_t)entry->Size;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_sys_mem_total_get(uint64_t *out)
{
    MEMORYSTATUSEX status = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    status.dwLength = (DWORD)sizeof(MEMORYSTATUSEX);

    if (LDG_UNLIKELY(!GlobalMemoryStatusEx(&status))) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(status.ullTotalPhys == 0)) { return LDG_ERR_UNSUPPORTED; }

    *out = (uint64_t)status.ullTotalPhys;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_mem_avail_get(uint64_t *out)
{
    MEMORYSTATUSEX status = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    status.dwLength = (DWORD)sizeof(MEMORYSTATUSEX);

    if (LDG_UNLIKELY(!GlobalMemoryStatusEx(&status))) { return LDG_ERR_UNSUPPORTED; }

    *out = (uint64_t)status.ullAvailPhys;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_env_get(const char *name, char *buff, uint64_t buff_size)
{
    const char *val = 0x0;

    if (LDG_UNLIKELY(!name || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(buff_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(buff, 0, (size_t)buff_size) != buff)) { return LDG_ERR_MEM_BAD; }

    val = getenv(name);
    if (!val) { return LDG_ERR_NOT_FOUND; }

    return ldg_str_rbrcpy(buff, val, (uint64_t)buff_size);
}

uint32_t ldg_sys_pid_get(uint64_t *pid)
{
    if (LDG_UNLIKELY(!pid)) { return LDG_ERR_FUNC_ARG_NULL; }

    *pid = (uint64_t)GetCurrentProcessId();

    return LDG_ERR_AOK;
}

static uint8_t sys_url_scheme_allowed_is(const char *url)
{
    if (strncmp(url, LDG_SYS_URL_SCHEME_HTTPS, sizeof(LDG_SYS_URL_SCHEME_HTTPS) - LDG_STR_TERM_SIZE) == 0) { return LDG_TRUTH_TRUE; }

    if (strncmp(url, LDG_SYS_URL_SCHEME_HTTP, sizeof(LDG_SYS_URL_SCHEME_HTTP) - LDG_STR_TERM_SIZE) == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_sys_url_open(const char *url)
{
    WCHAR wide[LDG_SYS_URL_WIDE_MAX] = LDG_ARR_ZERO_INIT;
    HINSTANCE res = 0x0;
    int32_t wide_len = 0;

    if (LDG_UNLIKELY(!url)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(url[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (sys_url_scheme_allowed_is(url) != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; }

    wide_len = MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, url, LDG_SYS_MB_NULL_TERM, wide, LDG_SYS_URL_WIDE_MAX);
    if (LDG_UNLIKELY(wide_len <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    res = ShellExecuteW(0x0, L"open", wide, 0x0, 0x0, SW_SHOWNORMAL);
    if (LDG_UNLIKELY((uintptr_t)res <= LDG_SYS_SHELL_EXEC_OK_MIN)) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_AOK;
}

static uint8_t sys_url_scheme_valid_is(const char *scheme)
{
    uint64_t idx = 0;

    if (scheme[0] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    if (ldg_char_alpha_is(scheme[0]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    for (idx = 0; scheme[idx] != LDG_STR_TERM; idx++)
    {
        if (idx >= LDG_SYS_URL_SCHEME_MAX) { return LDG_TRUTH_FALSE; }

        if (ldg_char_alnum_is(scheme[idx]) == LDG_TRUTH_TRUE) { continue; }

        if (scheme[idx] == '+' || scheme[idx] == '-' || scheme[idx] == '.') { continue; }

        return LDG_TRUTH_FALSE;
    }

    return LDG_TRUTH_TRUE;
}

static uint32_t sys_reg_str_set(const char *key_path, const char *val_name, const char *val)
{
    WCHAR wide_key[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR wide_name[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR wide_val[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    HKEY key = 0x0;
    LSTATUS status = 0;
    int32_t val_len = 0;

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, key_path, LDG_SYS_MB_NULL_TERM, wide_key, LDG_SYS_KEY_BUFF_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    val_len = MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, val, LDG_SYS_MB_NULL_TERM, wide_val, LDG_SYS_KEY_BUFF_MAX);
    if (LDG_UNLIKELY(val_len <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (val_name != 0x0) { if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, val_name, LDG_SYS_MB_NULL_TERM, wide_name, LDG_SYS_PATH_BUFF_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; } }

    status = RegCreateKeyExW(HKEY_CURRENT_USER, wide_key, LDG_SYS_REG_NO_OPT, 0x0, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0x0, &key, 0x0);
    if (LDG_UNLIKELY(status != ERROR_SUCCESS)) { return LDG_ERR_DENIED; }

    status = RegSetValueExW(key, (val_name != 0x0) ? wide_name : 0x0, LDG_SYS_REG_NO_OPT, REG_SZ, (const BYTE *)wide_val, (DWORD)((uint32_t)val_len * sizeof(WCHAR)));
    if (LDG_UNLIKELY(RegCloseKey(key) != ERROR_SUCCESS)) { return LDG_ERR_IO_CLOSE; }

    if (LDG_UNLIKELY(status != ERROR_SUCCESS)) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_url_scheme_reg(const char *scheme, const char *exec_path, const char *app_name)
{
    char exec_buff[LDG_SYS_EXEC_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char key_path[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char cmd_path[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char desc[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    char cmd_line[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR wide_exec[LDG_SYS_EXEC_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    DWORD exec_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!scheme)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sys_url_scheme_valid_is(scheme) != LDG_TRUTH_TRUE) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (exec_path != 0x0)
    {
        ret = ldg_io_path_resolve(exec_path, exec_buff, sizeof(exec_buff));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        exec_len = GetModuleFileNameW(0x0, wide_exec, LDG_SYS_EXEC_BUFF_MAX);
        if (LDG_UNLIKELY(exec_len == 0 || exec_len >= LDG_SYS_EXEC_BUFF_MAX)) { return LDG_ERR_IO_RD; }

        if (LDG_UNLIKELY(WideCharToMultiByte(CP_UTF8, LDG_SYS_MB_NO_FLAG, wide_exec, LDG_SYS_MB_NULL_TERM, exec_buff, LDG_SYS_EXEC_BUFF_MAX, 0x0, 0x0) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }
    }

    if (LDG_UNLIKELY(snprintf(key_path, sizeof(key_path), "%.*s%.*s", (int32_t)sizeof(LDG_SYS_CLASSES_KEY), LDG_SYS_CLASSES_KEY, (int32_t)LDG_SYS_URL_SCHEME_MAX, scheme) < 0)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(snprintf(cmd_path, sizeof(cmd_path), "%.*s%.*s", (int32_t)LDG_SYS_KEY_BUFF_MAX, key_path, (int32_t)sizeof(LDG_SYS_CMD_SUBKEY), LDG_SYS_CMD_SUBKEY) < 0)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(snprintf(desc, sizeof(desc), "%.*s%.*s", (int32_t)sizeof(LDG_SYS_URL_DESC_PREFIX), LDG_SYS_URL_DESC_PREFIX, (int32_t)LDG_SYS_APP_NAME_MAX, app_name ? app_name : scheme) < 0)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(snprintf(cmd_line, sizeof(cmd_line), "\"%.*s\" %.*s", (int32_t)LDG_SYS_EXEC_BUFF_MAX, exec_buff, (int32_t)sizeof(LDG_SYS_CMD_ARG_TOK), LDG_SYS_CMD_ARG_TOK) < 0)) { return LDG_ERR_IO_FMT; }

    ret = sys_reg_str_set(key_path, 0x0, desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_reg_str_set(key_path, LDG_SYS_URL_PROTO_VAL, "");
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return sys_reg_str_set(cmd_path, 0x0, cmd_line);
}

static uint32_t sys_secret_target_build(const char *app, const char *key, WCHAR *wide, int32_t wide_cap)
{
    char target[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(snprintf(target, sizeof(target), "%.*s/%.*s", (int32_t)LDG_SYS_SECRET_APP_MAX, app, (int32_t)LDG_SYS_SECRET_KEY_MAX, key) < 0)) { return LDG_ERR_IO_FMT; }

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, target, LDG_SYS_MB_NULL_TERM, wide, wide_cap) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_secret_set(const char *app, const char *key, const char *secret)
{
    WCHAR wide_target[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    CREDENTIALW cred = LDG_STRUCT_ZERO_INIT;
    uint64_t secret_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!app || !key || !secret)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = sys_secret_target_build(app, key, wide_target, LDG_SYS_KEY_BUFF_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    secret_len = (uint64_t)strlen(secret);
    if (LDG_UNLIKELY(secret_len > CRED_MAX_CREDENTIAL_BLOB_SIZE)) { return LDG_ERR_OVERFLOW; }

    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = wide_target;
    cred.CredentialBlobSize = (DWORD)secret_len;
    cred.CredentialBlob = (LPBYTE)secret;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    if (LDG_UNLIKELY(!CredWriteW(&cred, LDG_SYS_CRED_NO_FLAG))) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_secret_get(const char *app, const char *key, char *out, uint64_t out_size)
{
    WCHAR wide_target[LDG_SYS_KEY_BUFF_MAX] = LDG_ARR_ZERO_INIT;
    PCREDENTIALW cred = 0x0;
    uint64_t blob_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!app || !key || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = sys_secret_target_build(app, key, wide_target, LDG_SYS_KEY_BUFF_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (!CredReadW(wide_target, CRED_TYPE_GENERIC, LDG_SYS_CRED_NO_FLAG, &cred)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(!cred)) { return LDG_ERR_NOT_FOUND; }

    blob_size = (uint64_t)cred->CredentialBlobSize;
    if (LDG_UNLIKELY(blob_size + LDG_STR_TERM_SIZE > out_size))
    {
        CredFree(cred);
        return LDG_ERR_MEM_STR_TRUNC;
    }

    ret = ldg_mem_secure_cpy(out, cred->CredentialBlob, blob_size);
    out[blob_size] = LDG_STR_TERM;

    if (LDG_UNLIKELY(ldg_mem_secure_zero(cred->CredentialBlob, blob_size) != LDG_ERR_AOK))
    {
        CredFree(cred);
        return LDG_ERR_MEM_BAD;
    }

    CredFree(cred);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

typedef struct ldg_sys_notify_ctx
{
    HWND wnd;
    NOTIFYICONDATAW icon;
    HICON tray_icon;
    HICON balloon_icon;
    WCHAR tip[LDG_SYS_NOTIFY_TIP_MAX];
    uint8_t tip_set;
    uint8_t is_open;
    uint8_t pudding[6];
} ldg_sys_notify_ctx_t;

static ldg_sys_notify_ctx_t g_sys_notify = { .is_open = LDG_TRUTH_FALSE };

static uint32_t sys_icon_ld(const char *path, HICON *out)
{
    WCHAR wide[LDG_SYS_PATH_BUFF_MAX] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, path, LDG_SYS_MB_NULL_TERM, wide, LDG_SYS_PATH_BUFF_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    *out = (HICON)LoadImageW(0x0, wide, IMAGE_ICON, LDG_SYS_NOTIFY_NONE, LDG_SYS_NOTIFY_NONE, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (LDG_UNLIKELY(!(*out))) { return LDG_ERR_IO_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t sys_notify_arm(void)
{
    WNDCLASSEXW cls = LDG_STRUCT_ZERO_INIT;
    HMODULE inst = 0x0;

    if (g_sys_notify.is_open == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    inst = GetModuleHandleW(0x0);
    if (LDG_UNLIKELY(!inst)) { return LDG_ERR_NOT_INIT; }

    cls.cbSize = sizeof(WNDCLASSEXW);
    cls.lpfnWndProc = DefWindowProcW;
    cls.hInstance = inst;
    cls.lpszClassName = LDG_SYS_NOTIFY_CLASS;

    if (RegisterClassExW(&cls) == 0) { if (LDG_UNLIKELY(GetLastError() != ERROR_CLASS_ALREADY_EXISTS)) { return LDG_ERR_NOT_INIT; } }

    g_sys_notify.wnd = CreateWindowExW(LDG_SYS_NOTIFY_NONE, LDG_SYS_NOTIFY_CLASS, LDG_SYS_NOTIFY_CLASS, LDG_SYS_NOTIFY_NONE, LDG_SYS_NOTIFY_NONE, LDG_SYS_NOTIFY_NONE, LDG_SYS_NOTIFY_NONE, LDG_SYS_NOTIFY_NONE, HWND_MESSAGE, 0x0, inst, 0x0);
    if (LDG_UNLIKELY(!g_sys_notify.wnd)) { return LDG_ERR_NOT_INIT; }

    g_sys_notify.icon.cbSize = sizeof(NOTIFYICONDATAW);
    g_sys_notify.icon.hWnd = g_sys_notify.wnd;
    g_sys_notify.icon.uID = LDG_SYS_NOTIFY_ICON_ID;
    g_sys_notify.icon.uFlags = NIF_ICON | NIF_TIP;

    if (g_sys_notify.tray_icon != 0x0) { g_sys_notify.icon.hIcon = g_sys_notify.tray_icon; }
    else { g_sys_notify.icon.hIcon = LoadIconW(0x0, MAKEINTRESOURCEW(LDG_SYS_NOTIFY_ICON_APP)); }

    if (g_sys_notify.tip_set == LDG_TRUTH_TRUE) { if (LDG_UNLIKELY(!lstrcpynW(g_sys_notify.icon.szTip, g_sys_notify.tip, LDG_SYS_NOTIFY_TIP_MAX)))
        {
            DestroyWindow(g_sys_notify.wnd);
            g_sys_notify.wnd = 0x0;
            return LDG_ERR_MEM_STR_TRUNC;
        }
    }

    if (LDG_UNLIKELY(!Shell_NotifyIconW(NIM_ADD, &g_sys_notify.icon)))
    {
        DestroyWindow(g_sys_notify.wnd);
        g_sys_notify.wnd = 0x0;
        return LDG_ERR_NOT_INIT;
    }

    g_sys_notify.is_open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static DWORD sys_notify_flag_get(ldg_sys_notify_level_t level)
{
    if (level == LDG_SYS_NOTIFY_LEVEL_LOW) { return NIIF_INFO; }

    if (level == LDG_SYS_NOTIFY_LEVEL_CRIT) { return NIIF_ERROR; }

    return NIIF_WARNING;
}

uint32_t ldg_sys_notify(const char *title, const char *body, ldg_sys_notify_level_t level)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!title)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(title[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(level > LDG_SYS_NOTIFY_LEVEL_CRIT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = sys_notify_arm();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, title, LDG_SYS_MB_NULL_TERM, g_sys_notify.icon.szInfoTitle, LDG_SYS_NOTIFY_TITLE_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, (body != 0x0) ? body : "", LDG_SYS_MB_NULL_TERM, g_sys_notify.icon.szInfo, LDG_SYS_NOTIFY_BODY_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    g_sys_notify.icon.uFlags = NIF_ICON | NIF_TIP | NIF_INFO;

    if (g_sys_notify.balloon_icon != 0x0)
    {
        g_sys_notify.icon.hBalloonIcon = g_sys_notify.balloon_icon;
        g_sys_notify.icon.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
    }
    else { g_sys_notify.icon.dwInfoFlags = sys_notify_flag_get(level); }

    if (LDG_UNLIKELY(!Shell_NotifyIconW(NIM_MODIFY, &g_sys_notify.icon))) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint8_t ldg_sys_focus_is(void)
{
    HWND fg = 0x0;
    DWORD owner_pid = 0;

    fg = GetForegroundWindow();
    if (!fg) { return LDG_TRUTH_FALSE; }

    if (GetWindowThreadProcessId(fg, &owner_pid) == 0) { return LDG_ERR_NOT_INIT; }

    if (owner_pid == GetCurrentProcessId()) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_sys_notify_tray_icon_set(const char *path)
{
    HICON fresh = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = sys_icon_ld(path, &fresh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_sys_notify.tray_icon != 0x0) { DestroyIcon(g_sys_notify.tray_icon); }

    g_sys_notify.tray_icon = fresh;

    if (g_sys_notify.is_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    g_sys_notify.icon.uFlags = NIF_ICON;
    g_sys_notify.icon.hIcon = fresh;
    if (LDG_UNLIKELY(!Shell_NotifyIconW(NIM_MODIFY, &g_sys_notify.icon))) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_notify_icon_set(const char *path)
{
    HICON fresh = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = sys_icon_ld(path, &fresh);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_sys_notify.balloon_icon != 0x0) { DestroyIcon(g_sys_notify.balloon_icon); }

    g_sys_notify.balloon_icon = fresh;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_notify_tip_set(const char *tip)
{
    if (LDG_UNLIKELY(!tip)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_MB_NO_FLAG, tip, LDG_SYS_MB_NULL_TERM, g_sys_notify.tip, LDG_SYS_NOTIFY_TIP_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    g_sys_notify.tip_set = LDG_TRUTH_TRUE;

    if (g_sys_notify.is_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!lstrcpynW(g_sys_notify.icon.szTip, g_sys_notify.tip, LDG_SYS_NOTIFY_TIP_MAX))) { return LDG_ERR_MEM_STR_TRUNC; }

    g_sys_notify.icon.uFlags = NIF_TIP;
    if (LDG_UNLIKELY(!Shell_NotifyIconW(NIM_MODIFY, &g_sys_notify.icon))) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_notify_close(void)
{
    if (g_sys_notify.is_open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!Shell_NotifyIconW(NIM_DELETE, &g_sys_notify.icon))) { LDG_ERRLOG_WARN("sys: notify icon delete failed"); }

    if (g_sys_notify.wnd != 0x0)
    {
        DestroyWindow(g_sys_notify.wnd);
        g_sys_notify.wnd = 0x0;
    }

    if (g_sys_notify.tray_icon != 0x0)
    {
        DestroyIcon(g_sys_notify.tray_icon);
        g_sys_notify.tray_icon = 0x0;
    }

    if (g_sys_notify.balloon_icon != 0x0)
    {
        DestroyIcon(g_sys_notify.balloon_icon);
        g_sys_notify.balloon_icon = 0x0;
    }

    g_sys_notify.tip_set = LDG_TRUTH_FALSE;
    g_sys_notify.is_open = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}
