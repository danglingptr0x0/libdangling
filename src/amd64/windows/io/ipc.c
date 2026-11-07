#include <stdint.h>
#include <windows.h>

#include <dangling/io/ipc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/str/fmt.h>

#define LDG_IO_IPC_PIPE_FMT "\\\\.\\pipe\\%s"
#define LDG_IO_IPC_PIPE_PATH_CAP (LDG_IO_IPC_NAME_MAX + 16)
#define LDG_IO_IPC_PIPE_BUFF_SIZE 65536
#define LDG_IO_IPC_PIPE_WAIT_DEFAULT 0
#define LDG_IO_IPC_SHARE_NONE 0
#define LDG_IO_IPC_ATTR_NONE 0
#define LDG_IO_IPC_PEEK_SIZE_NONE 0

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

static uint32_t io_ipc_instance_create(const char *path, uint8_t first, HANDLE *out)
{
    DWORD open_mode = PIPE_ACCESS_DUPLEX;

    if (first == LDG_TRUTH_TRUE) { open_mode |= FILE_FLAG_FIRST_PIPE_INSTANCE; }

    *out = CreateNamedPipeA(path, open_mode, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES, LDG_IO_IPC_PIPE_BUFF_SIZE, LDG_IO_IPC_PIPE_BUFF_SIZE, LDG_IO_IPC_PIPE_WAIT_DEFAULT, 0x0);
    if (*out != INVALID_HANDLE_VALUE) { return LDG_ERR_AOK; }

    if (GetLastError() == ERROR_ACCESS_DENIED) { return LDG_ERR_EXISTS; }

    return LDG_ERR_IO_OPEN;
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
    DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
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

        return LDG_ERR_IO_OPEN;
    }

    if (LDG_UNLIKELY(!SetNamedPipeHandleState(pipe, &mode, 0x0, 0x0)))
    {
        if (!CloseHandle(pipe)) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_IO_OPEN;
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

    if (LDG_UNLIKELY(!ipc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (cap == 0) { return LDG_ERR_AOK; }

    if (!PeekNamedPipe((HANDLE)(uintptr_t)ipc->handle, 0x0, LDG_IO_IPC_PEEK_SIZE_NONE, 0x0, &avail, 0x0)) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_RD); }

    if (avail == 0) { return LDG_ERR_AOK; }

    want = (cap < (uint64_t)avail) ? (DWORD)cap : avail;

    if (!ReadFile((HANDLE)(uintptr_t)ipc->handle, dst, want, &got, 0x0)) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_RD); }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_wr(ldg_io_ipc_t *ipc, const ldg_byte_t *src, uint64_t len, uint64_t *out_sent)
{
    DWORD want = 0;
    DWORD sent = 0;

    if (LDG_UNLIKELY(!ipc || !src || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    want = (len < (uint64_t)MAXDWORD) ? (DWORD)len : MAXDWORD;

    if (!WriteFile((HANDLE)(uintptr_t)ipc->handle, src, want, &sent, 0x0)) { return io_ipc_broken_translate(ipc, LDG_ERR_IO_WR); }

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
