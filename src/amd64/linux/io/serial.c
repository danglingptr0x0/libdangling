#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define LDG_IO_SERIAL_SYSCALL_FAIL (-1)
#define LDG_IO_SERIAL_NO_FLAG 0
#define LDG_IO_SERIAL_PROBE_MAX 32
#define LDG_IO_SERIAL_PREFIX_CUNT 4
#define LDG_IO_SERIAL_STEP 1
#define LDG_IO_SERIAL_SYSFS_MAX 256
#define LDG_IO_SERIAL_ATTR_MAX 96
#define LDG_IO_SERIAL_DEV_PREFIX_LEN 5
#define LDG_IO_SERIAL_CRTSCTS 0x80000000U

#include <dangling/io/serial.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>

static uint32_t serial_baud_translate(ldg_io_serial_baud_t baud, speed_t *out)
{
    switch (baud)
    {
        case LDG_IO_SERIAL_BAUD_1200:
            *out = B1200;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_2400:
            *out = B2400;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_4800:
            *out = B4800;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_9600:
            *out = B9600;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_19200:
            *out = B19200;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_38400:
            *out = B38400;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_57600:
            *out = B57600;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_115200:
            *out = B115200;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_230400:
            *out = B230400;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_460800:
            *out = B460800;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_BAUD_921600:
            *out = B921600;
            return LDG_ERR_AOK;
        default:
            break;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t serial_data_translate(ldg_io_serial_data_t data, tcflag_t *out)
{
    switch (data)
    {
        case LDG_IO_SERIAL_DATA_5:
            *out = CS5;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_DATA_6:
            *out = CS6;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_DATA_7:
            *out = CS7;
            return LDG_ERR_AOK;
        case LDG_IO_SERIAL_DATA_8:
            *out = CS8;
            return LDG_ERR_AOK;
        default:
            break;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static uint32_t serial_termios_apply(int32_t fd, const ldg_io_serial_conf_t *conf)
{
    struct termios tio = LDG_STRUCT_ZERO_INIT;
    speed_t speed = 0;
    tcflag_t bits = 0;
    uint32_t ret = 0;

    ret = serial_baud_translate(conf->baud, &speed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = serial_data_translate(conf->data, &bits);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->parity > LDG_IO_SERIAL_PARITY_EVEN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->stop > LDG_IO_SERIAL_STOP_TWO)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->flow > LDG_IO_SERIAL_FLOW_HARD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(tcgetattr(fd, &tio) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    tio.c_cflag = bits | CLOCAL | CREAD;
    tio.c_iflag = LDG_IO_SERIAL_NO_FLAG;
    tio.c_oflag = LDG_IO_SERIAL_NO_FLAG;
    tio.c_lflag = LDG_IO_SERIAL_NO_FLAG;

    if (conf->parity == LDG_IO_SERIAL_PARITY_ODD) { tio.c_cflag |= PARENB | PARODD; }
    else if (conf->parity == LDG_IO_SERIAL_PARITY_EVEN) { tio.c_cflag |= PARENB; }

    if (conf->stop == LDG_IO_SERIAL_STOP_TWO) { tio.c_cflag |= CSTOPB; }

    if (conf->flow == LDG_IO_SERIAL_FLOW_SOFT) { tio.c_iflag |= IXON | IXOFF; }
    else if (conf->flow == LDG_IO_SERIAL_FLOW_HARD) { tio.c_cflag |= LDG_IO_SERIAL_CRTSCTS; }

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (LDG_UNLIKELY(cfsetispeed(&tio, speed) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(cfsetospeed(&tio, speed) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(tcsetattr(fd, TCSANOW, &tio) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_AOK;
}

static uint32_t serial_sysfs_rd(const char *path, char *out, uint64_t cap)
{
    int32_t fd = 0;
    int64_t got = 0;
    uint64_t idx = 0;

    out[0] = LDG_STR_TERM;

    fd = (int32_t)open(path, O_RDONLY);
    if (fd < 0) { return LDG_ERR_IO_NOT_FOUND; }

    got = (int64_t)read(fd, out, cap - LDG_STR_TERM_SIZE);

    if (LDG_UNLIKELY(close(fd) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (got <= 0) { return LDG_ERR_IO_RD; }

    out[got] = LDG_STR_TERM;

    for (idx = 0; out[idx] != LDG_STR_TERM; idx++) { if (out[idx] == '\n')
        {
            out[idx] = LDG_STR_TERM;
            break;
        }
    }

    if (out[0] == LDG_STR_TERM) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t serial_id_build(const char *dev_name, char *out, uint64_t cap)
{
    char attr_path[LDG_IO_SERIAL_SYSFS_MAX] = LDG_ARR_ZERO_INIT;
    char vendor[LDG_IO_SERIAL_ATTR_MAX] = LDG_ARR_ZERO_INIT;
    char product[LDG_IO_SERIAL_ATTR_MAX] = LDG_ARR_ZERO_INIT;
    char serial[LDG_IO_SERIAL_ATTR_MAX] = LDG_ARR_ZERO_INIT;
    int32_t len = 0;
    uint32_t ret = 0;

    out[0] = LDG_STR_TERM;

    len = snprintf(attr_path, LDG_IO_SERIAL_SYSFS_MAX, "/sys/class/tty/%.32s/device/../idVendor", dev_name);
    if (len <= 0) { return LDG_ERR_NOT_FOUND; }

    ret = serial_sysfs_rd(attr_path, vendor, LDG_IO_SERIAL_ATTR_MAX);
    if (ret != LDG_ERR_AOK) { return ret; }

    len = snprintf(attr_path, LDG_IO_SERIAL_SYSFS_MAX, "/sys/class/tty/%.32s/device/../idProduct", dev_name);
    if (len <= 0) { return LDG_ERR_NOT_FOUND; }

    ret = serial_sysfs_rd(attr_path, product, LDG_IO_SERIAL_ATTR_MAX);
    if (ret != LDG_ERR_AOK) { return ret; }

    len = snprintf(attr_path, LDG_IO_SERIAL_SYSFS_MAX, "/sys/class/tty/%.32s/device/../serial", dev_name);
    if (len <= 0) { return LDG_ERR_NOT_FOUND; }

    ret = serial_sysfs_rd(attr_path, serial, LDG_IO_SERIAL_ATTR_MAX);
    if (ret != LDG_ERR_AOK) { serial[0] = LDG_STR_TERM; }

    len = snprintf(out, cap, "USB\\VID_%.8s&PID_%.8s\\%.64s", vendor, product, serial);
    if (len <= 0) { return LDG_ERR_NOT_FOUND; }

    if ((uint64_t)len >= cap) { return LDG_ERR_MEM_STR_TRUNC; }

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

uint32_t ldg_io_serial_port_list(ldg_io_serial_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    const char *const prefix[LDG_IO_SERIAL_PREFIX_CUNT] = { "/dev/ttyS", "/dev/ttyUSB", "/dev/ttyACM", "/dev/ttyAMA" };
    char path[LDG_IO_SERIAL_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t found = 0;
    uint64_t group = 0;
    uint64_t idx = 0;
    int32_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (group = 0; group < LDG_IO_SERIAL_PREFIX_CUNT; group++)
    {
        for (idx = 0; idx < LDG_IO_SERIAL_PROBE_MAX; idx++)
        {
            len = snprintf(path, LDG_IO_SERIAL_PATH_MAX, "%.16s%" PRIu32, prefix[group], (uint32_t)idx);
            if (len <= 0) { continue; }

            if ((uint64_t)len >= LDG_IO_SERIAL_PATH_MAX) { continue; }

            if (access(path, F_OK) != 0) { continue; }

            ret = ldg_mem_secure_cpy(out[found].path, path, (uint64_t)len + LDG_STR_TERM_SIZE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = serial_id_build(&path[LDG_IO_SERIAL_DEV_PREFIX_LEN], out[found].id, LDG_IO_SERIAL_ID_MAX);
            if (ret != LDG_ERR_AOK) { out[found].id[0] = LDG_STR_TERM; }

            found += LDG_IO_SERIAL_STEP;
            if (found >= cap)
            {
                *out_cunt = found;
                return LDG_ERR_AOK;
            }
        }
    }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_open(const char *path, const ldg_io_serial_conf_t *conf, ldg_io_serial_t *out)
{
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_io_serial_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->handle = LDG_IO_SERIAL_HANDLE_NONE;

    fd = (int32_t)open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (LDG_UNLIKELY(fd < 0))
    {
        if (errno == EACCES) { return LDG_ERR_IO_PERM; }

        if (errno == ENOENT) { return LDG_ERR_IO_NOT_FOUND; }

        if (errno == EBUSY) { return LDG_ERR_BUSY; }

        return LDG_ERR_IO_OPEN;
    }

    ret = serial_termios_apply(fd, conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        close(fd);
        return ret;
    }

    out->handle = (uint64_t)(uint32_t)fd;
    out->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_conf_set(ldg_io_serial_t *port, const ldg_io_serial_conf_t *conf)
{
    if (LDG_UNLIKELY(!port || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    return serial_termios_apply((int32_t)port->handle, conf);
}

uint32_t ldg_io_serial_rd(ldg_io_serial_t *port, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got)
{
    int64_t got = 0;

    if (LDG_UNLIKELY(!port || !buff || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    got = (int64_t)read((int32_t)port->handle, buff, cap);
    if (got > 0)
    {
        *out_got = (uint64_t)got;
        return LDG_ERR_AOK;
    }

    if (got == 0) { return LDG_ERR_WOULD_BLOCK; }

    if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

    if (errno == EINTR) { return LDG_ERR_INTERRUPTED; }

    if (errno == EIO) { return LDG_ERR_CLOSED; }

    return LDG_ERR_IO_RD;
}

uint32_t ldg_io_serial_wr(ldg_io_serial_t *port, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent)
{
    int64_t sent = 0;

    if (LDG_UNLIKELY(!port || !buff || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_CLOSED; }

    if (len == 0) { return LDG_ERR_AOK; }

    sent = (int64_t)write((int32_t)port->handle, buff, len);
    if (sent >= 0)
    {
        *out_sent = (uint64_t)sent;
        return LDG_ERR_AOK;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

    if (errno == EINTR) { return LDG_ERR_INTERRUPTED; }

    if (errno == EIO) { return LDG_ERR_CLOSED; }

    return LDG_ERR_IO_WR;
}

uint32_t ldg_io_serial_flush(ldg_io_serial_t *port)
{
    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(tcflush((int32_t)port->handle, TCIOFLUSH) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_FLUSH; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_close(ldg_io_serial_t *port)
{
    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (port->handle == LDG_IO_SERIAL_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(close((int32_t)port->handle) != 0))
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
    int32_t bit = TIOCM_DTR;
    int32_t req = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    req = (en == LDG_TRUTH_TRUE) ? TIOCMBIS : TIOCMBIC;

    if (LDG_UNLIKELY(ioctl((int32_t)port->handle, req, &bit) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_serial_rts_set(ldg_io_serial_t *port, uint8_t en)
{
    int32_t bit = TIOCM_RTS;
    int32_t req = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    req = (en == LDG_TRUTH_TRUE) ? TIOCMBIS : TIOCMBIC;

    if (LDG_UNLIKELY(ioctl((int32_t)port->handle, req, &bit) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_serial_cts_is(ldg_io_serial_t *port)
{
    int32_t state = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ioctl((int32_t)port->handle, TIOCMGET, &state) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_RD; }

    if ((state & TIOCM_CTS) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_io_serial_dsr_is(ldg_io_serial_t *port)
{
    int32_t state = 0;

    if (LDG_UNLIKELY(!port)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(port->handle == LDG_IO_SERIAL_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ioctl((int32_t)port->handle, TIOCMGET, &state) == LDG_IO_SERIAL_SYSCALL_FAIL)) { return LDG_ERR_IO_RD; }

    if ((state & TIOCM_DSR) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
