#ifndef LDG_TERM_FOCUS_H
#define LDG_TERM_FOCUS_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/term/vt.h>

#define LDG_TERM_FOCUS_NODE_CAP_DEFAULT 64U
#define LDG_TERM_FOCUS_NODE_CAP_MAX 65536U
#define LDG_TERM_FOCUS_NODE_NONE UINT32_MAX
#define LDG_TERM_FOCUS_PARENT_NONE LDG_TERM_FOCUS_NODE_NONE

typedef enum ldg_term_focus_dir
{
    LDG_TERM_FOCUS_DIR_NEXT = 0,
    LDG_TERM_FOCUS_DIR_PREV
} ldg_term_focus_dir_t;

typedef uint32_t (*ldg_term_focus_key_cb_t)(uint32_t node_id, const ldg_term_key_event_t *event, void *user, uint8_t *consumed);

typedef struct ldg_term_focus_conf
{
    uint32_t node_cap;
    uint8_t pudding[4];
} ldg_term_focus_conf_t;

typedef struct ldg_term_focus ldg_term_focus_t;

LDG_EXPORT uint32_t ldg_term_focus_conf_default_get(ldg_term_focus_conf_t *out);
LDG_EXPORT uint32_t ldg_term_focus_create(const ldg_term_focus_conf_t *conf, ldg_term_focus_t **out);
LDG_EXPORT uint32_t ldg_term_focus_destroy(ldg_term_focus_t *focus);
LDG_EXPORT uint32_t ldg_term_focus_node_add(ldg_term_focus_t *focus, uint32_t node_id, uint32_t parent_id, ldg_term_focus_key_cb_t key_cb, void *user);
LDG_EXPORT uint32_t ldg_term_focus_node_remove(ldg_term_focus_t *focus, uint32_t node_id);
LDG_EXPORT uint32_t ldg_term_focus_set(ldg_term_focus_t *focus, uint32_t node_id);
LDG_EXPORT uint32_t ldg_term_focus_get(const ldg_term_focus_t *focus, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_focus_mov(ldg_term_focus_t *focus, uint32_t dir);
LDG_EXPORT uint32_t ldg_term_focus_key_route(ldg_term_focus_t *focus, const ldg_term_key_event_t *event, uint8_t *consumed);

#endif
