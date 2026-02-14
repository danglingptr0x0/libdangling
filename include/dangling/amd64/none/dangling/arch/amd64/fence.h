#ifndef LDG_ARCH_AMD64_FENCE_H
#define LDG_ARCH_AMD64_FENCE_H

#define LDG_MFENCE __asm__ __volatile__ ("mfence" ::: "memory")
#define LDG_SFENCE __asm__ __volatile__ ("sfence" ::: "memory")
#define LDG_LFENCE __asm__ __volatile__ ("lfence" ::: "memory")
#define LDG_BARRIER __asm__ __volatile__ ("" ::: "memory")
#define LDG_PAUSE __asm__ __volatile__ ("pause")

#define LDG_SMP_MB() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define LDG_SMP_WMB() __atomic_thread_fence(__ATOMIC_RELEASE)
#define LDG_SMP_RMB() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define LDG_SMP_SIG_FENCE() __atomic_signal_fence(__ATOMIC_SEQ_CST)

#endif
