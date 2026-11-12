#include <stdint.h>
#include <windows.h>
#include <aclapi.h>

#include <dangling/io/ipc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/str/fmt.h>

#define LDG_IO_IPC_PIPE_FMT "\\\\.\\pipe\\%s"
#define LDG_IO_IPC_PIPE_GLOB "\\\\.\\pipe\\*"
#define LDG_IO_IPC_PIPE_PATH_CAP (LDG_IO_IPC_NAME_MAX + 16)
#define LDG_IO_IPC_PIPE_BUFF_SIZE 65536
#define LDG_IO_IPC_PIPE_WAIT_DEFAULT 0
#define LDG_IO_IPC_SHARE_NONE 0
#define LDG_IO_IPC_ATTR_NONE 0
#define LDG_IO_IPC_PEEK_SIZE_NONE 0
#define LDG_IO_IPC_WAIT_NAP_MS 1
#define LDG_IO_IPC_DRAIN_CAP 512
#define LDG_IO_IPC_ACE_CUNT 3

typedef struct ldg_io_ipc_user
{
    TOKEN_USER tok;
    uint8_t sid_room[SECURITY_MAX_SID_SIZE];
} ldg_io_ipc_user_t;

typedef struct ldg_io_ipc_acl
{
    ACL acl;
    uint8_t ace_room[(sizeof(ACCESS_ALLOWED_ACE) + SECURITY_MAX_SID_SIZE) * LDG_IO_IPC_ACE_CUNT];
} ldg_io_ipc_acl_t;

typedef struct ldg_io_ipc_sid
{
    SID base;
    uint8_t room[SECURITY_MAX_SID_SIZE];
} ldg_io_ipc_sid_t;

static uint32_t io_ipc_rst(ldg_io_ipc_t *ipc)
{
    if (LDG_UNLIKELY(ldg_mem_secure_zero(ipc, sizeof(ldg_io_ipc_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    ipc->handle = LDG_IO_IPC_HANDLE_NONE;

    return LDG_ERR_AOK;
}

static uint32_t io_ipc_path_build(const char *name, char *out)
{
    uint64_t len = 0;
    uint64_t path_len = 0;

    if (LDG_UNLIKELY(ldg_str_len_get(name, LDG_IO_IPC_NAME_MAX, &len) != LDG_ERR_AOK)) { return LDG_ERR_STR_TRUNC; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    return ldg_str_fmt(out, LDG_IO_IPC_PIPE_PATH_CAP, &path_len, LDG_IO_IPC_PIPE_FMT, name);
}

static uint32_t io_ipc_user_get(ldg_io_ipc_user_t *out)
{
    HANDLE tok = 0x0;
    DWORD need = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok))) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!GetTokenInformation(tok, TokenUser, out, sizeof(ldg_io_ipc_user_t), &need))) { ret = LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!CloseHandle(tok)) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static uint32_t io_ipc_privileged_ace_add(ACL *acl, WELL_KNOWN_SID_TYPE kind)
{
    ldg_io_ipc_sid_t sid = LDG_STRUCT_ZERO_INIT;
    DWORD size = sizeof(ldg_io_ipc_sid_t);

    if (LDG_UNLIKELY(!CreateWellKnownSid(kind, 0x0, &sid, &size))) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, &sid))) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

static uint32_t io_ipc_instance_create(const char *path, uint8_t first, HANDLE *out)
{
    ldg_io_ipc_user_t user = LDG_STRUCT_ZERO_INIT;
    ldg_io_ipc_acl_t acl = LDG_STRUCT_ZERO_INIT;
    SECURITY_DESCRIPTOR desc = LDG_STRUCT_ZERO_INIT;
    SECURITY_ATTRIBUTES attr = LDG_STRUCT_ZERO_INIT;
    DWORD open_mode = PIPE_ACCESS_DUPLEX;
    uint32_t ret = 0;

    *out = INVALID_HANDLE_VALUE;

    ret = io_ipc_user_get(&user);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!InitializeAcl(&acl.acl, sizeof(ldg_io_ipc_acl_t), ACL_REVISION))) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!AddAccessAllowedAce(&acl.acl, ACL_REVISION, FILE_ALL_ACCESS, user.tok.User.Sid))) { return LDG_ERR_DENIED; }

    ret = io_ipc_privileged_ace_add(&acl.acl, WinBuiltinAdministratorsSid);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_privileged_ace_add(&acl.acl, WinLocalSystemSid);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!InitializeSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION))) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!SetSecurityDescriptorDacl(&desc, TRUE, &acl.acl, FALSE))) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!SetSecurityDescriptorOwner(&desc, user.tok.User.Sid, FALSE))) { return LDG_ERR_DENIED; }

    attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    attr.lpSecurityDescriptor = &desc;
    attr.bInheritHandle = FALSE;

    if (first == LDG_TRUTH_TRUE) { open_mode |= FILE_FLAG_FIRST_PIPE_INSTANCE; }

    *out = CreateNamedPipeA(path, open_mode, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES, LDG_IO_IPC_PIPE_BUFF_SIZE, LDG_IO_IPC_PIPE_BUFF_SIZE, LDG_IO_IPC_PIPE_WAIT_DEFAULT, &attr);
    if (*out != INVALID_HANDLE_VALUE) { return LDG_ERR_AOK; }

    if (GetLastError() == ERROR_ACCESS_DENIED) { return LDG_ERR_EXISTS; }

    return LDG_ERR_IO_OPEN;
}

static uint32_t io_ipc_owner_ck(HANDLE pipe)
{
    ldg_io_ipc_user_t user = LDG_STRUCT_ZERO_INIT;
    PSECURITY_DESCRIPTOR desc = 0x0;
    PSID owner = 0x0;
    uint32_t ret = 0;

    ret = io_ipc_user_get(&user);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(GetSecurityInfo(pipe, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION, &owner, 0x0, 0x0, 0x0, &desc) != ERROR_SUCCESS)) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(!owner)) { ret = LDG_ERR_DENIED; }
    else if (!EqualSid(owner, user.tok.User.Sid) && !IsWellKnownSid(owner, WinBuiltinAdministratorsSid) && !IsWellKnownSid(owner, WinLocalSystemSid)) { ret = LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(LocalFree(desc) != 0x0) && ret == LDG_ERR_AOK) { ret = LDG_ERR_MEM_BAD; }

    return ret;
}

static uint32_t io_ipc_broken_translate(ldg_io_ipc_t *ipc, uint32_t fallback)
{
    DWORD code = GetLastError();

    if (code == ERROR_BROKEN_PIPE || code == ERROR_PIPE_NOT_CONNECTED || code == ERROR_NO_DATA)
    {
        ipc->live = LDG_TRUTH_FALSE;
        return LDG_ERR_CLOSED;
    }

    return fallback;
}

static uint32_t io_ipc_msg_rest_drop(ldg_io_ipc_t *ipc)
{
    ldg_byte_t buff[LDG_IO_IPC_DRAIN_CAP] = LDG_ARR_ZERO_INIT;
    DWORD got = 0;

    for (;;)
    {
        if (ReadFile((HANDLE)(uintptr_t)ipc->handle, buff, LDG_IO_IPC_DRAIN_CAP, &got, 0x0)) { return LDG_ERR_AOK; }

        if (GetLastError() != ERROR_MORE_DATA) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_RD); }
    }
}

uint32_t ldg_io_ipc_listen(const char *name, ldg_io_ipc_t *out)
{
    char path[LDG_IO_IPC_PIPE_PATH_CAP] = LDG_ARR_ZERO_INIT;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_path_build(name, path);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_instance_create(path, LDG_TRUTH_TRUE, &pipe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->handle = (uint64_t)(uintptr_t)pipe;
    out->listening = LDG_TRUTH_TRUE;
    out->live = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(out->name, name, LDG_IO_IPC_NAME_MAX);
}

uint32_t ldg_io_ipc_accept(ldg_io_ipc_t *listener, ldg_io_ipc_t *out)
{
    char path[LDG_IO_IPC_PIPE_PATH_CAP] = LDG_ARR_ZERO_INIT;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    HANDLE next = INVALID_HANDLE_VALUE;
    DWORD code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!listener || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(listener->live != LDG_TRUTH_TRUE || listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    pipe = (HANDLE)(uintptr_t)listener->handle;

    if (!ConnectNamedPipe(pipe, 0x0))
    {
        code = GetLastError();

        if (code == ERROR_PIPE_LISTENING) { return LDG_ERR_WOULD_BLOCK; }

        if (code == ERROR_NO_DATA)
        {
            if (LDG_UNLIKELY(!DisconnectNamedPipe(pipe))) { return LDG_ERR_IO_CLOSE; }

            return LDG_ERR_WOULD_BLOCK;
        }

        if (code != ERROR_PIPE_CONNECTED) { return LDG_ERR_IO_OPEN; }
    }

    ret = io_ipc_path_build(listener->name, path);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_instance_create(path, LDG_TRUTH_FALSE, &next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    listener->handle = (uint64_t)(uintptr_t)next;
    out->handle = (uint64_t)(uintptr_t)pipe;
    out->live = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(out->name, listener->name, LDG_IO_IPC_NAME_MAX);
}

uint32_t ldg_io_ipc_connect(const char *name, ldg_io_ipc_t *out)
{
    char path[LDG_IO_IPC_PIPE_PATH_CAP] = LDG_ARR_ZERO_INIT;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    DWORD mode = PIPE_READMODE_MESSAGE | PIPE_NOWAIT;
    DWORD code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_path_build(name, path);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pipe = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, LDG_IO_IPC_SHARE_NONE, 0x0, OPEN_EXISTING, LDG_IO_IPC_ATTR_NONE, 0x0);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        code = GetLastError();

        if (code == ERROR_FILE_NOT_FOUND) { return LDG_ERR_NOT_FOUND; }

        if (code == ERROR_PIPE_BUSY) { return LDG_ERR_BUSY; }

        if (code == ERROR_ACCESS_DENIED) { return LDG_ERR_DENIED; }

        return LDG_ERR_IO_OPEN;
    }

    ret = io_ipc_owner_ck(pipe);
    if (ret == LDG_ERR_AOK && LDG_UNLIKELY(!SetNamedPipeHandleState(pipe, &mode, 0x0, 0x0))) { ret = LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (!CloseHandle(pipe)) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    out->handle = (uint64_t)(uintptr_t)pipe;
    out->live = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(out->name, name, LDG_IO_IPC_NAME_MAX);
}

uint32_t ldg_io_ipc_peek(ldg_io_ipc_t *ipc, uint64_t *out_avail)
{
    DWORD avail = 0;

    if (LDG_UNLIKELY(!ipc || !out_avail)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_avail = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (!PeekNamedPipe((HANDLE)(uintptr_t)ipc->handle, 0x0, LDG_IO_IPC_PEEK_SIZE_NONE, 0x0, &avail, 0x0)) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_RD); }

    *out_avail = (uint64_t)avail;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_rd(ldg_io_ipc_t *ipc, ldg_byte_t *dst, uint64_t cap, uint64_t *out_got)
{
    DWORD avail = 0;
    DWORD want = 0;
    DWORD got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ipc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (cap == 0) { return LDG_ERR_AOK; }

    if (!PeekNamedPipe((HANDLE)(uintptr_t)ipc->handle, 0x0, LDG_IO_IPC_PEEK_SIZE_NONE, 0x0, &avail, 0x0)) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_RD); }

    if (avail == 0) { return LDG_ERR_AOK; }

    want = (cap < (uint64_t)MAXDWORD) ? (DWORD)cap : MAXDWORD;

    if (ReadFile((HANDLE)(uintptr_t)ipc->handle, dst, want, &got, 0x0))
    {
        *out_got = (uint64_t)got;
        return LDG_ERR_AOK;
    }

    if (GetLastError() != ERROR_MORE_DATA) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_RD); }

    *out_got = (uint64_t)got;

    ret = io_ipc_msg_rest_drop(ipc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_OVERFLOW;
}

uint32_t ldg_io_ipc_wr(ldg_io_ipc_t *ipc, const ldg_byte_t *src, uint64_t len, uint64_t *out_sent)
{
    DWORD sent = 0;

    if (LDG_UNLIKELY(!ipc || !src || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > (uint64_t)MAXDWORD)) { return LDG_ERR_OVERFLOW; }

    if (!WriteFile((HANDLE)(uintptr_t)ipc->handle, src, (DWORD)len, &sent, 0x0)) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_WR); }

    *out_sent = (uint64_t)sent;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_close(ldg_io_ipc_t *ipc)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ipc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ipc->handle == LDG_IO_IPC_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!CloseHandle((HANDLE)(uintptr_t)ipc->handle))) { ret = LDG_ERR_IO_CLOSE; }

    ipc->handle = LDG_IO_IPC_HANDLE_NONE;
    ipc->live = LDG_TRUTH_FALSE;
    ipc->listening = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t ldg_io_ipc_accept_timed(ldg_io_ipc_t *listener, uint64_t timeout_ms, ldg_io_ipc_t *out)
{
    uint64_t start = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!listener || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(listener->live != LDG_TRUTH_TRUE || listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    start = (uint64_t)GetTickCount64();

    for (;;)
    {
        if (listener->cancelled == LDG_TRUTH_TRUE)
        {
            listener->cancelled = LDG_TRUTH_FALSE;
            return LDG_ERR_CANCELED;
        }

        ret = ldg_io_ipc_accept(listener, out);
        if (ret != LDG_ERR_WOULD_BLOCK) { return ret; }

        if (timeout_ms != LDG_IO_IPC_TIMEOUT_NONE && (uint64_t)GetTickCount64() - start >= timeout_ms) { return LDG_ERR_TIMEOUT; }

        Sleep(LDG_IO_IPC_WAIT_NAP_MS);
    }
}

uint32_t ldg_io_ipc_accept_cancel(ldg_io_ipc_t *listener)
{
    if (LDG_UNLIKELY(!listener)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(listener->live != LDG_TRUTH_TRUE || listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    listener->cancelled = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_rd_timed(ldg_io_ipc_t *ipc, ldg_byte_t *dst, uint64_t cap, uint64_t timeout_ms, uint64_t *out_got)
{
    uint64_t start = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ipc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    start = (uint64_t)GetTickCount64();

    for (;;)
    {
        ret = ldg_io_ipc_rd(ipc, dst, cap, out_got);
        if (ret != LDG_ERR_AOK || *out_got > 0) { return ret; }

        if (timeout_ms != LDG_IO_IPC_TIMEOUT_NONE && (uint64_t)GetTickCount64() - start >= timeout_ms) { return LDG_ERR_TIMEOUT; }

        Sleep(LDG_IO_IPC_WAIT_NAP_MS);
    }
}

uint32_t ldg_io_ipc_list(const char *prefix, ldg_io_ipc_name_t *out, uint64_t cap, uint64_t *out_cunt)
{
    WIN32_FIND_DATAA entry = LDG_STRUCT_ZERO_INIT;
    HANDLE find = INVALID_HANDLE_VALUE;
    uint64_t prefix_len = 0;
    uint64_t name_len = 0;
    uint32_t order = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!prefix || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(ldg_str_len_get(prefix, LDG_IO_IPC_NAME_MAX, &prefix_len) != LDG_ERR_AOK)) { return LDG_ERR_STR_TRUNC; }

    find = FindFirstFileA(LDG_IO_IPC_PIPE_GLOB, &entry);
    if (find == INVALID_HANDLE_VALUE) { return (GetLastError() == ERROR_FILE_NOT_FOUND) ? LDG_ERR_AOK : LDG_ERR_IO_OPEN; }

    do
    {
        if (*out_cunt >= cap) { break; }

        if (ldg_str_len_get(entry.cFileName, MAX_PATH, &name_len) != LDG_ERR_AOK) { continue; }

        if (name_len < prefix_len || name_len - prefix_len >= LDG_IO_IPC_NAME_MAX) { continue; }

        if (prefix_len > 0 && (ldg_mem_cmp(entry.cFileName, prefix, prefix_len, &order) != LDG_ERR_AOK || order != 0)) { continue; }

        if (ldg_str_rbrcpy(out[*out_cunt].text, entry.cFileName + prefix_len, LDG_IO_IPC_NAME_MAX) != LDG_ERR_AOK) { continue; }

        (*out_cunt)++;
    }
    while (FindNextFileA(find, &entry));

    if (LDG_UNLIKELY(!FindClose(find))) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}
