#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/term/key.h>

#define KEY_ESC_BYTE 0x1B
#define KEY_CSI_LEAD 0x5B
#define KEY_SS3_LEAD 0x4F
#define KEY_DEL_BYTE 0x7F
#define KEY_TAB_BYTE 0x09
#define KEY_ENTER_BYTE 0x0D
#define KEY_SPACE_BYTE 0x20
#define KEY_ASCII_LAST 0x7F
#define KEY_CTRL_LETTER_LAST 0x1A
#define KEY_CTRL_LOWER_BIT 0x60
#define KEY_CTRL_UPPER_BIT 0x40
#define KEY_SEMI 0x3B
#define KEY_COLON 0x3A
#define KEY_DIGIT_BASE 0x30
#define KEY_DIGIT_CUNT 10
#define KEY_PRIV_FIRST 0x3C
#define KEY_PRIV_LAST 0x3F
#define KEY_INTER_FIRST 0x20
#define KEY_INTER_LAST 0x2F
#define KEY_FINAL_FIRST 0x40
#define KEY_FINAL_LAST 0x7E
#define KEY_GROUP_MAX 6
#define KEY_SUB_MAX 8
#define KEY_SEQ_MAX 96
#define KEY_VAL_MAX 0x10FFFF
#define KEY_GROUP_CODE 0
#define KEY_GROUP_MOD 1
#define KEY_GROUP_TEXT 2
#define KEY_SUB_SHIFTED 1
#define KEY_SUB_BASE 2
#define KEY_SUB_KIND 1
#define KEY_MOD_BASE 1
#define KEY_SINGLE_SIZE 1
#define KEY_PAIR_SIZE 2
#define KEY_SS3_SIZE 3
#define KEY_X10_TAIL_SIZE 3
#define KEY_WIN32_FINAL '_'
#define KEY_WIN32_GROUP_VK 0
#define KEY_WIN32_GROUP_SCAN 1
#define KEY_WIN32_GROUP_UNICODE 2
#define KEY_WIN32_GROUP_DOWN 3
#define KEY_WIN32_GROUP_STATE 4
#define KEY_WIN32_STATE_RHS_ALT 0x0001U
#define KEY_WIN32_STATE_LHS_ALT 0x0002U
#define KEY_WIN32_STATE_RHS_CTRL 0x0004U
#define KEY_WIN32_STATE_LHS_CTRL 0x0008U
#define KEY_WIN32_STATE_SHIFT 0x0010U
#define KEY_WIN32_STATE_NUMLOCK 0x0020U
#define KEY_WIN32_STATE_CAPSLOCK 0x0080U
#define KEY_WIN32_STATE_ENHANCED 0x0100U
#define KEY_WIN32_VK_BACK 0x08
#define KEY_WIN32_VK_TAB 0x09
#define KEY_WIN32_VK_PAUSE 0x13
#define KEY_WIN32_VK_SNAPSHOT 0x2C
#define KEY_WIN32_VK_ENTER 0x0D
#define KEY_WIN32_VK_SHIFT 0x10
#define KEY_WIN32_VK_CONTROL 0x11
#define KEY_WIN32_VK_MENU 0x12
#define KEY_WIN32_VK_CAPITAL 0x14
#define KEY_WIN32_VK_ESCAPE 0x1B
#define KEY_WIN32_VK_SPACE 0x20
#define KEY_WIN32_VK_PRIOR 0x21
#define KEY_WIN32_VK_NEXT 0x22
#define KEY_WIN32_VK_END 0x23
#define KEY_WIN32_VK_HOME 0x24
#define KEY_WIN32_VK_ARROW_LHS 0x25
#define KEY_WIN32_VK_ARROW_UP 0x26
#define KEY_WIN32_VK_ARROW_RHS 0x27
#define KEY_WIN32_VK_ARROW_DOWN 0x28
#define KEY_WIN32_VK_INSERT 0x2D
#define KEY_WIN32_VK_DELETE 0x2E
#define KEY_WIN32_VK_DIGIT_FIRST 0x30
#define KEY_WIN32_VK_DIGIT_LAST 0x39
#define KEY_WIN32_VK_LETTER_FIRST 0x41
#define KEY_WIN32_VK_LETTER_LAST 0x5A
#define KEY_WIN32_VK_LHS_WIN 0x5B
#define KEY_WIN32_VK_RHS_WIN 0x5C
#define KEY_WIN32_VK_F1 0x70
#define KEY_WIN32_VK_F12 0x7B
#define KEY_WIN32_VK_LHS_SHIFT 0xA0
#define KEY_WIN32_VK_RHS_SHIFT 0xA1
#define KEY_WIN32_VK_LHS_CONTROL 0xA2
#define KEY_WIN32_VK_RHS_CONTROL 0xA3
#define KEY_WIN32_VK_LHS_MENU 0xA4
#define KEY_WIN32_VK_RHS_MENU 0xA5
#define KEY_WIN32_SCAN_RHS_SHIFT 0x36
#define KEY_WIN32_LOWER_OFF 0x20
#define KEY_KITTY_CAPS_LOCK 57358U
#define KEY_KITTY_LHS_SHIFT 57441U
#define KEY_KITTY_LHS_CONTROL 57442U
#define KEY_KITTY_LHS_ALT 57443U
#define KEY_KITTY_LHS_SUPER 57444U
#define KEY_KITTY_RHS_SHIFT 57447U
#define KEY_KITTY_RHS_CONTROL 57448U
#define KEY_KITTY_RHS_ALT 57449U
#define KEY_KITTY_RHS_SUPER 57450U
#define KEY_CODE_NONE 0U
#define KEY_ADJACENT_OFF 1
#define KEY_CODE_TAB 9
#define KEY_CODE_ENTER 13
#define KEY_CODE_ESC 27
#define KEY_CODE_BS 127
#define KEY_FUNC_FIRST 0xE000
#define KEY_FUNC_LAST 0xF8FF
#define KEY_TILDE_HOME 1
#define KEY_TILDE_INSERT 2
#define KEY_TILDE_DELETE 3
#define KEY_TILDE_END 4
#define KEY_TILDE_PAGE_UP 5
#define KEY_TILDE_PAGE_DOWN 6
#define KEY_TILDE_HOME_ALT 7
#define KEY_TILDE_END_ALT 8
#define KEY_TILDE_F1 11
#define KEY_TILDE_F2 12
#define KEY_TILDE_F3 13
#define KEY_TILDE_F4 14
#define KEY_TILDE_F5 15
#define KEY_TILDE_F6 17
#define KEY_TILDE_F7 18
#define KEY_TILDE_F8 19
#define KEY_TILDE_F9 20
#define KEY_TILDE_F10 21
#define KEY_TILDE_F11 23
#define KEY_TILDE_F12 24
#define KEY_TILDE_OTHER_KEYS 27
#define KEY_GROUP_OTHER_CODE 2
#define KEY_UPPER_FIRST 0x41
#define KEY_UPPER_LAST 0x5A
#define KEY_CASE_OFF 0x20

typedef struct key_seq
{
    uint32_t val[KEY_GROUP_MAX][KEY_SUB_MAX];
    uint32_t sub_cunt[KEY_GROUP_MAX];
    uint32_t group;
    uint32_t sub;
    uint64_t len;
    uint8_t lead;
    uint8_t final;
    uint8_t inter_is;
    uint8_t overflow_is;
    uint8_t broken_is;
    uint8_t pudding[3];
} key_seq_t;

static void key_event_clear(ldg_term_key_event_t *event)
{
    uint32_t idx = 0;

    event->codepoint = 0;
    event->shifted = 0;
    event->base = 0;
    event->mods = LDG_TERM_MOD_NONE;
    event->text_cunt = 0;
    event->key = LDG_TERM_KEY_NONE;
    event->kind = LDG_TERM_KEY_KIND_PRESS;

    while (idx < LDG_TERM_KEY_TEXT_MAX)
    {
        event->text[idx] = 0;
        idx++;
    }
}

static uint32_t key_byte_event_set(uint8_t byte, ldg_term_key_event_t *out)
{
    if (byte > KEY_ASCII_LAST) { return LDG_ERR_UNSUPPORTED; }

    if (byte == KEY_DEL_BYTE) { out->key = LDG_TERM_KEY_BACKSPACE; }
    else if (byte == KEY_ENTER_BYTE) { out->key = LDG_TERM_KEY_ENTER; }
    else if (byte == KEY_TAB_BYTE) { out->key = LDG_TERM_KEY_TAB; }
    else if (byte == KEY_ESC_BYTE) { out->key = LDG_TERM_KEY_ESCAPE; }

    if (out->key != LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

    if (byte >= KEY_SPACE_BYTE)
    {
        out->codepoint = byte;
        out->text[0] = byte;
        out->text_cunt = KEY_SINGLE_SIZE;

        return LDG_ERR_AOK;
    }

    out->mods = LDG_TERM_MOD_CTRL;

    if (byte == 0) { out->codepoint = KEY_SPACE_BYTE; }
    else if (byte <= KEY_CTRL_LETTER_LAST) { out->codepoint = (uint32_t)byte | KEY_CTRL_LOWER_BIT; }
    else { out->codepoint = (uint32_t)byte | KEY_CTRL_UPPER_BIT; }

    return LDG_ERR_AOK;
}

static uint8_t key_letter_key_get(uint8_t final)
{
    const char *letter = "ABCDHFPQRS";
    const uint8_t key[] = { LDG_TERM_KEY_UP, LDG_TERM_KEY_DOWN, LDG_TERM_KEY_RHS, LDG_TERM_KEY_LHS, LDG_TERM_KEY_HOME, LDG_TERM_KEY_END, LDG_TERM_KEY_F1, LDG_TERM_KEY_F2, LDG_TERM_KEY_F3, LDG_TERM_KEY_F4 };
    uint32_t idx = 0;

    while (idx < sizeof(key))
    {
        if ((uint8_t)letter[idx] == final) { return key[idx]; }

        idx++;
    }

    return LDG_TERM_KEY_NONE;
}

static uint8_t key_tilde_key_get(uint32_t num)
{
    const uint8_t code[] = { KEY_TILDE_HOME, KEY_TILDE_INSERT, KEY_TILDE_DELETE, KEY_TILDE_END, KEY_TILDE_PAGE_UP, KEY_TILDE_PAGE_DOWN, KEY_TILDE_HOME_ALT, KEY_TILDE_END_ALT, KEY_TILDE_F1, KEY_TILDE_F2, KEY_TILDE_F3, KEY_TILDE_F4, KEY_TILDE_F5, KEY_TILDE_F6, KEY_TILDE_F7, KEY_TILDE_F8, KEY_TILDE_F9, KEY_TILDE_F10, KEY_TILDE_F11, KEY_TILDE_F12 };
    const uint8_t key[] = { LDG_TERM_KEY_HOME, LDG_TERM_KEY_INSERT, LDG_TERM_KEY_DELETE, LDG_TERM_KEY_END, LDG_TERM_KEY_PAGE_UP, LDG_TERM_KEY_PAGE_DOWN, LDG_TERM_KEY_HOME, LDG_TERM_KEY_END, LDG_TERM_KEY_F1, LDG_TERM_KEY_F2, LDG_TERM_KEY_F3, LDG_TERM_KEY_F4, LDG_TERM_KEY_F5, LDG_TERM_KEY_F6, LDG_TERM_KEY_F7, LDG_TERM_KEY_F8, LDG_TERM_KEY_F9, LDG_TERM_KEY_F10, LDG_TERM_KEY_F11, LDG_TERM_KEY_F12 };
    uint32_t idx = 0;

    while (idx < sizeof(key))
    {
        if (code[idx] == num) { return key[idx]; }

        idx++;
    }

    return LDG_TERM_KEY_NONE;
}

static void key_seq_byte_cum(key_seq_t *seq, uint8_t byte)
{
    uint32_t *val = 0x0;

    if (byte == KEY_SEMI)
    {
        seq->group++;
        seq->sub = 0;

        return;
    }

    if (byte == KEY_COLON) { seq->sub++; }

    if (seq->group >= KEY_GROUP_MAX || seq->sub >= KEY_SUB_MAX)
    {
        seq->overflow_is = LDG_TRUTH_TRUE;

        return;
    }

    seq->sub_cunt[seq->group] = seq->sub + KEY_ADJACENT_OFF;

    if (byte == KEY_COLON) { return; }

    val = &seq->val[seq->group][seq->sub];
    *val = (*val * KEY_DIGIT_CUNT) + ((uint32_t)byte - KEY_DIGIT_BASE);

    if (*val > KEY_VAL_MAX) { seq->overflow_is = LDG_TRUTH_TRUE; }
}

static uint8_t key_seq_scan_is(key_seq_t *seq, const ldg_byte_t *src, uint64_t len)
{
    uint64_t pos = KEY_PAIR_SIZE;
    uint8_t byte = 0;

    if (pos < len && src[pos] >= KEY_PRIV_FIRST && src[pos] <= KEY_PRIV_LAST)
    {
        seq->lead = src[pos];
        pos++;
    }

    while (pos < len && pos < KEY_SEQ_MAX)
    {
        byte = src[pos];
        pos++;

        if (byte >= KEY_FINAL_FIRST && byte <= KEY_FINAL_LAST)
        {
            seq->final = byte;
            seq->len = pos;

            return LDG_TRUTH_TRUE;
        }

        if (byte >= KEY_INTER_FIRST && byte <= KEY_INTER_LAST) { seq->inter_is = LDG_TRUTH_TRUE; }
        else if (byte >= KEY_DIGIT_BASE && byte <= KEY_SEMI) { key_seq_byte_cum(seq, byte); }
        else { seq->broken_is = LDG_TRUTH_TRUE; }

        if (seq->broken_is) { break; }
    }

    seq->len = seq->broken_is ? (pos - KEY_ADJACENT_OFF) : pos;

    return LDG_TRUTH_FALSE;
}

static void key_seq_mod_apply(const key_seq_t *seq, ldg_term_key_event_t *out)
{
    uint32_t param = seq->val[KEY_GROUP_MOD][0];
    uint32_t kind = seq->val[KEY_GROUP_MOD][KEY_SUB_KIND];

    if (param > KEY_MOD_BASE) { out->mods |= (param - KEY_MOD_BASE) & LDG_TERM_MOD_MASK; }

    if (kind == LDG_TERM_KEY_KIND_REPEAT || kind == LDG_TERM_KEY_KIND_RELEASE) { out->kind = (uint8_t)kind; }
}

static uint32_t key_seq_code_event_set(const key_seq_t *seq, ldg_term_key_event_t *out)
{
    uint32_t code = seq->val[KEY_GROUP_CODE][0];
    uint32_t idx = 0;

    if (code == 0 || (code >= KEY_FUNC_FIRST && code <= KEY_FUNC_LAST)) { return LDG_ERR_UNSUPPORTED; }

    if (code == KEY_CODE_ESC) { out->key = LDG_TERM_KEY_ESCAPE; }
    else if (code == KEY_CODE_ENTER) { out->key = LDG_TERM_KEY_ENTER; }
    else if (code == KEY_CODE_TAB) { out->key = LDG_TERM_KEY_TAB; }
    else if (code == KEY_CODE_BS) { out->key = LDG_TERM_KEY_BACKSPACE; }
    else { out->codepoint = code; }

    out->shifted = seq->val[KEY_GROUP_CODE][KEY_SUB_SHIFTED];
    out->base = seq->val[KEY_GROUP_CODE][KEY_SUB_BASE];

    while (idx < seq->sub_cunt[KEY_GROUP_TEXT] && idx < LDG_TERM_KEY_TEXT_MAX)
    {
        out->text[idx] = seq->val[KEY_GROUP_TEXT][idx];
        idx++;
    }

    out->text_cunt = idx;

    return LDG_ERR_AOK;
}

static uint32_t key_seq_other_event_set(const key_seq_t *seq, ldg_term_key_event_t *out)
{
    uint32_t code = seq->val[KEY_GROUP_OTHER_CODE][0];

    if (seq->sub_cunt[KEY_GROUP_OTHER_CODE] == 0 || code == 0) { return LDG_ERR_UNSUPPORTED; }

    if (code == KEY_CODE_ESC) { out->key = LDG_TERM_KEY_ESCAPE; }
    else if (code == KEY_CODE_ENTER) { out->key = LDG_TERM_KEY_ENTER; }
    else if (code == KEY_CODE_TAB) { out->key = LDG_TERM_KEY_TAB; }
    else if (code == KEY_CODE_BS) { out->key = LDG_TERM_KEY_BACKSPACE; }

    if (out->key != LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

    if ((out->mods & LDG_TERM_MOD_SHIFT) == 0) { out->codepoint = code; }
    else
    {
        out->shifted = code;

        if (code >= KEY_UPPER_FIRST && code <= KEY_UPPER_LAST) { out->codepoint = code + KEY_CASE_OFF; }
    }

    if ((out->mods & (LDG_TERM_MOD_ALT | LDG_TERM_MOD_CTRL | LDG_TERM_MOD_SUPER | LDG_TERM_MOD_HYPER | LDG_TERM_MOD_META)) == 0 && code >= KEY_SPACE_BYTE && code != KEY_DEL_BYTE)
    {
        out->text[0] = code;
        out->text_cunt = KEY_SINGLE_SIZE;
    }

    return LDG_ERR_AOK;
}

static uint8_t key_win32_key_get(uint32_t vk)
{
    const uint8_t code[] = { KEY_WIN32_VK_ESCAPE, KEY_WIN32_VK_ENTER, KEY_WIN32_VK_TAB, KEY_WIN32_VK_BACK, KEY_WIN32_VK_ARROW_LHS, KEY_WIN32_VK_ARROW_UP, KEY_WIN32_VK_ARROW_RHS, KEY_WIN32_VK_ARROW_DOWN, KEY_WIN32_VK_HOME, KEY_WIN32_VK_END, KEY_WIN32_VK_INSERT, KEY_WIN32_VK_DELETE, KEY_WIN32_VK_PRIOR, KEY_WIN32_VK_NEXT, KEY_WIN32_VK_PAUSE, KEY_WIN32_VK_SNAPSHOT };
    const uint8_t key[] = { LDG_TERM_KEY_ESCAPE, LDG_TERM_KEY_ENTER, LDG_TERM_KEY_TAB, LDG_TERM_KEY_BACKSPACE, LDG_TERM_KEY_LHS, LDG_TERM_KEY_UP, LDG_TERM_KEY_RHS, LDG_TERM_KEY_DOWN, LDG_TERM_KEY_HOME, LDG_TERM_KEY_END, LDG_TERM_KEY_INSERT, LDG_TERM_KEY_DELETE, LDG_TERM_KEY_PAGE_UP, LDG_TERM_KEY_PAGE_DOWN, LDG_TERM_KEY_BREAK, LDG_TERM_KEY_PRINT };
    uint32_t idx = 0;

    if (vk >= KEY_WIN32_VK_F1 && vk <= KEY_WIN32_VK_F12) { return (uint8_t)(LDG_TERM_KEY_F1 + (vk - KEY_WIN32_VK_F1)); }

    while (idx < sizeof(key))
    {
        if (code[idx] == vk) { return key[idx]; }

        idx++;
    }

    return LDG_TERM_KEY_NONE;
}

static uint32_t key_win32_base_get(uint32_t vk)
{
    const uint8_t code[] = { 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xDB, 0xDC, 0xDD, 0xDE };
    const char base[] = ";=,-./`[\\]'";
    uint32_t idx = 0;

    if (vk >= KEY_WIN32_VK_LETTER_FIRST && vk <= KEY_WIN32_VK_LETTER_LAST) { return vk + KEY_WIN32_LOWER_OFF; }

    if ((vk >= KEY_WIN32_VK_DIGIT_FIRST && vk <= KEY_WIN32_VK_DIGIT_LAST) || vk == KEY_WIN32_VK_SPACE) { return vk; }

    while (idx < sizeof(code))
    {
        if (code[idx] == vk) { return (uint32_t)(uint8_t)base[idx]; }

        idx++;
    }

    return KEY_CODE_NONE;
}

static uint32_t key_scan_base_get(uint32_t scan)
{
    const uint8_t code[] = { 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x39 };
    const char base[] = "1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./ ";
    uint32_t idx = 0;

    while (idx < sizeof(code))
    {
        if (code[idx] == scan) { return (uint32_t)(uint8_t)base[idx]; }

        idx++;
    }

    return KEY_CODE_NONE;
}

static uint32_t key_win32_modifier_code_get(uint32_t vk, uint32_t scan, uint32_t state)
{
    uint8_t rhs = ((state & KEY_WIN32_STATE_ENHANCED) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (vk == KEY_WIN32_VK_SHIFT) { return (scan == KEY_WIN32_SCAN_RHS_SHIFT) ? KEY_KITTY_RHS_SHIFT : KEY_KITTY_LHS_SHIFT; }

    if (vk == KEY_WIN32_VK_CONTROL) { return (rhs == LDG_TRUTH_TRUE) ? KEY_KITTY_RHS_CONTROL : KEY_KITTY_LHS_CONTROL; }

    if (vk == KEY_WIN32_VK_MENU) { return (rhs == LDG_TRUTH_TRUE) ? KEY_KITTY_RHS_ALT : KEY_KITTY_LHS_ALT; }

    if (vk == KEY_WIN32_VK_LHS_SHIFT) { return KEY_KITTY_LHS_SHIFT; }

    if (vk == KEY_WIN32_VK_RHS_SHIFT) { return KEY_KITTY_RHS_SHIFT; }

    if (vk == KEY_WIN32_VK_LHS_CONTROL) { return KEY_KITTY_LHS_CONTROL; }

    if (vk == KEY_WIN32_VK_RHS_CONTROL) { return KEY_KITTY_RHS_CONTROL; }

    if (vk == KEY_WIN32_VK_LHS_MENU) { return KEY_KITTY_LHS_ALT; }

    if (vk == KEY_WIN32_VK_RHS_MENU) { return KEY_KITTY_RHS_ALT; }

    if (vk == KEY_WIN32_VK_LHS_WIN) { return KEY_KITTY_LHS_SUPER; }

    if (vk == KEY_WIN32_VK_RHS_WIN) { return KEY_KITTY_RHS_SUPER; }

    if (vk == KEY_WIN32_VK_CAPITAL) { return KEY_KITTY_CAPS_LOCK; }

    return KEY_CODE_NONE;
}

static uint8_t key_win32_mod_get(uint32_t state)
{
    uint8_t mods = LDG_TERM_MOD_NONE;

    if ((state & KEY_WIN32_STATE_SHIFT) != 0) { mods |= LDG_TERM_MOD_SHIFT; }

    if ((state & (KEY_WIN32_STATE_LHS_CTRL | KEY_WIN32_STATE_RHS_CTRL)) != 0) { mods |= LDG_TERM_MOD_CTRL; }

    if ((state & (KEY_WIN32_STATE_LHS_ALT | KEY_WIN32_STATE_RHS_ALT)) != 0) { mods |= LDG_TERM_MOD_ALT; }

    if ((state & KEY_WIN32_STATE_CAPSLOCK) != 0) { mods |= LDG_TERM_MOD_CAPS; }

    if ((state & KEY_WIN32_STATE_NUMLOCK) != 0) { mods |= LDG_TERM_MOD_NUM; }

    return mods;
}

static uint8_t key_win32_text_is(uint32_t uc, uint32_t mods)
{
    uint8_t ctrl = ((mods & LDG_TERM_MOD_CTRL) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t alt = ((mods & LDG_TERM_MOD_ALT) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (uc < KEY_SPACE_BYTE || uc == KEY_DEL_BYTE) { return LDG_TRUTH_FALSE; }

    if (ctrl != alt) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t key_win32_event_set(const key_seq_t *seq, ldg_term_key_event_t *out)
{
    uint32_t vk = seq->val[KEY_WIN32_GROUP_VK][0];
    uint32_t scan = seq->val[KEY_WIN32_GROUP_SCAN][0];
    uint32_t uc = seq->val[KEY_WIN32_GROUP_UNICODE][0];
    uint32_t state = seq->val[KEY_WIN32_GROUP_STATE][0];
    uint32_t code = KEY_CODE_NONE;
    uint32_t base = KEY_CODE_NONE;

    if (vk == 0 && uc == 0) { return LDG_ERR_UNSUPPORTED; }

    out->kind = (seq->val[KEY_WIN32_GROUP_DOWN][0] != 0) ? LDG_TERM_KEY_KIND_PRESS : LDG_TERM_KEY_KIND_RELEASE;
    out->mods = key_win32_mod_get(state);
    out->key = key_win32_key_get(vk);

    if (out->key == LDG_TERM_KEY_PRINT)
    {
        if (out->kind == LDG_TERM_KEY_KIND_PRESS) { return LDG_ERR_UNSUPPORTED; }

        out->kind = LDG_TERM_KEY_KIND_PRESS;
    }

    if (out->key != LDG_TERM_KEY_NONE) { return LDG_ERR_AOK; }

    code = key_win32_modifier_code_get(vk, scan, state);

    if (code == KEY_CODE_NONE) { code = key_win32_base_get(vk); }

    if (code == KEY_CODE_NONE && key_win32_text_is(uc, LDG_TERM_MOD_NONE) == LDG_TRUTH_TRUE) { code = uc; }

    if (code == KEY_CODE_NONE) { return LDG_ERR_UNSUPPORTED; }

    out->codepoint = code;

    if ((state & KEY_WIN32_STATE_ENHANCED) == 0) { base = key_scan_base_get(scan); }

    if (base != code) { out->base = base; }

    if (key_win32_text_is(uc, LDG_TERM_MOD_NONE) == LDG_TRUTH_TRUE && uc != code) { out->shifted = uc; }

    if (out->kind == LDG_TERM_KEY_KIND_PRESS && key_win32_text_is(uc, out->mods) == LDG_TRUTH_TRUE)
    {
        out->text[0] = uc;
        out->text_cunt = KEY_SINGLE_SIZE;
    }

    return LDG_ERR_AOK;
}

static uint32_t key_seq_event_set(const key_seq_t *seq, ldg_term_key_event_t *out)
{
    if (seq->lead != 0 || seq->inter_is || seq->overflow_is) { return LDG_ERR_UNSUPPORTED; }

    if (seq->final == KEY_WIN32_FINAL) { return key_win32_event_set(seq, out); }

    key_seq_mod_apply(seq, out);

    if (seq->final == 'u') { return key_seq_code_event_set(seq, out); }

    if (seq->final == '~' && seq->val[KEY_GROUP_CODE][0] == KEY_TILDE_OTHER_KEYS) { return key_seq_other_event_set(seq, out); }

    if (seq->final == '~') { out->key = key_tilde_key_get(seq->val[KEY_GROUP_CODE][0]); }
    else if (seq->val[KEY_GROUP_CODE][0] > KEY_MOD_BASE) { return LDG_ERR_UNSUPPORTED; }
    else if (seq->final == 'Z') { out->key = LDG_TERM_KEY_TAB; }
    else { out->key = key_letter_key_get(seq->final); }

    if (seq->final == 'Z') { out->mods |= LDG_TERM_MOD_SHIFT; }

    return (out->key != LDG_TERM_KEY_NONE) ? LDG_ERR_AOK : LDG_ERR_UNSUPPORTED;
}

static uint32_t key_seq_decode(const ldg_byte_t *src, uint64_t len, uint8_t final_is, ldg_term_key_event_t *out, uint64_t *out_used)
{
    key_seq_t seq = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (key_seq_scan_is(&seq, src, len) != LDG_TRUTH_TRUE)
    {
        if (!seq.broken_is && seq.len < KEY_SEQ_MAX && !final_is) { return LDG_ERR_AGAIN; }

        if (len > KEY_PAIR_SIZE || seq.broken_is)
        {
            *out_used = seq.len;

            return LDG_ERR_UNSUPPORTED;
        }

        *out_used = KEY_PAIR_SIZE;
        ret = key_byte_event_set(KEY_CSI_LEAD, out);
        out->mods |= LDG_TERM_MOD_ALT;

        return ret;
    }

    if (seq.final == 'M' && seq.lead == 0 && seq.sub_cunt[KEY_GROUP_CODE] == 0)
    {
        if ((len - seq.len) < KEY_X10_TAIL_SIZE && !final_is) { return LDG_ERR_AGAIN; }

        *out_used = ((len - seq.len) < KEY_X10_TAIL_SIZE) ? len : (seq.len + KEY_X10_TAIL_SIZE);

        return LDG_ERR_UNSUPPORTED;
    }

    *out_used = seq.len;

    ret = key_seq_event_set(&seq, out);

    if (ret != LDG_ERR_AOK) { key_event_clear(out); }

    return ret;
}

static uint32_t key_ss3_decode(const ldg_byte_t *src, uint64_t len, uint8_t final_is, ldg_term_key_event_t *out, uint64_t *out_used)
{
    uint32_t ret = 0;

    if (len < KEY_SS3_SIZE && !final_is) { return LDG_ERR_AGAIN; }

    if (len < KEY_SS3_SIZE)
    {
        *out_used = KEY_PAIR_SIZE;
        ret = key_byte_event_set(KEY_SS3_LEAD, out);
        out->mods |= LDG_TERM_MOD_ALT;

        return ret;
    }

    *out_used = KEY_SS3_SIZE;
    out->key = key_letter_key_get(src[KEY_PAIR_SIZE]);

    return (out->key != LDG_TERM_KEY_NONE) ? LDG_ERR_AOK : LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_term_key_decode(const ldg_byte_t *src, uint64_t len, uint8_t final_is, ldg_term_key_event_t *out, uint64_t *out_used)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src || !out || !out_used)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_used = 0;
    key_event_clear(out);

    if (LDG_UNLIKELY(final_is != LDG_TRUTH_TRUE && final_is != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (len == 0) { return LDG_ERR_EMPTY; }

    if (src[0] != KEY_ESC_BYTE || (len == KEY_SINGLE_SIZE && final_is))
    {
        *out_used = KEY_SINGLE_SIZE;

        return key_byte_event_set(src[0], out);
    }

    if (len == KEY_SINGLE_SIZE) { return LDG_ERR_AGAIN; }

    if (src[KEY_ADJACENT_OFF] == KEY_CSI_LEAD) { return key_seq_decode(src, len, final_is, out, out_used); }

    if (src[KEY_ADJACENT_OFF] == KEY_SS3_LEAD) { return key_ss3_decode(src, len, final_is, out, out_used); }

    *out_used = KEY_PAIR_SIZE;
    ret = key_byte_event_set(src[KEY_ADJACENT_OFF], out);
    out->mods |= LDG_TERM_MOD_ALT;

    return ret;
}
