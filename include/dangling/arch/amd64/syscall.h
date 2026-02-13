#ifndef LDG_ARCH_AMD64_SYSCALL_H
#define LDG_ARCH_AMD64_SYSCALL_H

#include <stdint.h>
#include <dangling/core/macros.h>

#ifdef __linux__
LDG_EXPORT int64_t ldg_syscall0(int64_t n);
LDG_EXPORT int64_t ldg_syscall1(int64_t n, int64_t a1);
LDG_EXPORT int64_t ldg_syscall2(int64_t n, int64_t a1, int64_t a2);
LDG_EXPORT int64_t ldg_syscall3(int64_t n, int64_t a1, int64_t a2, int64_t a3);
LDG_EXPORT int64_t ldg_syscall4(int64_t n, int64_t a1, int64_t a2, int64_t a3, int64_t a4);
#endif

LDG_EXPORT uint64_t ldg_rdtsc(void);
LDG_EXPORT uint64_t ldg_rdtscp(uint32_t *aux);

#ifdef __linux__
#define LDG_SYS_RD 0
#define LDG_SYS_WR 1
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

#endif
