#ifndef LDG_COMPUTE_H
#define LDG_COMPUTE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>

#define LDG_COMPUTE_DEV_CPU 0
#define LDG_COMPUTE_DEV_VK_DISCRETE UINT32_MAX
#define LDG_COMPUTE_DEV_VK_APU (UINT32_MAX - 1U)
#define LDG_COMPUTE_DEV_VK_CPU (UINT32_MAX - 2U)

typedef struct ldg_compute_desc
{
    void *dev;
    uint64_t slab_size;
    uint32_t dev_sel;
    uint32_t slab_cunt;
    uint32_t pipeline_pool_cunt;
    uint8_t pudding[4];
} ldg_compute_desc_t;

LDG_EXPORT uint32_t ldg_compute_create(const ldg_compute_desc_t *desc, void **out);
LDG_EXPORT uint32_t ldg_compute_destroy(void *compute);
LDG_EXPORT uint32_t ldg_compute_dev_get(void *compute, void **out);

#endif
