#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define LDG_IO_MIDI_SYSCALL_FAIL (-1)
#define LDG_IO_MIDI_STEP 1
#define LDG_IO_MIDI_CARD_MAX 8
#define LDG_IO_MIDI_DEV_MAX 8
#define LDG_IO_MIDI_PATH_MAX 64
#define LDG_IO_MIDI_PROC_MAX 96
#define LDG_IO_MIDI_RD_MAX 256
#define LDG_IO_MIDI_NODE_FMT "/dev/snd/midiC%" PRIu32 "D%" PRIu32
#define LDG_IO_MIDI_PROC_FMT "/proc/asound/card%" PRIu32 "/midi%" PRIu32
#define LDG_IO_MIDI_NAME_FMT "%.63s"
#define LDG_IO_MIDI_ONE_DATA_LHS 0xC0U
#define LDG_IO_MIDI_ONE_DATA_RHS 0xD0U

#include <dangling/io/midi.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

static uint64_t midi_ring_next_get(uint64_t idx)
{
    uint64_t next = idx;

    next++;

    return next % LDG_IO_MIDI_QUEUE_CAP;
}

static void midi_ring_push(ldg_io_midi_in_t *midi, const ldg_io_midi_msg_t *msg)
{
    uint64_t hd = 0;
    uint64_t next = 0;

    hd = LDG_RD_ONCE(midi->hd);
    next = midi_ring_next_get(hd);

    if (next == LDG_LD_ACQUIRE(midi->tail)) { return; }

    midi->msg[hd] = *msg;

    LDG_ST_RELEASE(midi->hd, next);
}

static uint32_t midi_ring_pop(ldg_io_midi_in_t *midi, ldg_io_midi_msg_t *out)
{
    uint64_t tail = 0;

    tail = LDG_RD_ONCE(midi->tail);

    if (tail == LDG_LD_ACQUIRE(midi->hd)) { return LDG_ERR_EMPTY; }

    *out = midi->msg[tail];

    LDG_ST_RELEASE(midi->tail, midi_ring_next_get(tail));

    return LDG_ERR_AOK;
}

static uint64_t midi_clock_get(void)
{
    struct timespec ts = LDG_STRUCT_ZERO_INIT;
    uint64_t now = 0;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == LDG_IO_MIDI_SYSCALL_FAIL) { return now; }

    now = ((uint64_t)ts.tv_sec * LDG_MS_PER_SEC) + ((uint64_t)ts.tv_nsec / LDG_NS_PER_MS);

    return now;
}

static uint32_t midi_data_cunt_get(uint8_t status)
{
    uint8_t kind = (uint8_t)(status & LDG_IO_MIDI_STATUS_MASK);

    if (kind == LDG_IO_MIDI_ONE_DATA_LHS || kind == LDG_IO_MIDI_ONE_DATA_RHS) { return LDG_IO_MIDI_STEP; }

    return LDG_IO_MIDI_DATA_CUNT_MAX;
}

static void midi_byte_feed(ldg_io_midi_in_t *midi, uint8_t byte, uint32_t stamp)
{
    ldg_io_midi_msg_t msg = LDG_STRUCT_ZERO_INIT;

    msg.timestamp = stamp;

    if (byte >= LDG_IO_MIDI_REALTIME_MIN)
    {
        msg.kind = byte;
        midi_ring_push(midi, &msg);
        return;
    }

    if (byte >= LDG_IO_MIDI_STATUS_MIN)
    {
        midi->pending_cunt = 0;

        if (byte >= LDG_IO_MIDI_SYS_MIN) { midi->run_status = 0; }
        else { midi->run_status = byte; }

        return;
    }

    if (midi->run_status == 0) { return; }

    midi->pending[midi->pending_cunt] = byte;
    midi->pending_cunt++;

    if (midi->pending_cunt < midi_data_cunt_get((uint8_t)midi->run_status)) { return; }

    msg.kind = (uint8_t)(midi->run_status & LDG_IO_MIDI_STATUS_MASK);
    msg.channel = (uint8_t)(midi->run_status & LDG_IO_MIDI_CHANNEL_MASK);
    msg.data[0] = midi->pending[0];
    msg.data[1] = midi->pending[1];

    midi_ring_push(midi, &msg);

    midi->pending_cunt = 0;
    midi->pending[0] = 0;
    midi->pending[1] = 0;
}

static uint32_t midi_name_rd(uint32_t card, uint32_t dev, const char *path, char *out, uint64_t cap)
{
    char proc_path[LDG_IO_MIDI_PROC_MAX] = LDG_ARR_ZERO_INIT;
    char line[LDG_IO_MIDI_PORT_NAME_MAX] = LDG_ARR_ZERO_INIT;
    FILE *file = 0x0;
    char *got = 0x0;
    uint64_t idx = 0;
    int32_t len = 0;

    len = snprintf(proc_path, LDG_IO_MIDI_PROC_MAX, LDG_IO_MIDI_PROC_FMT, card, dev);
    if (len <= 0) { return LDG_ERR_IO_FMT; }

    file = fopen(proc_path, "r");
    if (!file)
    {
        len = snprintf(out, cap, LDG_IO_MIDI_NAME_FMT, path);
        if (len <= 0) { return LDG_ERR_IO_FMT; }

        return LDG_ERR_AOK;
    }

    got = fgets(line, (int32_t)sizeof(line), file);

    if (fclose(file) != 0) { return LDG_ERR_IO_CLOSE; }

    if (!got)
    {
        len = snprintf(out, cap, LDG_IO_MIDI_NAME_FMT, path);
        if (len <= 0) { return LDG_ERR_IO_FMT; }

        return LDG_ERR_AOK;
    }

    while (line[idx] != LDG_STR_TERM)
    {
        if (line[idx] == '\n' || line[idx] == '\r') { line[idx] = LDG_STR_TERM; break; }

        idx++;
    }

    len = snprintf(out, cap, LDG_IO_MIDI_NAME_FMT, line);
    if (len <= 0) { return LDG_ERR_IO_FMT; }

    return LDG_ERR_AOK;
}

static uint32_t midi_node_at_get(uint64_t want, char *out, uint64_t cap, uint32_t *out_card, uint32_t *out_dev)
{
    char path[LDG_IO_MIDI_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t found = 0;
    uint32_t card = 0;
    uint32_t dev = 0;
    int32_t len = 0;

    for (card = 0; card < LDG_IO_MIDI_CARD_MAX; card++)
    {
        for (dev = 0; dev < LDG_IO_MIDI_DEV_MAX; dev++)
        {
            len = snprintf(path, LDG_IO_MIDI_PATH_MAX, LDG_IO_MIDI_NODE_FMT, card, dev);
            if (len <= 0) { continue; }

            if (access(path, F_OK) == LDG_IO_MIDI_SYSCALL_FAIL) { continue; }

            if (found == want)
            {
                len = snprintf(out, cap, LDG_IO_MIDI_NAME_FMT, path);
                if (len <= 0) { return LDG_ERR_IO_FMT; }

                *out_card = card;
                *out_dev = dev;

                return LDG_ERR_AOK;
            }

            found++;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t midi_port_list_build(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    char path[LDG_IO_MIDI_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t found = 0;
    uint32_t card = 0;
    uint32_t dev = 0;
    int32_t len = 0;

    for (card = 0; card < LDG_IO_MIDI_CARD_MAX && found < cap; card++)
    {
        for (dev = 0; dev < LDG_IO_MIDI_DEV_MAX && found < cap; dev++)
        {
            len = snprintf(path, LDG_IO_MIDI_PATH_MAX, LDG_IO_MIDI_NODE_FMT, card, dev);
            if (len <= 0) { continue; }

            if (access(path, F_OK) == LDG_IO_MIDI_SYSCALL_FAIL) { continue; }

            if (midi_name_rd(card, dev, path, out[found].name, LDG_IO_MIDI_PORT_NAME_MAX) != LDG_ERR_AOK) { continue; }

            found++;
        }
    }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

static uint32_t midi_stale_drain(int32_t fd)
{
    ldg_byte_t buff[LDG_IO_MIDI_RD_MAX] = LDG_ARR_ZERO_INIT;
    int64_t rd = 0;

    rd = (int64_t)read(fd, buff, (uint64_t)sizeof(buff));
    while (rd > 0)
    {
        if ((uint64_t)rd < (uint64_t)sizeof(buff)) { return LDG_ERR_AOK; }

        rd = (int64_t)read(fd, buff, (uint64_t)sizeof(buff));
    }

    if (rd == LDG_IO_MIDI_SYSCALL_FAIL && errno != EAGAIN && errno != EWOULDBLOCK) { return LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_in_port_list(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return midi_port_list_build(out, cap, out_cunt);
}

uint32_t ldg_io_midi_out_port_list(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return midi_port_list_build(out, cap, out_cunt);
}

uint32_t ldg_io_midi_in_open(uint64_t idx, ldg_io_midi_in_t *out)
{
    char path[LDG_IO_MIDI_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t card = 0;
    uint32_t dev = 0;
    uint32_t ret = 0;
    int32_t fd = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_io_midi_in_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->handle = LDG_IO_MIDI_HANDLE_NONE;

    ret = midi_node_at_get(idx, path, LDG_IO_MIDI_PATH_MAX, &card, &dev);
    if (ret != LDG_ERR_AOK) { return ret; }

    fd = open(path, O_RDONLY | O_NONBLOCK);
    if (LDG_UNLIKELY(fd < 0))
    {
        if (errno == EACCES) { return LDG_ERR_IO_PERM; }

        if (errno == EBUSY) { return LDG_ERR_BUSY; }

        return LDG_ERR_IO_OPEN;
    }

    ret = midi_stale_drain(fd);
    if (ret != LDG_ERR_AOK)
    {
        close(fd);
        return ret;
    }

    out->handle = (uint64_t)fd;
    out->base = midi_clock_get();
    out->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_in_poll(ldg_io_midi_in_t *midi, ldg_io_midi_msg_t *out, uint64_t cap, uint64_t *out_cunt)
{
    ldg_byte_t buff[LDG_IO_MIDI_RD_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t got = 0;
    uint64_t idx = 0;
    uint32_t stamp = 0;
    int64_t rd = 0;

    if (LDG_UNLIKELY(!midi || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(midi->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    stamp = (uint32_t)(midi_clock_get() - midi->base);

    rd = (int64_t)read((int32_t)midi->handle, buff, (uint64_t)sizeof(buff));
    while (rd > 0)
    {
        for (idx = 0; idx < (uint64_t)rd; idx++) { midi_byte_feed(midi, buff[idx], stamp); }

        if ((uint64_t)rd < (uint64_t)sizeof(buff)) { break; }

        rd = (int64_t)read((int32_t)midi->handle, buff, (uint64_t)sizeof(buff));
    }

    while (got < cap)
    {
        if (midi_ring_pop(midi, &out[got]) != LDG_ERR_AOK) { break; }

        got++;
    }

    *out_cunt = got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_in_close(ldg_io_midi_in_t *midi)
{
    if (LDG_UNLIKELY(!midi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (midi->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    midi->open = LDG_TRUTH_FALSE;

    LDG_SMP_MB();

    if (close((int32_t)midi->handle) == LDG_IO_MIDI_SYSCALL_FAIL)
    {
        midi->handle = LDG_IO_MIDI_HANDLE_NONE;
        return LDG_ERR_IO_CLOSE;
    }

    midi->handle = LDG_IO_MIDI_HANDLE_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_out_open(uint64_t idx, ldg_io_midi_out_t *out)
{
    char path[LDG_IO_MIDI_PATH_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t card = 0;
    uint32_t dev = 0;
    uint32_t ret = 0;
    int32_t fd = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_io_midi_out_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->handle = LDG_IO_MIDI_HANDLE_NONE;

    ret = midi_node_at_get(idx, path, LDG_IO_MIDI_PATH_MAX, &card, &dev);
    if (ret != LDG_ERR_AOK) { return ret; }

    fd = open(path, O_WRONLY | O_NONBLOCK);
    if (LDG_UNLIKELY(fd < 0))
    {
        if (errno == EACCES) { return LDG_ERR_IO_PERM; }

        if (errno == EBUSY) { return LDG_ERR_BUSY; }

        return LDG_ERR_IO_OPEN;
    }

    out->handle = (uint64_t)fd;
    out->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_out_send(ldg_io_midi_out_t *midi, const ldg_io_midi_msg_t *msg)
{
    ldg_byte_t wire[LDG_IO_MIDI_DATA_CUNT_MAX + LDG_IO_MIDI_STEP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    int64_t sent = 0;
    uint8_t status = 0;

    if (LDG_UNLIKELY(!midi || !msg)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(midi->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    if (LDG_UNLIKELY(msg->kind < LDG_IO_MIDI_STATUS_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (msg->kind >= LDG_IO_MIDI_REALTIME_MIN)
    {
        wire[0] = msg->kind;
        len = LDG_IO_MIDI_STEP;
    }
    else
    {
        if (LDG_UNLIKELY(msg->kind >= LDG_IO_MIDI_SYS_MIN)) { return LDG_ERR_UNSUPPORTED; }

        if (LDG_UNLIKELY(msg->channel >= LDG_IO_MIDI_CHANNEL_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

        status = (uint8_t)((msg->kind & LDG_IO_MIDI_STATUS_MASK) | msg->channel);

        wire[0] = status;
        wire[1] = (ldg_byte_t)(msg->data[0] & LDG_IO_MIDI_DATA_MASK);
        wire[2] = (ldg_byte_t)(msg->data[1] & LDG_IO_MIDI_DATA_MASK);

        len = midi_data_cunt_get(status) + LDG_IO_MIDI_STEP;
    }

    sent = (int64_t)write((int32_t)midi->handle, wire, len);
    if (sent == LDG_IO_MIDI_SYSCALL_FAIL)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return LDG_ERR_WOULD_BLOCK; }

        return LDG_ERR_IO_WR;
    }

    if ((uint64_t)sent != len) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_out_close(ldg_io_midi_out_t *midi)
{
    if (LDG_UNLIKELY(!midi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (midi->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    midi->open = LDG_TRUTH_FALSE;

    if (close((int32_t)midi->handle) == LDG_IO_MIDI_SYSCALL_FAIL)
    {
        midi->handle = LDG_IO_MIDI_HANDLE_NONE;
        return LDG_ERR_IO_CLOSE;
    }

    midi->handle = LDG_IO_MIDI_HANDLE_NONE;

    return LDG_ERR_AOK;
}
