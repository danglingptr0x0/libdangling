#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

#define LDG_SYS_PROC_SNAP_NO_PID 0
#define LDG_SYS_PROC_CASE_SHIFT 32
#define LDG_SYS_PROC_UPPER_LOW 'A'
#define LDG_SYS_PROC_UPPER_HIGH 'Z'

#define LDG_SYS_PROC_CMD_MAX 8192
#define LDG_SYS_PROC_PATH_MAX 1024
#define LDG_SYS_PROC_MB_NO_FLAG 0
#define LDG_SYS_PROC_MB_NULL_TERM (-1)
#define LDG_SYS_PROC_PIPE_NO_SIZE 0
#define LDG_SYS_PROC_QUOTE_CHAR '"'
#define LDG_SYS_PROC_ESCAPE_CHAR '\\'
#define LDG_SYS_PROC_SPACE_CHAR ' '
#define LDG_SYS_PROC_TAB_CHAR '\t'
#define LDG_SYS_PROC_KILL_CODE 137
#define LDG_SYS_PROC_STEP 1

#include <dangling/sys/proc.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

static uint8_t proc_quote_needed_is(const char *arg)
{
    uint64_t idx = 0;

    if (arg[0] == LDG_STR_TERM) { return LDG_TRUTH_TRUE; }

    for (idx = 0; arg[idx] != LDG_STR_TERM; idx++)
    {
        if (arg[idx] == LDG_SYS_PROC_SPACE_CHAR) { return LDG_TRUTH_TRUE; }

        if (arg[idx] == LDG_SYS_PROC_TAB_CHAR) { return LDG_TRUTH_TRUE; }

        if (arg[idx] == LDG_SYS_PROC_QUOTE_CHAR) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t proc_arg_append(char *cmd, uint64_t cap, uint64_t *pos, const char *arg)
{
    uint64_t idx = 0;
    uint64_t slash = 0;
    uint64_t run = 0;

    if (proc_quote_needed_is(arg) == LDG_TRUTH_FALSE)
    {
        for (idx = 0; arg[idx] != LDG_STR_TERM; idx++)
        {
            if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

            cmd[*pos] = arg[idx];
            *pos += LDG_SYS_PROC_STEP;
        }

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

    cmd[*pos] = LDG_SYS_PROC_QUOTE_CHAR;
    *pos += LDG_SYS_PROC_STEP;

    for (idx = 0; arg[idx] != LDG_STR_TERM; idx++)
    {
        if (arg[idx] == LDG_SYS_PROC_ESCAPE_CHAR)
        {
            run = 0;
            while (arg[idx] == LDG_SYS_PROC_ESCAPE_CHAR)
            {
                run += LDG_SYS_PROC_STEP;
                idx += LDG_SYS_PROC_STEP;
            }

            if (arg[idx] == LDG_STR_TERM) { run *= 2; }
            else if (arg[idx] == LDG_SYS_PROC_QUOTE_CHAR) { run = (run * 2) + LDG_SYS_PROC_STEP; }

            for (slash = 0; slash < run; slash++)
            {
                if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

                cmd[*pos] = LDG_SYS_PROC_ESCAPE_CHAR;
                *pos += LDG_SYS_PROC_STEP;
            }

            if (arg[idx] == LDG_STR_TERM) { break; }

            if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

            cmd[*pos] = arg[idx];
            *pos += LDG_SYS_PROC_STEP;
            continue;
        }

        if (arg[idx] == LDG_SYS_PROC_QUOTE_CHAR)
        {
            if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

            cmd[*pos] = LDG_SYS_PROC_ESCAPE_CHAR;
            *pos += LDG_SYS_PROC_STEP;
        }

        if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

        cmd[*pos] = arg[idx];
        *pos += LDG_SYS_PROC_STEP;
    }

    if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

    cmd[*pos] = LDG_SYS_PROC_QUOTE_CHAR;
    *pos += LDG_SYS_PROC_STEP;

    return LDG_ERR_AOK;
}

static uint32_t proc_cmd_build(const char *const *argv, char *cmd, uint64_t cap)
{
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    cmd[0] = LDG_STR_TERM;

    for (idx = 0; argv[idx] != 0x0; idx++)
    {
        if (LDG_UNLIKELY(idx >= LDG_SYS_PROC_ARG_MAX)) { return LDG_ERR_OVERFLOW; }

        if (idx > 0)
        {
            if (LDG_UNLIKELY(pos + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

            cmd[pos] = LDG_SYS_PROC_SPACE_CHAR;
            pos += LDG_SYS_PROC_STEP;
        }

        ret = proc_arg_append(cmd, cap, &pos, argv[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    cmd[pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t proc_handle_close(uint64_t *slot)
{
    if (*slot == LDG_SYS_PROC_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!CloseHandle((HANDLE)(uintptr_t)*slot)))
    {
        *slot = LDG_SYS_PROC_HANDLE_NONE;
        return LDG_ERR_IO_CLOSE;
    }

    *slot = LDG_SYS_PROC_HANDLE_NONE;

    return LDG_ERR_AOK;
}

static uint32_t proc_pipe_rd(HANDLE pipe, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    DWORD avail = 0;
    DWORD got = 0;

    *out_got = 0;

    if (!PeekNamedPipe(pipe, 0x0, LDG_SYS_PROC_PIPE_NO_SIZE, 0x0, &avail, 0x0))
    {
        if (GetLastError() == ERROR_BROKEN_PIPE) { return LDG_ERR_EOF; }

        return LDG_ERR_IO_RD;
    }

    if (avail == 0) { return LDG_ERR_WOULD_BLOCK; }

    if ((uint64_t)avail > cap) { avail = (DWORD)cap; }

    if (!ReadFile(pipe, buff, avail, &got, 0x0))
    {
        if (GetLastError() == ERROR_BROKEN_PIPE) { return LDG_ERR_EOF; }

        return LDG_ERR_IO_RD;
    }

    if (got == 0) { return LDG_ERR_EOF; }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

static uint32_t proc_state_refresh(ldg_sys_proc_t *proc)
{
    DWORD code = 0;

    if (proc->alive != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!GetExitCodeProcess((HANDLE)(uintptr_t)proc->handle, &code))) { return LDG_ERR_NOT_FOUND; }

    if (code == STILL_ACTIVE) { return LDG_ERR_AOK; }

    proc->alive = LDG_TRUTH_FALSE;
    proc->exit_code = (uint32_t)code;

    return LDG_ERR_AOK;
}

static uint32_t proc_wide_convert(const char *src, WCHAR *out, int32_t cap)
{
    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_SYS_PROC_MB_NO_FLAG, src, LDG_SYS_PROC_MB_NULL_TERM, out, cap) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_spawn(const char *path, const char *const *argv, const char *cwd, ldg_sys_proc_t *out)
{
    SECURITY_ATTRIBUTES sec = LDG_STRUCT_ZERO_INIT;
    STARTUPINFOW start = LDG_STRUCT_ZERO_INIT;
    PROCESS_INFORMATION info = LDG_STRUCT_ZERO_INIT;
    char cmd[LDG_SYS_PROC_CMD_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR wide_cmd[LDG_SYS_PROC_CMD_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR wide_path[LDG_SYS_PROC_PATH_MAX] = LDG_ARR_ZERO_INIT;
    WCHAR wide_cwd[LDG_SYS_PROC_PATH_MAX] = LDG_ARR_ZERO_INIT;
    HANDLE in_rd = INVALID_HANDLE_VALUE;
    HANDLE in_wr = INVALID_HANDLE_VALUE;
    HANDLE out_rd = INVALID_HANDLE_VALUE;
    HANDLE out_wr = INVALID_HANDLE_VALUE;
    HANDLE err_rd = INVALID_HANDLE_VALUE;
    HANDLE err_wr = INVALID_HANDLE_VALUE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !argv || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_sys_proc_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->handle = LDG_SYS_PROC_HANDLE_NONE;
    out->pipe_in = LDG_SYS_PROC_HANDLE_NONE;
    out->pipe_out = LDG_SYS_PROC_HANDLE_NONE;
    out->pipe_err = LDG_SYS_PROC_HANDLE_NONE;
    out->exit_code = UINT32_MAX;

    ret = proc_cmd_build(argv, cmd, LDG_SYS_PROC_CMD_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = proc_wide_convert(path, wide_path, LDG_SYS_PROC_PATH_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = proc_wide_convert(cmd, wide_cmd, LDG_SYS_PROC_CMD_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (cwd != 0x0)
    {
        ret = proc_wide_convert(cwd, wide_cwd, LDG_SYS_PROC_PATH_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    sec.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec.bInheritHandle = TRUE;
    sec.lpSecurityDescriptor = 0x0;

    if (LDG_UNLIKELY(!CreatePipe(&in_rd, &in_wr, &sec, LDG_SYS_PROC_PIPE_NO_SIZE))) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(!CreatePipe(&out_rd, &out_wr, &sec, LDG_SYS_PROC_PIPE_NO_SIZE)))
    {
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        return LDG_ERR_IO_OPEN;
    }

    if (LDG_UNLIKELY(!CreatePipe(&err_rd, &err_wr, &sec, LDG_SYS_PROC_PIPE_NO_SIZE)))
    {
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        CloseHandle(out_rd);
        CloseHandle(out_wr);
        return LDG_ERR_IO_OPEN;
    }

    SetHandleInformation(in_wr, HANDLE_FLAG_INHERIT, LDG_SYS_PROC_PIPE_NO_SIZE);
    SetHandleInformation(out_rd, HANDLE_FLAG_INHERIT, LDG_SYS_PROC_PIPE_NO_SIZE);
    SetHandleInformation(err_rd, HANDLE_FLAG_INHERIT, LDG_SYS_PROC_PIPE_NO_SIZE);

    start.cb = sizeof(STARTUPINFOW);
    start.dwFlags = STARTF_USESTDHANDLES;
    start.hStdInput = in_rd;
    start.hStdOutput = out_wr;
    start.hStdError = err_wr;

    if (LDG_UNLIKELY(!CreateProcessW(wide_path, wide_cmd, 0x0, 0x0, TRUE, CREATE_NO_WINDOW, 0x0, (cwd != 0x0) ? wide_cwd : 0x0, &start, &info)))
    {
        CloseHandle(in_rd);
        CloseHandle(in_wr);
        CloseHandle(out_rd);
        CloseHandle(out_wr);
        CloseHandle(err_rd);
        CloseHandle(err_wr);
        return LDG_ERR_IO_NOT_FOUND;
    }

    CloseHandle(in_rd);
    CloseHandle(out_wr);
    CloseHandle(err_wr);
    CloseHandle(info.hThread);

    out->handle = (uint64_t)(uintptr_t)info.hProcess;
    out->pid = (uint32_t)info.dwProcessId;
    out->pipe_in = (uint64_t)(uintptr_t)in_wr;
    out->pipe_out = (uint64_t)(uintptr_t)out_rd;
    out->pipe_err = (uint64_t)(uintptr_t)err_rd;
    out->alive = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_stdin_wr(ldg_sys_proc_t *proc, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent)
{
    DWORD sent = 0;

    if (LDG_UNLIKELY(!proc || !buff || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(proc->pipe_in == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!WriteFile((HANDLE)(uintptr_t)proc->pipe_in, buff, (DWORD)len, &sent, 0x0))
    {
        if (GetLastError() == ERROR_BROKEN_PIPE) { return LDG_ERR_CLOSED; }

        return LDG_ERR_IO_WR;
    }

    *out_sent = (uint64_t)sent;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_stdout_rd(ldg_sys_proc_t *proc, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    if (LDG_UNLIKELY(!proc || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->pipe_out == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    return proc_pipe_rd((HANDLE)(uintptr_t)proc->pipe_out, buff, cap, out_got);
}

uint32_t ldg_sys_proc_stderr_rd(ldg_sys_proc_t *proc, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    if (LDG_UNLIKELY(!proc || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->pipe_err == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    return proc_pipe_rd((HANDLE)(uintptr_t)proc->pipe_err, buff, cap, out_got);
}

uint32_t ldg_sys_proc_exit_code_get(ldg_sys_proc_t *proc, uint32_t *out_code)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out_code)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = UINT32_MAX;

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    ret = proc_state_refresh(proc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (proc->alive == LDG_TRUTH_TRUE) { return LDG_ERR_AGAIN; }

    *out_code = proc->exit_code;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_kill(ldg_sys_proc_t *proc)
{
    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (proc->alive != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!TerminateProcess((HANDLE)(uintptr_t)proc->handle, LDG_SYS_PROC_KILL_CODE)))
    {
        if (GetLastError() == ERROR_ACCESS_DENIED) { return LDG_ERR_AOK; }

        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_close(ldg_sys_proc_t *proc)
{
    uint32_t ret = 0;
    uint32_t first = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = proc_handle_close(&proc->pipe_in);
    if (ret != LDG_ERR_AOK) { first = ret; }

    ret = proc_handle_close(&proc->pipe_out);
    if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }

    ret = proc_handle_close(&proc->pipe_err);
    if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }

    ret = proc_handle_close(&proc->handle);
    if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }

    proc->alive = LDG_TRUTH_FALSE;

    return first;
}

static char proc_lower_get(char raw)
{
    char res = raw;

    if (raw >= LDG_SYS_PROC_UPPER_LOW && raw <= LDG_SYS_PROC_UPPER_HIGH) { res = (char)(raw + LDG_SYS_PROC_CASE_SHIFT); }

    return res;
}

static uint8_t proc_name_match_is(const char *lhs, const char *rhs)
{
    uint64_t idx = 0;

    while (lhs[idx] != LDG_STR_TERM && rhs[idx] != LDG_STR_TERM)
    {
        if (proc_lower_get(lhs[idx]) != proc_lower_get(rhs[idx])) { return LDG_TRUTH_FALSE; }

        idx++;
    }

    return (lhs[idx] == LDG_STR_TERM && rhs[idx] == LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

uint32_t ldg_sys_proc_list(ldg_sys_proc_info_t *out, uint64_t cap, uint64_t *out_cunt)
{
    PROCESSENTRY32W entry = LDG_STRUCT_ZERO_INIT;
    HANDLE snap = INVALID_HANDLE_VALUE;
    uint64_t found = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, LDG_SYS_PROC_SNAP_NO_PID);
    if (LDG_UNLIKELY(snap == INVALID_HANDLE_VALUE)) { return LDG_ERR_DENIED; }

    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snap, &entry))
    {
        if (LDG_UNLIKELY(!CloseHandle(snap))) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_NOT_FOUND;
    }

    while (found < cap)
    {
        out[found].pid = LDG_SYS_PROC_PID_NONE;
        out[found].parent_pid = LDG_SYS_PROC_PID_NONE;
        out[found].name[0] = LDG_STR_TERM;

        if (WideCharToMultiByte(CP_UTF8, LDG_SYS_PROC_MB_NO_FLAG, entry.szExeFile, LDG_SYS_PROC_MB_NULL_TERM, out[found].name, LDG_SYS_PROC_NAME_MAX, 0x0, 0x0) > 0)
        {
            out[found].pid = (uint32_t)entry.th32ProcessID;
            out[found].parent_pid = (uint32_t)entry.th32ParentProcessID;
            found++;
        }

        if (!Process32NextW(snap, &entry)) { break; }
    }

    if (LDG_UNLIKELY(!CloseHandle(snap))) { return LDG_ERR_IO_CLOSE; }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint8_t ldg_sys_proc_name_running_is(const char *name)
{
    PROCESSENTRY32W entry = LDG_STRUCT_ZERO_INIT;
    HANDLE snap = INVALID_HANDLE_VALUE;
    char text[LDG_SYS_PROC_NAME_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t res = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, LDG_SYS_PROC_SNAP_NO_PID);
    if (LDG_UNLIKELY(snap == INVALID_HANDLE_VALUE)) { return LDG_ERR_DENIED; }

    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snap, &entry))
    {
        if (LDG_UNLIKELY(!CloseHandle(snap))) { return LDG_ERR_IO_CLOSE; }

        return LDG_TRUTH_FALSE;
    }

    while (res != LDG_TRUTH_TRUE)
    {
        if (WideCharToMultiByte(CP_UTF8, LDG_SYS_PROC_MB_NO_FLAG, entry.szExeFile, LDG_SYS_PROC_MB_NULL_TERM, text, LDG_SYS_PROC_NAME_MAX, 0x0, 0x0) > 0) { if (proc_name_match_is(text, name) == LDG_TRUTH_TRUE) { res = LDG_TRUTH_TRUE; } }

        if (!Process32NextW(snap, &entry)) { break; }
    }

    if (LDG_UNLIKELY(!CloseHandle(snap))) { return LDG_ERR_IO_CLOSE; }

    return res;
}

uint8_t ldg_sys_proc_alive_is(ldg_sys_proc_t *proc)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (proc->handle == LDG_SYS_PROC_HANDLE_NONE) { return LDG_TRUTH_FALSE; }

    if (proc->alive != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ret = proc_state_refresh(proc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_NOT_FOUND; }

    if (proc->alive == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t proc_class_get(ldg_sys_proc_priority_t lvl, DWORD *out)
{
    if (lvl == LDG_SYS_PROC_PRIORITY_IDLE)
    {
        *out = IDLE_PRIORITY_CLASS;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_LOW)
    {
        *out = BELOW_NORMAL_PRIORITY_CLASS;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_NORMAL)
    {
        *out = NORMAL_PRIORITY_CLASS;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_HIGH)
    {
        *out = HIGH_PRIORITY_CLASS;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_REALTIME)
    {
        *out = REALTIME_PRIORITY_CLASS;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static ldg_sys_proc_priority_t proc_priority_classify(DWORD cls)
{
    if (cls == IDLE_PRIORITY_CLASS) { return LDG_SYS_PROC_PRIORITY_IDLE; }

    if (cls == BELOW_NORMAL_PRIORITY_CLASS) { return LDG_SYS_PROC_PRIORITY_LOW; }

    if (cls == NORMAL_PRIORITY_CLASS) { return LDG_SYS_PROC_PRIORITY_NORMAL; }

    if (cls == ABOVE_NORMAL_PRIORITY_CLASS) { return LDG_SYS_PROC_PRIORITY_HIGH; }

    if (cls == HIGH_PRIORITY_CLASS) { return LDG_SYS_PROC_PRIORITY_HIGH; }

    if (cls == REALTIME_PRIORITY_CLASS) { return LDG_SYS_PROC_PRIORITY_REALTIME; }

    return LDG_SYS_PROC_PRIORITY_NONE;
}

uint32_t ldg_sys_proc_self_priority_set(ldg_sys_proc_priority_t lvl)
{
    DWORD cls = 0;
    uint32_t ret = 0;

    ret = proc_class_get(lvl, &cls);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (SetPriorityClass(GetCurrentProcess(), cls) == FALSE) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_self_priority_get(ldg_sys_proc_priority_t *out)
{
    DWORD cls = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_SYS_PROC_PRIORITY_NONE;

    cls = GetPriorityClass(GetCurrentProcess());
    if (LDG_UNLIKELY(cls == 0)) { return LDG_ERR_DENIED; }

    *out = proc_priority_classify(cls);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_self_affinity_set(uint64_t mask)
{
    if (LDG_UNLIKELY(mask == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)mask) == FALSE) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_self_affinity_get(uint64_t *out)
{
    DWORD_PTR proc_mask = 0;
    DWORD_PTR sys_mask = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask) == FALSE) { return LDG_ERR_DENIED; }

    *out = (uint64_t)proc_mask;

    return LDG_ERR_AOK;
}
