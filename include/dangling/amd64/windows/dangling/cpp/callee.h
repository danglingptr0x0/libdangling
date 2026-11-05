#ifndef LDG_CPP_CALLEE_H
#define LDG_CPP_CALLEE_H

#define LDG_CPP_STRUCT_RET_TYPE(ret_type) ret_type *
#define LDG_CPP_STRUCT_SELF_PARAMS(ret_type) void *self, ret_type *ldg_cpp_struct_out
#define LDG_CPP_STRUCT_RET(res) do { *ldg_cpp_struct_out = (res); return ldg_cpp_struct_out; } while (0)

#endif
