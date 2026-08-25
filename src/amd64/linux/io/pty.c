#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define LDG_IO_PTY_SYSCALL_FAIL (-1)
#define LDG_IO_PTY_NO_FLAG 0
#define LDG_IO_PTY_UNLOCK 0
#define LDG_IO_PTY_CHILD 0
#define LDG_IO_PTY_CTTY_ARG 0
#define LDG_IO_PTY_ENV_OVERWRITE 1
#define LDG_IO_PTY_STDIN_FD 0
#define LDG_IO_PTY_STDOUT_FD 1
#define LDG_IO_PTY_STDERR_FD 2
#define LDG_IO_PTY_EXEC_FAIL 127
#define LDG_IO_PTY_SIG_SHIFT 128
#define LDG_IO_PTY_NODE_MAX 64
#define LDG_IO_PTY_SHELL_DEFAULT "/bin/sh"
#define LDG_IO_PTY_TERM_DEFAULT "xterm-256color"
#define LDG_IO_PTY_PTMX_PATH "/dev/ptmx"
#define LDG_IO_PTY_SLAVE_FMT "/dev/pts/%" PRIu32
#define LDG_IO_PTY_SHELL_FLAG "-i"
#define LDG_IO_PTY_TERM_ENV "TERM"

#include <dangling/io/pty.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

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

static uint32_t pty_winsize_apply(int32_t fd, uint32_t col_cunt, uint32_t row_cunt)
{
    struct winsize ws = LDG_STRUCT_ZERO_INIT;

    ws.ws_col = (uint16_t)col_cunt;
    ws.ws_row = (uint16_t)row_cunt;

    if (ioctl(fd, TIOCSWINSZ, &ws) == LDG_IO_PTY_SYSCALL_FAIL) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

static void pty_reap(ldg_io_pty_t *pty)
{
    int32_t status = 0;
    pid_t seen = 0;

    if (pty->alive != LDG_TRUTH_TRUE) { return; }

    seen = waitpid((pid_t)pty->pid, &status, WNOHANG);
    if (seen == LDG_IO_PTY_SYSCALL_FAIL)
    {
        pty->alive = LDG_TRUTH_FALSE;
        return;
    }

    if (seen == LDG_IO_PTY_CHILD) { return; }

    if (WIFEXITED(status)) { pty->exit_code = (uint32_t)WEXITSTATUS(status); }
    else if (WIFSIGNALED(status)) { pty->exit_code = LDG_IO_PTY_SIG_SHIFT + (uint32_t)WTERMSIG(status); }

    pty->alive = LDG_TRUTH_FALSE;
}

static void pty_child_enter(const ldg_io_pty_conf_t *conf, int32_t master, int32_t slave)
{
    if (setsid() == LDG_IO_PTY_SYSCALL_FAIL) { _exit(LDG_IO_PTY_EXEC_FAIL); }

    if (ioctl(slave, TIOCSCTTY, LDG_IO_PTY_CTTY_ARG) == LDG_IO_PTY_SYSCALL_FAIL) { _exit(LDG_IO_PTY_EXEC_FAIL); }

    if (dup2(slave, LDG_IO_PTY_STDIN_FD) == LDG_IO_PTY_SYSCALL_FAIL) { _exit(LDG_IO_PTY_EXEC_FAIL); }

    if (dup2(slave, LDG_IO_PTY_STDOUT_FD) == LDG_IO_PTY_SYSCALL_FAIL) { _exit(LDG_IO_PTY_EXEC_FAIL); }

    if (dup2(slave, LDG_IO_PTY_STDERR_FD) == LDG_IO_PTY_SYSCALL_FAIL) { _exit(LDG_IO_PTY_EXEC_FAIL); }

    if (slave > LDG_IO_PTY_STDERR_FD) { close(slave); }

    close(master);

    if (conf->term_name) { setenv(LDG_IO_PTY_TERM_ENV, conf->term_name, LDG_IO_PTY_ENV_OVERWRITE); }

    if (conf->cwd) { if (chdir(conf->cwd) == LDG_IO_PTY_SYSCALL_FAIL) { _exit(LDG_IO_PTY_EXEC_FAIL); } }

    execl(conf->shell, conf->shell, LDG_IO_PTY_SHELL_FLAG, (char *)0x0);

    _exit(LDG_IO_PTY_EXEC_FAIL);
}

uint32_t ldg_io_pty_open(const ldg_io_pty_conf_t *conf, ldg_io_pty_t *out)
{
    char node[LDG_IO_PTY_NODE_MAX] = LDG_ARR_ZERO_INIT;
    int32_t master = 0;
    int32_t slave = 0;
    int32_t unlock = LDG_IO_PTY_UNLOCK;
    uint32_t idx = 0;
    uint32_t ret = 0;
    int32_t len = 0;
    pid_t child = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_io_pty_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->rd_handle = LDG_IO_PTY_HANDLE_NONE;
    out->wr_handle = LDG_IO_PTY_HANDLE_NONE;
    out->ctl_handle = LDG_IO_PTY_HANDLE_NONE;
    out->proc_handle = LDG_IO_PTY_HANDLE_NONE;

    if (LDG_UNLIKELY(!conf->shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->col_cunt == 0 || conf->col_cunt > LDG_IO_PTY_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->row_cunt == 0 || conf->row_cunt > LDG_IO_PTY_ROW_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    master = open(LDG_IO_PTY_PTMX_PATH, O_RDWR | O_NOCTTY);
    if (LDG_UNLIKELY(master < 0)) { return LDG_ERR_IO_OPEN; }

    if (ioctl(master, TIOCSPTLCK, &unlock) == LDG_IO_PTY_SYSCALL_FAIL)
    {
        close(master);
        return LDG_ERR_IO_OPEN;
    }

    if (ioctl(master, TIOCGPTN, &idx) == LDG_IO_PTY_SYSCALL_FAIL)
    {
        close(master);
        return LDG_ERR_IO_OPEN;
    }

    len = snprintf(node, LDG_IO_PTY_NODE_MAX, LDG_IO_PTY_SLAVE_FMT, idx);
    if (LDG_UNLIKELY(len <= 0))
    {
        close(master);
        return LDG_ERR_IO_FMT;
    }

    slave = open(node, O_RDWR | O_NOCTTY);
    if (LDG_UNLIKELY(slave < 0))
    {
        close(master);
        return LDG_ERR_IO_OPEN;
    }

    ret = pty_winsize_apply(slave, conf->col_cunt, conf->row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        close(slave);
        close(master);
        return ret;
    }

    child = fork();
    if (child == LDG_IO_PTY_SYSCALL_FAIL)
    {
        close(slave);
        close(master);
        return LDG_ERR_IO_OPEN;
    }

    if (child == LDG_IO_PTY_CHILD) { pty_child_enter(conf, master, slave); }

    close(slave);

    if (fcntl(master, F_SETFL, O_NONBLOCK) == LDG_IO_PTY_SYSCALL_FAIL)
    {
        close(master);
        return LDG_ERR_IO_OPEN;
    }

    out->rd_handle = (uint64_t)(uint32_t)master;
    out->wr_handle = (uint64_t)(uint32_t)master;
    out->pid = (uint32_t)child;
    out->open = LDG_TRUTH_TRUE;
    out->alive = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_rd(ldg_io_pty_t *pty, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    int64_t got = 0;

    if (LDG_UNLIKELY(!pty || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pty->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    got = (int64_t)read((int32_t)pty->rd_handle, buff, cap);
    if (got == LDG_IO_PTY_SYSCALL_FAIL)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

        if (errno == EIO)
        {
            pty_reap(pty);
            return LDG_ERR_EOF;
        }

        return LDG_ERR_IO_RD;
    }

    if (got == 0)
    {
        pty_reap(pty);
        return LDG_ERR_EOF;
    }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_wr(ldg_io_pty_t *pty, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent)
{
    int64_t sent = 0;

    if (LDG_UNLIKELY(!pty || !buff || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pty->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    sent = (int64_t)write((int32_t)pty->wr_handle, buff, len);
    if (sent == LDG_IO_PTY_SYSCALL_FAIL)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

        return LDG_ERR_IO_WR;
    }

    *out_sent = (uint64_t)sent;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_resize(ldg_io_pty_t *pty, uint32_t col_cunt, uint32_t row_cunt)
{
    if (LDG_UNLIKELY(!pty)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pty->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    if (LDG_UNLIKELY(col_cunt == 0 || col_cunt > LDG_IO_PTY_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > LDG_IO_PTY_ROW_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return pty_winsize_apply((int32_t)pty->rd_handle, col_cunt, row_cunt);
}

uint32_t ldg_io_pty_exit_code_get(ldg_io_pty_t *pty, uint32_t *out_code)
{
    if (LDG_UNLIKELY(!pty || !out_code)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_code = 0;

    pty_reap(pty);

    if (pty->alive == LDG_TRUTH_TRUE) { return LDG_ERR_AGAIN; }

    *out_code = pty->exit_code;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_pty_close(ldg_io_pty_t *pty)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!pty)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (pty->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    pty->open = LDG_TRUTH_FALSE;

    if (pty->alive == LDG_TRUTH_TRUE)
    {
        kill((pid_t)pty->pid, SIGHUP);
        waitpid((pid_t)pty->pid, 0x0, LDG_IO_PTY_NO_FLAG);
        pty->alive = LDG_TRUTH_FALSE;
    }

    if (close((int32_t)pty->rd_handle) == LDG_IO_PTY_SYSCALL_FAIL) { ret = LDG_ERR_IO_CLOSE; }

    pty->rd_handle = LDG_IO_PTY_HANDLE_NONE;
    pty->wr_handle = LDG_IO_PTY_HANDLE_NONE;

    return ret;
}

uint8_t ldg_io_pty_alive_is(ldg_io_pty_t *pty)
{
    if (LDG_UNLIKELY(!pty)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (pty->open != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (pty->alive != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    pty_reap(pty);

    if (pty->alive == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
