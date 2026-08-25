#ifndef LDG_ARCH_MISC_ATOMIC_H
#define LDG_ARCH_MISC_ATOMIC_H

#include <dangling/core/macros.h>

#define LDG_RD_ONCE(obj) (*(volatile __typeof__(obj) *) & (obj))
#define LDG_WR_ONCE(obj, val) do { *(volatile __typeof__(obj) *) & (obj) = (val); } while (0)

#define LDG_RD_ONCE_AGGREGATE(obj) (obj)
#define LDG_WR_ONCE_AGGREGATE(obj, val) do { (obj) = (val); } while (0)

#define LDG_LD_ACQUIRE(obj) (*(volatile __typeof__(obj) *) & (obj))
#define LDG_ST_RELEASE(obj, val) do { *(volatile __typeof__(obj) *) & (obj) = (val); } while (0)

#define LDG_FETCH_ADD(obj, val) ({ __typeof__(obj) prev = (obj); (obj) = (__typeof__(obj))(prev + (val)); prev; })
#define LDG_FETCH_SUB(obj, val) ({ __typeof__(obj) prev = (obj); (obj) = (__typeof__(obj))(prev - (val)); prev; })
#define LDG_ADD_FETCH(obj, val) ({ (obj) = (__typeof__(obj))((obj) + (val)); (obj); })
#define LDG_SUB_FETCH(obj, val) ({ (obj) = (__typeof__(obj))((obj) - (val)); (obj); })

#define LDG_CAS(ptr, expected, desired) ({ uint8_t swapped = LDG_TRUTH_FALSE; if (*(ptr) == *(expected)) { *(ptr) = (desired); swapped = LDG_TRUTH_TRUE; } \
                                           else { *(expected) = *(ptr); } \
                                           swapped; })
#define LDG_CAS_WEAK(ptr, expected, desired) LDG_CAS(ptr, expected, desired)

#endif
