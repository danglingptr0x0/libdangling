#include <stdint.h>
#include <windows.h>

#include <dangling/sys/scan.h>
#include <dangling/sys/lib.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>

#define LDG_SYS_SCAN_LIB "amsi.dll"
#define LDG_SYS_SCAN_MB_NULL_TERM (-1)
#define LDG_SYS_SCAN_RES_BLK_MIN 0x4000

typedef HRESULT (WINAPI *sys_scan_init_fn_t)(LPCWSTR app_name, void **out_ctx);
typedef void (WINAPI *sys_scan_uninit_fn_t)(void *amsi_ctx);
typedef HRESULT (WINAPI *sys_scan_sesh_open_fn_t)(void *amsi_ctx, void **out_sesh);
typedef void (WINAPI *sys_scan_sesh_close_fn_t)(void *amsi_ctx, void *sesh);
typedef HRESULT (WINAPI *sys_scan_buff_fn_t)(void *amsi_ctx, const void *buff, ULONG len, LPCWSTR content_name, void *sesh, int32_t *out_res);

typedef struct sys_scan_ctx
{
    void *lib;
    void *amsi_ctx;
    void *sesh;
    sys_scan_uninit_fn_t uninit_fn;
    sys_scan_sesh_close_fn_t sesh_close_fn;
    sys_scan_buff_fn_t buff_fn;
} sys_scan_ctx_t;

static uint32_t sys_scan_sym_get(void *lib, const char *name, void *out, uint64_t out_size)
{
    ldg_sys_lib_func_t func = 0x0;
    uint32_t ret = 0;

    ret = ldg_sys_lib_sym_get(lib, name, &func);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_cpy(out, &func, out_size);
}

static uint32_t sys_scan_teardown(sys_scan_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (ctx->sesh && ctx->sesh_close_fn) { ctx->sesh_close_fn(ctx->amsi_ctx, ctx->sesh); }

    if (ctx->amsi_ctx && ctx->uninit_fn) { ctx->uninit_fn(ctx->amsi_ctx); }

    if (ctx->lib && ldg_sys_lib_close(ctx->lib) != LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ldg_mem_dealloc(ctx) != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = LDG_ERR_MEM_BAD; }

    return ret;
}

uint32_t ldg_sys_scan_ctx_create(const char *app_name, void **out)
{
    WCHAR wide_name[LDG_SYS_SCAN_NAME_MAX] = LDG_ARR_ZERO_INIT;
    sys_scan_ctx_t *ctx = 0x0;
    sys_scan_init_fn_t init_fn = 0x0;
    sys_scan_sesh_open_fn_t sesh_open_fn = 0x0;
    void *blk = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!app_name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, app_name, LDG_SYS_SCAN_MB_NULL_TERM, wide_name, LDG_SYS_SCAN_NAME_MAX) <= 0)) { return LDG_ERR_STR_INVALID; }

    ret = ldg_mem_alloc(sizeof(sys_scan_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (sys_scan_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_secure_zero(ctx, sizeof(sys_scan_ctx_t)) != LDG_ERR_AOK))
    {
        if (ldg_mem_dealloc(ctx) != LDG_ERR_AOK) { return LDG_ERR_MEM_BAD; }

        return LDG_ERR_MEM_BAD;
    }

    if (ldg_sys_lib_open(LDG_SYS_SCAN_LIB, &ctx->lib) != LDG_ERR_AOK)
    {
        if (sys_scan_teardown(ctx) != LDG_ERR_AOK) { return LDG_ERR_MEM_BAD; }

        return LDG_ERR_UNSUPPORTED;
    }

    ret = sys_scan_sym_get(ctx->lib, "AmsiInitialize", &init_fn, sizeof(init_fn));
    if (ret == LDG_ERR_AOK) { ret = sys_scan_sym_get(ctx->lib, "AmsiUninitialize", &ctx->uninit_fn, sizeof(ctx->uninit_fn)); }

    if (ret == LDG_ERR_AOK) { ret = sys_scan_sym_get(ctx->lib, "AmsiOpenSession", &sesh_open_fn, sizeof(sesh_open_fn)); }

    if (ret == LDG_ERR_AOK) { ret = sys_scan_sym_get(ctx->lib, "AmsiCloseSession", &ctx->sesh_close_fn, sizeof(ctx->sesh_close_fn)); }

    if (ret == LDG_ERR_AOK) { ret = sys_scan_sym_get(ctx->lib, "AmsiScanBuffer", &ctx->buff_fn, sizeof(ctx->buff_fn)); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (sys_scan_teardown(ctx) != LDG_ERR_AOK) { return LDG_ERR_MEM_BAD; }

        return LDG_ERR_UNSUPPORTED;
    }

    if (LDG_UNLIKELY(FAILED(init_fn(wide_name, &ctx->amsi_ctx))))
    {
        ctx->amsi_ctx = 0x0;
        if (sys_scan_teardown(ctx) != LDG_ERR_AOK) { return LDG_ERR_MEM_BAD; }

        return LDG_ERR_DENIED;
    }

    if (LDG_UNLIKELY(FAILED(sesh_open_fn(ctx->amsi_ctx, &ctx->sesh))))
    {
        ctx->sesh = 0x0;
        if (sys_scan_teardown(ctx) != LDG_ERR_AOK) { return LDG_ERR_MEM_BAD; }

        return LDG_ERR_DENIED;
    }

    *out = ctx;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_scan_ctx_destroy(void **ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = sys_scan_teardown((sys_scan_ctx_t *)*ctx);
    *ctx = 0x0;

    return ret;
}

uint8_t ldg_sys_scan_clean_is(void *ctx, const ldg_byte_t *buff, uint64_t len, const char *content_name)
{
    WCHAR wide_name[LDG_SYS_SCAN_NAME_MAX] = LDG_ARR_ZERO_INIT;
    sys_scan_ctx_t *scan = (sys_scan_ctx_t *)ctx;
    int32_t res = 0;

    if (LDG_UNLIKELY(!scan || !buff || !content_name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0 || len > (uint64_t)ULONG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, content_name, LDG_SYS_SCAN_MB_NULL_TERM, wide_name, LDG_SYS_SCAN_NAME_MAX) <= 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(FAILED(scan->buff_fn(scan->amsi_ctx, buff, (ULONG)len, wide_name, scan->sesh, &res)))) { return LDG_ERR_DENIED; }

    if (res < LDG_SYS_SCAN_RES_BLK_MIN) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
