#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/wait.h>

#include <dangling/str/str.h>

#define LDG_SYS_PROC_PATH_PROC "/proc"
#define LDG_SYS_PROC_FMT_COMM "/proc/%s/comm"
#define LDG_SYS_PROC_FMT_STAT "/proc/%s/stat"
#define LDG_SYS_PROC_PATH_CAP 128
#define LDG_SYS_PROC_STAT_CAP 256
#define LDG_SYS_PROC_NEWLINE '\n'
#define LDG_SYS_PROC_PAREN_CLOSE ')'
#define LDG_SYS_PROC_SPACE ' '
#define LDG_SYS_PROC_STEP 1
#define LDG_SYS_PROC_DEC_CAP 16
#define LDG_SYS_PROC_NO_PARENT UINT32_MAX
#define LDG_SYS_PROC_PPID_FIELD 2

#define LDG_SYS_PROC_PIPE_RD 0
#define LDG_SYS_PROC_PIPE_WR 1
#define LDG_SYS_PROC_PIPE_CUNT 2
#define LDG_SYS_PROC_EXEC_FAIL 127
#define LDG_SYS_PROC_SIG_SHIFT 128
#define LDG_SYS_PROC_NO_FLAG 0
#define LDG_SYS_PROC_SYSCALL_FAIL (-1)
#define LDG_SYS_PROC_PRIO_KIND_PROC 0
#define LDG_SYS_PROC_PRIO_SELF 0
#define LDG_SYS_PROC_NICE_BIAS 20
#define LDG_SYS_PROC_NICE_IDLE 19
#define LDG_SYS_PROC_NICE_LOW 10
#define LDG_SYS_PROC_NICE_NORMAL 0
#define LDG_SYS_PROC_NICE_HIGH_MAG 10
#define LDG_SYS_PROC_NICE_REALTIME_MAG 20
#define LDG_SYS_PROC_NICE_LOW_EDGE 5
#define LDG_SYS_PROC_NICE_IDLE_EDGE 15
#define LDG_SYS_PROC_MASK_SIZE 8

#include <dangling/sys/proc.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/arch/amd64/syscall.h>

static uint32_t proc_fd_close(uint64_t *slot)
{
    if (*slot == LDG_SYS_PROC_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(close((int32_t)*slot) != 0))
    {
        *slot = LDG_SYS_PROC_HANDLE_NONE;
        return LDG_ERR_IO_CLOSE;
    }

    *slot = LDG_SYS_PROC_HANDLE_NONE;

    return LDG_ERR_AOK;
}

static uint32_t proc_nonblock_set(int32_t fd)
{
    int32_t flags = 0;

    flags = fcntl(fd, F_GETFL, LDG_SYS_PROC_NO_FLAG);
    if (LDG_UNLIKELY(flags == LDG_SYS_PROC_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == LDG_SYS_PROC_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_AOK;
}

static uint32_t proc_pipe_rd(int32_t fd, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    int64_t got = 0;

    *out_got = 0;

    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_CLOSED; }

    got = (int64_t)read(fd, buff, cap);
    if (got > 0)
    {
        *out_got = (uint64_t)got;
        return LDG_ERR_AOK;
    }

    if (got == 0) { return LDG_ERR_EOF; }

    if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

    if (errno == EINTR) { return LDG_ERR_INTERRUPTED; }

    return LDG_ERR_IO_RD;
}

static uint32_t proc_reap(ldg_sys_proc_t *proc)
{
    int32_t status = 0;
    int32_t res = 0;

    if (proc->alive != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    res = (int32_t)waitpid((int32_t)proc->pid, &status, WNOHANG);
    if (res == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(res == LDG_SYS_PROC_SYSCALL_FAIL))
    {
        if (errno == EINTR) { return LDG_ERR_AOK; }

        proc->alive = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    proc->alive = LDG_TRUTH_FALSE;

    if (WIFEXITED(status)) { proc->exit_code = (uint32_t)WEXITSTATUS(status); }
    else if (WIFSIGNALED(status)) { proc->exit_code = LDG_SYS_PROC_SIG_SHIFT + (uint32_t)WTERMSIG(status); }

    return LDG_ERR_AOK;
}

static void proc_pipe_pair_close(int32_t *pair)
{
    close(pair[LDG_SYS_PROC_PIPE_RD]);
    close(pair[LDG_SYS_PROC_PIPE_WR]);
}

uint32_t ldg_sys_proc_spawn(const char *path, const char *const *argv, const char *cwd, ldg_sys_proc_t *out)
{
    int32_t pipe_in[LDG_SYS_PROC_PIPE_CUNT] = LDG_ARR_ZERO_INIT;
    int32_t pipe_out[LDG_SYS_PROC_PIPE_CUNT] = LDG_ARR_ZERO_INIT;
    int32_t pipe_err[LDG_SYS_PROC_PIPE_CUNT] = LDG_ARR_ZERO_INIT;
    int32_t pid = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !argv || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_sys_proc_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->handle = LDG_SYS_PROC_HANDLE_NONE;
    out->pipe_in = LDG_SYS_PROC_HANDLE_NONE;
    out->pipe_out = LDG_SYS_PROC_HANDLE_NONE;
    out->pipe_err = LDG_SYS_PROC_HANDLE_NONE;
    out->exit_code = UINT32_MAX;

    if (LDG_UNLIKELY(pipe(pipe_in) != 0)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(pipe(pipe_out) != 0))
    {
        proc_pipe_pair_close(pipe_in);
        return LDG_ERR_IO_OPEN;
    }

    if (LDG_UNLIKELY(pipe(pipe_err) != 0))
    {
        proc_pipe_pair_close(pipe_in);
        proc_pipe_pair_close(pipe_out);
        return LDG_ERR_IO_OPEN;
    }

    pid = (int32_t)fork();
    if (LDG_UNLIKELY(pid == LDG_SYS_PROC_SYSCALL_FAIL))
    {
        proc_pipe_pair_close(pipe_in);
        proc_pipe_pair_close(pipe_out);
        proc_pipe_pair_close(pipe_err);
        return LDG_ERR_BUSY;
    }

    if (pid == 0)
    {
        if (dup2(pipe_in[LDG_SYS_PROC_PIPE_RD], STDIN_FILENO) == LDG_SYS_PROC_SYSCALL_FAIL) { _exit(LDG_SYS_PROC_EXEC_FAIL); }

        if (dup2(pipe_out[LDG_SYS_PROC_PIPE_WR], STDOUT_FILENO) == LDG_SYS_PROC_SYSCALL_FAIL) { _exit(LDG_SYS_PROC_EXEC_FAIL); }

        if (dup2(pipe_err[LDG_SYS_PROC_PIPE_WR], STDERR_FILENO) == LDG_SYS_PROC_SYSCALL_FAIL) { _exit(LDG_SYS_PROC_EXEC_FAIL); }

        proc_pipe_pair_close(pipe_in);
        proc_pipe_pair_close(pipe_out);
        proc_pipe_pair_close(pipe_err);

        if (cwd != 0x0) { if (chdir(cwd) != 0) { _exit(LDG_SYS_PROC_EXEC_FAIL); } }

        execv(path, (char *const *)argv);

        _exit(LDG_SYS_PROC_EXEC_FAIL);
    }

    close(pipe_in[LDG_SYS_PROC_PIPE_RD]);
    close(pipe_out[LDG_SYS_PROC_PIPE_WR]);
    close(pipe_err[LDG_SYS_PROC_PIPE_WR]);

    ret = proc_nonblock_set(pipe_out[LDG_SYS_PROC_PIPE_RD]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        close(pipe_in[LDG_SYS_PROC_PIPE_WR]);
        close(pipe_out[LDG_SYS_PROC_PIPE_RD]);
        close(pipe_err[LDG_SYS_PROC_PIPE_RD]);
        return ret;
    }

    ret = proc_nonblock_set(pipe_err[LDG_SYS_PROC_PIPE_RD]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        close(pipe_in[LDG_SYS_PROC_PIPE_WR]);
        close(pipe_out[LDG_SYS_PROC_PIPE_RD]);
        close(pipe_err[LDG_SYS_PROC_PIPE_RD]);
        return ret;
    }

    ret = proc_nonblock_set(pipe_in[LDG_SYS_PROC_PIPE_WR]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        close(pipe_in[LDG_SYS_PROC_PIPE_WR]);
        close(pipe_out[LDG_SYS_PROC_PIPE_RD]);
        close(pipe_err[LDG_SYS_PROC_PIPE_RD]);
        return ret;
    }

    out->pid = (uint32_t)pid;
    out->handle = (uint64_t)pid;
    out->pipe_in = (uint64_t)(uint32_t)pipe_in[LDG_SYS_PROC_PIPE_WR];
    out->pipe_out = (uint64_t)(uint32_t)pipe_out[LDG_SYS_PROC_PIPE_RD];
    out->pipe_err = (uint64_t)(uint32_t)pipe_err[LDG_SYS_PROC_PIPE_RD];
    out->alive = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_stdin_wr(ldg_sys_proc_t *proc, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent)
{
    int64_t sent = 0;

    if (LDG_UNLIKELY(!proc || !buff || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(proc->pipe_in == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    if (len == 0) { return LDG_ERR_AOK; }

    sent = (int64_t)write((int32_t)proc->pipe_in, buff, len);
    if (sent >= 0)
    {
        *out_sent = (uint64_t)sent;
        return LDG_ERR_AOK;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

    if (errno == EINTR) { return LDG_ERR_INTERRUPTED; }

    if (errno == EPIPE) { return LDG_ERR_CLOSED; }

    return LDG_ERR_IO_WR;
}

uint32_t ldg_sys_proc_stdout_rd(ldg_sys_proc_t *proc, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    if (LDG_UNLIKELY(!proc || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->pipe_out == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    return proc_pipe_rd((int32_t)proc->pipe_out, buff, cap, out_got);
}

uint32_t ldg_sys_proc_stderr_rd(ldg_sys_proc_t *proc, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    if (LDG_UNLIKELY(!proc || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->pipe_err == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    return proc_pipe_rd((int32_t)proc->pipe_err, buff, cap, out_got);
}

uint32_t ldg_sys_proc_exit_code_get(ldg_sys_proc_t *proc, uint32_t *out_code)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out_code)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = UINT32_MAX;

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    ret = proc_reap(proc);
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

    if (LDG_UNLIKELY(kill((int32_t)proc->pid, SIGKILL) != 0))
    {
        if (errno == ESRCH) { return LDG_ERR_AOK; }

        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_close(ldg_sys_proc_t *proc)
{
    uint32_t ret = 0;
    uint32_t first = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = proc_fd_close(&proc->pipe_in);
    if (ret != LDG_ERR_AOK) { first = ret; }

    ret = proc_fd_close(&proc->pipe_out);
    if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }

    ret = proc_fd_close(&proc->pipe_err);
    if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }

    if (proc->alive == LDG_TRUTH_TRUE)
    {
        ret = proc_reap(proc);
        if (ret != LDG_ERR_AOK && first == LDG_ERR_AOK) { first = ret; }
    }

    proc->handle = LDG_SYS_PROC_HANDLE_NONE;
    proc->alive = LDG_TRUTH_FALSE;

    return first;
}

static uint8_t proc_digits_only_is(const char *text)
{
    uint64_t idx = 0;

    if (text[idx] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    while (text[idx] != LDG_STR_TERM)
    {
        if (ldg_char_digit_is(text[idx]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        idx++;
    }

    return LDG_TRUTH_TRUE;
}

static uint32_t proc_text_rd(const char *path, char *buff, uint64_t cap)
{
    int32_t fd = 0;
    int64_t got = 0;

    buff[0] = LDG_STR_TERM;

    fd = open(path, O_RDONLY);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    got = read(fd, buff, cap - LDG_SYS_PROC_STEP);

    if (LDG_UNLIKELY(close(fd) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (LDG_UNLIKELY(got < 0)) { return LDG_ERR_IO_RD; }

    buff[got] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t proc_parent_get(const char *pid_text)
{
    char path[LDG_SYS_PROC_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char stat[LDG_SYS_PROC_STAT_CAP] = LDG_ARR_ZERO_INIT;
    char text[LDG_SYS_PROC_DEC_CAP] = LDG_ARR_ZERO_INIT;
    dword_t parent = 0;
    uint64_t idx = 0;
    uint64_t field = 0;
    uint64_t span = 0;

    if (snprintf(path, sizeof(path), LDG_SYS_PROC_FMT_STAT, pid_text) < 0) { return LDG_SYS_PROC_NO_PARENT; }

    if (proc_text_rd(path, stat, sizeof(stat)) != LDG_ERR_AOK) { return LDG_SYS_PROC_NO_PARENT; }

    while (stat[idx] != LDG_STR_TERM && stat[idx] != LDG_SYS_PROC_PAREN_CLOSE) { idx++; }

    while (stat[idx] != LDG_STR_TERM && field < LDG_SYS_PROC_PPID_FIELD)
    {
        if (stat[idx] == LDG_SYS_PROC_SPACE) { field++; }

        idx++;
    }

    span = 0;

    while (ldg_char_digit_is(stat[idx]) == LDG_TRUTH_TRUE && span + LDG_SYS_PROC_STEP < LDG_SYS_PROC_DEC_CAP)
    {
        text[span] = stat[idx];
        span++;
        idx++;
    }

    text[span] = LDG_STR_TERM;

    if (span == 0) { return LDG_SYS_PROC_NO_PARENT; }

    if (ldg_str_to_dec(text, &parent) != LDG_ERR_AOK) { return LDG_SYS_PROC_NO_PARENT; }

    return parent;
}

uint32_t ldg_sys_proc_list(ldg_sys_proc_info_t *out, uint64_t cap, uint64_t *out_cunt)
{
    DIR *dir = 0x0;
    struct dirent *ent = 0x0;
    char path[LDG_SYS_PROC_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char comm[LDG_SYS_PROC_NAME_MAX] = LDG_ARR_ZERO_INIT;
    dword_t pid = 0;
    uint64_t found = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dir = opendir(LDG_SYS_PROC_PATH_PROC);
    if (LDG_UNLIKELY(!dir)) { return LDG_ERR_IO_OPEN; }

    ent = readdir(dir);

    while (ent != 0x0 && found < cap)
    {
        if (proc_digits_only_is(ent->d_name) == LDG_TRUTH_TRUE && snprintf(path, sizeof(path), LDG_SYS_PROC_FMT_COMM, ent->d_name) > 0 && proc_text_rd(path, comm, sizeof(comm)) == LDG_ERR_AOK)
        {
            for (idx = 0; comm[idx] != LDG_STR_TERM; idx++) { if (comm[idx] == LDG_SYS_PROC_NEWLINE)
                {
                    comm[idx] = LDG_STR_TERM;
                    break;
                }
            }

            out[found].pid = LDG_SYS_PROC_PID_NONE;
            out[found].parent_pid = LDG_SYS_PROC_PID_NONE;

            if (ldg_str_to_dec(ent->d_name, &pid) != LDG_ERR_AOK)
            {
                ent = readdir(dir);
                continue;
            }

            out[found].pid = pid;
            out[found].parent_pid = proc_parent_get(ent->d_name);

            for (idx = 0; idx < LDG_SYS_PROC_NAME_MAX; idx++) { out[found].name[idx] = comm[idx]; }

            out[found].name[LDG_SYS_PROC_NAME_MAX - LDG_SYS_PROC_STEP] = LDG_STR_TERM;
            found++;
        }

        ent = readdir(dir);
    }

    if (LDG_UNLIKELY(closedir(dir) != 0)) { return LDG_ERR_IO_CLOSE; }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint8_t ldg_sys_proc_name_running_is(const char *name)
{
    DIR *dir = 0x0;
    struct dirent *ent = 0x0;
    char path[LDG_SYS_PROC_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char comm[LDG_SYS_PROC_NAME_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t idx = 0;
    uint8_t res = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    dir = opendir(LDG_SYS_PROC_PATH_PROC);
    if (LDG_UNLIKELY(!dir)) { return LDG_ERR_IO_OPEN; }

    ent = readdir(dir);

    while (ent != 0x0 && res != LDG_TRUTH_TRUE)
    {
        if (proc_digits_only_is(ent->d_name) == LDG_TRUTH_TRUE && snprintf(path, sizeof(path), LDG_SYS_PROC_FMT_COMM, ent->d_name) > 0 && proc_text_rd(path, comm, sizeof(comm)) == LDG_ERR_AOK)
        {
            for (idx = 0; comm[idx] != LDG_STR_TERM; idx++) { if (comm[idx] == LDG_SYS_PROC_NEWLINE)
                {
                    comm[idx] = LDG_STR_TERM;
                    break;
                }
            }

            if (strcmp(comm, name) == 0) { res = LDG_TRUTH_TRUE; }
        }

        ent = readdir(dir);
    }

    if (LDG_UNLIKELY(closedir(dir) != 0)) { return LDG_ERR_IO_CLOSE; }

    return res;
}

uint8_t ldg_sys_proc_alive_is(ldg_sys_proc_t *proc)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (proc->handle == LDG_SYS_PROC_HANDLE_NONE) { return LDG_TRUTH_FALSE; }

    if (proc->alive != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ret = proc_reap(proc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    if (proc->alive == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t proc_nice_get(ldg_sys_proc_priority_t lvl, int64_t *out)
{
    int64_t mag = 0;

    if (lvl == LDG_SYS_PROC_PRIORITY_IDLE)
    {
        *out = LDG_SYS_PROC_NICE_IDLE;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_LOW)
    {
        *out = LDG_SYS_PROC_NICE_LOW;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_NORMAL)
    {
        *out = LDG_SYS_PROC_NICE_NORMAL;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_HIGH)
    {
        mag = LDG_SYS_PROC_NICE_HIGH_MAG;
        *out = -mag;
        return LDG_ERR_AOK;
    }

    if (lvl == LDG_SYS_PROC_PRIORITY_REALTIME)
    {
        mag = LDG_SYS_PROC_NICE_REALTIME_MAG;
        *out = -mag;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static ldg_sys_proc_priority_t proc_priority_classify(int64_t nice)
{
    int64_t low_edge = LDG_SYS_PROC_NICE_LOW_EDGE;
    int64_t realtime_mag = LDG_SYS_PROC_NICE_REALTIME_MAG;
    int64_t high_edge = 0;
    int64_t realtime_edge = 0;

    high_edge = -low_edge;
    realtime_edge = -realtime_mag;

    if (nice >= LDG_SYS_PROC_NICE_IDLE_EDGE) { return LDG_SYS_PROC_PRIORITY_IDLE; }

    if (nice >= low_edge) { return LDG_SYS_PROC_PRIORITY_LOW; }

    if (nice > high_edge) { return LDG_SYS_PROC_PRIORITY_NORMAL; }

    if (nice > realtime_edge) { return LDG_SYS_PROC_PRIORITY_HIGH; }

    return LDG_SYS_PROC_PRIORITY_REALTIME;
}

uint32_t ldg_sys_proc_self_priority_set(ldg_sys_proc_priority_t lvl)
{
    int64_t nice = 0;
    int64_t ret = 0;
    uint32_t mapped = 0;

    mapped = proc_nice_get(lvl, &nice);
    if (LDG_UNLIKELY(mapped != LDG_ERR_AOK)) { return mapped; }

    ret = ldg_syscall3(LDG_SYS_SETPRIORITY, LDG_SYS_PROC_PRIO_KIND_PROC, LDG_SYS_PROC_PRIO_SELF, nice);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_self_priority_get(ldg_sys_proc_priority_t *out)
{
    int64_t ret = 0;
    int64_t nice = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_SYS_PROC_PRIORITY_NONE;

    ret = ldg_syscall2(LDG_SYS_GETPRIORITY, LDG_SYS_PROC_PRIO_KIND_PROC, LDG_SYS_PROC_PRIO_SELF);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_DENIED; }

    nice = LDG_SYS_PROC_NICE_BIAS - ret;

    *out = proc_priority_classify(nice);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_self_affinity_set(uint64_t mask)
{
    uint64_t local = mask;
    int64_t ret = 0;

    if (LDG_UNLIKELY(mask == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_syscall3(LDG_SYS_SCHED_SETAFFINITY, LDG_SYS_PROC_PRIO_SELF, LDG_SYS_PROC_MASK_SIZE, (int64_t)(uintptr_t)&local);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_proc_self_affinity_get(uint64_t *out)
{
    uint64_t local = 0;
    int64_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = ldg_syscall3(LDG_SYS_SCHED_GETAFFINITY, LDG_SYS_PROC_PRIO_SELF, LDG_SYS_PROC_MASK_SIZE, (int64_t)(uintptr_t)&local);
    if (LDG_UNLIKELY(ret < 0)) { return LDG_ERR_DENIED; }

    *out = local;

    return LDG_ERR_AOK;
}
