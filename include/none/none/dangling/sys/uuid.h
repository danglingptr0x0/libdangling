#ifndef LDG_SYS_UUID_H
#define LDG_SYS_UUID_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_UUID_BYTE_SIZE 16
#define LDG_UUID_STR_SIZE 37

LDG_EXPORT uint32_t ldg_sys_uuid_gen(uint8_t uuid[LDG_UUID_BYTE_SIZE]);
LDG_EXPORT uint32_t ldg_sys_uuid_to_str(const uint8_t uuid[LDG_UUID_BYTE_SIZE], char *str, uint64_t str_size);

#endif
