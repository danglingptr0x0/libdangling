#include <stdio.h>
#include <windows.h>

#define LDG_IO_PTY_NO_FLAG 0
#define LDG_IO_PTY_PASSTHROUGH_FLAG 0x8
#define LDG_IO_PTY_ATTR_CUNT 1
#define LDG_IO_PTY_ATTR_MAX 256
#define LDG_IO_PTY_CMD_MAX 1024
#define LDG_IO_PTY_PIPE_NO_SIZE 0
#define LDG_IO_PTY_MB_NULL_TERM (-1)
#define LDG_IO_PTY_SHELL_DEFAULT "cmd.exe"
#define LDG_IO_PTY_TERM_DEFAULT "xterm-256color"
#define LDG_IO_PTY_KERNEL_MOD L"kernel32.dll"
#define LDG_IO_PTY_CREATE_SYM "CreatePseudoConsole"
#define LDG_IO_PTY_RESIZE_SYM "ResizePseudoConsole"
#define LDG_IO_PTY_CLOSE_SYM "ClosePseudoConsole"

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

#ifndef EXTENDED_STARTUPINFO_PRESENT
#define EXTENDED_STARTUPINFO_PRESENT 0x00080000
#endif

#include <dangling/io/pty.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

typedef void *ldg_hpcon_t;
typedef HRESULT (WINAPI *ldg_conpty_create_fn)(COORD, HANDLE, HANDLE, DWORD, ldg_hpcon_t *);
typedef HRESULT (WINAPI *ldg_conpty_resize_fn)(ldg_hpcon_t, COORD);
typedef void (WINAPI *ldg_conpty_close_fn)(ldg_hpcon_t);

uint32_t ldg_io_pty_conf_default_get(ldg_io_pty_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->shell = LDG_IO_PTY_SHELL_DEFAULT;
    out->cwd = 0x0;
    out->term_name = LDG_IO_PTY_TERM_DEFAULT;
    out->col_cunt = LDG_IO_PTY_COL_DEFAULT;
    out->row_cunt = LDG_IO_PTY_ROW_DEFAULT;

    return LDG_ERR_AOK;
}

static uint32_t pty_wide_convert(const char *src, WCHAR *out, int32_t cap)
{
    if (MultiByteToWideChar(CP_UTF8, LDG_IO_PTY_NO_FLAG, src, LDG_IO_PTY_MB_NULL_TERM, out, cap) <= 0) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

static uint32_t pty_create_fn_get(ldg_conpty_create_fn *out)
{
    HMODULE mod = 0x0;

    mod = GetModuleHandleW(LDG_IO_PTY_KERNEL_MOD);
    if (LDG_UNLIKELY(!mod)) { return LDG_ERR_UNSUPPORTED; }

    *out = (ldg_conpty_create_fn)(void *)GetProcAddress(mod, LDG_IO_PTY_CREATE_SYM);
    if (LDG_UNLIKELY(!*out)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static uint32_t pty_resize_fn_get(ldg_conpty_resize_fn *out)
{
    HMODULE mod = 0x0;

    mod = GetModuleHandleW(LDG_IO_PTY_KERNEL_MOD);
    if (LDG_UNLIKELY(!mod)) { return LDG_ERR_UNSUPPORTED; }

    *out = (ldg_conpty_resize_fn)(void *)GetProcAddress(mod, LDG_IO_PTY_RESIZE_SYM);
    if (LDG_UNLIKELY(!*out)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static uint32_t pty_close_fn_get(ldg_conpty_close_fn *out)
{
    HMODULE mod = 0x0;

    mod = GetModuleHandleW(LDG_IO_PTY_KERNEL_MOD);
    if (LDG_UNLIKELY(!mod)) { return LDG_ERR_UNSUPPORTED; }

    *out = (ldg_conpty_close_fn)(void *)GetProcAddress(mod, LDG_IO_PTY_CLOSE_SYM);
    if (LDG_UNLIKELY(!*out)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static void pty_state_refresh(ldg_io_pty_t *pty)
{
    DWORD code = 0;

    if (pty->alive != LDG_TRUTH_TRUE) { return; }

    if (GetExitCodeProcess((HANDLE)(uintptr_t)pty->proc_handle, &code) == FALSE)
    {
        pty->alive = LDG_TRUTH_FALSE;
        return;
    }

    if (code == STILL_ACTIVE) { return; }

    pty->exit_code = (uint32_t)code;
    pty->alive = LDG_TRUTH_FALSE;
}

uint32_t ldg_io_pty_open(const ldg_io_pty_conf_t *conf, ldg_io_pty_t *out)
{
    STARTUPINFOEXW si = LDG_STRUCT_ZERO_INIT;
    PROCESS_INFORMATION pi = LDG_STRUCT_ZERO_INIT;
    COORD size = LDG_STRUCT_ZERO_INIT;
    WCHAR cmd[LDG_IO_PTY_CMD_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR dir[LDG_IO_PTY_CMD_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t attr_buff[LDG_IO_PTY_ATTR_MAX] = LDG_ARR_ZERO_INIT;
    ldg_conpty_create_fn create = 0x0;
    ldg_conpty_close_fn destroy = 0x0;
    ldg_hpcon_t hpc = 0x0;
    HANDLE in_rd = INVALID_HANDLE_VALUE;
    HANDLE in_wr = INVALID_HANDLE_VALUE;
    HANDLE rd_end = INVALID_HANDLE_VALUE;
    HANDLE wr_end = INVALID_HANDLE_VALUE;
    LPWSTR dir_ptr = 0x0;
    SIZE_T attr_size = 0;
    uint32_t ret = 0;
    uint8_t pass_on = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_io_pty_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->rd_handle = LDG_IO_PTY_HANDLE_NONE;
    out->wr_handle = LDG_IO_PTY_HANDLE_NONE;
    out->ctl_handle = LDG_IO_PTY_HANDLE_NONE;
    out->proc_handle = LDG_IO_PTY_HANDLE_NONE;

    if (LDG_UNLIKELY(!conf->shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->col_cunt == 0 || conf->col_cunt > LDG_IO_PTY_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->row_cunt == 0 || conf->row_cunt > LDG_IO_PTY_ROW_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = pty_create_fn_get(&create);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = pty_close_fn_get(&destroy);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = pty_wide_convert(conf->shell, cmd, LDG_IO_PTY_CMD_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->cwd)
    {
        ret = pty_wide_convert(conf->cwd, dir, LDG_IO_PTY_CMD_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        dir_ptr = dir;
    }

    if (CreatePipe(&in_rd, &in_wr, 0x0, LDG_IO_PTY_PIPE_NO_SIZE) == FALSE) { return LDG_ERR_IO_OPEN; }

    if (CreatePipe(&rd_end, &wr_end, 0x0, LDG_IO_PTY_PIPE_NO_SIZE) == FALSE)
    {
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        return LDG_ERR_IO_OPEN;
    }

    size.X = (SHORT)conf->col_cunt;
    size.Y = (SHORT)conf->row_cunt;

    pass_on = LDG_TRUTH_TRUE;

    if (FAILED(create(size, in_rd, wr_end, LDG_IO_PTY_PASSTHROUGH_FLAG, &hpc)))
    {
        pass_on = LDG_TRUTH_FALSE;
        LDG_ERRLOG_WARN("io: conpty passthrough refused; vt sequences may be consumed by the host");
    }

    if (pass_on != LDG_TRUTH_TRUE && FAILED(create(size, in_rd, wr_end, LDG_IO_PTY_NO_FLAG, &hpc)))
    {
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        CloseHandle(rd_end);
        CloseHandle(wr_end);
        return LDG_ERR_IO_OPEN;
    }

    CloseHandle(in_rd);
    CloseHandle(wr_end);

    InitializeProcThreadAttributeList(0x0, LDG_IO_PTY_ATTR_CUNT, LDG_IO_PTY_NO_FLAG, &attr_size);

    if (LDG_UNLIKELY(attr_size > (SIZE_T)LDG_IO_PTY_ATTR_MAX))
    {
        destroy(hpc);
        CloseHandle(in_wr);
        CloseHandle(rd_end);
        return LDG_ERR_FULL;
    }

    si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)(void *)attr_buff;

    if (InitializeProcThreadAttributeList(si.lpAttributeList, LDG_IO_PTY_ATTR_CUNT, LDG_IO_PTY_NO_FLAG, &attr_size) == FALSE)
    {
        destroy(hpc);
        CloseHandle(in_wr);
        CloseHandle(rd_end);
        return LDG_ERR_IO_OPEN;
    }

    if (UpdateProcThreadAttribute(si.lpAttributeList, LDG_IO_PTY_NO_FLAG, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpc, sizeof(ldg_hpcon_t), 0x0, 0x0) == FALSE)
    {
        DeleteProcThreadAttributeList(si.lpAttributeList);
        destroy(hpc);
        CloseHandle(in_wr);
        CloseHandle(rd_end);
        return LDG_ERR_IO_OPEN;
    }

    si.StartupInfo.cb = (DWORD)sizeof(STARTUPINFOEXW);

    if (CreateProcessW(0x0, cmd, 0x0, 0x0, FALSE, EXTENDED_STARTUPINFO_PRESENT, 0x0, dir_ptr, &si.StartupInfo, &pi) == FALSE)
    {
        DeleteProcThreadAttributeList(si.lpAttributeList);
        destroy(hpc);
        CloseHandle(in_wr);
        CloseHandle(rd_end);
        return LDG_ERR_IO_OPEN;
    }

    DeleteProcThreadAttributeList(si.lpAttributeList);
    CloseHandle(pi.hThread);

    out->rd_handle = (uint64_t)(uintptr_t)rd_end;
    out->wr_handle = (uint64_t)(uintptr_t)in_wr;
    out->ctl_handle = (uint64_t)(uintptr_t)hpc;
    out->proc_handle = (uint64_t)(uintptr_t)pi.hProcess;
    out->pid = (uint32_t)pi.dwProcessId;
    out->open = LDG_TRUTH_TRUE;
    out->passthrough = pass_on;
    out->alive = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_rd(ldg_io_pty_t *pty, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    DWORD avail = 0;
    DWORD got = 0;
    DWORD want = 0;

    if (LDG_UNLIKELY(!pty || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pty->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    if (PeekNamedPipe((HANDLE)(uintptr_t)pty->rd_handle, 0x0, LDG_IO_PTY_PIPE_NO_SIZE, 0x0, &avail, 0x0) == FALSE)
    {
        pty_state_refresh(pty);
        return LDG_ERR_EOF;
    }

    if (avail == 0) { return LDG_ERR_WOULD_BLOCK; }

    want = (DWORD)cap;
    if ((uint64_t)avail < cap) { want = avail; }

    if (ReadFile((HANDLE)(uintptr_t)pty->rd_handle, buff, want, &got, 0x0) == FALSE)
    {
        pty_state_refresh(pty);
        return LDG_ERR_EOF;
    }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_wr(ldg_io_pty_t *pty, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent)
{
    DWORD sent = 0;

    if (LDG_UNLIKELY(!pty || !buff || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pty->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    if (WriteFile((HANDLE)(uintptr_t)pty->wr_handle, buff, (DWORD)len, &sent, 0x0) == FALSE) { return LDG_ERR_IO_WR; }

    *out_sent = (uint64_t)sent;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_resize(ldg_io_pty_t *pty, uint32_t col_cunt, uint32_t row_cunt)
{
    ldg_conpty_resize_fn resize = 0x0;
    COORD size = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pty)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pty->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    if (LDG_UNLIKELY(col_cunt == 0 || col_cunt > LDG_IO_PTY_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > LDG_IO_PTY_ROW_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = pty_resize_fn_get(&resize);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    size.X = (SHORT)col_cunt;
    size.Y = (SHORT)row_cunt;

    if (FAILED(resize((ldg_hpcon_t)(uintptr_t)pty->ctl_handle, size))) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_exit_code_get(ldg_io_pty_t *pty, uint32_t *out_code)
{
    if (LDG_UNLIKELY(!pty || !out_code)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = 0;

    pty_state_refresh(pty);

    if (pty->alive == LDG_TRUTH_TRUE) { return LDG_ERR_AGAIN; }

    *out_code = pty->exit_code;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_close(ldg_io_pty_t *pty)
{
    ldg_conpty_close_fn destroy = 0x0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!pty)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (pty->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    pty->open = LDG_TRUTH_FALSE;

    ret = pty_close_fn_get(&destroy);
    if (ret == LDG_ERR_AOK) { destroy((ldg_hpcon_t)(uintptr_t)pty->ctl_handle); }

    CloseHandle((HANDLE)(uintptr_t)pty->wr_handle);
    CloseHandle((HANDLE)(uintptr_t)pty->rd_handle);
    CloseHandle((HANDLE)(uintptr_t)pty->proc_handle);

    pty->rd_handle = LDG_IO_PTY_HANDLE_NONE;
    pty->wr_handle = LDG_IO_PTY_HANDLE_NONE;
    pty->ctl_handle = LDG_IO_PTY_HANDLE_NONE;
    pty->proc_handle = LDG_IO_PTY_HANDLE_NONE;
    pty->alive = LDG_TRUTH_FALSE;

    return ret;
}

uint8_t ldg_io_pty_alive_is(ldg_io_pty_t *pty)
{
    if (LDG_UNLIKELY(!pty)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (pty->open != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (pty->alive != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    pty_state_refresh(pty);

    if (pty->alive == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
