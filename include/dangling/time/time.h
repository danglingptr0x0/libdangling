#ifndef LDG_TIME_TIME_H
#define LDG_TIME_TIME_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint64_t ldg_time_epoch_ms_get(void);
LDG_EXPORT uint64_t ldg_time_epoch_ns_get(void);
LDG_EXPORT double ldg_time_monotonic_get(void);

#endif
