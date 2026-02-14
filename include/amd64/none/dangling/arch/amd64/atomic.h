#ifndef LDG_ARCH_AMD64_ATOMIC_H
#define LDG_ARCH_AMD64_ATOMIC_H

#define LDG_READ_ONCE(x) __atomic_load_n(&(x), __ATOMIC_RELAXED)
#define LDG_WRITE_ONCE(x, val) __atomic_store_n(&(x), (val), __ATOMIC_RELAXED)

#define LDG_READ_ONCE_AGGREGATE(x) ({ typeof(x) __v; __atomic_load(&(x), &__v, __ATOMIC_RELAXED); __v; })
#define LDG_WRITE_ONCE_AGGREGATE(x, val) do { typeof(x) __w = (val); __atomic_store(&(x), &__w, __ATOMIC_RELAXED); } while (0)

#define LDG_LOAD_ACQUIRE(x) __atomic_load_n(&(x), __ATOMIC_ACQUIRE)
#define LDG_STORE_RELEASE(x, val) __atomic_store_n(&(x), (val), __ATOMIC_RELEASE)

#define LDG_FETCH_ADD(x, val) __atomic_fetch_add(&(x), (val), __ATOMIC_SEQ_CST)
#define LDG_FETCH_SUB(x, val) __atomic_fetch_sub(&(x), (val), __ATOMIC_SEQ_CST)
#define LDG_ADD_FETCH(x, val) __atomic_add_fetch(&(x), (val), __ATOMIC_SEQ_CST)
#define LDG_SUB_FETCH(x, val) __atomic_sub_fetch(&(x), (val), __ATOMIC_SEQ_CST)

#define LDG_CAS(ptr, expected, desired) __atomic_compare_exchange_n((ptr), (expected), (desired), 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)
#define LDG_CAS_WEAK(ptr, expected, desired) __atomic_compare_exchange_n((ptr), (expected), (desired), 1, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)

#endif
