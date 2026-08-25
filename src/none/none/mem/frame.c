#include <stdint.h>

#include <dangling/mem/frame.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/arith.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>

#define LDG_MEM_FRAME_WORD_BITS ((uint64_t)sizeof(uint64_t) * LDG_BYTE_BITS)
#define LDG_MEM_FRAME_WORD_BIAS (LDG_MEM_FRAME_WORD_BITS - 1)
#define LDG_MEM_FRAME_ALIGN_BIAS ((uint64_t)LDG_MEM_FRAME_WIDTH - 1)
#define LDG_MEM_FRAME_BIT_MASK ((uint64_t)LDG_BIT(0))
#define LDG_MEM_FRAME_BITMAP_CUNT 2
#define LDG_MEM_FRAME_OCCUPIED_FILL LDG_BYTE_MASK

typedef struct ldg_mem_frame_state
{
    uint64_t *occupancy;
    uint64_t *run_start;
    uint64_t base;
    uint64_t frame_cunt;
    uint64_t free_cunt;
    uint64_t cursor;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_mem_frame_state_t;

static ldg_mem_frame_state_t* frame_state_get(void)
{
    static ldg_mem_frame_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint8_t frame_bit_is(const uint64_t *bitmap, uint64_t idx)
{
    uint64_t word = 0;
    uint64_t shift = 0;

    if (LDG_UNLIKELY(!bitmap)) { return LDG_ERR_FUNC_ARG_NULL; }

    word = bitmap[idx / LDG_MEM_FRAME_WORD_BITS];
    shift = idx % LDG_MEM_FRAME_WORD_BITS;

    if (((word >> shift) & LDG_MEM_FRAME_BIT_MASK) == LDG_MEM_FRAME_BIT_MASK) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t frame_bit_set(uint64_t *bitmap, uint64_t idx)
{
    if (LDG_UNLIKELY(!bitmap)) { return LDG_ERR_FUNC_ARG_NULL; }

    bitmap[idx / LDG_MEM_FRAME_WORD_BITS] |= (LDG_MEM_FRAME_BIT_MASK << (idx % LDG_MEM_FRAME_WORD_BITS));

    return LDG_ERR_AOK;
}

static uint32_t frame_bit_clear(uint64_t *bitmap, uint64_t idx)
{
    if (LDG_UNLIKELY(!bitmap)) { return LDG_ERR_FUNC_ARG_NULL; }

    bitmap[idx / LDG_MEM_FRAME_WORD_BITS] &= ~(LDG_MEM_FRAME_BIT_MASK << (idx % LDG_MEM_FRAME_WORD_BITS));

    return LDG_ERR_AOK;
}

static uint32_t frame_range_occupy(ldg_mem_frame_state_t *state, uint64_t first, uint64_t cunt)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(first >= state->frame_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(cunt > state->frame_cunt - first)) { return LDG_ERR_BOUNDS; }

    for (idx = first; idx < first + cunt; idx++)
    {
        if (frame_bit_is(state->occupancy, idx) == LDG_TRUTH_TRUE) { continue; }

        ret = frame_bit_set(state->occupancy, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(state->free_cunt == 0)) { return LDG_ERR_MEM_CORRUPTION; }

        state->free_cunt--;
    }

    return LDG_ERR_AOK;
}

static uint32_t frame_range_release(ldg_mem_frame_state_t *state, uint64_t first, uint64_t cunt)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(first >= state->frame_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(cunt > state->frame_cunt - first)) { return LDG_ERR_BOUNDS; }

    for (idx = first; idx < first + cunt; idx++)
    {
        if (frame_bit_is(state->occupancy, idx) != LDG_TRUTH_TRUE) { continue; }

        ret = frame_bit_clear(state->occupancy, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        state->free_cunt++;
    }

    return LDG_ERR_AOK;
}

static uint64_t frame_run_cunt_measure(const ldg_mem_frame_state_t *state, uint64_t first)
{
    uint64_t idx = 0;
    uint64_t run = 0;

    if (LDG_UNLIKELY(!state)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(first >= state->frame_cunt)) { return UINT64_MAX; }

    if (frame_bit_is(state->run_start, first) != LDG_TRUTH_TRUE) { return UINT64_MAX; }

    for (idx = first; idx < state->frame_cunt; idx++)
    {
        if (idx > first && frame_bit_is(state->run_start, idx) == LDG_TRUTH_TRUE) { break; }

        if (frame_bit_is(state->occupancy, idx) != LDG_TRUTH_TRUE) { break; }

        run++;
    }

    if (LDG_UNLIKELY(run == 0)) { return UINT64_MAX; }

    return run;
}

static uint32_t frame_free_run_find(const ldg_mem_frame_state_t *state, uint64_t cunt, uint64_t first, uint64_t limit, uint64_t *out)
{
    uint64_t idx = 0;
    uint64_t run = 0;
    uint64_t run_first = 0;

    if (LDG_UNLIKELY(!state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT64_MAX;

    for (idx = first; idx < limit; idx++)
    {
        if (frame_bit_is(state->occupancy, idx) == LDG_TRUTH_TRUE)
        {
            run = 0;
            continue;
        }

        if (run == 0) { run_first = idx; }

        run++;

        if (run >= cunt)
        {
            *out = run_first;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_FULL;
}

uint32_t ldg_mem_frame_init(const ldg_mem_extent_t *extent, uint32_t cunt)
{
    ldg_mem_frame_state_t *state = 0x0;
    uint64_t lowest = 0;
    uint64_t highest = 0;
    uint64_t host_base = 0;
    uint64_t host_end = 0;
    uint64_t aligned_base = 0;
    uint64_t aligned_end = 0;
    uint64_t frame_cunt = 0;
    uint64_t word_cunt = 0;
    uint64_t bitmap_width = 0;
    uint64_t meta_base = 0;
    uint64_t meta_cunt = 0;
    uint64_t meta_idx = 0;
    uint64_t first = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!extent)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0 || cunt > LDG_MEM_EXTENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    state = frame_state_get();

    if (state->is_init == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    lowest = UINT64_MAX;

    for (idx = 0; idx < cunt; idx++)
    {
        if (LDG_UNLIKELY(extent[idx].kind != (uint32_t)LDG_MEM_EXTENT_KIND_SYSMEM)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(extent[idx].width < LDG_MEM_FRAME_WIDTH)) { return LDG_ERR_FUNC_ARG_INVALID; }

        ret = ldg_arith_uint64_add(extent[idx].base, extent[idx].width, &host_end);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

        if (extent[idx].base < lowest) { lowest = extent[idx].base; }

        if (host_end > highest) { highest = host_end; }
    }

    aligned_base = (lowest + LDG_MEM_FRAME_ALIGN_BIAS) & ~LDG_MEM_FRAME_ALIGN_BIAS;
    aligned_end = highest & ~LDG_MEM_FRAME_ALIGN_BIAS;

    if (LDG_UNLIKELY(aligned_end <= aligned_base)) { return LDG_ERR_FUNC_ARG_INVALID; }

    frame_cunt = (aligned_end - aligned_base) / LDG_MEM_FRAME_WIDTH;
    word_cunt = (frame_cunt + LDG_MEM_FRAME_WORD_BIAS) / LDG_MEM_FRAME_WORD_BITS;
    bitmap_width = word_cunt * (uint64_t)sizeof(uint64_t) * LDG_MEM_FRAME_BITMAP_CUNT;
    meta_cunt = (bitmap_width + LDG_MEM_FRAME_ALIGN_BIAS) / LDG_MEM_FRAME_WIDTH;
    meta_base = UINT64_MAX;

    for (idx = 0; idx < cunt; idx++)
    {
        host_base = (extent[idx].base + LDG_MEM_FRAME_ALIGN_BIAS) & ~LDG_MEM_FRAME_ALIGN_BIAS;
        host_end = (extent[idx].base + extent[idx].width) & ~LDG_MEM_FRAME_ALIGN_BIAS;

        if (host_end <= host_base) { continue; }

        if (host_end - host_base >= bitmap_width)
        {
            meta_base = host_base;

            break;
        }
    }

    if (LDG_UNLIKELY(meta_base == UINT64_MAX)) { return LDG_ERR_FULL; }

    state->occupancy = (uint64_t *)(uintptr_t)meta_base;
    state->run_start = state->occupancy + word_cunt;
    state->base = aligned_base;
    state->frame_cunt = frame_cunt;
    state->free_cunt = 0;
    state->cursor = 0;

    ret = ldg_mem_fill(state->occupancy, LDG_MEM_FRAME_OCCUPIED_FILL, word_cunt * (uint64_t)sizeof(uint64_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(state->run_start, word_cunt * (uint64_t)sizeof(uint64_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        host_base = (extent[idx].base + LDG_MEM_FRAME_ALIGN_BIAS) & ~LDG_MEM_FRAME_ALIGN_BIAS;
        host_end = (extent[idx].base + extent[idx].width) & ~LDG_MEM_FRAME_ALIGN_BIAS;

        if (host_end <= host_base) { continue; }

        first = (host_base - aligned_base) / LDG_MEM_FRAME_WIDTH;

        ret = frame_range_release(state, first, (host_end - host_base) / LDG_MEM_FRAME_WIDTH);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    meta_idx = (meta_base - aligned_base) / LDG_MEM_FRAME_WIDTH;

    ret = frame_range_occupy(state, meta_idx, meta_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = frame_bit_set(state->run_start, meta_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_frame_shutdown(void)
{
    ldg_mem_frame_state_t *state = 0x0;

    state = frame_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_mem_zero(state, (uint64_t)sizeof(ldg_mem_frame_state_t));
}

uint32_t ldg_mem_frame_alloc(uint64_t cunt, uint64_t *out)
{
    ldg_mem_frame_state_t *state = 0x0;
    uint64_t wrap_first = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT64_MAX;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    state = frame_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(cunt > state->free_cunt)) { return LDG_ERR_FULL; }

    ret = frame_free_run_find(state, cunt, state->cursor, state->frame_cunt, &idx);
    if (ret != LDG_ERR_AOK)
    {
        ret = frame_free_run_find(state, cunt, wrap_first, state->frame_cunt, &idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_FULL; }
    }

    ret = frame_range_occupy(state, idx, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = frame_bit_set(state->run_start, idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->cursor = idx + cunt;

    if (state->cursor >= state->frame_cunt) { state->cursor = 0; }

    *out = state->base + (idx * LDG_MEM_FRAME_WIDTH);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_frame_dealloc(uint64_t addr)
{
    ldg_mem_frame_state_t *state = 0x0;
    uint64_t idx = 0;
    uint64_t run = 0;
    uint32_t ret = 0;

    state = frame_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY((addr & LDG_MEM_FRAME_ALIGN_BIAS) != 0)) { return LDG_ERR_MEM_ALIGNMENT; }

    if (LDG_UNLIKELY(addr < state->base)) { return LDG_ERR_BOUNDS; }

    idx = (addr - state->base) / LDG_MEM_FRAME_WIDTH;

    if (LDG_UNLIKELY(idx >= state->frame_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(frame_bit_is(state->run_start, idx) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    run = frame_run_cunt_measure(state, idx);
    if (LDG_UNLIKELY(run == UINT64_MAX)) { return LDG_ERR_MEM_CORRUPTION; }

    ret = frame_bit_clear(state->run_start, idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return frame_range_release(state, idx, run);
}

uint64_t ldg_mem_frame_run_cunt_get(uint64_t addr)
{
    ldg_mem_frame_state_t *state = 0x0;
    uint64_t idx = 0;

    state = frame_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (LDG_UNLIKELY((addr & LDG_MEM_FRAME_ALIGN_BIAS) != 0)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(addr < state->base)) { return UINT64_MAX; }

    idx = (addr - state->base) / LDG_MEM_FRAME_WIDTH;

    if (LDG_UNLIKELY(idx >= state->frame_cunt)) { return UINT64_MAX; }

    return frame_run_cunt_measure(state, idx);
}

uint64_t ldg_mem_frame_free_cunt_get(void)
{
    ldg_mem_frame_state_t *state = 0x0;

    state = frame_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return state->free_cunt;
}

uint64_t ldg_mem_frame_total_cunt_get(void)
{
    ldg_mem_frame_state_t *state = 0x0;

    state = frame_state_get();

    if (LDG_UNLIKELY(state->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return state->frame_cunt;
}
