#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>

#include <dangling/io/midi.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

#define LDG_IO_MIDI_BYTE_MASK 0xFFU
#define LDG_IO_MIDI_DATA_LHS_SHIFT 8
#define LDG_IO_MIDI_DATA_RHS_SHIFT 16
#define LDG_IO_MIDI_NO_FLAG 0
#define LDG_IO_MIDI_NAME_FMT "%.63s"

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

static void CALLBACK midi_in_cb(HMIDIIN handle, UINT kind, DWORD_PTR user, DWORD_PTR param_lhs, DWORD_PTR param_rhs)
{
    ldg_io_midi_in_t *midi = (ldg_io_midi_in_t *)(void *)(uintptr_t)user;
    ldg_io_midi_msg_t msg = LDG_STRUCT_ZERO_INIT;
    uint32_t packed = 0;
    uint8_t status = 0;

    if (kind != MIM_DATA) { return; }

    if (LDG_UNLIKELY(!midi)) { return; }

    if (LDG_UNLIKELY((uint64_t)(uintptr_t)handle != midi->handle)) { return; }

    packed = (uint32_t)param_lhs;
    status = (uint8_t)(packed & LDG_IO_MIDI_BYTE_MASK);

    if (LDG_UNLIKELY(status < LDG_IO_MIDI_STATUS_MIN)) { return; }

    msg.timestamp = (uint32_t)param_rhs;

    if (status >= LDG_IO_MIDI_REALTIME_MIN) { msg.kind = status; }
    else
    {
        if (status >= LDG_IO_MIDI_SYS_MIN) { return; }

        msg.kind = (uint8_t)(status & LDG_IO_MIDI_STATUS_MASK);
        msg.channel = (uint8_t)(status & LDG_IO_MIDI_CHANNEL_MASK);
        msg.data[0] = (uint8_t)((packed >> LDG_IO_MIDI_DATA_LHS_SHIFT) & LDG_IO_MIDI_DATA_MASK);
        msg.data[1] = (uint8_t)((packed >> LDG_IO_MIDI_DATA_RHS_SHIFT) & LDG_IO_MIDI_DATA_MASK);
    }

    midi_ring_push(midi, &msg);
}

uint32_t ldg_io_midi_in_port_list(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    MIDIINCAPSA caps = LDG_STRUCT_ZERO_INIT;
    uint64_t found = 0;
    UINT total = 0;
    UINT idx = 0;
    int32_t len = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = midiInGetNumDevs();

    for (idx = 0; idx < total && found < cap; idx++)
    {
        if (midiInGetDevCapsA(idx, &caps, (UINT)sizeof(MIDIINCAPSA)) != MMSYSERR_NOERROR) { continue; }

        len = snprintf(out[found].name, LDG_IO_MIDI_PORT_NAME_MAX, LDG_IO_MIDI_NAME_FMT, caps.szPname);
        if (len <= 0) { continue; }

        found++;
    }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_out_port_list(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt)
{
    MIDIOUTCAPSA caps = LDG_STRUCT_ZERO_INIT;
    uint64_t found = 0;
    UINT total = 0;
    UINT idx = 0;
    int32_t len = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = midiOutGetNumDevs();

    for (idx = 0; idx < total && found < cap; idx++)
    {
        if (midiOutGetDevCapsA(idx, &caps, (UINT)sizeof(MIDIOUTCAPSA)) != MMSYSERR_NOERROR) { continue; }

        len = snprintf(out[found].name, LDG_IO_MIDI_PORT_NAME_MAX, LDG_IO_MIDI_NAME_FMT, caps.szPname);
        if (len <= 0) { continue; }

        found++;
    }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_in_open(uint64_t idx, ldg_io_midi_in_t *out)
{
    HMIDIIN handle = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_io_midi_in_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->handle = LDG_IO_MIDI_HANDLE_NONE;

    if (LDG_UNLIKELY(idx >= (uint64_t)midiInGetNumDevs())) { return LDG_ERR_NOT_FOUND; }

    if (midiInOpen(&handle, (UINT)idx, (DWORD_PTR)midi_in_cb, (DWORD_PTR)(uintptr_t)out, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) { return LDG_ERR_IO_OPEN; }

    out->handle = (uint64_t)(uintptr_t)handle;

    LDG_SMP_MB();

    if (midiInStart(handle) != MMSYSERR_NOERROR)
    {
        midiInClose(handle);
        out->handle = LDG_IO_MIDI_HANDLE_NONE;
        return LDG_ERR_IO_OPEN;
    }

    out->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_in_poll(ldg_io_midi_in_t *midi, ldg_io_midi_msg_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint64_t got = 0;

    if (LDG_UNLIKELY(!midi || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(midi->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

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
    HMIDIIN handle = 0x0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!midi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (midi->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    handle = (HMIDIIN)(uintptr_t)midi->handle;

    midi->open = LDG_TRUTH_FALSE;

    LDG_SMP_MB();

    if (midiInStop(handle) != MMSYSERR_NOERROR) { ret = LDG_ERR_IO_CLOSE; }

    if (midiInReset(handle) != MMSYSERR_NOERROR && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (midiInClose(handle) != MMSYSERR_NOERROR && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    midi->handle = LDG_IO_MIDI_HANDLE_NONE;

    return ret;
}

uint32_t ldg_io_midi_out_open(uint64_t idx, ldg_io_midi_out_t *out)
{
    HMIDIOUT handle = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, (uint64_t)sizeof(ldg_io_midi_out_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->handle = LDG_IO_MIDI_HANDLE_NONE;

    if (LDG_UNLIKELY(idx >= (uint64_t)midiOutGetNumDevs())) { return LDG_ERR_NOT_FOUND; }

    if (midiOutOpen(&handle, (UINT)idx, LDG_IO_MIDI_NO_FLAG, LDG_IO_MIDI_NO_FLAG, CALLBACK_NULL) != MMSYSERR_NOERROR) { return LDG_ERR_IO_OPEN; }

    out->handle = (uint64_t)(uintptr_t)handle;
    out->open = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_out_send(ldg_io_midi_out_t *midi, const ldg_io_midi_msg_t *msg)
{
    DWORD packed = 0;
    uint8_t status = 0;

    if (LDG_UNLIKELY(!midi || !msg)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(midi->open != LDG_TRUTH_TRUE)) { return LDG_ERR_CLOSED; }

    if (LDG_UNLIKELY(msg->kind < LDG_IO_MIDI_STATUS_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (msg->kind >= LDG_IO_MIDI_REALTIME_MIN) { status = msg->kind; }
    else
    {
        if (LDG_UNLIKELY(msg->kind >= LDG_IO_MIDI_SYS_MIN)) { return LDG_ERR_UNSUPPORTED; }

        if (LDG_UNLIKELY(msg->channel >= LDG_IO_MIDI_CHANNEL_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

        status = (uint8_t)((msg->kind & LDG_IO_MIDI_STATUS_MASK) | msg->channel);
    }

    packed = (DWORD)status;
    packed |= (DWORD)((uint32_t)(msg->data[0] & LDG_IO_MIDI_DATA_MASK) << LDG_IO_MIDI_DATA_LHS_SHIFT);
    packed |= (DWORD)((uint32_t)(msg->data[1] & LDG_IO_MIDI_DATA_MASK) << LDG_IO_MIDI_DATA_RHS_SHIFT);

    if (midiOutShortMsg((HMIDIOUT)(uintptr_t)midi->handle, packed) != MMSYSERR_NOERROR) { return LDG_ERR_IO_WR; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_midi_out_close(ldg_io_midi_out_t *midi)
{
    HMIDIOUT handle = 0x0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!midi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (midi->open != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    handle = (HMIDIOUT)(uintptr_t)midi->handle;

    midi->open = LDG_TRUTH_FALSE;

    if (midiOutReset(handle) != MMSYSERR_NOERROR) { ret = LDG_ERR_IO_CLOSE; }

    if (midiOutClose(handle) != MMSYSERR_NOERROR && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    midi->handle = LDG_IO_MIDI_HANDLE_NONE;

    return ret;
}
