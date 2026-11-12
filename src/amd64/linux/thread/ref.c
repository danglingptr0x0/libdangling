#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <dangling/thread/ref.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>

#include "uc.h"

#define LDG_THREAD_REF_PARK_MAX 64
#define LDG_THREAD_REF_ID_NONE 0
#define LDG_THREAD_REF_SIG_BACK 3
#define LDG_THREAD_REF_SIG_PROBE 0
#define LDG_THREAD_REF_FUTEX_WAIT 0
#define LDG_THREAD_REF_FUTEX_WAKE 1
#define LDG_THREAD_REF_WAKE_ALL 0x7FFFFFFF
#define LDG_THREAD_REF_WAIT_SLICE_NS 10000000
#define LDG_THREAD_REF_WAIT_TRY_MAX 100
#define LDG_THREAD_REF_TASK_FMT "/proc/%u32/task"
#define LDG_THREAD_REF_STATUS_FMT "/proc/%u32/status"
#define LDG_THREAD_REF_PROC_PATH_CAP 32
#define LDG_THREAD_REF_STATUS_CAP 2048
#define LDG_THREAD_REF_TRACER_KEY "TracerPid:"
#define LDG_THREAD_REF_TRACER_KEY_LEN 10
#define LDG_THREAD_REF_TRACER_NONE '0'
#define LDG_THREAD_REF_TASK_NAME_MAX 32
#define LDG_THREAD_REF_NO_ARG 0
#define LDG_THREAD_REF_TRACE_REGS_GET 12
#define LDG_THREAD_REF_TRACE_REGS_SET 13
#define LDG_THREAD_REF_TRACE_DETACH 17
#define LDG_THREAD_REF_TRACE_SEIZE 0x4206
#define LDG_THREAD_REF_TRACE_STOP 0x4207
#define LDG_THREAD_REF_WAIT_ALL 0x40000000

typedef enum ldg_thread_ref_park_state
{
    LDG_THREAD_REF_PARK_IDLE = 0,
    LDG_THREAD_REF_PARK_REQD,
    LDG_THREAD_REF_PARK_PARKED,
    LDG_THREAD_REF_PARK_RESUMING,
    LDG_THREAD_REF_PARK_STATE_CUNT
} ldg_thread_ref_park_state_t;

typedef struct ldg_thread_ref_park
{
    ldg_thread_regs_t regs;
    uint64_t id;
    uint32_t state;
    uint8_t regs_dirty;
    uint8_t pudding[3];
} ldg_thread_ref_park_t;

typedef struct ldg_thread_ref_state
{
    struct sigaction prev;
    ldg_thread_ref_park_t park[LDG_THREAD_REF_PARK_MAX];
    uint32_t installed;
    uint8_t pudding[4];
} ldg_thread_ref_state_t;

static ldg_thread_ref_state_t g_thread_ref = { .installed = LDG_TRUTH_FALSE };

static int32_t thread_ref_sig_get(void)
{
    return (int32_t)SIGRTMAX - LDG_THREAD_REF_SIG_BACK;
}

static int64_t thread_ref_word_wait(uint32_t *word, uint32_t seen)
{
    struct timespec slice = LDG_STRUCT_ZERO_INIT;

    slice.tv_nsec = LDG_THREAD_REF_WAIT_SLICE_NS;

    return ldg_syscall4(LDG_SYS_FUTEX, (int64_t)(uintptr_t)word, LDG_THREAD_REF_FUTEX_WAIT, (int64_t)seen, (int64_t)(uintptr_t)&slice);
}

static uint8_t thread_ref_traced_is(uint32_t pid)
{
    char path[LDG_THREAD_REF_PROC_PATH_CAP] = LDG_ARR_ZERO_INIT;
    char text[LDG_THREAD_REF_STATUS_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t path_len = 0;
    uint64_t key_idx = 0;
    uint64_t idx = 0;
    int64_t got = 0;
    int32_t fd = 0;

    if (ldg_str_fmt(path, LDG_THREAD_REF_PROC_PATH_CAP, &path_len, LDG_THREAD_REF_STATUS_FMT, pid) != LDG_ERR_AOK) { return LDG_TRUTH_INDETERMINATE; }

    fd = (int32_t)open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) { return LDG_TRUTH_INDETERMINATE; }

    got = (int64_t)read(fd, text, LDG_THREAD_REF_STATUS_CAP - LDG_STR_TERM_SIZE);

    if (close(fd) != 0 || got <= 0) { return LDG_TRUTH_INDETERMINATE; }

    if (ldg_str_substring_find(text, LDG_THREAD_REF_TRACER_KEY, (uint64_t)got, &key_idx) != LDG_ERR_AOK) { return LDG_TRUTH_INDETERMINATE; }

    for (idx = key_idx + LDG_THREAD_REF_TRACER_KEY_LEN; idx < (uint64_t)got; idx++)
    {
        if (ldg_char_space_is((uint8_t)text[idx]) == LDG_TRUTH_TRUE) { continue; }

        return (text[idx] != LDG_THREAD_REF_TRACER_NONE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }

    return LDG_TRUTH_INDETERMINATE;
}

static uint32_t thread_ref_outside_suspend(ldg_thread_ref_t *ref)
{
    int64_t res = 0;
    int32_t status = 0;
    int32_t waited = 0;

    res = ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_SEIZE, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, LDG_THREAD_REF_NO_ARG);
    if (res + (int64_t)ESRCH == 0) { return LDG_ERR_NOT_FOUND; }

    if (res < 0) { return (thread_ref_traced_is(ref->pid) == LDG_TRUTH_TRUE) ? LDG_ERR_BUSY : LDG_ERR_DENIED; }

    res = ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_STOP, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, LDG_THREAD_REF_NO_ARG);

    while (res == 0)
    {
        waited = (int32_t)waitpid((int32_t)ref->id, &status, LDG_THREAD_REF_WAIT_ALL);
        if (waited < 0 && errno == EINTR) { continue; }

        if (waited > 0 && WIFSTOPPED(status))
        {
            ref->suspended = LDG_TRUTH_TRUE;
            return LDG_ERR_AOK;
        }

        break;
    }

    if (ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_DETACH, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, LDG_THREAD_REF_NO_ARG) < 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_DENIED;
}

static uint32_t thread_ref_outside_resume(ldg_thread_ref_t *ref)
{
    ref->suspended = LDG_TRUTH_FALSE;

    if (ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_DETACH, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, LDG_THREAD_REF_NO_ARG) < 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t thread_ref_outside_regs_get(const ldg_thread_ref_t *ref, ldg_thread_regs_t *out)
{
    struct user_regs_struct raw = LDG_STRUCT_ZERO_INIT;

    if (ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_REGS_GET, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, (int64_t)(uintptr_t)&raw) < 0) { return LDG_ERR_DENIED; }

    out->gpr[LDG_THREAD_REG_RAX] = (uint64_t)raw.rax;
    out->gpr[LDG_THREAD_REG_RCX] = (uint64_t)raw.rcx;
    out->gpr[LDG_THREAD_REG_RDX] = (uint64_t)raw.rdx;
    out->gpr[LDG_THREAD_REG_RBX] = (uint64_t)raw.rbx;
    out->gpr[LDG_THREAD_REG_RSP] = (uint64_t)raw.rsp;
    out->gpr[LDG_THREAD_REG_RBP] = (uint64_t)raw.rbp;
    out->gpr[LDG_THREAD_REG_RSI] = (uint64_t)raw.rsi;
    out->gpr[LDG_THREAD_REG_RDI] = (uint64_t)raw.rdi;
    out->gpr[LDG_THREAD_REG_R8] = (uint64_t)raw.r8;
    out->gpr[LDG_THREAD_REG_R9] = (uint64_t)raw.r9;
    out->gpr[LDG_THREAD_REG_R10] = (uint64_t)raw.r10;
    out->gpr[LDG_THREAD_REG_R11] = (uint64_t)raw.r11;
    out->gpr[LDG_THREAD_REG_R12] = (uint64_t)raw.r12;
    out->gpr[LDG_THREAD_REG_R13] = (uint64_t)raw.r13;
    out->gpr[LDG_THREAD_REG_R14] = (uint64_t)raw.r14;
    out->gpr[LDG_THREAD_REG_R15] = (uint64_t)raw.r15;
    out->rip = (uint64_t)raw.rip;
    out->rflags = (uint64_t)raw.eflags;

    return LDG_ERR_AOK;
}

static uint32_t thread_ref_outside_regs_set(const ldg_thread_ref_t *ref, const ldg_thread_regs_t *regs)
{
    struct user_regs_struct raw = LDG_STRUCT_ZERO_INIT;

    if (ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_REGS_GET, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, (int64_t)(uintptr_t)&raw) < 0) { return LDG_ERR_DENIED; }

    raw.rax = regs->gpr[LDG_THREAD_REG_RAX];
    raw.rcx = regs->gpr[LDG_THREAD_REG_RCX];
    raw.rdx = regs->gpr[LDG_THREAD_REG_RDX];
    raw.rbx = regs->gpr[LDG_THREAD_REG_RBX];
    raw.rsp = regs->gpr[LDG_THREAD_REG_RSP];
    raw.rbp = regs->gpr[LDG_THREAD_REG_RBP];
    raw.rsi = regs->gpr[LDG_THREAD_REG_RSI];
    raw.rdi = regs->gpr[LDG_THREAD_REG_RDI];
    raw.r8 = regs->gpr[LDG_THREAD_REG_R8];
    raw.r9 = regs->gpr[LDG_THREAD_REG_R9];
    raw.r10 = regs->gpr[LDG_THREAD_REG_R10];
    raw.r11 = regs->gpr[LDG_THREAD_REG_R11];
    raw.r12 = regs->gpr[LDG_THREAD_REG_R12];
    raw.r13 = regs->gpr[LDG_THREAD_REG_R13];
    raw.r14 = regs->gpr[LDG_THREAD_REG_R14];
    raw.r15 = regs->gpr[LDG_THREAD_REG_R15];
    raw.rip = regs->rip;
    raw.eflags = regs->rflags;

    if (ldg_syscall4(LDG_SYS_PTRACE, LDG_THREAD_REF_TRACE_REGS_SET, (int64_t)ref->id, LDG_THREAD_REF_NO_ARG, (int64_t)(uintptr_t)&raw) < 0) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

static void thread_ref_sig_cb(int32_t sig, siginfo_t *info, void *uc)
{
    ldg_thread_ref_park_t *park = 0x0;
    uint64_t self = 0;
    uint32_t idx = 0;

    if (!info || !uc || sig == 0) { return; }

    self = (uint64_t)ldg_syscall0(LDG_SYS_GETTID);

    for (idx = 0; idx < LDG_THREAD_REF_PARK_MAX; idx++)
    {
        if (LDG_RD_ONCE(g_thread_ref.park[idx].id) != self || LDG_RD_ONCE(g_thread_ref.park[idx].state) != LDG_THREAD_REF_PARK_REQD) { continue; }

        park = &g_thread_ref.park[idx];
        break;
    }

    if (!park) { return; }

    thread_uc_regs_ld(uc, &park->regs);
    LDG_ST_RELEASE(park->state, LDG_THREAD_REF_PARK_PARKED);

    if (ldg_syscall4(LDG_SYS_FUTEX, (int64_t)(uintptr_t)&park->state, LDG_THREAD_REF_FUTEX_WAKE, LDG_THREAD_REF_WAKE_ALL, LDG_THREAD_REF_NO_ARG) < 0) { return; }

    while (LDG_LD_ACQUIRE(park->state) == LDG_THREAD_REF_PARK_PARKED) { if (thread_ref_word_wait(&park->state, LDG_THREAD_REF_PARK_PARKED) < 0) { continue; } }

    if (park->regs_dirty == LDG_TRUTH_TRUE)
    {
        thread_uc_regs_st(uc, &park->regs);
        park->regs_dirty = LDG_TRUTH_FALSE;
    }

    LDG_ST_RELEASE(park->state, LDG_THREAD_REF_PARK_IDLE);

    if (ldg_syscall4(LDG_SYS_FUTEX, (int64_t)(uintptr_t)&park->state, LDG_THREAD_REF_FUTEX_WAKE, LDG_THREAD_REF_WAKE_ALL, LDG_THREAD_REF_NO_ARG) < 0) { return; }
}

static uint32_t thread_ref_handler_install(void)
{
    struct sigaction act = LDG_STRUCT_ZERO_INIT;
    uint32_t expected = LDG_TRUTH_FALSE;

    if (!LDG_CAS(&g_thread_ref.installed, &expected, LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    act.sa_sigaction = thread_ref_sig_cb;
    act.sa_flags = SA_SIGINFO | SA_RESTART;

    if (LDG_UNLIKELY(sigemptyset(&act.sa_mask) != 0 || sigaction(thread_ref_sig_get(), &act, &g_thread_ref.prev) != 0))
    {
        LDG_ST_RELEASE(g_thread_ref.installed, LDG_TRUTH_FALSE);
        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

static uint32_t thread_ref_state_await(ldg_thread_ref_park_t *park, uint32_t leaving)
{
    uint32_t attempt = 0;

    for (attempt = 0; attempt < LDG_THREAD_REF_WAIT_TRY_MAX; attempt++)
    {
        if (LDG_LD_ACQUIRE(park->state) != leaving) { return LDG_ERR_AOK; }

        if (thread_ref_word_wait(&park->state, leaving) < 0) { continue; }
    }

    if (LDG_LD_ACQUIRE(park->state) != leaving) { return LDG_ERR_AOK; }

    return LDG_ERR_TIMEOUT;
}

static uint32_t thread_ref_park_get(const ldg_thread_ref_t *ref, ldg_thread_ref_park_t **out)
{
    *out = 0x0;

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE || ref->handle >= LDG_THREAD_REF_PARK_MAX)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(g_thread_ref.park[ref->handle].id != ref->id)) { return LDG_ERR_NOT_INIT; }

    *out = &g_thread_ref.park[ref->handle];

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_id_list(const ldg_sys_proc_t *proc, uint64_t *out, uint64_t cap, uint64_t *out_cunt)
{
    char path[LDG_THREAD_REF_PROC_PATH_CAP] = LDG_ARR_ZERO_INIT;
    DIR *dir = 0x0;
    struct dirent *entry = 0x0;
    uint64_t path_len = 0;
    uint64_t id = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!proc || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_str_fmt(path, LDG_THREAD_REF_PROC_PATH_CAP, &path_len, LDG_THREAD_REF_TASK_FMT, proc->pid);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dir = opendir(path);
    if (LDG_UNLIKELY(!dir)) { return (errno == EACCES) ? LDG_ERR_DENIED : LDG_ERR_NOT_FOUND; }

    for (entry = readdir(dir); entry && *out_cunt < cap; entry = readdir(dir))
    {
        if (ldg_str_uint_parse(entry->d_name, LDG_BASE_DEC, LDG_THREAD_REF_TASK_NAME_MAX, &id) != LDG_ERR_AOK) { continue; }

        out[*out_cunt] = id;
        (*out_cunt)++;
    }

    if (LDG_UNLIKELY(closedir(dir) != 0)) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t ldg_thread_id_self_get(uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint64_t)ldg_syscall0(LDG_SYS_GETTID);

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_open(const ldg_sys_proc_t *proc, uint64_t id, ldg_thread_ref_t *out)
{
    uint64_t expected = 0;
    int64_t probe = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_thread_ref_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->handle = LDG_THREAD_REF_HANDLE_NONE;

    if (LDG_UNLIKELY(id == LDG_THREAD_REF_ID_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    probe = ldg_syscall3(LDG_SYS_TGKILL, (int64_t)proc->pid, (int64_t)id, LDG_THREAD_REF_SIG_PROBE);
    if (probe != 0) { return (probe + (int64_t)EPERM == 0) ? LDG_ERR_DENIED : LDG_ERR_NOT_FOUND; }

    out->pid = proc->pid;

    if (proc->pid != (uint32_t)getpid())
    {
        out->id = id;
        out->handle = id;
        out->outside = LDG_TRUTH_TRUE;
        out->live = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    ret = thread_ref_handler_install();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_THREAD_REF_PARK_MAX; idx++)
    {
        expected = LDG_THREAD_REF_ID_NONE;
        if (!LDG_CAS(&g_thread_ref.park[idx].id, &expected, id)) { continue; }

        g_thread_ref.park[idx].regs_dirty = LDG_TRUTH_FALSE;
        LDG_ST_RELEASE(g_thread_ref.park[idx].state, LDG_THREAD_REF_PARK_IDLE);
        out->id = id;
        out->handle = idx;
        out->live = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FULL;
}

uint8_t ldg_thread_ref_exited_is(const ldg_thread_ref_t *ref)
{
    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ldg_syscall3(LDG_SYS_TGKILL, (int64_t)ref->pid, (int64_t)ref->id, LDG_THREAD_REF_SIG_PROBE) + (int64_t)ESRCH == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_thread_ref_suspend(ldg_thread_ref_t *ref)
{
    ldg_thread_ref_park_t *park = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ref->suspended == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (ref->outside == LDG_TRUTH_TRUE) { return thread_ref_outside_suspend(ref); }

    ret = thread_ref_park_get(ref, &park);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ref->id == (uint64_t)ldg_syscall0(LDG_SYS_GETTID))) { return LDG_ERR_FUNC_ARG_INVALID; }

    LDG_ST_RELEASE(park->state, LDG_THREAD_REF_PARK_REQD);

    if (ldg_syscall3(LDG_SYS_TGKILL, (int64_t)ref->pid, (int64_t)ref->id, (int64_t)thread_ref_sig_get()) != 0)
    {
        LDG_ST_RELEASE(park->state, LDG_THREAD_REF_PARK_IDLE);
        return LDG_ERR_NOT_FOUND;
    }

    ret = thread_ref_state_await(park, LDG_THREAD_REF_PARK_REQD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        LDG_ST_RELEASE(park->state, LDG_THREAD_REF_PARK_IDLE);
        return ret;
    }

    ref->suspended = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_resume(ldg_thread_ref_t *ref)
{
    ldg_thread_ref_park_t *park = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE || ref->suspended != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ref->outside == LDG_TRUTH_TRUE) { return thread_ref_outside_resume(ref); }

    ret = thread_ref_park_get(ref, &park);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    LDG_ST_RELEASE(park->state, LDG_THREAD_REF_PARK_RESUMING);
    ref->suspended = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ldg_syscall4(LDG_SYS_FUTEX, (int64_t)(uintptr_t)&park->state, LDG_THREAD_REF_FUTEX_WAKE, LDG_THREAD_REF_WAKE_ALL, LDG_THREAD_REF_NO_ARG) < 0)) { return LDG_ERR_DENIED; }

    return thread_ref_state_await(park, LDG_THREAD_REF_PARK_RESUMING);
}

uint32_t ldg_thread_ref_regs_get(ldg_thread_ref_t *ref, ldg_thread_regs_t *out)
{
    ldg_thread_ref_park_t *park = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ref || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE || ref->suspended != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ref->outside == LDG_TRUTH_TRUE) { return thread_ref_outside_regs_get(ref, out); }

    ret = thread_ref_park_get(ref, &park);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_cpy(out, &park->regs, sizeof(ldg_thread_regs_t));
}

uint32_t ldg_thread_ref_regs_set(ldg_thread_ref_t *ref, const ldg_thread_regs_t *regs)
{
    ldg_thread_ref_park_t *park = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ref || !regs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE || ref->suspended != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ref->outside == LDG_TRUTH_TRUE) { return thread_ref_outside_regs_set(ref, regs); }

    ret = thread_ref_park_get(ref, &park);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_cpy(&park->regs, regs, sizeof(ldg_thread_regs_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    park->regs_dirty = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_close(ldg_thread_ref_t *ref)
{
    ldg_thread_ref_park_t *park = 0x0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ref->handle == LDG_THREAD_REF_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (ref->suspended == LDG_TRUTH_TRUE) { ret = ldg_thread_ref_resume(ref); }

    if (ref->outside != LDG_TRUTH_TRUE && thread_ref_park_get(ref, &park) == LDG_ERR_AOK) { LDG_ST_RELEASE(park->id, LDG_THREAD_REF_ID_NONE); }

    ref->handle = LDG_THREAD_REF_HANDLE_NONE;
    ref->live = LDG_TRUTH_FALSE;

    return ret;
}
