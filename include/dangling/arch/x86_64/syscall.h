#ifndef LDG_ARCH_X86_64_SYSCALL_H
#define LDG_ARCH_X86_64_SYSCALL_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT long ldg_syscall0(long n);
LDG_EXPORT long ldg_syscall1(long n, long a1);
LDG_EXPORT long ldg_syscall2(long n, long a1, long a2);
LDG_EXPORT long ldg_syscall3(long n, long a1, long a2, long a3);
LDG_EXPORT long ldg_syscall4(long n, long a1, long a2, long a3, long a4);

LDG_EXPORT uint64_t ldg_rdtsc(void);
LDG_EXPORT uint64_t ldg_rdtscp(uint32_t *aux);

#define LDG_SYS_READ 0
#define LDG_SYS_WRITE 1
#define LDG_SYS_OPEN 2
#define LDG_SYS_CLOSE 3
#define LDG_SYS_MMAP 9
#define LDG_SYS_MPROTECT 10
#define LDG_SYS_MUNMAP 11
#define LDG_SYS_PIPE 22
#define LDG_SYS_DUP2 33
#define LDG_SYS_NANOSLEEP 35
#define LDG_SYS_GETPID 39
#define LDG_SYS_CLOCK_GETTIME 228
#define LDG_SYS_OPENAT 257
#define LDG_SYS_GETRANDOM 318

#endif
