#ifndef LDG_ARCH_DANGCSA_DLU_H
#define LDG_ARCH_DANGCSA_DLU_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/arch/dangcsa/dangcsa.h>

#define LDG_DANGCSA_DLU_RAM_SIZE_DEFAULT 65536
#define LDG_DANGCSA_DLU_TX_CAP_DEFAULT 4096
#define LDG_DANGCSA_DLU_RAM_LIMIT 0x08000000U
#define LDG_DANGCSA_DLU_STACK_TOP 0x0000000000000FF8ULL
#define LDG_DANGCSA_DLU_TIMER_CUNT 2
#define LDG_DANGCSA_DLU_LAST_NONE 24
#define LDG_DANGCSA_DLU_FAULT_NONE 0

typedef struct ldg_dangcsa_dlu ldg_dangcsa_dlu_t;

typedef uint32_t (*ldg_dangcsa_dlu_dev_rd_cb_t)(void *ctx, uint64_t addr, uint32_t width, uint64_t *out);
typedef uint32_t (*ldg_dangcsa_dlu_dev_wr_cb_t)(void *ctx, uint64_t addr, uint32_t width, uint64_t val);
typedef uint32_t (*ldg_dangcsa_dlu_dev_tick_cb_t)(void *ctx, ldg_dangcsa_dlu_t *dlu);

typedef struct ldg_dangcsa_dlu_conf
{
    uint32_t ram_size;
    uint32_t tx_cap;
    uint8_t trap_hlt_is;
    uint8_t pudding[7];
} ldg_dangcsa_dlu_conf_t;

struct ldg_dangcsa_dlu
{
    uint8_t *ram;
    uint8_t *tx;
    ldg_dangcsa_dlu_dev_rd_cb_t dev_rd_cb;
    ldg_dangcsa_dlu_dev_wr_cb_t dev_wr_cb;
    ldg_dangcsa_dlu_dev_tick_cb_t dev_tick_cb;
    void *dev_ctx;
    ldg_dangcsa_dlu_conf_t conf;
    uint64_t reg[LDG_DANGCSA_REG_CUNT];
    uint64_t exc_handler;
    uint64_t exc_cause;
    uint64_t last_val;
    uint32_t led;
    uint32_t timer_tgt[LDG_DANGCSA_DLU_TIMER_CUNT];
    uint32_t timer_oneshot[LDG_DANGCSA_DLU_TIMER_CUNT];
    uint32_t last_exc;
    uint32_t last_reg;
    uint32_t fault;
    uint32_t step_cunt;
    uint32_t tx_cunt;
    uint8_t carry;
    uint8_t hlt_is;
    uint8_t is_init;
    uint8_t pudding[5];
};

LDG_EXPORT uint32_t ldg_dangcsa_dlu_conf_default_get(ldg_dangcsa_dlu_conf_t *out);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_init(ldg_dangcsa_dlu_t *dlu, const ldg_dangcsa_dlu_conf_t *conf);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_shutdown(ldg_dangcsa_dlu_t *dlu);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_rst(ldg_dangcsa_dlu_t *dlu);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_ram_wr(ldg_dangcsa_dlu_t *dlu, uint64_t addr, const uint8_t *src, uint32_t len);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_ram_rd(const ldg_dangcsa_dlu_t *dlu, uint64_t addr, uint8_t *dst, uint32_t len);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_reg_get(const ldg_dangcsa_dlu_t *dlu, uint32_t reg, uint64_t *out);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_reg_set(ldg_dangcsa_dlu_t *dlu, uint32_t reg, uint64_t val);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_loc_set(ldg_dangcsa_dlu_t *dlu, uint64_t loc);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_exec(ldg_dangcsa_dlu_t *dlu, ldg_qword_t raw);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_step(ldg_dangcsa_dlu_t *dlu);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_run(ldg_dangcsa_dlu_t *dlu, uint32_t step_max, uint32_t *out_cunt);
LDG_EXPORT uint8_t ldg_dangcsa_dlu_hlt_is(const ldg_dangcsa_dlu_t *dlu);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_tx_get(const ldg_dangcsa_dlu_t *dlu, uint8_t *dst, uint32_t cap, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_tx_rst(ldg_dangcsa_dlu_t *dlu);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_dev_set(ldg_dangcsa_dlu_t *dlu, ldg_dangcsa_dlu_dev_rd_cb_t rd_cb, ldg_dangcsa_dlu_dev_wr_cb_t wr_cb, ldg_dangcsa_dlu_dev_tick_cb_t tick_cb, void *ctx);
LDG_EXPORT uint32_t ldg_dangcsa_dlu_hlt(ldg_dangcsa_dlu_t *dlu, uint32_t fault);

#endif
