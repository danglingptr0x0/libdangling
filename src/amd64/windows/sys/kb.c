#include <windows.h>
#include <setupapi.h>
#include <devpropdef.h>

#define LDG_SYS_KB_CLASS_GUID { 0x4D36E96B, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } }
#define LDG_SYS_KB_BUS_DESC_GUID { 0x540B947E, 0x8B40, 0x45BC, { 0xA8, 0xA2, 0x6A, 0x0B, 0x89, 0x4C, 0xBD, 0xA2 } }
#define LDG_SYS_KB_BUS_DESC_PID 4
#define LDG_SYS_KB_PARENT_GUID { 0x4340A6C5, 0x93FA, 0x4706, { 0x97, 0x2C, 0x7B, 0x64, 0x80, 0x08, 0xA5, 0xA7 } }
#define LDG_SYS_KB_PARENT_PID 8
#define LDG_SYS_KB_ID_CAP 512
#define LDG_SYS_KB_DEPTH_MAX 4
#define LDG_SYS_KB_SEEN_MAX 64
#define LDG_SYS_KB_ID_DIGIT_MAX 8
#define LDG_SYS_KB_NO_FLAG 0
#define LDG_SYS_KB_STEP 1
#define LDG_SYS_KB_WSTR_TERM L'\0'
#define LDG_SYS_KB_TAG_SEP L'_'
#define LDG_SYS_KB_TAG_SEP_ALT L'&'
#define LDG_SYS_KB_ASCII_MAX 0x7F
#define LDG_SYS_KB_UTF8_TERMINATED (-1)
#define LDG_SYS_KB_VID_TAG L"VID"
#define LDG_SYS_KB_PID_TAG L"PID"
#define LDG_SYS_KB_TAG_LEN 3

#include <dangling/sys/info.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/str/str.h>

static uint32_t sys_kb_id_field_get(const WCHAR *id, const WCHAR *tag, uint16_t *out)
{
    uint64_t val = 0;
    uint32_t pos = 0;
    uint32_t idx = 0;
    uint32_t seen = 0;
    byte_t nipple = 0;

    *out = 0;

    for (pos = 0; pos < LDG_SYS_KB_ID_CAP && id[pos] != LDG_SYS_KB_WSTR_TERM; pos++)
    {
        for (idx = 0; idx < LDG_SYS_KB_TAG_LEN; idx++) { if (id[pos + idx] != tag[idx]) { break; } }

        if (idx != LDG_SYS_KB_TAG_LEN) { continue; }

        pos += LDG_SYS_KB_TAG_LEN;

        if (id[pos] != LDG_SYS_KB_TAG_SEP && id[pos] != LDG_SYS_KB_TAG_SEP_ALT) { continue; }

        pos += LDG_SYS_KB_STEP;
        val = 0;
        seen = 0;

        while (seen < LDG_SYS_KB_ID_DIGIT_MAX && id[pos] != LDG_SYS_KB_WSTR_TERM && id[pos] <= LDG_SYS_KB_ASCII_MAX)
        {
            if (ldg_hex_to_nipple((char)id[pos], &nipple) != LDG_ERR_AOK) { break; }

            val = (val << LDG_NIBBLE_BITS) | (uint64_t)nipple;
            seen++;
            pos++;
        }

        if (seen == 0) { return LDG_ERR_NOT_FOUND; }

        *out = (uint16_t)(val & UINT16_MAX);

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t sys_kb_prop_str_get(HDEVINFO set, SP_DEVINFO_DATA *data, const DEVPROPKEY *key, WCHAR *out)
{
    DEVPROPTYPE type = 0;
    DWORD needed = 0;

    out[0] = LDG_SYS_KB_WSTR_TERM;

    if (!SetupDiGetDevicePropertyW(set, data, key, &type, (PBYTE)out, (DWORD)(LDG_SYS_KB_ID_CAP * sizeof(WCHAR)), &needed, LDG_SYS_KB_NO_FLAG)) { return LDG_ERR_NOT_FOUND; }

    out[LDG_SYS_KB_ID_CAP - LDG_SYS_KB_STEP] = LDG_SYS_KB_WSTR_TERM;

    if (type != DEVPROP_TYPE_STRING || out[0] == LDG_SYS_KB_WSTR_TERM)
    {
        out[0] = LDG_SYS_KB_WSTR_TERM;

        return LDG_ERR_NOT_FOUND;
    }

    return LDG_ERR_AOK;
}

static uint32_t sys_kb_utf8_cpy(const WCHAR *src, char *out)
{
    int32_t wrote = 0;

    out[0] = LDG_STR_TERM;

    wrote = WideCharToMultiByte(CP_UTF8, LDG_SYS_KB_NO_FLAG, src, LDG_SYS_KB_UTF8_TERMINATED, out, LDG_SYS_KB_NAME_MAX, 0x0, 0x0);
    if (LDG_UNLIKELY(wrote <= 0))
    {
        out[0] = LDG_STR_TERM;

        return LDG_ERR_MEM_STR_TRUNC;
    }

    out[LDG_SYS_KB_NAME_MAX - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t sys_kb_key_get(const WCHAR *id, uint64_t *out)
{
    uint32_t len = 0;

    while (len < LDG_SYS_KB_ID_CAP && id[len] != LDG_SYS_KB_WSTR_TERM) { len++; }

    return ldg_hash_fnv1a_64_compute((const uint8_t *)id, (uint64_t)len * sizeof(WCHAR), out);
}

static uint32_t sys_kb_dev_rd(HDEVINFO set, SP_DEVINFO_DATA *data, HDEVINFO walk_set, ldg_sys_kb_t *out, uint64_t *out_key)
{
    const DEVPROPKEY bus_desc = { LDG_SYS_KB_BUS_DESC_GUID, LDG_SYS_KB_BUS_DESC_PID };
    const DEVPROPKEY parent = { LDG_SYS_KB_PARENT_GUID, LDG_SYS_KB_PARENT_PID };
    SP_DEVINFO_DATA walk = LDG_STRUCT_ZERO_INIT;
    SP_DEVINFO_DATA up = LDG_STRUCT_ZERO_INIT;
    HDEVINFO walk_owner = INVALID_HANDLE_VALUE;
    WCHAR key_id[LDG_SYS_KB_ID_CAP] = LDG_ARR_ZERO_INIT;
    WCHAR up_id[LDG_SYS_KB_ID_CAP] = LDG_ARR_ZERO_INIT;
    WCHAR name[LDG_SYS_KB_ID_CAP] = LDG_ARR_ZERO_INIT;
    DWORD needed = 0;
    uint32_t depth = 0;
    uint16_t up_vid = 0;
    uint16_t up_pid = 0;

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_sys_kb_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    *out_key = 0;

    if (!SetupDiGetDeviceInstanceIdW(set, data, key_id, LDG_SYS_KB_ID_CAP, &needed)) { return LDG_ERR_NOT_FOUND; }

    key_id[LDG_SYS_KB_ID_CAP - LDG_SYS_KB_STEP] = LDG_SYS_KB_WSTR_TERM;

    if (sys_kb_id_field_get(key_id, LDG_SYS_KB_VID_TAG, &out->vid) != LDG_ERR_AOK) { out->vid = 0; }

    if (sys_kb_id_field_get(key_id, LDG_SYS_KB_PID_TAG, &out->pid) != LDG_ERR_AOK) { out->pid = 0; }

    if (sys_kb_prop_str_get(set, data, &bus_desc, name) != LDG_ERR_AOK) { name[0] = LDG_SYS_KB_WSTR_TERM; }

    walk_owner = set;
    walk = *data;

    for (depth = 0; depth < LDG_SYS_KB_DEPTH_MAX && out->vid != 0; depth++)
    {
        if (sys_kb_prop_str_get(walk_owner, &walk, &parent, up_id) != LDG_ERR_AOK) { break; }

        if (sys_kb_id_field_get(up_id, LDG_SYS_KB_VID_TAG, &up_vid) != LDG_ERR_AOK) { break; }

        if (sys_kb_id_field_get(up_id, LDG_SYS_KB_PID_TAG, &up_pid) != LDG_ERR_AOK) { break; }

        if (up_vid != out->vid || up_pid != out->pid) { break; }

        up.cbSize = (DWORD)sizeof(SP_DEVINFO_DATA);

        if (!SetupDiOpenDeviceInfoW(walk_set, up_id, 0x0, LDG_SYS_KB_NO_FLAG, &up)) { break; }

        if (LDG_UNLIKELY(ldg_mem_secure_cpy(key_id, up_id, (uint64_t)sizeof(key_id)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        if (name[0] == LDG_SYS_KB_WSTR_TERM) { if (sys_kb_prop_str_get(walk_set, &up, &bus_desc, name) != LDG_ERR_AOK) { name[0] = LDG_SYS_KB_WSTR_TERM; } }

        walk_owner = walk_set;
        walk = up;
    }

    if (name[0] == LDG_SYS_KB_WSTR_TERM)
    {
        if (!SetupDiGetDeviceRegistryPropertyW(set, data, SPDRP_DEVICEDESC, 0x0, (PBYTE)name, (DWORD)sizeof(name), &needed)) { name[0] = LDG_SYS_KB_WSTR_TERM; }

        name[LDG_SYS_KB_ID_CAP - LDG_SYS_KB_STEP] = LDG_SYS_KB_WSTR_TERM;
    }

    if (name[0] != LDG_SYS_KB_WSTR_TERM) { if (sys_kb_utf8_cpy(name, out->name) != LDG_ERR_AOK) { out->name[0] = LDG_STR_TERM; } }

    return sys_kb_key_get(key_id, out_key);
}

uint32_t ldg_sys_kb_list(ldg_sys_kb_t *out, uint32_t cap, uint32_t *out_cunt)
{
    GUID kb_class = LDG_SYS_KB_CLASS_GUID;
    SP_DEVINFO_DATA data = LDG_STRUCT_ZERO_INIT;
    HDEVINFO set = INVALID_HANDLE_VALUE;
    HDEVINFO walk_set = INVALID_HANDLE_VALUE;
    ldg_sys_kb_t entry = LDG_STRUCT_ZERO_INIT;
    uint64_t seen[LDG_SYS_KB_SEEN_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t key = 0;
    uint32_t seen_cunt = 0;
    uint32_t total = 0;
    uint32_t idx = 0;
    uint32_t probe = 0;
    uint8_t dup = LDG_TRUTH_FALSE;
    uint8_t released = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    set = SetupDiGetClassDevsW(&kb_class, 0x0, 0x0, DIGCF_PRESENT);
    if (LDG_UNLIKELY(set == INVALID_HANDLE_VALUE)) { return LDG_ERR_UNSUPPORTED; }

    walk_set = SetupDiCreateDeviceInfoList(0x0, 0x0);
    if (LDG_UNLIKELY(walk_set == INVALID_HANDLE_VALUE))
    {
        if (!SetupDiDestroyDeviceInfoList(set)) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_UNSUPPORTED;
    }

    data.cbSize = (DWORD)sizeof(SP_DEVINFO_DATA);

    for (idx = 0; SetupDiEnumDeviceInfo(set, idx, &data); idx++)
    {
        if (sys_kb_dev_rd(set, &data, walk_set, &entry, &key) != LDG_ERR_AOK) { continue; }

        dup = LDG_TRUTH_FALSE;

        for (probe = 0; probe < seen_cunt; probe++) { if (seen[probe] == key) { dup = LDG_TRUTH_TRUE; } }

        if (dup == LDG_TRUTH_TRUE) { continue; }

        if (seen_cunt < LDG_SYS_KB_SEEN_MAX)
        {
            seen[seen_cunt] = key;
            seen_cunt++;
        }

        if (total < cap) { out[total] = entry; }

        total++;
    }

    released = SetupDiDestroyDeviceInfoList(walk_set) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (!SetupDiDestroyDeviceInfoList(set)) { released = LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(released != LDG_TRUTH_TRUE)) { return LDG_ERR_IO_CLOSE; }

    *out_cunt = (total > cap) ? cap : total;

    if (total > cap) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}
