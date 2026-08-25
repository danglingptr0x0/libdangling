#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <setupapi.h>

#define LDG_IO_SERIAL_NO_FLAG 0
#define LDG_IO_SERIAL_PROBE_MAX 256
#define LDG_IO_SERIAL_WIDE_MAX 128
#define LDG_IO_SERIAL_MB_NO_FLAG 0
#define LDG_IO_SERIAL_MB_NULL_TERM (-1)
#define LDG_IO_SERIAL_STEP 1
#define LDG_IO_SERIAL_TIMEOUT_NONE 0
#define LDG_IO_SERIAL_DATA_BITS_5 5
#define LDG_IO_SERIAL_DATA_BITS_6 6
#define LDG_IO_SERIAL_DATA_BITS_7 7
#define LDG_IO_SERIAL_DATA_BITS_8 8
#define LDG_IO_SERIAL_RATE_230400 230400
#define LDG_IO_SERIAL_RATE_460800 460800
#define LDG_IO_SERIAL_RATE_921600 921600
#define LDG_IO_SERIAL_PORTS_CLASS_GUID { 0x4D36E978, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } }
#define LDG_IO_SERIAL_PORT_VAL L"PortName"

#include <dangling/io/serial.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

static uint32_t serial_baud_translate(ldg_io_serial_baud_t baud, DWORD *out)
{
    switch (baud)
    {
        case LDG_IO_SERIAL_BAUD_1200:
            *out = CBR_1200;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_2400:
            *out = CBR_2400;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_4800:
            *out = CBR_4800;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_9600:
            *out = CBR_9600;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_19200:
            *out = CBR_19200;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_38400:
            *out = CBR_38400;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_57600:
            *out = CBR_57600;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_115200:
            *out = CBR_115200;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_230400:
            *out = LDG_IO_SERIAL_RATE_230400;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_460800:
            *out = LDG_IO_SERIAL_RATE_460800;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_921600:
            *out = LDG_IO_SERIAL_RATE_921600;
            return LDG_ERR_AOK;
        default:
            break;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t serial_data_translate(ldg_io_serial_data_t data, BYTE *out)
{
    switch (data)
    {
        case LDG_IO_SERIAL_DATA_5:
            *out = LDG_IO_SERIAL_DATA_BITS_5;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_DATA_6:
            *out = LDG_IO_SERIAL_DATA_BITS_6;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_DATA_7:
            *out = LDG_IO_SERIAL_DATA_BITS_7;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_DATA_8:
            *out = LDG_IO_SERIAL_DATA_BITS_8;
            return LDG_ERR_AOK;
        default:
            break;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t serial_dcb_apply(HANDLE handle, const ldg_io_serial_conf_t *conf)
{
    DCB dcb = LDG_STRUCT_ZERO_INIT;
    COMMTIMEOUTS timeout = LDG_STRUCT_ZERO_INIT;
    DWORD rate = 0;
    BYTE bits = 0;
    uint32_t ret = 0;

    ret = serial_baud_translate(conf->baud, &rate);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = serial_data_translate(conf->data, &bits);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->parity > LDG_IO_SERIAL_PARITY_EVEN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->stop > LDG_IO_SERIAL_STOP_TWO)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->flow > LDG_IO_SERIAL_FLOW_HARD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dcb.DCBlength = sizeof(DCB);

    if (LDG_UNLIKELY(!GetCommState(handle, &dcb))) { return LDG_ERR_IO_OPEN; }

    dcb.BaudRate = rate;
    dcb.ByteSize = bits;
    dcb.fBinary = TRUE;
    dcb.fParity = (conf->parity == LDG_IO_SERIAL_PARITY_NONE) ? FALSE : TRUE;
    dcb.Parity = NOPARITY;

    if (conf->parity == LDG_IO_SERIAL_PARITY_ODD) { dcb.Parity = ODDPARITY; }
    else if (conf->parity == LDG_IO_SERIAL_PARITY_EVEN) { dcb.Parity = EVENPARITY; }

    dcb.StopBits = (conf->stop == LDG_IO_SERIAL_STOP_TWO) ? TWOSTOPBITS : ONESTOPBIT;

    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fAbortOnError = FALSE;

    if (conf->flow == LDG_IO_SERIAL_FLOW_SOFT)
    {
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
    }
    else if (conf->flow == LDG_IO_SERIAL_FLOW_HARD)
    {
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    }

    if (LDG_UNLIKELY(!SetCommState(handle, &dcb))) { return LDG_ERR_IO_OPEN; }

    timeout.ReadIntervalTimeout = MAXDWORD;
    timeout.ReadTotalTimeoutMultiplier = LDG_IO_SERIAL_TIMEOUT_NONE;
    timeout.ReadTotalTimeoutConstant = LDG_IO_SERIAL_TIMEOUT_NONE;
    timeout.WriteTotalTimeoutMultiplier = LDG_IO_SERIAL_TIMEOUT_NONE;
    timeout.WriteTotalTimeoutConstant = LDG_IO_SERIAL_TIMEOUT_NONE;

    if (LDG_UNLIKELY(!SetCommTimeouts(handle, &timeout))) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_conf_default_get(ldg_io_serial_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->baud = LDG_IO_SERIAL_BAUD_115200;
    out->data = LDG_IO_SERIAL_DATA_8;
    out->parity = LDG_IO_SERIAL_PARITY_NONE;
    out->stop = LDG_IO_SERIAL_STOP_ONE;
    out->flow = LDG_IO_SERIAL_FLOW_NONE;

    return LDG_ERR_AOK;
}

static uint32_t serial_port_name_rd(HDEVINFO info_set, SP_DEVINFO_DATA *dev, char *out, uint64_t cap)
{
    HKEY key = 0x0;
    WCHAR wide_name[LDG_IO_SERIAL_WIDE_MAX] = LDG_ARR_ZERO_INIT;
    DWORD size = sizeof(wide_name);
    DWORD kind = 0;

    out[0] = LDG_STR_TERM;

    key = SetupDiOpenDevRegKey(info_set, dev, DICS_FLAG_GLOBAL, LDG_IO_SERIAL_NO_FLAG, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE) { return LDG_ERR_NOT_FOUND; }

    if (RegQueryValueExW(key, LDG_IO_SERIAL_PORT_VAL, 0x0, &kind, (LPBYTE)wide_name, &size) != ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return LDG_ERR_NOT_FOUND;
    }

    RegCloseKey(key);

    if (WideCharToMultiByte(CP_UTF8, LDG_IO_SERIAL_MB_NO_FLAG, wide_name, LDG_IO_SERIAL_MB_NULL_TERM, out, (int32_t)cap, 0x0, 0x0) <= 0) { return LDG_ERR_MEM_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_port_list(ldg_io_serial_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    GUID ports_class = LDG_IO_SERIAL_PORTS_CLASS_GUID;
    SP_DEVINFO_DATA dev = LDG_STRUCT_ZERO_INIT;
    HDEVINFO info_set = INVALID_HANDLE_VALUE;
    WCHAR wide_id[LDG_IO_SERIAL_ID_MAX] = LDG_ARR_ZERO_INIT;
    char name[LDG_IO_SERIAL_PATH_MAX] = LDG_ARR_ZERO_INIT;
    char path[LDG_IO_SERIAL_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t found = 0;
    DWORD idx = 0;
    int32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    info_set = SetupDiGetClassDevsW(&ports_class, 0x0, 0x0, DIGCF_PRESENT);
    if (LDG_UNLIKELY(info_set == INVALID_HANDLE_VALUE)) { return LDG_ERR_NOT_FOUND; }

    dev.cbSize = sizeof(SP_DEVINFO_DATA);

    for (idx = 0; SetupDiEnumDeviceInfo(info_set, idx, &dev); idx++)
    {
        ret = serial_port_name_rd(info_set, &dev, name, LDG_IO_SERIAL_PATH_MAX);
        if (ret != LDG_ERR_AOK) { continue; }

        len = snprintf(path, LDG_IO_SERIAL_PATH_MAX, "\\\\.\\%.32s", name);
        if (len <= 0) { continue; }

        if ((uint64_t)len >= LDG_IO_SERIAL_PATH_MAX) { continue; }

        ret = ldg_mem_secure_cpy(out[found].path, path, (uint64_t)len + LDG_STR_TERM_SIZE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            SetupDiDestroyDeviceInfoList(info_set);
            return ret;
        }

        out[found].id[0] = LDG_STR_TERM;

        if (SetupDiGetDeviceInstanceIdW(info_set, &dev, wide_id, LDG_IO_SERIAL_ID_MAX, 0x0)) { if (WideCharToMultiByte(CP_UTF8, LDG_IO_SERIAL_MB_NO_FLAG, wide_id, LDG_IO_SERIAL_MB_NULL_TERM, out[found].id, LDG_IO_SERIAL_ID_MAX, 0x0, 0x0) <= 0) { out[found].id[0] = LDG_STR_TERM; } }

        found += LDG_IO_SERIAL_STEP;
        if (found >= cap) { break; }
    }

    SetupDiDestroyDeviceInfoList(info_set);

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_open(const char *path, const ldg_io_serial_conf_t *conf, ldg_io_serial_t *out)
{
    WCHAR wide_path[LDG_IO_SERIAL_WIDE_MAX] = LDG_ARR_ZERO_INIT;
    HANDLE handle = INVALID_HANDLE_VALUE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_io_serial_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->handle = LDG_IO_SERIAL_HANDLE_NONE;

    if (LDG_UNLIKELY(MultiByteToWideChar(CP_UTF8, LDG_IO_SERIAL_MB_NO_FLAG, path, LDG_IO_SERIAL_MB_NULL_TERM, wide_path, LDG_IO_SERIAL_WIDE_MAX) <= 0)) { return LDG_ERR_MEM_STR_TRUNC; }

    handle = CreateFileW(wide_path, GENERIC_READ | GENERIC_WRITE, LDG_IO_SERIAL_NO_FLAG, 0x0, OPEN_EXISTING, LDG_IO_SERIAL_NO_FLAG, 0x0);
    if (LDG_UNLIKELY(handle == INVALID_HANDLE_VALUE))
    {
        if (GetLastError() == ERROR_ACCESS_DENIED) { return LDG_ERR_IO_PERM; }

        if (GetLastError() == ERROR_FILE_NOT_FOUND) { return LDG_ERR_IO_NOT_FOUND; }

        return LDG_ERR_IO_OPEN;
    }

    ret = serial_dcb_apply(handle, conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        CloseHandle(handle);
        return ret;
    }

    out->handle = (uint64_t)(uintptr_t)handle;
    out->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_conf_set(ldg_io_serial_t *port, const ldg_io_serial_conf_t *conf)
{
    if (LDG_UNLIKELY(!port || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    return serial_dcb_apply((HANDLE)(uintptr_t)port->handle, conf);
}

uint32_t ldg_io_serial_rd(ldg_io_serial_t *port, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    DWORD got = 0;

    if (LDG_UNLIKELY(!port || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    if (LDG_UNLIKELY(cap > UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!ReadFile((HANDLE)(uintptr_t)port->handle, buff, (DWORD)cap, &got, 0x0))
    {
        if (GetLastError() == ERROR_OPERATION_ABORTED) { return LDG_ERR_CLOSED; }

        return LDG_ERR_IO_RD;
    }

    if (got == 0) { return LDG_ERR_WOULD_BLOCK; }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_wr(ldg_io_serial_t *port, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent)
{
    DWORD sent = 0;

    if (LDG_UNLIKELY(!port || !buff || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!WriteFile((HANDLE)(uintptr_t)port->handle, buff, (DWORD)len, &sent, 0x0))
    {
        if (GetLastError() == ERROR_OPERATION_ABORTED) { return LDG_ERR_CLOSED; }

        return LDG_ERR_IO_WR;
    }

    *out_sent = (uint64_t)sent;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_flush(ldg_io_serial_t *port)
{
    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!PurgeComm((HANDLE)(uintptr_t)port->handle, PURGE_RXCLEAR | PURGE_TXCLEAR))) { return LDG_ERR_IO_FLUSH; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_close(ldg_io_serial_t *port)
{
    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (port->handle == LDG_IO_SERIAL_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!CloseHandle((HANDLE)(uintptr_t)port->handle)))
    {
        port->handle = LDG_IO_SERIAL_HANDLE_NONE;
        port->open = LDG_TRUTH_FALSE;
        return LDG_ERR_IO_CLOSE;
    }

    port->handle = LDG_IO_SERIAL_HANDLE_NONE;
    port->open = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_dtr_set(ldg_io_serial_t *port, uint8_t en)
{
    DWORD func = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    func = (en == LDG_TRUTH_TRUE) ? SETDTR : CLRDTR;

    if (LDG_UNLIKELY(!EscapeCommFunction((HANDLE)(uintptr_t)port->handle, func))) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_rts_set(ldg_io_serial_t *port, uint8_t en)
{
    DWORD func = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    func = (en == LDG_TRUTH_TRUE) ? SETRTS : CLRRTS;

    if (LDG_UNLIKELY(!EscapeCommFunction((HANDLE)(uintptr_t)port->handle, func))) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_serial_cts_is(ldg_io_serial_t *port)
{
    DWORD state = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!GetCommModemStatus((HANDLE)(uintptr_t)port->handle, &state))) { return LDG_ERR_IO_RD; }

    if ((state & MS_CTS_ON) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_io_serial_dsr_is(ldg_io_serial_t *port)
{
    DWORD state = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!GetCommModemStatus((HANDLE)(uintptr_t)port->handle, &state))) { return LDG_ERR_IO_RD; }

    if ((state & MS_DSR_ON) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
