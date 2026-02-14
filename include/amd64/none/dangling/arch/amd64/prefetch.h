#ifndef LDG_ARCH_AMD64_PREFETCH_H
#define LDG_ARCH_AMD64_PREFETCH_H

#define LDG_PREFETCH_R(addr) __builtin_prefetch((addr), 0, 3)
#define LDG_PREFETCH_W(addr) __builtin_prefetch((addr), 1, 3)
#define LDG_PREFETCH_NTA(addr) __asm__ __volatile__ ("prefetchnta %0" : : "m" (*(const volatile char *)(addr)))

#endif
