#ifndef LDG_IO_SERIAL_H
#define LDG_IO_SERIAL_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_IO_SERIAL_PATH_MAX 64
#define LDG_IO_SERIAL_ID_MAX 192
#define LDG_IO_SERIAL_PORT_MAX 32
#define LDG_IO_SERIAL_HANDLE_NONE UINT64_MAX

typedef enum ldg_io_serial_baud
{
    LDG_IO_SERIAL_BAUD_1200 = 0,
    LDG_IO_SERIAL_BAUD_2400,
    LDG_IO_SERIAL_BAUD_4800,
    LDG_IO_SERIAL_BAUD_9600,
    LDG_IO_SERIAL_BAUD_19200,
    LDG_IO_SERIAL_BAUD_38400,
    LDG_IO_SERIAL_BAUD_57600,
    LDG_IO_SERIAL_BAUD_115200,
    LDG_IO_SERIAL_BAUD_230400,
    LDG_IO_SERIAL_BAUD_460800,
    LDG_IO_SERIAL_BAUD_921600
} ldg_io_serial_baud_t;

typedef enum ldg_io_serial_data
{
    LDG_IO_SERIAL_DATA_5 = 0,
    LDG_IO_SERIAL_DATA_6,
    LDG_IO_SERIAL_DATA_7,
    LDG_IO_SERIAL_DATA_8
} ldg_io_serial_data_t;

typedef enum ldg_io_serial_parity
{
    LDG_IO_SERIAL_PARITY_NONE = 0,
    LDG_IO_SERIAL_PARITY_ODD,
    LDG_IO_SERIAL_PARITY_EVEN
} ldg_io_serial_parity_t;

typedef enum ldg_io_serial_stop
{
    LDG_IO_SERIAL_STOP_ONE = 0,
    LDG_IO_SERIAL_STOP_TWO
} ldg_io_serial_stop_t;

typedef enum ldg_io_serial_flow
{
    LDG_IO_SERIAL_FLOW_NONE = 0,
    LDG_IO_SERIAL_FLOW_SOFT,
    LDG_IO_SERIAL_FLOW_HARD
} ldg_io_serial_flow_t;

typedef struct ldg_io_serial_conf
{
    ldg_io_serial_baud_t baud;
    ldg_io_serial_data_t data;
    ldg_io_serial_parity_t parity;
    ldg_io_serial_stop_t stop;
    ldg_io_serial_flow_t flow;
} ldg_io_serial_conf_t;

typedef struct ldg_io_serial
{
    uint64_t handle;
    uint8_t open;
    uint8_t pudding[7];
} ldg_io_serial_t;

typedef struct ldg_io_serial_port
{
    char path[LDG_IO_SERIAL_PATH_MAX];
    char id[LDG_IO_SERIAL_ID_MAX];
} ldg_io_serial_port_t;

LDG_EXPORT uint32_t ldg_io_serial_conf_default_get(ldg_io_serial_conf_t *out);
LDG_EXPORT uint32_t ldg_io_serial_port_list(ldg_io_serial_port_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_io_serial_open(const char *path, const ldg_io_serial_conf_t *conf, ldg_io_serial_t *out);
LDG_EXPORT uint32_t ldg_io_serial_conf_set(ldg_io_serial_t *port, const ldg_io_serial_conf_t *conf);
LDG_EXPORT uint32_t ldg_io_serial_rd(ldg_io_serial_t *port, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_io_serial_wr(ldg_io_serial_t *port, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent);
LDG_EXPORT uint32_t ldg_io_serial_flush(ldg_io_serial_t *port);
LDG_EXPORT uint32_t ldg_io_serial_close(ldg_io_serial_t *port);
LDG_EXPORT uint32_t ldg_io_serial_dtr_set(ldg_io_serial_t *port, uint8_t en);
LDG_EXPORT uint32_t ldg_io_serial_rts_set(ldg_io_serial_t *port, uint8_t en);
LDG_EXPORT uint8_t ldg_io_serial_cts_is(ldg_io_serial_t *port);
LDG_EXPORT uint8_t ldg_io_serial_dsr_is(ldg_io_serial_t *port);

#endif
