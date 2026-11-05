#ifndef LDG_CPP_H
#define LDG_CPP_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_CPP_ARG_MAX 16
#define LDG_CPP_STRUCT_SIZE_MAX 8
#define LDG_CPP_DTOR_NONE UINT32_MAX

typedef enum ldg_cpp_arg_kind
{
    LDG_CPP_ARG_KIND_INT,
    LDG_CPP_ARG_KIND_PTR,
    LDG_CPP_ARG_KIND_FP32,
    LDG_CPP_ARG_KIND_FP64,
    LDG_CPP_ARG_KIND_STRUCT
} ldg_cpp_arg_kind_t;

typedef struct ldg_cpp_arg
{
    const void *ptr;
    uint64_t val;
    double fp64;
    float fp32;
    uint32_t size;
    uint32_t kind;
    uint8_t pudding[4];
} ldg_cpp_arg_t;

typedef struct ldg_cpp_class_desc
{
    const uint32_t *name_id;
    uint32_t method_cunt;
    uint32_t dtor_idx;
} ldg_cpp_class_desc_t;

LDG_EXPORT uint32_t ldg_cpp_slot_get(const ldg_cpp_class_desc_t *desc, uint32_t method_idx, uint32_t *out);
LDG_EXPORT uint32_t ldg_cpp_void_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt);
LDG_EXPORT uint32_t ldg_cpp_uint8_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint8_t *out);
LDG_EXPORT uint32_t ldg_cpp_uint16_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint16_t *out);
LDG_EXPORT uint32_t ldg_cpp_uint32_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_cpp_uint64_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_cpp_fp32_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, float *out);
LDG_EXPORT uint32_t ldg_cpp_fp64_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, double *out);
LDG_EXPORT uint32_t ldg_cpp_struct_call(void *obj, uint32_t slot, const ldg_cpp_arg_t *arg, uint32_t arg_cunt, void *out, uint32_t out_size);

#endif
