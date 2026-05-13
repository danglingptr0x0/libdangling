#ifndef LDG_THREAD_YIELD_H
#define LDG_THREAD_YIELD_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_thread_yield(uint64_t ns);

#endif
