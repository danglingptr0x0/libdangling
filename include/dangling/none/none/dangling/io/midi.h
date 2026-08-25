#ifndef LDG_IO_MIDI_H
#define LDG_IO_MIDI_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_IO_MIDI_PORT_MAX 32
#define LDG_IO_MIDI_PORT_NAME_MAX 64
#define LDG_IO_MIDI_QUEUE_CAP 256
#define LDG_IO_MIDI_HANDLE_NONE UINT64_MAX

#define LDG_IO_MIDI_STATUS_MASK 0xF0U
#define LDG_IO_MIDI_CHANNEL_MASK 0x0FU
#define LDG_IO_MIDI_DATA_MASK 0x7FU
#define LDG_IO_MIDI_STATUS_MIN 0x80U
#define LDG_IO_MIDI_SYS_MIN 0xF0U
#define LDG_IO_MIDI_REALTIME_MIN 0xF8U

#define LDG_IO_MIDI_CHANNEL_CUNT 16
#define LDG_IO_MIDI_PITCH_BEND_CENTER 0x2000U
#define LDG_IO_MIDI_DATA_CUNT_MAX 2

typedef enum ldg_io_midi_msg_kind
{
    LDG_IO_MIDI_MSG_KIND_NONE = 0,
    LDG_IO_MIDI_MSG_KIND_NOTE_OFF = 0x80,
    LDG_IO_MIDI_MSG_KIND_NOTE_ON = 0x90,
    LDG_IO_MIDI_MSG_KIND_AFTERTOUCH_POLY = 0xA0,
    LDG_IO_MIDI_MSG_KIND_CTRL_CHANGE = 0xB0,
    LDG_IO_MIDI_MSG_KIND_PROG_CHANGE = 0xC0,
    LDG_IO_MIDI_MSG_KIND_AFTERTOUCH_CHAN = 0xD0,
    LDG_IO_MIDI_MSG_KIND_PITCH_BEND = 0xE0,
    LDG_IO_MIDI_MSG_KIND_CLOCK = 0xF8,
    LDG_IO_MIDI_MSG_KIND_START = 0xFA,
    LDG_IO_MIDI_MSG_KIND_CONT = 0xFB,
    LDG_IO_MIDI_MSG_KIND_STOP = 0xFC,
    LDG_IO_MIDI_MSG_KIND_ACTIVE_SENSE = 0xFE,
    LDG_IO_MIDI_MSG_KIND_RST = UINT8_MAX
} ldg_io_midi_msg_kind_t;

typedef struct ldg_io_midi_msg
{
    uint32_t timestamp;
    uint8_t kind;
    uint8_t channel;
    uint8_t data[LDG_IO_MIDI_DATA_CUNT_MAX];
} ldg_io_midi_msg_t;

typedef struct ldg_io_midi_port
{
    char name[LDG_IO_MIDI_PORT_NAME_MAX];
} ldg_io_midi_port_t;

typedef struct ldg_io_midi_in
{
    ldg_io_midi_msg_t msg[LDG_IO_MIDI_QUEUE_CAP];
    uint64_t handle;
    uint64_t base;
    volatile uint64_t hd;
    volatile uint64_t tail;
    uint32_t run_status;
    uint32_t pending_cunt;
    uint8_t pending[LDG_IO_MIDI_DATA_CUNT_MAX];
    uint8_t open;
    uint8_t pudding[5];
} LDG_ALIGNED ldg_io_midi_in_t;

typedef struct ldg_io_midi_out
{
    uint64_t handle;
    uint8_t open;
    uint8_t pudding[7];
} ldg_io_midi_out_t;

LDG_EXPORT uint32_t ldg_io_midi_in_port_list(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_io_midi_out_port_list(ldg_io_midi_port_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_io_midi_in_open(uint64_t idx, ldg_io_midi_in_t *out);
LDG_EXPORT uint32_t ldg_io_midi_in_poll(ldg_io_midi_in_t *midi, ldg_io_midi_msg_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_io_midi_in_close(ldg_io_midi_in_t *midi);
LDG_EXPORT uint32_t ldg_io_midi_out_open(uint64_t idx, ldg_io_midi_out_t *out);
LDG_EXPORT uint32_t ldg_io_midi_out_send(ldg_io_midi_out_t *midi, const ldg_io_midi_msg_t *msg);
LDG_EXPORT uint32_t ldg_io_midi_out_close(ldg_io_midi_out_t *midi);

#endif
