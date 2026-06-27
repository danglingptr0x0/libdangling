#include <stdint.h>

#include <dangling/thread/sync.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>


uint32_t ldg_mut_init(ldg_mut_t *mut, uint8_t shared)
{
    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    mut->is_shared = shared;
    mut->is_init = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_destroy(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    mut->is_init = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_lock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mut->is_init)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_unlock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mut->is_init)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mut_trylock(ldg_mut_t *mut)
{
    if (LDG_UNLIKELY(!mut)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mut->is_init)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}
