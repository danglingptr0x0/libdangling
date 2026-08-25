#ifndef LDG_TERM_VT_H
#define LDG_TERM_VT_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/term/term.h>

#define LDG_TERM_VT_PARAM_MAX 16
#define LDG_TERM_VT_INTER_MAX 4
#define LDG_TERM_VT_OSC_MAX 8192
#define LDG_TERM_VT_REPLY_MAX 64
#define LDG_TERM_VT_CLIP_MAX 4096
#define LDG_TERM_VT_TITLE_MAX 256
#define LDG_TERM_VT_TITLE_STACK_MAX 8
#define LDG_TERM_VT_KBD_STACK_MAX 16
#define LDG_TERM_VT_KBD_SCREEN_CUNT 2

#define LDG_TERM_VT_KBD_DISAMBIGUATE 0x1
#define LDG_TERM_VT_KBD_EVENT_TYPE 0x2
#define LDG_TERM_VT_KBD_ALT_KEY 0x4
#define LDG_TERM_VT_KBD_ALL_ESC 0x8
#define LDG_TERM_VT_KBD_TEXT 0x10
#define LDG_TERM_VT_CH_NONE UINT8_MAX

#define LDG_TERM_MOD_NONE 0x0
#define LDG_TERM_MOD_SHIFT 0x1
#define LDG_TERM_MOD_ALT 0x2
#define LDG_TERM_MOD_CTRL 0x4
#define LDG_TERM_MOD_SUPER 0x8
#define LDG_TERM_MOD_HYPER 0x10
#define LDG_TERM_MOD_META 0x20
#define LDG_TERM_MOD_CAPS 0x40
#define LDG_TERM_MOD_NUM 0x80
#define LDG_TERM_MOD_MASK 0xFF

#define LDG_TERM_KEY_TEXT_MAX 8

typedef enum ldg_term_vt_state
{
    LDG_TERM_VT_STATE_GROUND = 0,
    LDG_TERM_VT_STATE_ESC,
    LDG_TERM_VT_STATE_ESC_INTER,
    LDG_TERM_VT_STATE_CSI_ENTRY,
    LDG_TERM_VT_STATE_CSI_PARAM,
    LDG_TERM_VT_STATE_CSI_INTER,
    LDG_TERM_VT_STATE_CSI_IGNORE,
    LDG_TERM_VT_STATE_DCS_ENTRY,
    LDG_TERM_VT_STATE_DCS_PARAM,
    LDG_TERM_VT_STATE_DCS_INTER,
    LDG_TERM_VT_STATE_DCS_PASS,
    LDG_TERM_VT_STATE_DCS_IGNORE,
    LDG_TERM_VT_STATE_OSC,
    LDG_TERM_VT_STATE_OSC_ESC,
    LDG_TERM_VT_STATE_STR,
    LDG_TERM_VT_STATE_STR_ESC,
    LDG_TERM_VT_STATE_CHARSET,
    LDG_TERM_VT_STATE_VT52_ROW,
    LDG_TERM_VT_STATE_VT52_COL
} ldg_term_vt_state_t;

typedef enum ldg_term_vt_str
{
    LDG_TERM_VT_STR_NONE = 0,
    LDG_TERM_VT_STR_DCS,
    LDG_TERM_VT_STR_APC,
    LDG_TERM_VT_STR_PM,
    LDG_TERM_VT_STR_SOS
} ldg_term_vt_str_t;

typedef enum ldg_term_vt_mode
{
    LDG_TERM_VT_MODE_NONE = 0,
    LDG_TERM_VT_MODE_APP_CURSOR = 0x1,
    LDG_TERM_VT_MODE_ORIG = 0x2,
    LDG_TERM_VT_MODE_WRAP = 0x4,
    LDG_TERM_VT_MODE_APP_KEYPAD = 0x8,
    LDG_TERM_VT_MODE_FOCUS = 0x10,
    LDG_TERM_VT_MODE_PASTE_BRACKET = 0x20,
    LDG_TERM_VT_MODE_MOUSE_NORMAL = 0x40,
    LDG_TERM_VT_MODE_MOUSE_BUTTON = 0x80,
    LDG_TERM_VT_MODE_MOUSE_ANY = 0x100,
    LDG_TERM_VT_MODE_MOUSE_SGR = 0x200,
    LDG_TERM_VT_MODE_COLM = 0x400,
    LDG_TERM_VT_MODE_SCLM = 0x800,
    LDG_TERM_VT_MODE_ARM = 0x1000,
    LDG_TERM_VT_MODE_INLM = 0x2000,
    LDG_TERM_VT_MODE_ANSI = 0x4000,
    LDG_TERM_VT_MODE_RESIZE = 0x8000
} ldg_term_vt_mode_t;

#define LDG_TERM_VT_UNHANDLED_MODE_MAX 16

#define LDG_TERM_MOUSE_KIND_PRESS 1
#define LDG_TERM_MOUSE_KIND_RELEASE 2
#define LDG_TERM_MOUSE_KIND_MOTION 3

#define LDG_TERM_MOUSE_BTN_LHS 0
#define LDG_TERM_MOUSE_BTN_MIDDLE 1
#define LDG_TERM_MOUSE_BTN_RHS 2
#define LDG_TERM_MOUSE_BTN_NONE 3
#define LDG_TERM_MOUSE_BTN_WHEEL_UP 4
#define LDG_TERM_MOUSE_BTN_WHEEL_DOWN 5

typedef struct ldg_term_mouse_event
{
    uint32_t col;
    uint32_t row;
    uint32_t mods;
    uint8_t button;
    uint8_t kind;
    uint8_t pudding[2];
} ldg_term_mouse_event_t;

typedef enum ldg_term_key
{
    LDG_TERM_KEY_NONE = 0,
    LDG_TERM_KEY_UP,
    LDG_TERM_KEY_DOWN,
    LDG_TERM_KEY_RHS,
    LDG_TERM_KEY_LHS,
    LDG_TERM_KEY_HOME,
    LDG_TERM_KEY_END,
    LDG_TERM_KEY_INSERT,
    LDG_TERM_KEY_DELETE,
    LDG_TERM_KEY_PAGE_UP,
    LDG_TERM_KEY_PAGE_DOWN,
    LDG_TERM_KEY_BACKSPACE,
    LDG_TERM_KEY_TAB,
    LDG_TERM_KEY_ENTER,
    LDG_TERM_KEY_ESCAPE,
    LDG_TERM_KEY_F1,
    LDG_TERM_KEY_F2,
    LDG_TERM_KEY_F3,
    LDG_TERM_KEY_F4,
    LDG_TERM_KEY_F5,
    LDG_TERM_KEY_F6,
    LDG_TERM_KEY_F7,
    LDG_TERM_KEY_F8,
    LDG_TERM_KEY_F9,
    LDG_TERM_KEY_F10,
    LDG_TERM_KEY_F11,
    LDG_TERM_KEY_F12,
    LDG_TERM_KEY_CUNT
} ldg_term_key_t;

#define LDG_TERM_KEY_KIND_PRESS 1
#define LDG_TERM_KEY_KIND_REPEAT 2
#define LDG_TERM_KEY_KIND_RELEASE 3

typedef struct ldg_term_key_event
{
    uint32_t codepoint;
    uint32_t shifted;
    uint32_t base;
    uint32_t mods;
    uint32_t text[LDG_TERM_KEY_TEXT_MAX];
    uint32_t text_cunt;
    uint8_t key;
    uint8_t kind;
    uint8_t pudding[2];
} ldg_term_key_event_t;

typedef struct ldg_term_vt
{
    ldg_term_t *term;
    uint32_t param[LDG_TERM_VT_PARAM_MAX];
    uint8_t param_seen[LDG_TERM_VT_PARAM_MAX];
    uint8_t param_sub[LDG_TERM_VT_PARAM_MAX];
    char osc[LDG_TERM_VT_OSC_MAX];
    uint8_t inter[LDG_TERM_VT_INTER_MAX];
    uint32_t state;
    uint32_t param_cunt;
    uint32_t inter_cunt;
    uint32_t osc_len;
    ldg_byte_t reply[LDG_TERM_VT_REPLY_MAX];
    ldg_byte_t clip[LDG_TERM_VT_CLIP_MAX];
    char title[LDG_TERM_VT_TITLE_MAX];
    char title_stack[LDG_TERM_VT_TITLE_STACK_MAX][LDG_TERM_VT_TITLE_MAX];
    uint32_t title_stack_len[LDG_TERM_VT_TITLE_STACK_MAX];
    uint32_t title_depth;
    uint32_t title_len;
    uint32_t clip_len;
    uint32_t reply_len;
    uint32_t mode;
    uint32_t kbd_flags[LDG_TERM_VT_KBD_SCREEN_CUNT];
    uint32_t kbd_stack[LDG_TERM_VT_KBD_SCREEN_CUNT][LDG_TERM_VT_KBD_STACK_MAX];
    uint32_t kbd_stack_len[LDG_TERM_VT_KBD_SCREEN_CUNT];
    uint32_t unhandled_mode[LDG_TERM_VT_UNHANDLED_MODE_MAX];
    uint32_t unhandled_mode_cunt;
    uint32_t vt52_row;
    uint32_t gfx_pending;
    uint32_t gfx_id;
    uint32_t gfx_fmt;
    uint32_t gfx_width;
    uint32_t gfx_height;
    uint8_t pudding_hi[4];
    uint64_t unhandled_lo;
    uint64_t unhandled_hi;
    uint8_t last_ch;
    uint8_t cursor_shape;
    uint8_t priv;
    uint8_t priv_lead;
    uint8_t str_kind;
    uint8_t charset_slot;
    uint8_t pudding[2];
} ldg_term_vt_t;

LDG_EXPORT uint32_t ldg_term_vt_init(ldg_term_vt_t *vt, ldg_term_t *term);
LDG_EXPORT uint32_t ldg_term_vt_rst(ldg_term_vt_t *vt);
LDG_EXPORT uint32_t ldg_term_vt_feed(ldg_term_vt_t *vt, const ldg_byte_t *buff, uint64_t len);
LDG_EXPORT uint32_t ldg_term_vt_title_get(const ldg_term_vt_t *vt, char *out, uint64_t cap);
LDG_EXPORT uint32_t ldg_term_vt_clipboard_get(ldg_term_vt_t *vt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_clipboard_encode(const ldg_byte_t *text, uint64_t len, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_vt_reply_drain(ldg_term_vt_t *vt, ldg_byte_t *buff, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_vt_mode_get(const ldg_term_vt_t *vt, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_vt_cursor_shape_get(const ldg_term_vt_t *vt, uint8_t *out);
LDG_EXPORT uint32_t ldg_term_vt_unhandled_get(const ldg_term_vt_t *vt, ldg_byte_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_term_key_encode(const ldg_term_vt_t *vt, const ldg_term_key_event_t *event, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_mouse_encode(const ldg_term_vt_t *vt, const ldg_term_mouse_event_t *event, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_resize_encode(const ldg_term_vt_t *vt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_paste_encode(const ldg_term_vt_t *vt, const ldg_byte_t *text, uint64_t len, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_term_focus_encode(const ldg_term_vt_t *vt, uint8_t focused, ldg_byte_t *out, uint64_t cap, uint64_t *out_len);

#endif
