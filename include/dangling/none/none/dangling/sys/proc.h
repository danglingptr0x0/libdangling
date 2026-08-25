#ifndef LDG_SYS_PROC_H
#define LDG_SYS_PROC_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_SYS_PROC_ARG_MAX 64
#define LDG_SYS_PROC_HANDLE_NONE UINT64_MAX
#define LDG_SYS_PROC_AFFINITY_CPU_MAX 64
#define LDG_SYS_PROC_NAME_MAX 64
#define LDG_SYS_PROC_PID_NONE UINT32_MAX

typedef enum ldg_sys_proc_priority
{
    LDG_SYS_PROC_PRIORITY_NONE = 0,
    LDG_SYS_PROC_PRIORITY_IDLE,
    LDG_SYS_PROC_PRIORITY_LOW,
    LDG_SYS_PROC_PRIORITY_NORMAL,
    LDG_SYS_PROC_PRIORITY_HIGH,
    LDG_SYS_PROC_PRIORITY_REALTIME
} ldg_sys_proc_priority_t;

typedef struct ldg_sys_proc
{
    uint64_t handle;
    uint64_t pipe_in;
    uint64_t pipe_out;
    uint64_t pipe_err;
    uint32_t pid;
    uint32_t exit_code;
    uint8_t alive;
    uint8_t pudding[7];
} ldg_sys_proc_t;

typedef struct ldg_sys_proc_info
{
    uint32_t pid;
    uint32_t parent_pid;
    char name[LDG_SYS_PROC_NAME_MAX];
} ldg_sys_proc_info_t;

LDG_EXPORT uint32_t ldg_sys_proc_list(ldg_sys_proc_info_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint8_t ldg_sys_proc_name_running_is(const char *name);
LDG_EXPORT uint32_t ldg_sys_proc_spawn(const char *path, const char *const *argv, const char *cwd, ldg_sys_proc_t *out);
LDG_EXPORT uint32_t ldg_sys_proc_stdin_wr(ldg_sys_proc_t *proc, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent);
LDG_EXPORT uint32_t ldg_sys_proc_stdout_rd(ldg_sys_proc_t *proc, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_sys_proc_stderr_rd(ldg_sys_proc_t *proc, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_sys_proc_exit_code_get(ldg_sys_proc_t *proc, uint32_t *out_code);
LDG_EXPORT uint32_t ldg_sys_proc_kill(ldg_sys_proc_t *proc);
LDG_EXPORT uint32_t ldg_sys_proc_close(ldg_sys_proc_t *proc);
LDG_EXPORT uint8_t ldg_sys_proc_alive_is(ldg_sys_proc_t *proc);
LDG_EXPORT uint32_t ldg_sys_proc_self_priority_set(ldg_sys_proc_priority_t lvl);
LDG_EXPORT uint32_t ldg_sys_proc_self_priority_get(ldg_sys_proc_priority_t *out);
LDG_EXPORT uint32_t ldg_sys_proc_self_affinity_set(uint64_t mask);
LDG_EXPORT uint32_t ldg_sys_proc_self_affinity_get(uint64_t *out);

#endif
