#ifndef LDG_ECS_QUERY_H
#define LDG_ECS_QUERY_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_ECS_QUERY_COMP_MAX 16

typedef struct ldg_ecs_query
{
    uint32_t comp_id[LDG_ECS_QUERY_COMP_MAX];
    uint32_t comp_cunt;
} ldg_ecs_query_t;

typedef struct ldg_ecs_query_iter
{
    uint64_t entity;
    uint32_t pivot_comp_id;
    uint32_t pivot_cunt;
    uint32_t pivot_pos;
    uint8_t pudding[4];
} ldg_ecs_query_iter_t;

#endif
