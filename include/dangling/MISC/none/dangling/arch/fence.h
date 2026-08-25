#ifndef LDG_ARCH_MISC_FENCE_H
#define LDG_ARCH_MISC_FENCE_H

#define LDG_MFENCE __asm__ __volatile__ ("" ::: "memory")
#define LDG_SFENCE __asm__ __volatile__ ("" ::: "memory")
#define LDG_LFENCE __asm__ __volatile__ ("" ::: "memory")
#define LDG_BARRIER __asm__ __volatile__ ("" ::: "memory")
#define LDG_PAUSE __asm__ __volatile__ ("" ::: "memory")

#define LDG_SMP_MB() __asm__ __volatile__ ("" ::: "memory")
#define LDG_SMP_WMB() __asm__ __volatile__ ("" ::: "memory")
#define LDG_SMP_RMB() __asm__ __volatile__ ("" ::: "memory")
#define LDG_SMP_SIG_FENCE() __asm__ __volatile__ ("" ::: "memory")

#endif
