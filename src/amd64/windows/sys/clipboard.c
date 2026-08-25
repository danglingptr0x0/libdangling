#include <windows.h>

#include <dangling/sys/clipboard.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SYS_CLIPBOARD_NO_WINDOW 0x0

uint32_t ldg_sys_clipboard_set(const ldg_byte_t *text, uint64_t len)
{
    HGLOBAL blk = 0x0;
    ldg_byte_t *raw = 0x0;
    uint64_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0 || len > LDG_SYS_CLIPBOARD_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    blk = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)(len + LDG_STR_TERM_SIZE));
    if (LDG_UNLIKELY(!blk)) { return LDG_ERR_ALLOC_NULL; }

    raw = (ldg_byte_t *)GlobalLock(blk);
    if (LDG_UNLIKELY(!raw))
    {
        GlobalFree(blk);
        return LDG_ERR_MEM_BAD;
    }

    for (idx = 0; idx < len; idx++) { raw[idx] = text[idx]; }

    raw[len] = LDG_STR_TERM;

    if (LDG_UNLIKELY(!GlobalUnlock(blk) && GetLastError() != NO_ERROR))
    {
        GlobalFree(blk);
        return LDG_ERR_MEM_BAD;
    }

    if (LDG_UNLIKELY(!OpenClipboard(LDG_SYS_CLIPBOARD_NO_WINDOW)))
    {
        GlobalFree(blk);
        return LDG_ERR_BUSY;
    }

    if (LDG_UNLIKELY(!EmptyClipboard())) { ret = LDG_ERR_DENIED; }

    if (ret == LDG_ERR_AOK) { if (LDG_UNLIKELY(!SetClipboardData(CF_TEXT, blk))) { ret = LDG_ERR_DENIED; } }

    if (LDG_UNLIKELY(!CloseClipboard() && ret == LDG_ERR_AOK)) { ret = LDG_ERR_IO_CLOSE; }

    if (ret != LDG_ERR_AOK) { GlobalFree(blk); }

    return ret;
}

uint32_t ldg_sys_clipboard_get(ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    HANDLE blk = 0x0;
    const ldg_byte_t *raw = 0x0;
    uint64_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!OpenClipboard(LDG_SYS_CLIPBOARD_NO_WINDOW))) { return LDG_ERR_BUSY; }

    blk = GetClipboardData(CF_TEXT);
    if (!blk)
    {
        if (LDG_UNLIKELY(!CloseClipboard())) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_EMPTY;
    }

    raw = (const ldg_byte_t *)GlobalLock(blk);
    if (LDG_UNLIKELY(!raw))
    {
        if (LDG_UNLIKELY(!CloseClipboard())) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_MEM_BAD;
    }

    while (raw[idx] != LDG_STR_TERM && idx < cap)
    {
        out[idx] = raw[idx];
        idx++;
    }

    if (raw[idx] != LDG_STR_TERM) { ret = LDG_ERR_FULL; }

    if (LDG_UNLIKELY(!GlobalUnlock(blk) && GetLastError() != NO_ERROR)) { ret = LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(!CloseClipboard() && ret == LDG_ERR_AOK)) { ret = LDG_ERR_IO_CLOSE; }

    if (ret == LDG_ERR_AOK) { *out_len = idx; }

    return ret;
}
