#include <stdint.h>

#include <dangling/term/vt.h>
#include <dangling/term/term.h>
#include <dangling/str/str.h>
#include <dangling/compress/inflate.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TERM_VT_STEP 1
#define LDG_TERM_VT_DECIMAL 10
#define LDG_TERM_VT_ESC_CH 0x1B
#define LDG_TERM_VT_BEL_CH 0x07
#define LDG_TERM_VT_BS_CH 0x08
#define LDG_TERM_VT_HT_CH 0x09
#define LDG_TERM_VT_LF_CH 0x0A
#define LDG_TERM_VT_VTAB_CH 0x0B
#define LDG_TERM_VT_FF_CH 0x0C
#define LDG_TERM_VT_CR_CH 0x0D
#define LDG_TERM_VT_SPACE_CH 0x20
#define LDG_TERM_VT_DEL_CH 0x7F
#define LDG_TERM_VT_DIGIT_LOW 0x30
#define LDG_TERM_VT_DIGIT_HIGH 0x39
#define LDG_TERM_VT_INTER_LOW 0x20
#define LDG_TERM_VT_INTER_HIGH 0x2F
#define LDG_TERM_VT_FINAL_LOW 0x40
#define LDG_TERM_VT_FINAL_HIGH 0x7E
#define LDG_TERM_VT_CAN_CH 0x18
#define LDG_TERM_VT_SUB_CH 0x1A
#define LDG_TERM_VT_C0_HIGH 0x1F
#define LDG_TERM_VT_PRIV_LOW 0x3C
#define LDG_TERM_VT_PRIV_HIGH 0x3F
#define LDG_TERM_VT_COLON_CH 0x3A
#define LDG_TERM_VT_SEMI_CH 0x3B
#define LDG_TERM_VT_ESC_FINAL_LOW 0x30
#define LDG_TERM_VT_CSI_LEAD_CH 0x5B
#define LDG_TERM_VT_OSC_LEAD_CH 0x5D
#define LDG_TERM_VT_DCS_LEAD_CH 0x50
#define LDG_TERM_VT_APC_LEAD_CH 0x5F
#define LDG_TERM_VT_PM_LEAD_CH 0x5E
#define LDG_TERM_VT_SOS_LEAD_CH 0x58
#define LDG_TERM_VT_ST_CH 0x5C
#define LDG_TERM_VT_CHARSET_G0 0x28
#define LDG_TERM_VT_CHARSET_G1 0x29
#define LDG_TERM_VT_CHARSET_GFX 0x30
#define LDG_TERM_VT_CHARSET_UK 0x41
#define LDG_TERM_VT_SO_CH 0x0E
#define LDG_TERM_VT_SI_CH 0x0F
#define LDG_TERM_VT_HASH_CH 0x23
#define LDG_TERM_VT_BANG_CH 0x21
#define LDG_TERM_VT_DOLLAR_CH 0x24

#define LDG_TERM_VT_SGR_RST 0
#define LDG_TERM_VT_SGR_BOLD 1
#define LDG_TERM_VT_SGR_DIM 2
#define LDG_TERM_VT_SGR_ITALIC 3
#define LDG_TERM_VT_SGR_UNDERLINE 4
#define LDG_TERM_VT_SGR_BLINK 5
#define LDG_TERM_VT_SGR_REVERSE 7
#define LDG_TERM_VT_SGR_HIDDEN 8
#define LDG_TERM_VT_SGR_STRIKE 9
#define LDG_TERM_VT_SGR_NO_BOLD 22
#define LDG_TERM_VT_SGR_NO_ITALIC 23
#define LDG_TERM_VT_SGR_NO_UNDERLINE 24
#define LDG_TERM_VT_SGR_NO_BLINK 25
#define LDG_TERM_VT_SGR_NO_REVERSE 27
#define LDG_TERM_VT_SGR_NO_HIDDEN 28
#define LDG_TERM_VT_SGR_NO_STRIKE 29
#define LDG_TERM_VT_SGR_FG_LOW 30
#define LDG_TERM_VT_SGR_FG_HIGH 37
#define LDG_TERM_VT_SGR_FG_EXT 38
#define LDG_TERM_VT_SGR_FG_DEFAULT 39
#define LDG_TERM_VT_SGR_BG_LOW 40
#define LDG_TERM_VT_SGR_BG_HIGH 47
#define LDG_TERM_VT_SGR_BG_EXT 48
#define LDG_TERM_VT_SGR_BG_DEFAULT 49
#define LDG_TERM_VT_SGR_FG_BRIGHT_LOW 90
#define LDG_TERM_VT_SGR_FG_BRIGHT_HIGH 97
#define LDG_TERM_VT_SGR_BG_BRIGHT_LOW 100
#define LDG_TERM_VT_SGR_BG_BRIGHT_HIGH 107
#define LDG_TERM_VT_SGR_EXT_PALETTE 5
#define LDG_TERM_VT_SGR_EXT_RGB 2

#define LDG_TERM_VT_MODE_CURSOR 25
#define LDG_TERM_VT_MODE_ALT_OLD 47
#define LDG_TERM_VT_MODE_ALT_BUFF 1047
#define LDG_TERM_VT_MODE_ALT_CURSOR 1048
#define LDG_TERM_VT_MODE_ALT_FULL 1049

#define LDG_TERM_VT_PALETTE_BASE 16
#define LDG_TERM_VT_PALETTE_GRAY 232
#define LDG_TERM_VT_PALETTE_MAX 255
#define LDG_TERM_VT_CUBE_SIDE 6
#define LDG_TERM_VT_CUBE_PLANE 36
#define LDG_TERM_VT_CUBE_BASE 55
#define LDG_TERM_VT_CUBE_STEP 40
#define LDG_TERM_VT_GRAY_BASE 8
#define LDG_TERM_VT_GRAY_STEP 10
#define LDG_TERM_VT_BRIGHT_BIT 8

#define LDG_TERM_VT_RED_IDX 0
#define LDG_TERM_VT_GREEN_IDX 1
#define LDG_TERM_VT_BLUE_IDX 2

#define LDG_TERM_VT_DIM_LEVEL 0x80
#define LDG_TERM_VT_FULL_LEVEL 0xC0
#define LDG_TERM_VT_PEAK_LEVEL 0xFE
#define LDG_TERM_VT_OFF_LEVEL 0
#define LDG_TERM_VT_PARAM_FIRST 0
#define LDG_TERM_VT_ANSI_BLACK 0
#define LDG_TERM_VT_ANSI_RED 1
#define LDG_TERM_VT_ANSI_GREEN 2
#define LDG_TERM_VT_ANSI_YELLOW 3
#define LDG_TERM_VT_ANSI_BLUE 4
#define LDG_TERM_VT_ANSI_MAGENTA 5
#define LDG_TERM_VT_ANSI_CYAN 6

static void vt_ansi_color_get(uint32_t idx, uint8_t *out)
{
    uint32_t base = idx % LDG_TERM_VT_BRIGHT_BIT;
    uint8_t low = LDG_TERM_VT_DIM_LEVEL;
    uint8_t high = LDG_TERM_VT_FULL_LEVEL;

    if (idx >= LDG_TERM_VT_BRIGHT_BIT)
    {
        low = LDG_TERM_VT_PEAK_LEVEL;
        high = LDG_TERM_VT_PEAK_LEVEL;
    }

    out[LDG_TERM_VT_RED_IDX] = 0;
    out[LDG_TERM_VT_GREEN_IDX] = 0;
    out[LDG_TERM_VT_BLUE_IDX] = 0;

    if (base == LDG_TERM_VT_ANSI_BLACK)
    {
        if (idx >= LDG_TERM_VT_BRIGHT_BIT)
        {
            out[LDG_TERM_VT_RED_IDX] = LDG_TERM_VT_DIM_LEVEL;
            out[LDG_TERM_VT_GREEN_IDX] = LDG_TERM_VT_DIM_LEVEL;
            out[LDG_TERM_VT_BLUE_IDX] = LDG_TERM_VT_DIM_LEVEL;
        }

        return;
    }

    if (base == LDG_TERM_VT_ANSI_RED) { out[LDG_TERM_VT_RED_IDX] = low; }
    else if (base == LDG_TERM_VT_ANSI_GREEN) { out[LDG_TERM_VT_GREEN_IDX] = low; }
    else if (base == LDG_TERM_VT_ANSI_YELLOW)
    {
        out[LDG_TERM_VT_RED_IDX] = low;
        out[LDG_TERM_VT_GREEN_IDX] = low;
    }
    else if (base == LDG_TERM_VT_ANSI_BLUE) { out[LDG_TERM_VT_BLUE_IDX] = low; }
    else if (base == LDG_TERM_VT_ANSI_MAGENTA)
    {
        out[LDG_TERM_VT_RED_IDX] = low;
        out[LDG_TERM_VT_BLUE_IDX] = low;
    }
    else if (base == LDG_TERM_VT_ANSI_CYAN)
    {
        out[LDG_TERM_VT_GREEN_IDX] = low;
        out[LDG_TERM_VT_BLUE_IDX] = low;
    }
    else
    {
        out[LDG_TERM_VT_RED_IDX] = high;
        out[LDG_TERM_VT_GREEN_IDX] = high;
        out[LDG_TERM_VT_BLUE_IDX] = high;
    }
}

static void vt_palette_get(uint32_t idx, uint8_t *out)
{
    uint32_t val = 0;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;

    if (idx < LDG_TERM_VT_PALETTE_BASE)
    {
        vt_ansi_color_get(idx, out);
        return;
    }

    if (idx >= LDG_TERM_VT_PALETTE_GRAY)
    {
        val = LDG_TERM_VT_GRAY_BASE + ((idx - LDG_TERM_VT_PALETTE_GRAY) * LDG_TERM_VT_GRAY_STEP);

        out[LDG_TERM_VT_RED_IDX] = (uint8_t)val;
        out[LDG_TERM_VT_GREEN_IDX] = (uint8_t)val;
        out[LDG_TERM_VT_BLUE_IDX] = (uint8_t)val;

        return;
    }

    val = idx - LDG_TERM_VT_PALETTE_BASE;
    red = val / LDG_TERM_VT_CUBE_PLANE;
    green = (val / LDG_TERM_VT_CUBE_SIDE) % LDG_TERM_VT_CUBE_SIDE;
    blue = val % LDG_TERM_VT_CUBE_SIDE;

    out[LDG_TERM_VT_RED_IDX] = (red > 0) ? (uint8_t)(LDG_TERM_VT_CUBE_BASE + (red * LDG_TERM_VT_CUBE_STEP)) : LDG_TERM_VT_OFF_LEVEL;
    out[LDG_TERM_VT_GREEN_IDX] = (green > 0) ? (uint8_t)(LDG_TERM_VT_CUBE_BASE + (green * LDG_TERM_VT_CUBE_STEP)) : LDG_TERM_VT_OFF_LEVEL;
    out[LDG_TERM_VT_BLUE_IDX] = (blue > 0) ? (uint8_t)(LDG_TERM_VT_CUBE_BASE + (blue * LDG_TERM_VT_CUBE_STEP)) : LDG_TERM_VT_OFF_LEVEL;
}

static uint32_t vt_param_get(const ldg_term_vt_t *vt, uint32_t idx, uint32_t fallback)
{
    if (idx >= vt->param_cunt) { return fallback; }

    if (vt->param_seen[idx] != LDG_TRUTH_TRUE) { return fallback; }

    return vt->param[idx];
}

static uint32_t vt_param_cunt_get(const ldg_term_vt_t *vt, uint32_t idx)
{
    uint32_t val = 0;

    val = vt_param_get(vt, idx, LDG_TERM_VT_STEP);

    if (val < LDG_TERM_VT_STEP) { return LDG_TERM_VT_STEP; }

    return val;
}

static void vt_param_rst(ldg_term_vt_t *vt)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_TERM_VT_PARAM_MAX; idx++)
    {
        vt->param[idx] = 0;
        vt->param_seen[idx] = LDG_TRUTH_FALSE;
        vt->param_sub[idx] = LDG_TRUTH_FALSE;
    }

    vt->param_cunt = 0;
    vt->inter_cunt = 0;
    vt->osc_len = 0;
    vt->priv = LDG_TRUTH_FALSE;
    vt->priv_lead = 0;
    vt->str_kind = LDG_TERM_VT_STR_NONE;
}

static void vt_sgr_apply(ldg_term_vt_t *vt)
{
    uint8_t rgb[LDG_TERM_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t code = 0;
    uint32_t kind = 0;
    uint32_t val = 0;
    uint32_t base = 0;

    if (vt->param_cunt == 0)
    {
        ldg_term_attr_rst(vt->term);
        return;
    }

    for (idx = 0; idx < vt->param_cunt; idx++)
    {
        code = vt_param_get(vt, idx, LDG_TERM_VT_SGR_RST);

        if (code == LDG_TERM_VT_SGR_RST) { ldg_term_attr_rst(vt->term); }
        else if (code == LDG_TERM_VT_SGR_BOLD) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_BOLD); }
        else if (code == LDG_TERM_VT_SGR_DIM) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_DIM); }
        else if (code == LDG_TERM_VT_SGR_ITALIC) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_ITALIC); }
        else if (code == LDG_TERM_VT_SGR_UNDERLINE) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_UNDERLINE); }
        else if (code == LDG_TERM_VT_SGR_BLINK) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_BLINK); }
        else if (code == LDG_TERM_VT_SGR_REVERSE) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_REVERSE); }
        else if (code == LDG_TERM_VT_SGR_HIDDEN) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_HIDDEN); }
        else if (code == LDG_TERM_VT_SGR_STRIKE) { ldg_term_attr_set(vt->term, LDG_TERM_ATTR_STRIKE); }
        else if (code == LDG_TERM_VT_SGR_NO_BOLD) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_BOLD | LDG_TERM_ATTR_DIM); }
        else if (code == LDG_TERM_VT_SGR_NO_ITALIC) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_ITALIC); }
        else if (code == LDG_TERM_VT_SGR_NO_UNDERLINE) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_UNDERLINE); }
        else if (code == LDG_TERM_VT_SGR_NO_BLINK) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_BLINK); }
        else if (code == LDG_TERM_VT_SGR_NO_REVERSE) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_REVERSE); }
        else if (code == LDG_TERM_VT_SGR_NO_HIDDEN) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_HIDDEN); }
        else if (code == LDG_TERM_VT_SGR_NO_STRIKE) { ldg_term_attr_clear(vt->term, LDG_TERM_ATTR_STRIKE); }
        else if (code >= LDG_TERM_VT_SGR_FG_LOW && code <= LDG_TERM_VT_SGR_FG_HIGH)
        {
            vt_ansi_color_get(code - LDG_TERM_VT_SGR_FG_LOW, rgb);
            ldg_term_fg_set(vt->term, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]);
        }
        else if (code >= LDG_TERM_VT_SGR_BG_LOW && code <= LDG_TERM_VT_SGR_BG_HIGH)
        {
            vt_ansi_color_get(code - LDG_TERM_VT_SGR_BG_LOW, rgb);
            ldg_term_bg_set(vt->term, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]);
        }
        else if (code >= LDG_TERM_VT_SGR_FG_BRIGHT_LOW && code <= LDG_TERM_VT_SGR_FG_BRIGHT_HIGH)
        {
            vt_ansi_color_get((code - LDG_TERM_VT_SGR_FG_BRIGHT_LOW) + LDG_TERM_VT_BRIGHT_BIT, rgb);
            ldg_term_fg_set(vt->term, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]);
        }
        else if (code >= LDG_TERM_VT_SGR_BG_BRIGHT_LOW && code <= LDG_TERM_VT_SGR_BG_BRIGHT_HIGH)
        {
            vt_ansi_color_get((code - LDG_TERM_VT_SGR_BG_BRIGHT_LOW) + LDG_TERM_VT_BRIGHT_BIT, rgb);
            ldg_term_bg_set(vt->term, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]);
        }
        else if (code == LDG_TERM_VT_SGR_FG_DEFAULT) { ldg_term_fg_set(vt->term, vt->term->default_fg[LDG_TERM_VT_RED_IDX], vt->term->default_fg[LDG_TERM_VT_GREEN_IDX], vt->term->default_fg[LDG_TERM_VT_BLUE_IDX]); }
        else if (code == LDG_TERM_VT_SGR_BG_DEFAULT) { ldg_term_bg_set(vt->term, vt->term->default_bg[LDG_TERM_VT_RED_IDX], vt->term->default_bg[LDG_TERM_VT_GREEN_IDX], vt->term->default_bg[LDG_TERM_VT_BLUE_IDX]); }
        else if (code == LDG_TERM_VT_SGR_FG_EXT || code == LDG_TERM_VT_SGR_BG_EXT)
        {
            kind = vt_param_get(vt, idx + LDG_TERM_VT_STEP, LDG_TERM_VT_SGR_RST);

            if (kind == LDG_TERM_VT_SGR_EXT_PALETTE)
            {
                val = vt_param_get(vt, idx + LDG_TERM_VT_STEP + LDG_TERM_VT_STEP, LDG_TERM_VT_SGR_RST);

                if (val > LDG_TERM_VT_PALETTE_MAX) { val = LDG_TERM_VT_PALETTE_MAX; }

                vt_palette_get(val, rgb);

                idx += LDG_TERM_VT_STEP;
                idx += LDG_TERM_VT_STEP;
            }
            else if (kind == LDG_TERM_VT_SGR_EXT_RGB)
            {
                base = idx + LDG_TERM_VT_STEP + LDG_TERM_VT_STEP;

                if (base < vt->param_cunt && vt->param_sub[base] == LDG_TRUTH_TRUE && vt->param_seen[base] != LDG_TRUTH_TRUE) { base += LDG_TERM_VT_STEP; }

                rgb[LDG_TERM_VT_RED_IDX] = (uint8_t)vt_param_get(vt, base, LDG_TERM_VT_SGR_RST);
                rgb[LDG_TERM_VT_GREEN_IDX] = (uint8_t)vt_param_get(vt, base + LDG_TERM_VT_STEP, LDG_TERM_VT_SGR_RST);
                rgb[LDG_TERM_VT_BLUE_IDX] = (uint8_t)vt_param_get(vt, base + LDG_TERM_VT_STEP + LDG_TERM_VT_STEP, LDG_TERM_VT_SGR_RST);

                idx = base + LDG_TERM_VT_STEP + LDG_TERM_VT_STEP;
            }
            else { continue; }

            if (code == LDG_TERM_VT_SGR_FG_EXT) { ldg_term_fg_set(vt->term, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]); }
            else { ldg_term_bg_set(vt->term, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]); }
        }
    }
}

#define LDG_TERM_VT_DEC_APP_CURSOR 1
#define LDG_TERM_VT_DEC_ORIG 6
#define LDG_TERM_VT_DEC_WRAP 7
#define LDG_TERM_VT_DEC_FOCUS 1004
#define LDG_TERM_VT_DEC_MOUSE_NORMAL 1000
#define LDG_TERM_VT_DEC_MOUSE_BUTTON 1002
#define LDG_TERM_VT_DEC_MOUSE_ANY 1003
#define LDG_TERM_VT_DEC_MOUSE_SGR 1006
#define LDG_TERM_VT_ANSI_IRM 4
#define LDG_TERM_VT_ANSI_LNM 20
#define LDG_TERM_VT_DEC_SCREEN 5
#define LDG_TERM_VT_DEC_COLM 3
#define LDG_TERM_VT_DEC_SCLM 4
#define LDG_TERM_VT_DEC_ARM 8
#define LDG_TERM_VT_DEC_INLM 9
#define LDG_TERM_VT_DEC_ANM 2
#define LDG_TERM_VT_DEC_LRMM 69
#define LDG_TERM_VT_DEC_SYNC 2026
#define LDG_TERM_VT_DEC_RESIZE 2048
#define LDG_TERM_VT_TBC_ALL 3
#define LDG_TERM_VT_RQM_SET 1
#define LDG_TERM_VT_RQM_RST 2
#define LDG_TERM_VT_RQM_UNKNOWN 0
#define LDG_TERM_VT_DEC_PASTE 2004

static void vt_mode_bit_set(ldg_term_vt_t *vt, uint32_t bit, uint8_t en)
{
    if (en == LDG_TRUTH_TRUE)
    {
        vt->mode |= bit;
        return;
    }

    vt->mode &= ~bit;
}

static void vt_unhandled_mode_mark(ldg_term_vt_t *vt, uint32_t code)
{
    uint32_t idx = 0;

    for (idx = 0; idx < vt->unhandled_mode_cunt; idx++) { if (vt->unhandled_mode[idx] == code) { return; } }

    if (vt->unhandled_mode_cunt >= LDG_TERM_VT_UNHANDLED_MODE_MAX) { return; }

    vt->unhandled_mode[vt->unhandled_mode_cunt] = code;
    vt->unhandled_mode_cunt++;
}

static void vt_mode_apply(ldg_term_vt_t *vt, uint8_t en)
{
    uint32_t idx = 0;
    uint32_t code = 0;

    if (vt->priv != LDG_TRUTH_TRUE)
    {
        for (idx = 0; idx < vt->param_cunt; idx++)
        {
            code = vt_param_get(vt, idx, LDG_TERM_VT_SGR_RST);

            if (code == LDG_TERM_VT_ANSI_IRM) { ldg_term_insert_mode_set(vt->term, en); }
            else if (code == LDG_TERM_VT_ANSI_LNM) { ldg_term_newline_mode_set(vt->term, en); }
        }

        return;
    }

    for (idx = 0; idx < vt->param_cunt; idx++)
    {
        code = vt_param_get(vt, idx, LDG_TERM_VT_SGR_RST);

        if (code == LDG_TERM_VT_DEC_APP_CURSOR) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_APP_CURSOR, en); }
        else if (code == LDG_TERM_VT_DEC_ORIG)
        {
            vt_mode_bit_set(vt, LDG_TERM_VT_MODE_ORIG, en);
            ldg_term_orig_set(vt->term, en);
        }
        else if (code == LDG_TERM_VT_DEC_WRAP)
        {
            vt_mode_bit_set(vt, LDG_TERM_VT_MODE_WRAP, en);
            ldg_term_autowrap_set(vt->term, en);
        }
        else if (code == LDG_TERM_VT_DEC_FOCUS) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_FOCUS, en); }
        else if (code == LDG_TERM_VT_DEC_MOUSE_NORMAL) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_MOUSE_NORMAL, en); }
        else if (code == LDG_TERM_VT_DEC_MOUSE_BUTTON) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_MOUSE_BUTTON, en); }
        else if (code == LDG_TERM_VT_DEC_MOUSE_ANY) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_MOUSE_ANY, en); }
        else if (code == LDG_TERM_VT_DEC_MOUSE_SGR) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_MOUSE_SGR, en); }
        else if (code == LDG_TERM_VT_DEC_SCREEN) { ldg_term_reverse_set(vt->term, en); }
        else if (code == LDG_TERM_VT_DEC_COLM) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_COLM, en); ldg_term_display_erase(vt->term, LDG_TERM_ERASE_ALL); }
        else if (code == LDG_TERM_VT_DEC_SCLM) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_SCLM, en); }
        else if (code == LDG_TERM_VT_DEC_ARM) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_ARM, en); }
        else if (code == LDG_TERM_VT_DEC_INLM) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_INLM, en); }
        else if (code == LDG_TERM_VT_DEC_ANM) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_ANSI, en); }
        else if (code == LDG_TERM_VT_DEC_SYNC) { ldg_term_sync_set(vt->term, en); }
        else if (code == LDG_TERM_VT_DEC_LRMM) { ldg_term_margin_side_en(vt->term, en); }
        else if (code == LDG_TERM_VT_DEC_RESIZE) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_RESIZE, en); }
        else if (code == LDG_TERM_VT_DEC_PASTE) { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_PASTE_BRACKET, en); }
        else if (code == LDG_TERM_VT_MODE_CURSOR) { ldg_term_cursor_visible_set(vt->term, en); }
        else if (code == LDG_TERM_VT_MODE_ALT_CURSOR) { if (en == LDG_TRUTH_TRUE) { ldg_term_cursor_save(vt->term); }
            else { ldg_term_cursor_restore(vt->term); } }
        else if (code == LDG_TERM_VT_MODE_ALT_BUFF || code == LDG_TERM_VT_MODE_ALT_OLD) { if (en == LDG_TRUTH_TRUE) { ldg_term_alt_enter(vt->term); }
            else { ldg_term_alt_exit(vt->term); } }
        else if (code == LDG_TERM_VT_MODE_ALT_FULL)
        {
            if (en == LDG_TRUTH_TRUE)
            {
                ldg_term_cursor_save(vt->term);
                ldg_term_alt_enter(vt->term);
            }
            else
            {
                ldg_term_alt_exit(vt->term);
                ldg_term_cursor_restore(vt->term);
            }
        }
        else { vt_unhandled_mode_mark(vt, code); }
    }
}

#define LDG_TERM_VT_PAIR_STEP 2
#define LDG_TERM_VT_OSC_SEP ';'
#define LDG_TERM_VT_OSC_CLIP_HIGH '5'
#define LDG_TERM_VT_OSC_CLIP_LOW '2'
#define LDG_TERM_VT_OSC_TITLE_BOTH '0'
#define LDG_TERM_VT_OSC_TITLE_WIN '2'
#define LDG_TERM_VT_OSC_LEAD 0x5D
#define LDG_TERM_VT_BEL_BYTE 0x07
#define LDG_TERM_VT_CLIP_SEL 'c'

#define LDG_TERM_VT_ESC_BYTE 0x1B
#define LDG_TERM_VT_CSI_LEAD 0x5B
#define LDG_TERM_VT_DIGIT_BASE 0x30
#define LDG_TERM_VT_NUM_RADIX 10
#define LDG_TERM_VT_NUM_CAP 8
#define LDG_TERM_VT_SPACE_INTER 0x20
#define LDG_TERM_VT_DA_KIND 6
#define LDG_TERM_VT_DSR_STATUS 5
#define LDG_TERM_VT_DSR_CURSOR 6
#define LDG_TERM_VT_DSR_OK 0

static void vt_reply_byte_wr(ldg_term_vt_t *vt, uint8_t byte)
{
    if (vt->reply_len >= LDG_TERM_VT_REPLY_MAX) { return; }

    vt->reply[vt->reply_len] = byte;
    vt->reply_len++;
}

static void vt_reply_num_wr(ldg_term_vt_t *vt, uint32_t val)
{
    uint8_t digit[LDG_TERM_VT_NUM_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t rest = val;
    uint32_t cunt = 0;
    uint32_t idx = 0;

    do
    {
        if (cunt >= LDG_TERM_VT_NUM_CAP) { return; }

        digit[cunt] = (uint8_t)(LDG_TERM_VT_DIGIT_BASE + (rest % LDG_TERM_VT_NUM_RADIX));
        rest = rest / LDG_TERM_VT_NUM_RADIX;
        cunt++;
    }
    while (rest > 0);

    for (idx = cunt; idx > 0; idx--) { vt_reply_byte_wr(vt, digit[idx - LDG_TERM_VT_STEP]); }
}

static void vt_reply_da_wr(ldg_term_vt_t *vt)
{
    vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
    vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
    vt_reply_byte_wr(vt, '?');
    vt_reply_num_wr(vt, LDG_TERM_VT_DA_KIND);
    vt_reply_byte_wr(vt, 'c');
}

#define LDG_TERM_VT_PARM_REPORT 2
#define LDG_TERM_VT_PARM_NO_PARITY 1
#define LDG_TERM_VT_PARM_EIGHT_BIT 1
#define LDG_TERM_VT_PARM_SPEED 120
#define LDG_TERM_VT_PARM_CLKMUL 1
#define LDG_TERM_VT_PARM_FLAGS 0

static void vt_reply_parm_wr(ldg_term_vt_t *vt)
{
    uint32_t sol = 0;

    sol = vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST);

    vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
    vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
    vt_reply_num_wr(vt, sol + LDG_TERM_VT_PARM_REPORT);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, LDG_TERM_VT_PARM_NO_PARITY);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, LDG_TERM_VT_PARM_EIGHT_BIT);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, LDG_TERM_VT_PARM_SPEED);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, LDG_TERM_VT_PARM_SPEED);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, LDG_TERM_VT_PARM_CLKMUL);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, LDG_TERM_VT_PARM_FLAGS);
    vt_reply_byte_wr(vt, 'x');
}

static void vt_reply_dsr_wr(ldg_term_vt_t *vt, uint32_t kind)
{
    uint32_t row = 0;

    if (kind == LDG_TERM_VT_DSR_STATUS)
    {
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
        vt_reply_num_wr(vt, LDG_TERM_VT_DSR_OK);
        vt_reply_byte_wr(vt, 'n');
        return;
    }

    if (kind != LDG_TERM_VT_DSR_CURSOR) { return; }

    row = vt->term->cursor_row;

    if (ldg_term_orig_is(vt->term) == LDG_TRUTH_TRUE) { row -= vt->term->margin_top; }

    vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
    vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
    vt_reply_num_wr(vt, row + LDG_TERM_VT_STEP);
    vt_reply_byte_wr(vt, ';');
    vt_reply_num_wr(vt, vt->term->cursor_col + LDG_TERM_VT_STEP);
    vt_reply_byte_wr(vt, 'R');
}

#define LDG_TERM_VT_FINAL_BASE 0x40
#define LDG_TERM_VT_FINAL_SPAN 64
#define LDG_TERM_VT_BIT_ONE 1ULL

#define LDG_TERM_VT_KBD_MODE_ALL 1
#define LDG_TERM_VT_KBD_MODE_SET 2
#define LDG_TERM_VT_KBD_MODE_CLEAR 3
#define LDG_TERM_VT_KBD_QUERY_CH 0x3F
#define LDG_TERM_VT_KBD_PUSH_CH 0x3E
#define LDG_TERM_VT_KBD_POP_CH 0x3C
#define LDG_TERM_VT_KBD_SET_CH 0x3D

static uint32_t vt_kbd_screen_get(const ldg_term_vt_t *vt)
{
    if (ldg_term_alt_active_is(vt->term) == LDG_TRUTH_TRUE) { return LDG_TERM_VT_STEP; }

    return LDG_TERM_VT_PARAM_FIRST;
}

static void vt_kbd_rst(ldg_term_vt_t *vt)
{
    uint32_t screen = 0;

    for (screen = 0; screen < LDG_TERM_VT_KBD_SCREEN_CUNT; screen++)
    {
        vt->kbd_flags[screen] = 0;
        vt->kbd_stack_len[screen] = 0;
    }
}

static void vt_kbd_push(ldg_term_vt_t *vt, uint32_t screen, uint32_t flags)
{
    uint32_t idx = 0;
    uint32_t len = 0;

    len = vt->kbd_stack_len[screen];

    if (len >= LDG_TERM_VT_KBD_STACK_MAX)
    {
        for (idx = 0; idx + LDG_TERM_VT_STEP < LDG_TERM_VT_KBD_STACK_MAX; idx++) { vt->kbd_stack[screen][idx] = vt->kbd_stack[screen][idx + LDG_TERM_VT_STEP]; }

        len = LDG_TERM_VT_KBD_STACK_MAX - LDG_TERM_VT_STEP;
    }

    vt->kbd_stack[screen][len] = vt->kbd_flags[screen];
    vt->kbd_stack_len[screen] = len + LDG_TERM_VT_STEP;
    vt->kbd_flags[screen] = flags;
}

static void vt_kbd_pop(ldg_term_vt_t *vt, uint32_t screen, uint32_t cunt)
{
    uint32_t step = 0;

    for (step = 0; step < cunt; step++)
    {
        if (vt->kbd_stack_len[screen] == 0)
        {
            vt->kbd_flags[screen] = 0;
            return;
        }

        vt->kbd_stack_len[screen]--;
        vt->kbd_flags[screen] = vt->kbd_stack[screen][vt->kbd_stack_len[screen]];
    }
}

static void vt_kbd_apply(ldg_term_vt_t *vt)
{
    uint32_t screen = 0;
    uint32_t flags = 0;
    uint32_t mode = 0;

    screen = vt_kbd_screen_get(vt);

    if (vt->priv_lead == LDG_TERM_VT_KBD_QUERY_CH)
    {
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
        vt_reply_byte_wr(vt, LDG_TERM_VT_KBD_QUERY_CH);
        vt_reply_num_wr(vt, vt->kbd_flags[screen]);
        vt_reply_byte_wr(vt, 'u');
        return;
    }

    if (vt->priv_lead == LDG_TERM_VT_KBD_PUSH_CH)
    {
        flags = vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST);
        vt_kbd_push(vt, screen, flags);
        return;
    }

    if (vt->priv_lead == LDG_TERM_VT_KBD_POP_CH)
    {
        vt_kbd_pop(vt, screen, vt_param_cunt_get(vt, LDG_TERM_VT_PARAM_FIRST));
        return;
    }

    flags = vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST);
    mode = vt_param_cunt_get(vt, LDG_TERM_VT_STEP);

    if (mode == LDG_TERM_VT_KBD_MODE_SET) { vt->kbd_flags[screen] |= flags; }
    else if (mode == LDG_TERM_VT_KBD_MODE_CLEAR) { vt->kbd_flags[screen] &= ~flags; }
    else { vt->kbd_flags[screen] = flags; }
}

static uint32_t vt_mode_state_get(const ldg_term_vt_t *vt, uint32_t code)
{
    if (code == LDG_TERM_VT_DEC_APP_CURSOR) { return (vt->mode & LDG_TERM_VT_MODE_APP_CURSOR) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_ORIG) { return (ldg_term_orig_is(vt->term) == LDG_TRUTH_TRUE) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_WRAP) { return (ldg_term_autowrap_is(vt->term) == LDG_TRUTH_TRUE) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_SCREEN) { return (ldg_term_reverse_is(vt->term) == LDG_TRUTH_TRUE) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_MODE_CURSOR) { return (ldg_term_cursor_visible_is(vt->term) == LDG_TRUTH_TRUE) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_FOCUS) { return (vt->mode & LDG_TERM_VT_MODE_FOCUS) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_PASTE) { return (vt->mode & LDG_TERM_VT_MODE_PASTE_BRACKET) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_MOUSE_NORMAL) { return (vt->mode & LDG_TERM_VT_MODE_MOUSE_NORMAL) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_MOUSE_BUTTON) { return (vt->mode & LDG_TERM_VT_MODE_MOUSE_BUTTON) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_MOUSE_ANY) { return (vt->mode & LDG_TERM_VT_MODE_MOUSE_ANY) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_DEC_MOUSE_SGR) { return (vt->mode & LDG_TERM_VT_MODE_MOUSE_SGR) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    if (code == LDG_TERM_VT_MODE_ALT_FULL || code == LDG_TERM_VT_MODE_ALT_BUFF || code == LDG_TERM_VT_MODE_ALT_OLD) { return (ldg_term_alt_active_is(vt->term) == LDG_TRUTH_TRUE) ? LDG_TERM_VT_RQM_SET : LDG_TERM_VT_RQM_RST; }

    return LDG_TERM_VT_RQM_UNKNOWN;
}

static void vt_rqm_reply(ldg_term_vt_t *vt)
{
    uint32_t code = 0;
    uint32_t state = 0;

    code = vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST);
    state = vt_mode_state_get(vt, code);

    vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
    vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);

    if (vt->priv == LDG_TRUTH_TRUE) { vt_reply_byte_wr(vt, LDG_TERM_VT_KBD_QUERY_CH); }

    vt_reply_num_wr(vt, code);
    vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
    vt_reply_num_wr(vt, state);
    vt_reply_byte_wr(vt, LDG_TERM_VT_DOLLAR_CH);
    vt_reply_byte_wr(vt, 'y');
}

static void vt_rep_apply(ldg_term_vt_t *vt, uint32_t cunt)
{
    uint32_t step = 0;

    if (vt->last_ch == LDG_TERM_VT_CH_NONE) { return; }

    if (cunt > vt->term->col_cunt) { cunt = vt->term->col_cunt; }

    for (step = 0; step < cunt; step++) { ldg_term_ch_put(vt->term, vt->last_ch); }
}

static void vt_unhandled_mark(ldg_term_vt_t *vt, uint8_t final)
{
    uint32_t slot = 0;

    if (final < LDG_TERM_VT_FINAL_BASE) { return; }

    slot = (uint32_t)final - LDG_TERM_VT_FINAL_BASE;

    if (slot >= LDG_TERM_VT_FINAL_SPAN) { vt->unhandled_hi |= LDG_TERM_VT_BIT_ONE << (slot - LDG_TERM_VT_FINAL_SPAN); }
    else { vt->unhandled_lo |= LDG_TERM_VT_BIT_ONE << slot; }
}

#define LDG_TERM_VT_WINOP_TITLE_PUSH 22
#define LDG_TERM_VT_WINOP_TITLE_POP 23
#define LDG_TERM_VT_WINOP_PIXEL_ASK 14
#define LDG_TERM_VT_WINOP_TEXT_ASK 18
#define LDG_TERM_VT_WINOP_PIXEL_REPLY 4
#define LDG_TERM_VT_WINOP_TEXT_REPLY 8
#define LDG_TERM_VT_CELL_WIDTH_PX 8
#define LDG_TERM_VT_CELL_HEIGHT_PX 16

static void vt_title_push(ldg_term_vt_t *vt)
{
    uint32_t idx = 0;

    if (vt->title_depth >= LDG_TERM_VT_TITLE_STACK_MAX) { return; }

    for (idx = 0; idx < vt->title_len && idx < LDG_TERM_VT_TITLE_MAX; idx++) { vt->title_stack[vt->title_depth][idx] = vt->title[idx]; }

    vt->title_stack[vt->title_depth][vt->title_len] = LDG_STR_TERM;
    vt->title_stack_len[vt->title_depth] = vt->title_len;
    vt->title_depth++;
}

static void vt_title_pop(ldg_term_vt_t *vt)
{
    uint32_t idx = 0;

    if (vt->title_depth == 0) { return; }

    vt->title_depth--;
    vt->title_len = vt->title_stack_len[vt->title_depth];

    for (idx = 0; idx < vt->title_len && idx < LDG_TERM_VT_TITLE_MAX; idx++) { vt->title[idx] = vt->title_stack[vt->title_depth][idx]; }

    vt->title[vt->title_len] = LDG_STR_TERM;
}

static void vt_winop_apply(ldg_term_vt_t *vt, uint32_t op)
{
    if (op == LDG_TERM_VT_WINOP_TITLE_PUSH)
    {
        vt_title_push(vt);
        return;
    }

    if (op == LDG_TERM_VT_WINOP_TITLE_POP)
    {
        vt_title_pop(vt);
        return;
    }

    if (op == LDG_TERM_VT_WINOP_TEXT_ASK)
    {
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
        vt_reply_num_wr(vt, LDG_TERM_VT_WINOP_TEXT_REPLY);
        vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
        vt_reply_num_wr(vt, vt->term->row_cunt);
        vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
        vt_reply_num_wr(vt, vt->term->col_cunt);
        vt_reply_byte_wr(vt, 't');
        return;
    }

    if (op == LDG_TERM_VT_WINOP_PIXEL_ASK)
    {
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, LDG_TERM_VT_CSI_LEAD);
        vt_reply_num_wr(vt, LDG_TERM_VT_WINOP_PIXEL_REPLY);
        vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
        vt_reply_num_wr(vt, vt->term->row_cunt * LDG_TERM_VT_CELL_HEIGHT_PX);
        vt_reply_byte_wr(vt, LDG_TERM_VT_OSC_SEP);
        vt_reply_num_wr(vt, vt->term->col_cunt * LDG_TERM_VT_CELL_WIDTH_PX);
        vt_reply_byte_wr(vt, 't');
        return;
    }
}

static void vt_csi_dispatch(ldg_term_vt_t *vt, uint8_t final)
{
    uint32_t first = 0;
    uint32_t second = 0;
    int32_t delta = 0;

    if (vt->priv_lead != 0 && final == 'u')
    {
        vt_kbd_apply(vt);
        return;
    }

    if (final == 'p' && vt->inter_cunt > 0 && vt->inter[LDG_TERM_VT_PARAM_FIRST] == LDG_TERM_VT_DOLLAR_CH)
    {
        vt_rqm_reply(vt);
        return;
    }

    if (vt->priv_lead != 0 && vt->priv_lead != LDG_TERM_VT_PRIV_HIGH)
    {
        vt_unhandled_mark(vt, final);
        return;
    }

    first = vt_param_cunt_get(vt, LDG_TERM_VT_PARAM_FIRST);

    if (final == 'A')
    {
        delta = -(int32_t)first;
        ldg_term_cursor_row_mov(vt->term, delta);
    }
    else if (final == 'B') { ldg_term_cursor_row_mov(vt->term, (int32_t)first); }
    else if (final == 'C') { ldg_term_cursor_col_mov(vt->term, (int32_t)first); }
    else if (final == 'D')
    {
        delta = -(int32_t)first;
        ldg_term_cursor_col_mov(vt->term, delta);
    }
    else if (final == 'G') { ldg_term_cursor_set(vt->term, first - LDG_TERM_VT_STEP, vt->term->cursor_row); }
    else if (final == 'd') { ldg_term_cursor_set(vt->term, vt->term->cursor_col, first - LDG_TERM_VT_STEP); }
    else if (final == 'H' || final == 'f')
    {
        second = vt_param_cunt_get(vt, LDG_TERM_VT_STEP);
        ldg_term_cursor_set(vt->term, second - LDG_TERM_VT_STEP, first - LDG_TERM_VT_STEP);
    }
    else if (final == 'J') { ldg_term_display_erase(vt->term, (ldg_term_erase_t)vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST)); }
    else if (final == 'K') { ldg_term_line_erase(vt->term, (ldg_term_erase_t)vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST)); }
    else if (final == 'L') { ldg_term_line_insert(vt->term, first); }
    else if (final == 'M') { ldg_term_line_delete(vt->term, first); }
    else if (final == 'P') { ldg_term_ch_delete(vt->term, first); }
    else if (final == 'S') { ldg_term_scroll_up(vt->term, first); }
    else if (final == 'T') { ldg_term_scroll_down(vt->term, first); }
    else if (final == 'h') { vt_mode_apply(vt, LDG_TRUTH_TRUE); }
    else if (final == 'l') { vt_mode_apply(vt, LDG_TRUTH_FALSE); }
    else if (final == 'm') { vt_sgr_apply(vt); }
    else if (final == 'I') { ldg_term_tab_advance_cunt(vt->term, first); }
    else if (final == 'Z') { ldg_term_tab_back(vt->term, first); }
    else if (final == 'g') { ldg_term_tab_clear(vt->term, (vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST) == LDG_TERM_VT_TBC_ALL) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }
    else if (final == '`') { ldg_term_cursor_set(vt->term, first - LDG_TERM_VT_STEP, vt->term->cursor_row); }
    else if (final == 'E')
    {
        ldg_term_cursor_row_mov(vt->term, (int32_t)first);
        ldg_term_carriage_ret(vt->term);
    }
    else if (final == 'F')
    {
        delta = -(int32_t)first;
        ldg_term_cursor_row_mov(vt->term, delta);
        ldg_term_carriage_ret(vt->term);
    }
    else if (final == 'p' && vt->inter_cunt > 0 && vt->inter[LDG_TERM_VT_PARAM_FIRST] == LDG_TERM_VT_BANG_CH) { ldg_term_soft_rst(vt->term); }
    else if (final == 's') { if (vt->param_cunt > LDG_TERM_VT_STEP)
        {
            second = vt_param_get(vt, LDG_TERM_VT_STEP, vt->term->col_cunt);
            ldg_term_margin_side_set(vt->term, first - LDG_TERM_VT_STEP, second - LDG_TERM_VT_STEP);
        }
        else { ldg_term_cursor_save(vt->term); } }
    else if (final == 'u') { ldg_term_cursor_restore(vt->term); }
    else if (final == 'r') { if (vt->param_cunt == 0) { ldg_term_margin_rst(vt->term); }
        else
        {
            second = vt_param_get(vt, LDG_TERM_VT_STEP, vt->term->row_cunt);
            ldg_term_margin_set(vt->term, first - LDG_TERM_VT_STEP, second - LDG_TERM_VT_STEP);
        } }
    else if (final == '@') { ldg_term_ch_insert(vt->term, first); }
    else if (final == 'X') { ldg_term_ch_erase(vt->term, first); }
    else if (final == 'b') { vt_rep_apply(vt, first); }
    else if (final == 't') { vt_winop_apply(vt, first); }
    else if (final == 'c') { vt_reply_da_wr(vt); }
    else if (final == 'x') { vt_reply_parm_wr(vt); }
    else if (final == 'n') { vt_reply_dsr_wr(vt, first); }
    else if (final == 'q' && vt->inter_cunt > 0 && vt->inter[LDG_TERM_VT_PARAM_FIRST] == LDG_TERM_VT_SPACE_INTER) { vt->cursor_shape = (uint8_t)vt_param_get(vt, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_SGR_RST); }
    else { vt_unhandled_mark(vt, final); }
}

static void vt_esc_dispatch(ldg_term_vt_t *vt, uint8_t byte)
{
    int32_t step = LDG_TERM_VT_STEP;

    if (vt->inter_cunt > 0)
    {
        if (vt->inter[LDG_TERM_VT_PARAM_FIRST] != LDG_TERM_VT_HASH_CH) { return; }

        if (byte == '8') { ldg_term_align_fill(vt->term); }
        else if (byte == '3') { ldg_term_line_attr_set(vt->term, LDG_TERM_LINE_DOUBLE_TOP); }
        else if (byte == '4') { ldg_term_line_attr_set(vt->term, LDG_TERM_LINE_DOUBLE_BOTTOM); }
        else if (byte == '5') { ldg_term_line_attr_set(vt->term, LDG_TERM_LINE_SINGLE); }
        else if (byte == '6') { ldg_term_line_attr_set(vt->term, LDG_TERM_LINE_DOUBLE_WIDTH); }

        return;
    }

    if (byte == '7') { ldg_term_cursor_save(vt->term); }
    else if (byte == '8') { ldg_term_cursor_restore(vt->term); }
    else if (byte == 'c') { ldg_term_rst(vt->term); }
    else if (byte == 'M') { if (vt->term->cursor_row == vt->term->margin_top) { ldg_term_scroll_down(vt->term, LDG_TERM_VT_STEP); }
        else { ldg_term_cursor_row_mov(vt->term, -step); } }
    else if (byte == 'E')
    {
        ldg_term_carriage_ret(vt->term);
        ldg_term_line_feed(vt->term);
    }
    else if (byte == 'D') { ldg_term_line_feed(vt->term); }
    else if (byte == 'H') { ldg_term_tab_set(vt->term); }
    else if (byte == '=') { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_APP_KEYPAD, LDG_TRUTH_TRUE); }
    else if (byte == '>') { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_APP_KEYPAD, LDG_TRUTH_FALSE); }
}

static void vt_ctrl_exec(ldg_term_vt_t *vt, uint8_t byte)
{
    if (byte == LDG_TERM_VT_BS_CH) { ldg_term_backspace(vt->term); }
    else if (byte == LDG_TERM_VT_HT_CH) { ldg_term_tab_advance(vt->term); }
    else if (byte == LDG_TERM_VT_LF_CH || byte == LDG_TERM_VT_VTAB_CH || byte == LDG_TERM_VT_FF_CH) { ldg_term_line_feed(vt->term); }
    else if (byte == LDG_TERM_VT_CR_CH) { ldg_term_carriage_ret(vt->term); }
    else if (byte == LDG_TERM_VT_SO_CH) { ldg_term_charset_shift(vt->term, LDG_TRUTH_TRUE); }
    else if (byte == LDG_TERM_VT_SI_CH) { ldg_term_charset_shift(vt->term, LDG_TRUTH_FALSE); }
}

static void vt_ground_feed(ldg_term_vt_t *vt, uint8_t byte)
{
    if (byte <= LDG_TERM_VT_C0_HIGH)
    {
        vt_ctrl_exec(vt, byte);
        return;
    }

    if (byte == LDG_TERM_VT_DEL_CH) { return; }

    ldg_term_ch_put(vt->term, byte);
    vt->last_ch = byte;
}

static void vt_title_set(ldg_term_vt_t *vt, uint64_t start)
{
    uint64_t span = 0;
    uint64_t idx = 0;

    span = vt->osc_len - start;

    if (span >= LDG_TERM_VT_TITLE_MAX) { span = LDG_TERM_VT_TITLE_MAX - LDG_TERM_VT_STEP; }

    for (idx = 0; idx < span; idx++) { vt->title[idx] = vt->osc[start + idx]; }

    vt->title[span] = LDG_STR_TERM;
    vt->title_len = (uint32_t)span;
}

#define LDG_TERM_VT_OSC_TITLE_BOTH_CODE 0
#define LDG_TERM_VT_OSC_TITLE_WIN_CODE 2
#define LDG_TERM_VT_OSC_TITLE_ICON 1
#define LDG_TERM_VT_OSC_PALETTE 4
#define LDG_TERM_VT_OSC_CWD 7
#define LDG_TERM_VT_OSC_LINK 8
#define LDG_TERM_VT_OSC_FG 10
#define LDG_TERM_VT_OSC_BG 11
#define LDG_TERM_VT_OSC_CURSOR 12
#define LDG_TERM_VT_OSC_CLIP 52
#define LDG_TERM_VT_OSC_PALETTE_RST 104
#define LDG_TERM_VT_OSC_MARK 133
#define LDG_TERM_VT_HEX_LEAD '#'
#define LDG_TERM_VT_COLOR_SLASH '/'
#define LDG_TERM_VT_COLOR_COLON ':'
#define LDG_TERM_VT_HEX_PAIR 2
#define LDG_TERM_VT_HEX_RADIX 16
#define LDG_TERM_VT_NIBBLE_NONE 0

static uint32_t vt_osc_code_get(const ldg_term_vt_t *vt, uint64_t *out_body)
{
    uint64_t idx = 0;
    uint32_t code = 0;

    while (idx < vt->osc_len && vt->osc[idx] >= LDG_TERM_VT_DIGIT_LOW && vt->osc[idx] <= LDG_TERM_VT_DIGIT_HIGH)
    {
        code = (code * LDG_TERM_VT_DECIMAL) + (uint32_t)((uint8_t)vt->osc[idx] - LDG_TERM_VT_DIGIT_LOW);
        idx++;
    }

    if (idx < vt->osc_len && vt->osc[idx] == LDG_TERM_VT_OSC_SEP) { idx++; }

    *out_body = idx;

    return code;
}

static uint8_t vt_hex_nibble_get(char ch)
{
    if (ch >= LDG_TERM_VT_DIGIT_LOW && ch <= LDG_TERM_VT_DIGIT_HIGH) { return (uint8_t)(ch - LDG_TERM_VT_DIGIT_LOW); }

    if (ch >= 'a' && ch <= 'f') { return (uint8_t)(ch - 'a' + LDG_TERM_VT_DECIMAL); }

    if (ch >= 'A' && ch <= 'F') { return (uint8_t)(ch - 'A' + LDG_TERM_VT_DECIMAL); }

    return LDG_TERM_VT_NIBBLE_NONE;
}

static uint32_t vt_color_parse(const ldg_term_vt_t *vt, uint64_t start, uint64_t stop, uint8_t *out)
{
    uint64_t idx = 0;
    uint32_t comp = 0;
    uint32_t digit = 0;

    if (start >= stop) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (vt->osc[start] == LDG_TERM_VT_HEX_LEAD)
    {
        idx = start + LDG_TERM_VT_STEP;

        for (comp = 0; comp < LDG_TERM_RGB_CUNT; comp++)
        {
            if (idx + LDG_TERM_VT_HEX_PAIR > stop) { return LDG_ERR_FUNC_ARG_INVALID; }

            out[comp] = (uint8_t)((vt_hex_nibble_get(vt->osc[idx]) * LDG_TERM_VT_HEX_RADIX) + vt_hex_nibble_get(vt->osc[idx + LDG_TERM_VT_STEP]));
            idx += LDG_TERM_VT_HEX_PAIR;
        }

        return LDG_ERR_AOK;
    }

    idx = start;

    while (idx < stop && vt->osc[idx] != LDG_TERM_VT_COLOR_COLON) { idx++; }

    if (idx >= stop) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx++;

    for (comp = 0; comp < LDG_TERM_RGB_CUNT; comp++)
    {
        digit = 0;

        while (idx < stop && vt->osc[idx] != LDG_TERM_VT_COLOR_SLASH)
        {
            digit = (digit * LDG_TERM_VT_HEX_RADIX) + vt_hex_nibble_get(vt->osc[idx]);
            idx++;
        }

        out[comp] = (uint8_t)digit;

        if (idx < stop) { idx++; }
    }

    return LDG_ERR_AOK;
}

static void vt_osc_palette_apply(ldg_term_vt_t *vt, uint64_t body)
{
    uint64_t idx = 0;
    uint64_t stop = 0;
    uint32_t slot = 0;
    uint8_t rgb[LDG_TERM_RGB_CUNT] = LDG_ARR_ZERO_INIT;

    idx = body;

    while (idx < vt->osc_len)
    {
        slot = 0;

        while (idx < vt->osc_len && vt->osc[idx] >= LDG_TERM_VT_DIGIT_LOW && vt->osc[idx] <= LDG_TERM_VT_DIGIT_HIGH)
        {
            slot = (slot * LDG_TERM_VT_DECIMAL) + (uint32_t)((uint8_t)vt->osc[idx] - LDG_TERM_VT_DIGIT_LOW);
            idx++;
        }

        if (idx >= vt->osc_len || vt->osc[idx] != LDG_TERM_VT_OSC_SEP) { return; }

        idx++;
        stop = idx;

        while (stop < vt->osc_len && vt->osc[stop] != LDG_TERM_VT_OSC_SEP) { stop++; }

        if (vt_color_parse(vt, idx, stop, rgb) == LDG_ERR_AOK) { ldg_term_palette_set(vt->term, slot, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]); }

        idx = stop;

        if (idx < vt->osc_len) { idx++; }
    }
}

static void vt_osc_link_apply(ldg_term_vt_t *vt, uint64_t body)
{
    uint64_t idx = 0;

    idx = body;

    while (idx < vt->osc_len && vt->osc[idx] != LDG_TERM_VT_OSC_SEP) { idx++; }

    if (idx >= vt->osc_len) { return; }

    idx++;

    if (idx >= vt->osc_len)
    {
        ldg_term_link_close(vt->term);
        return;
    }

    ldg_term_link_open(vt->term, &vt->osc[idx], vt->osc_len - idx);
}

static void vt_osc_mark_apply(ldg_term_vt_t *vt, uint64_t body)
{
    if (body >= vt->osc_len) { return; }

    if (vt->osc[body] == 'A') { ldg_term_mark_set(vt->term, LDG_TERM_MARK_PROMPT); }
    else if (vt->osc[body] == 'B') { ldg_term_mark_set(vt->term, LDG_TERM_MARK_INPUT); }
    else if (vt->osc[body] == 'C') { ldg_term_mark_set(vt->term, LDG_TERM_MARK_OUTPUT); }
}

static void vt_osc_clip_apply(ldg_term_vt_t *vt, uint64_t body)
{
    uint64_t idx = 0;
    uint64_t got = 0;

    idx = body;

    while (idx < vt->osc_len && vt->osc[idx] != LDG_TERM_VT_OSC_SEP) { idx++; }

    if (idx >= vt->osc_len) { return; }

    idx++;

    if (ldg_str_base64_decode(&vt->osc[idx], vt->osc_len - idx, vt->clip, LDG_TERM_VT_CLIP_MAX, &got) != LDG_ERR_AOK) { return; }

    vt->clip_len = (uint32_t)got;
}

static void vt_osc_role_apply(ldg_term_vt_t *vt, uint64_t body, uint32_t role)
{
    uint8_t rgb[LDG_TERM_RGB_CUNT] = LDG_ARR_ZERO_INIT;

    if (vt_color_parse(vt, body, vt->osc_len, rgb) != LDG_ERR_AOK) { return; }

    ldg_term_role_color_set(vt->term, role, rgb[LDG_TERM_VT_RED_IDX], rgb[LDG_TERM_VT_GREEN_IDX], rgb[LDG_TERM_VT_BLUE_IDX]);
}

static void vt_osc_dispatch(ldg_term_vt_t *vt)
{
    uint64_t body = 0;
    uint32_t code = 0;

    if (vt->osc_len == 0) { return; }

    code = vt_osc_code_get(vt, &body);

    if (code == LDG_TERM_VT_OSC_TITLE_WIN_CODE || code == LDG_TERM_VT_OSC_TITLE_BOTH_CODE || code == LDG_TERM_VT_OSC_TITLE_ICON) { vt_title_set(vt, body); }
    else if (code == LDG_TERM_VT_OSC_CLIP) { vt_osc_clip_apply(vt, body); }
    else if (code == LDG_TERM_VT_OSC_PALETTE) { vt_osc_palette_apply(vt, body); }
    else if (code == LDG_TERM_VT_OSC_PALETTE_RST) { ldg_term_palette_rst(vt->term); }
    else if (code == LDG_TERM_VT_OSC_FG) { vt_osc_role_apply(vt, body, LDG_TERM_COLOR_FG); }
    else if (code == LDG_TERM_VT_OSC_BG) { vt_osc_role_apply(vt, body, LDG_TERM_COLOR_BG); }
    else if (code == LDG_TERM_VT_OSC_CURSOR) { vt_osc_role_apply(vt, body, LDG_TERM_COLOR_CURSOR); }
    else if (code == LDG_TERM_VT_OSC_CWD) { ldg_term_cwd_set(vt->term, &vt->osc[body], vt->osc_len - body); }
    else if (code == LDG_TERM_VT_OSC_LINK) { vt_osc_link_apply(vt, body); }
    else if (code == LDG_TERM_VT_OSC_MARK) { vt_osc_mark_apply(vt, body); }
}

static void vt_str_collect(ldg_term_vt_t *vt, uint8_t byte)
{
    if (vt->osc_len + LDG_TERM_VT_STEP < LDG_TERM_VT_OSC_MAX)
    {
        vt->osc[vt->osc_len] = (char)byte;
        vt->osc_len++;
        vt->osc[vt->osc_len] = LDG_STR_TERM;
    }
}

static void vt_param_digit_add(ldg_term_vt_t *vt, uint8_t byte)
{
    uint32_t slot = 0;

    if (vt->param_cunt == 0) { vt->param_cunt = LDG_TERM_VT_STEP; }

    slot = vt->param_cunt - LDG_TERM_VT_STEP;

    if (slot < LDG_TERM_VT_PARAM_MAX)
    {
        vt->param[slot] = (vt->param[slot] * LDG_TERM_VT_DECIMAL) + (uint32_t)(byte - LDG_TERM_VT_DIGIT_LOW);
        vt->param_seen[slot] = LDG_TRUTH_TRUE;
    }
}

static void vt_param_sep_add(ldg_term_vt_t *vt, uint8_t sub)
{
    if (vt->param_cunt == 0) { vt->param_cunt = LDG_TERM_VT_STEP; }

    if (vt->param_cunt < LDG_TERM_VT_PARAM_MAX)
    {
        vt->param_sub[vt->param_cunt] = sub;
        vt->param_cunt++;
    }
}

static void vt_inter_add(ldg_term_vt_t *vt, uint8_t byte)
{
    if (vt->inter_cunt < LDG_TERM_VT_INTER_MAX)
    {
        vt->inter[vt->inter_cunt] = byte;
        vt->inter_cunt++;
    }
}

#define LDG_TERM_VT_VT52_ADDR_BIAS 0x20

static void vt_vt52_feed(ldg_term_vt_t *vt, uint8_t byte)
{
    int32_t step = LDG_TERM_VT_STEP;

    if (byte == 'A') { ldg_term_cursor_row_mov(vt->term, -step); }
    else if (byte == 'B') { ldg_term_cursor_row_mov(vt->term, step); }
    else if (byte == 'C') { ldg_term_cursor_col_mov(vt->term, step); }
    else if (byte == 'D') { ldg_term_cursor_col_mov(vt->term, -step); }
    else if (byte == 'F') { ldg_term_charset_set(vt->term, LDG_TRUTH_FALSE, LDG_TERM_CHARSET_GRAPHIC); }
    else if (byte == 'G') { ldg_term_charset_set(vt->term, LDG_TRUTH_FALSE, LDG_TERM_CHARSET_ASCII); }
    else if (byte == 'H') { ldg_term_cursor_set(vt->term, LDG_TERM_VT_PARAM_FIRST, LDG_TERM_VT_PARAM_FIRST); }
    else if (byte == 'I') { if (vt->term->cursor_row == vt->term->margin_top) { ldg_term_scroll_down(vt->term, LDG_TERM_VT_STEP); }
        else { ldg_term_cursor_row_mov(vt->term, -step); } }
    else if (byte == 'J') { ldg_term_display_erase(vt->term, LDG_TERM_ERASE_TO_END); }
    else if (byte == 'K') { ldg_term_line_erase(vt->term, LDG_TERM_ERASE_TO_END); }
    else if (byte == 'Y')
    {
        vt->state = LDG_TERM_VT_STATE_VT52_ROW;
        return;
    }
    else if (byte == 'Z')
    {
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, '/');
        vt_reply_byte_wr(vt, 'Z');
    }
    else if (byte == '=') { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_APP_KEYPAD, LDG_TRUTH_TRUE); }
    else if (byte == '>') { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_APP_KEYPAD, LDG_TRUTH_FALSE); }
    else if (byte == '<') { vt_mode_bit_set(vt, LDG_TERM_VT_MODE_ANSI, LDG_TRUTH_TRUE); }

    vt->state = LDG_TERM_VT_STATE_GROUND;
}

static void vt_esc_feed(ldg_term_vt_t *vt, uint8_t byte)
{
    if (!(vt->mode & LDG_TERM_VT_MODE_ANSI))
    {
        vt_vt52_feed(vt, byte);
        return;
    }

    if (byte == LDG_TERM_VT_CSI_LEAD_CH)
    {
        vt->state = LDG_TERM_VT_STATE_CSI_ENTRY;
        return;
    }

    if (byte == LDG_TERM_VT_OSC_LEAD_CH)
    {
        vt->state = LDG_TERM_VT_STATE_OSC;
        return;
    }

    if (byte == LDG_TERM_VT_DCS_LEAD_CH)
    {
        vt->str_kind = LDG_TERM_VT_STR_DCS;
        vt->state = LDG_TERM_VT_STATE_DCS_ENTRY;
        return;
    }

    if (byte == LDG_TERM_VT_APC_LEAD_CH)
    {
        vt->str_kind = LDG_TERM_VT_STR_APC;
        vt->state = LDG_TERM_VT_STATE_STR;
        return;
    }

    if (byte == LDG_TERM_VT_PM_LEAD_CH)
    {
        vt->str_kind = LDG_TERM_VT_STR_PM;
        vt->state = LDG_TERM_VT_STATE_STR;
        return;
    }

    if (byte == LDG_TERM_VT_SOS_LEAD_CH)
    {
        vt->str_kind = LDG_TERM_VT_STR_SOS;
        vt->state = LDG_TERM_VT_STATE_STR;
        return;
    }

    if (byte == LDG_TERM_VT_CHARSET_G0 || byte == LDG_TERM_VT_CHARSET_G1)
    {
        vt->charset_slot = (byte == LDG_TERM_VT_CHARSET_G1) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        vt->state = LDG_TERM_VT_STATE_CHARSET;
        return;
    }

    if (byte >= LDG_TERM_VT_INTER_LOW && byte <= LDG_TERM_VT_INTER_HIGH)
    {
        vt_inter_add(vt, byte);
        vt->state = LDG_TERM_VT_STATE_ESC_INTER;
        return;
    }

    if (byte >= LDG_TERM_VT_ESC_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH)
    {
        vt_esc_dispatch(vt, byte);
        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    vt->state = LDG_TERM_VT_STATE_GROUND;
}

static void vt_csi_feed(ldg_term_vt_t *vt, uint8_t byte)
{
    if (byte >= LDG_TERM_VT_DIGIT_LOW && byte <= LDG_TERM_VT_DIGIT_HIGH)
    {
        vt_param_digit_add(vt, byte);
        vt->state = LDG_TERM_VT_STATE_CSI_PARAM;
        return;
    }

    if (byte == LDG_TERM_VT_SEMI_CH)
    {
        vt_param_sep_add(vt, LDG_TRUTH_FALSE);
        vt->state = LDG_TERM_VT_STATE_CSI_PARAM;
        return;
    }

    if (byte == LDG_TERM_VT_COLON_CH)
    {
        vt_param_sep_add(vt, LDG_TRUTH_TRUE);
        vt->state = LDG_TERM_VT_STATE_CSI_PARAM;
        return;
    }

    if (byte >= LDG_TERM_VT_PRIV_LOW && byte <= LDG_TERM_VT_PRIV_HIGH)
    {
        if (vt->state != LDG_TERM_VT_STATE_CSI_ENTRY)
        {
            vt->state = LDG_TERM_VT_STATE_CSI_IGNORE;
            return;
        }

        vt->priv_lead = byte;

        if (byte == LDG_TERM_VT_PRIV_HIGH) { vt->priv = LDG_TRUTH_TRUE; }

        vt->state = LDG_TERM_VT_STATE_CSI_PARAM;
        return;
    }

    if (byte >= LDG_TERM_VT_INTER_LOW && byte <= LDG_TERM_VT_INTER_HIGH)
    {
        vt_inter_add(vt, byte);
        vt->state = LDG_TERM_VT_STATE_CSI_INTER;
        return;
    }

    if (byte >= LDG_TERM_VT_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH)
    {
        vt_csi_dispatch(vt, byte);
        vt_param_rst(vt);
        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    vt->state = LDG_TERM_VT_STATE_CSI_IGNORE;
}

static void vt_dcs_feed(ldg_term_vt_t *vt, uint8_t byte)
{
    if (byte >= LDG_TERM_VT_DIGIT_LOW && byte <= LDG_TERM_VT_DIGIT_HIGH)
    {
        vt_param_digit_add(vt, byte);
        vt->state = LDG_TERM_VT_STATE_DCS_PARAM;
        return;
    }

    if (byte == LDG_TERM_VT_SEMI_CH)
    {
        vt_param_sep_add(vt, LDG_TRUTH_FALSE);
        vt->state = LDG_TERM_VT_STATE_DCS_PARAM;
        return;
    }

    if (byte == LDG_TERM_VT_COLON_CH)
    {
        vt_param_sep_add(vt, LDG_TRUTH_TRUE);
        vt->state = LDG_TERM_VT_STATE_DCS_PARAM;
        return;
    }

    if (byte >= LDG_TERM_VT_PRIV_LOW && byte <= LDG_TERM_VT_PRIV_HIGH)
    {
        if (vt->state != LDG_TERM_VT_STATE_DCS_ENTRY)
        {
            vt->state = LDG_TERM_VT_STATE_DCS_IGNORE;
            return;
        }

        vt->priv_lead = byte;
        vt->state = LDG_TERM_VT_STATE_DCS_PARAM;
        return;
    }

    if (byte >= LDG_TERM_VT_INTER_LOW && byte <= LDG_TERM_VT_INTER_HIGH)
    {
        vt_inter_add(vt, byte);
        vt->state = LDG_TERM_VT_STATE_DCS_INTER;
        return;
    }

    if (byte >= LDG_TERM_VT_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH)
    {
        vt_inter_add(vt, byte);
        vt->osc_len = 0;
        vt->state = LDG_TERM_VT_STATE_DCS_PASS;
        return;
    }

    vt->state = LDG_TERM_VT_STATE_DCS_IGNORE;
}

#define LDG_TERM_VT_GFX_LEAD 'G'
#define LDG_TERM_VT_GFX_SEP ','
#define LDG_TERM_VT_GFX_BODY ';'
#define LDG_TERM_VT_GFX_EQ '='
#define LDG_TERM_VT_GFX_ACT_XFER 't'
#define LDG_TERM_VT_GFX_ACT_SHOW 'T'
#define LDG_TERM_VT_GFX_ACT_PUT 'p'
#define LDG_TERM_VT_GFX_ACT_DROP 'd'
#define LDG_TERM_VT_GFX_ACT_QUERY 'q'
#define LDG_TERM_VT_GFX_KEY_ACT 'a'
#define LDG_TERM_VT_GFX_KEY_FMT 'f'
#define LDG_TERM_VT_GFX_KEY_WIDTH 's'
#define LDG_TERM_VT_GFX_KEY_HEIGHT 'v'
#define LDG_TERM_VT_GFX_KEY_ID 'i'
#define LDG_TERM_VT_GFX_KEY_PLACE 'p'
#define LDG_TERM_VT_GFX_KEY_COLS 'c'
#define LDG_TERM_VT_GFX_KEY_ROWS 'r'
#define LDG_TERM_VT_GFX_KEY_LAYER 'z'
#define LDG_TERM_VT_GFX_KEY_MORE 'm'

typedef struct vt_gfx_cmd
{
    uint32_t fmt;
    uint32_t width;
    uint32_t height;
    uint32_t id;
    uint32_t placement_id;
    uint32_t cols;
    uint32_t rows;
    uint32_t layer;
    uint32_t more;
    uint64_t body;
    uint8_t action;
    uint8_t pudding[7];
} vt_gfx_cmd_t;

static void vt_gfx_key_apply(vt_gfx_cmd_t *cmd, uint8_t key, uint32_t val, uint8_t letter)
{
    if (key == LDG_TERM_VT_GFX_KEY_ACT) { cmd->action = letter; }
    else if (key == LDG_TERM_VT_GFX_KEY_FMT) { cmd->fmt = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_WIDTH) { cmd->width = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_HEIGHT) { cmd->height = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_ID) { cmd->id = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_PLACE) { cmd->placement_id = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_COLS) { cmd->cols = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_ROWS) { cmd->rows = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_LAYER) { cmd->layer = val; }
    else if (key == LDG_TERM_VT_GFX_KEY_MORE) { cmd->more = val; }
}

static void vt_gfx_parse(const ldg_term_vt_t *vt, vt_gfx_cmd_t *cmd)
{
    uint64_t idx = 0;
    uint32_t val = 0;
    uint8_t key = 0;
    uint8_t letter = 0;
    uint8_t seen = LDG_TRUTH_FALSE;

    cmd->body = vt->osc_len;
    idx = LDG_TERM_VT_STEP;

    while (idx < vt->osc_len)
    {
        if (vt->osc[idx] == LDG_TERM_VT_GFX_BODY)
        {
            cmd->body = idx + LDG_TERM_VT_STEP;
            break;
        }

        key = (uint8_t)vt->osc[idx];
        idx++;

        if (idx >= vt->osc_len || vt->osc[idx] != LDG_TERM_VT_GFX_EQ) { break; }

        idx++;
        val = 0;
        letter = 0;
        seen = LDG_TRUTH_FALSE;

        while (idx < vt->osc_len && vt->osc[idx] != LDG_TERM_VT_GFX_SEP && vt->osc[idx] != LDG_TERM_VT_GFX_BODY)
        {
            if (vt->osc[idx] >= LDG_TERM_VT_DIGIT_LOW && vt->osc[idx] <= LDG_TERM_VT_DIGIT_HIGH)
            {
                val = (val * LDG_TERM_VT_DECIMAL) + (uint32_t)((uint8_t)vt->osc[idx] - LDG_TERM_VT_DIGIT_LOW);
                seen = LDG_TRUTH_TRUE;
            }
            else { letter = (uint8_t)vt->osc[idx]; }

            idx++;
        }

        if (seen != LDG_TRUTH_TRUE && letter == 0) { val = 0; }

        vt_gfx_key_apply(cmd, key, val, letter);

        if (idx < vt->osc_len && vt->osc[idx] == LDG_TERM_VT_GFX_SEP) { idx++; }
    }
}

#define LDG_TERM_VT_PNG_SIG_LEN 8
#define LDG_TERM_VT_PNG_CHUNK_HDR 8
#define LDG_TERM_VT_PNG_CRC_LEN 4
#define LDG_TERM_VT_PNG_IHDR_LEN 13
#define LDG_TERM_VT_PNG_DEPTH_BYTE 8
#define LDG_TERM_VT_PNG_COLOR_GRAY 0
#define LDG_TERM_VT_PNG_COLOR_RGB 2
#define LDG_TERM_VT_PNG_COLOR_GRAY_ALPHA 4
#define LDG_TERM_VT_PNG_COLOR_RGBA 6
#define LDG_TERM_VT_PNG_CHAN_GRAY 1
#define LDG_TERM_VT_PNG_CHAN_GRAY_ALPHA 2
#define LDG_TERM_VT_PNG_CHAN_RGB 3
#define LDG_TERM_VT_PNG_CHAN_RGBA 4
#define LDG_TERM_VT_PNG_EDGE 0
#define LDG_TERM_VT_PNG_DIM_MAX 8192
#define LDG_TERM_VT_ZLIB_HDR_LEN 2
#define LDG_TERM_VT_BYTE_BITS 8
#define LDG_TERM_VT_FILTER_SUB 1
#define LDG_TERM_VT_FILTER_UP 2
#define LDG_TERM_VT_FILTER_AVG 3
#define LDG_TERM_VT_FILTER_PAETH 4

static uint32_t vt_png_dword_get(const uint8_t *src)
{
    uint32_t val = 0;

    val = (uint32_t)src[0];
    val = (val << LDG_TERM_VT_BYTE_BITS) | (uint32_t)src[1];
    val = (val << LDG_TERM_VT_BYTE_BITS) | (uint32_t)src[2];
    val = (val << LDG_TERM_VT_BYTE_BITS) | (uint32_t)src[3];

    return val;
}

static uint8_t vt_png_paeth_get(uint8_t lhs, uint8_t above, uint8_t corner)
{
    int32_t est = 0;
    int32_t da = 0;
    int32_t db = 0;
    int32_t dc = 0;

    est = (int32_t)lhs + (int32_t)above - (int32_t)corner;
    da = est - (int32_t)lhs;
    db = est - (int32_t)above;
    dc = est - (int32_t)corner;

    if (da < 0) { da = -da; }

    if (db < 0) { db = -db; }

    if (dc < 0) { dc = -dc; }

    if (da <= db && da <= dc) { return lhs; }

    if (db <= dc) { return above; }

    return corner;
}

static void vt_png_unfilter(uint8_t *raw, uint32_t width, uint32_t height, uint32_t chan)
{
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t stride = 0;
    uint32_t src = 0;
    uint32_t dst = 0;
    uint8_t filter = 0;
    uint8_t lhs = 0;
    uint8_t above = 0;
    uint8_t corner = 0;

    stride = width * chan;

    for (row = 0; row < height; row++)
    {
        src = row * (stride + LDG_TERM_VT_STEP);
        filter = raw[src];
        src++;
        dst = row * stride;

        for (idx = 0; idx < stride; idx++)
        {
            lhs = LDG_TERM_VT_PNG_EDGE;
            above = LDG_TERM_VT_PNG_EDGE;
            corner = LDG_TERM_VT_PNG_EDGE;

            if (idx >= chan) { lhs = raw[dst + idx - chan]; }

            if (row > 0) { above = raw[dst - stride + idx]; }

            if (row > 0 && idx >= chan) { corner = raw[dst - stride + idx - chan]; }

            if (filter == LDG_TERM_VT_FILTER_SUB) { raw[dst + idx] = (uint8_t)(raw[src + idx] + lhs); }
            else if (filter == LDG_TERM_VT_FILTER_UP) { raw[dst + idx] = (uint8_t)(raw[src + idx] + above); }
            else if (filter == LDG_TERM_VT_FILTER_AVG) { raw[dst + idx] = (uint8_t)(raw[src + idx] + (uint8_t)(((uint32_t)lhs + (uint32_t)above) / LDG_TERM_VT_PAIR_STEP)); }
            else if (filter == LDG_TERM_VT_FILTER_PAETH) { raw[dst + idx] = (uint8_t)(raw[src + idx] + vt_png_paeth_get(lhs, above, corner)); }
            else { raw[dst + idx] = raw[src + idx]; }
        }
    }
}

static uint32_t vt_png_chan_get(uint8_t color, uint32_t *out)
{
    if (color == LDG_TERM_VT_PNG_COLOR_RGB) { *out = LDG_TERM_VT_PNG_CHAN_RGB; return LDG_ERR_AOK; }

    if (color == LDG_TERM_VT_PNG_COLOR_RGBA) { *out = LDG_TERM_VT_PNG_CHAN_RGBA; return LDG_ERR_AOK; }

    if (color == LDG_TERM_VT_PNG_COLOR_GRAY) { *out = LDG_TERM_VT_PNG_CHAN_GRAY; return LDG_ERR_AOK; }

    if (color == LDG_TERM_VT_PNG_COLOR_GRAY_ALPHA) { *out = LDG_TERM_VT_PNG_CHAN_GRAY_ALPHA; return LDG_ERR_AOK; }

    return LDG_ERR_UNSUPPORTED;
}

static uint32_t vt_png_decode(ldg_term_vt_t *vt, const uint8_t *src, uint64_t len, uint32_t *out_width, uint32_t *out_height, uint8_t *out_fmt, uint32_t *out_size)
{
    uint64_t pos = 0;
    uint64_t idat = 0;
    uint64_t got = 0;
    uint64_t pixel = 0;
    uint64_t raw_span = 0;
    uint64_t rgb_span = 0;
    uint8_t gray = 0;
    uint32_t chunk_len = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t chan = 0;
    uint32_t raw_need = 0;
    uint8_t depth = 0;
    uint8_t color = 0;

    if (len < LDG_TERM_VT_PNG_SIG_LEN) { return LDG_ERR_FUNC_ARG_INVALID; }

    pos = LDG_TERM_VT_PNG_SIG_LEN;

    while (pos + LDG_TERM_VT_PNG_CHUNK_HDR <= len)
    {
        chunk_len = vt_png_dword_get(&src[pos]);

        if (pos + (uint64_t)LDG_TERM_VT_PNG_CHUNK_HDR + (uint64_t)chunk_len > len) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (src[pos + 4] == 'I' && src[pos + 5] == 'H' && src[pos + 6] == 'D' && src[pos + 7] == 'R')
        {
            if (chunk_len < LDG_TERM_VT_PNG_IHDR_LEN) { return LDG_ERR_FUNC_ARG_INVALID; }

            width = vt_png_dword_get(&src[pos + LDG_TERM_VT_PNG_CHUNK_HDR]);
            height = vt_png_dword_get(&src[pos + LDG_TERM_VT_PNG_CHUNK_HDR + 4]);
            depth = src[pos + LDG_TERM_VT_PNG_CHUNK_HDR + 8];
            color = src[pos + LDG_TERM_VT_PNG_CHUNK_HDR + 9];

            if (depth != LDG_TERM_VT_PNG_DEPTH_BYTE) { return LDG_ERR_UNSUPPORTED; }

            if (src[pos + LDG_TERM_VT_PNG_CHUNK_HDR + 12] != LDG_TERM_VT_PNG_EDGE) { return LDG_ERR_UNSUPPORTED; }
        }
        else if (src[pos + 4] == 'I' && src[pos + 5] == 'D' && src[pos + 6] == 'A' && src[pos + 7] == 'T')
        {
            if (idat + chunk_len > vt->term->buff_cap) { return LDG_ERR_FULL; }

            for (got = 0; got < chunk_len; got++) { vt->term->buff[idat + got] = src[pos + LDG_TERM_VT_PNG_CHUNK_HDR + got]; }

            idat += chunk_len;
        }
        else if (src[pos + 4] == 'I' && src[pos + 5] == 'E' && src[pos + 6] == 'N' && src[pos + 7] == 'D') { break; }

        pos += (uint64_t)LDG_TERM_VT_PNG_CHUNK_HDR + (uint64_t)chunk_len + (uint64_t)LDG_TERM_VT_PNG_CRC_LEN;
    }

    if (width == 0 || height == 0 || idat <= LDG_TERM_VT_ZLIB_HDR_LEN) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (width > LDG_TERM_VT_PNG_DIM_MAX || height > LDG_TERM_VT_PNG_DIM_MAX) { return LDG_ERR_UNSUPPORTED; }

    if (vt_png_chan_get(color, &chan) != LDG_ERR_AOK) { return LDG_ERR_UNSUPPORTED; }

    raw_span = (uint64_t)height * (((uint64_t)width * (uint64_t)chan) + LDG_TERM_VT_STEP);
    rgb_span = (uint64_t)width * (uint64_t)height * (uint64_t)LDG_TERM_VT_PNG_CHAN_RGB;

    if (idat + raw_span > vt->term->buff_cap) { return LDG_ERR_FULL; }

    if (idat + rgb_span > vt->term->buff_cap) { return LDG_ERR_FULL; }

    raw_need = (uint32_t)raw_span;

    if (ldg_compress_inflate(&vt->term->buff[LDG_TERM_VT_ZLIB_HDR_LEN], idat - LDG_TERM_VT_ZLIB_HDR_LEN, &vt->term->buff[idat], vt->term->buff_cap - idat, &got) != LDG_ERR_AOK) { return LDG_ERR_INVALID; }

    if (got < raw_need) { return LDG_ERR_INVALID; }

    vt_png_unfilter(&vt->term->buff[idat], width, height, chan);

    if (chan == LDG_TERM_VT_PNG_CHAN_RGB) { *out_fmt = LDG_TERM_IMG_FMT_RGB; }
    else if (chan == LDG_TERM_VT_PNG_CHAN_RGBA) { *out_fmt = LDG_TERM_IMG_FMT_RGBA; }
    else
    {
        pixel = (uint64_t)width * (uint64_t)height;
        while (pixel > 0)
        {
            pixel--;
            gray = vt->term->buff[idat + (pixel * chan)];
            vt->term->buff[idat + (pixel * LDG_TERM_VT_PNG_CHAN_RGB)] = gray;
            vt->term->buff[idat + (pixel * LDG_TERM_VT_PNG_CHAN_RGB) + LDG_TERM_VT_STEP] = gray;
            vt->term->buff[idat + (pixel * LDG_TERM_VT_PNG_CHAN_RGB) + LDG_TERM_VT_PAIR_STEP] = gray;
        }

        chan = LDG_TERM_VT_PNG_CHAN_RGB;
        *out_fmt = LDG_TERM_IMG_FMT_RGB;
    }

    *out_width = width;
    *out_height = height;
    *out_size = (uint32_t)((uint64_t)width * (uint64_t)height * (uint64_t)chan);

    for (pixel = 0; pixel < (uint64_t)(*out_size); pixel++) { vt->term->buff[pixel] = vt->term->buff[idat + pixel]; }

    return LDG_ERR_AOK;
}

static void vt_gfx_dispatch(ldg_term_vt_t *vt)
{
    vt_gfx_cmd_t cmd = LDG_STRUCT_ZERO_INIT;
    ldg_term_placement_t placement = LDG_STRUCT_ZERO_INIT;
    uint64_t got = 0;
    uint32_t col = 0;
    uint32_t row = 0;
    uint32_t png_size = 0;
    uint8_t png_fmt = 0;

    if (vt->osc_len == 0) { return; }

    if (vt->osc[LDG_TERM_VT_PARAM_FIRST] != LDG_TERM_VT_GFX_LEAD) { return; }

    vt_gfx_parse(vt, &cmd);

    if (cmd.action == LDG_TERM_VT_GFX_ACT_DROP)
    {
        ldg_term_img_drop(vt->term, cmd.id);
        return;
    }

    if (cmd.action == LDG_TERM_VT_GFX_ACT_QUERY)
    {
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, LDG_TERM_VT_APC_LEAD_CH);
        vt_reply_byte_wr(vt, LDG_TERM_VT_GFX_LEAD);
        vt_reply_byte_wr(vt, LDG_TERM_VT_GFX_KEY_ID);
        vt_reply_byte_wr(vt, LDG_TERM_VT_GFX_EQ);
        vt_reply_num_wr(vt, cmd.id);
        vt_reply_byte_wr(vt, LDG_TERM_VT_GFX_BODY);
        vt_reply_byte_wr(vt, 'O');
        vt_reply_byte_wr(vt, 'K');
        vt_reply_byte_wr(vt, LDG_TERM_VT_ESC_BYTE);
        vt_reply_byte_wr(vt, LDG_TERM_VT_ST_CH);
        return;
    }

    if (cmd.action == LDG_TERM_VT_GFX_ACT_XFER || cmd.action == LDG_TERM_VT_GFX_ACT_SHOW || cmd.action == 0)
    {
        if (cmd.body >= vt->osc_len) { return; }

        if (!vt->term->buff) { return; }

        if (ldg_str_base64_decode(&vt->osc[cmd.body], vt->osc_len - cmd.body, &vt->term->buff[vt->gfx_pending], vt->term->buff_cap - vt->gfx_pending, &got) != LDG_ERR_AOK) { return; }

        got += vt->gfx_pending;

        if (cmd.more != 0)
        {
            vt->gfx_pending = (uint32_t)got;
            vt->gfx_id = cmd.id;
            vt->gfx_fmt = cmd.fmt;
            vt->gfx_width = cmd.width;
            vt->gfx_height = cmd.height;
            return;
        }

        if (vt->gfx_pending > 0)
        {
            cmd.id = vt->gfx_id;
            cmd.fmt = vt->gfx_fmt;
            cmd.width = vt->gfx_width;
            cmd.height = vt->gfx_height;
        }

        vt->gfx_pending = 0;

        if (cmd.fmt == LDG_TERM_IMG_FMT_PNG)
        {
            if (vt_png_decode(vt, vt->term->buff, got, &cmd.width, &cmd.height, &png_fmt, &png_size) != LDG_ERR_AOK) { return; }

            if (ldg_term_img_st(vt->term, cmd.id, cmd.width, cmd.height, png_fmt, vt->term->buff, png_size) != LDG_ERR_AOK) { return; }
        }
        else if (ldg_term_img_st(vt->term, cmd.id, cmd.width, cmd.height, (uint8_t)cmd.fmt, vt->term->buff, (uint32_t)got) != LDG_ERR_AOK) { return; }
    }

    if (cmd.action != LDG_TERM_VT_GFX_ACT_SHOW && cmd.action != LDG_TERM_VT_GFX_ACT_PUT) { return; }

    ldg_term_cursor_get(vt->term, &col, &row);

    placement.img_id = cmd.id;
    placement.placement_id = cmd.placement_id;
    placement.col = col;
    placement.row = row;
    placement.col_span = cmd.cols;
    placement.row_span = cmd.rows;
    placement.layer = (int32_t)cmd.layer;

    ldg_term_placement_add(vt->term, &placement);
}

#define LDG_TERM_VT_SIXEL_LOW 0x3F
#define LDG_TERM_VT_SIXEL_HIGH 0x7E
#define LDG_TERM_VT_SIXEL_BAND 6
#define LDG_TERM_VT_SIXEL_REPEAT '!'
#define LDG_TERM_VT_SIXEL_COLOR '#'
#define LDG_TERM_VT_SIXEL_RASTER '"'
#define LDG_TERM_VT_SIXEL_CR '$'
#define LDG_TERM_VT_SIXEL_LF '-'
#define LDG_TERM_VT_SIXEL_SEP ';'
#define LDG_TERM_VT_SIXEL_MODEL_RGB 2
#define LDG_TERM_VT_SIXEL_SCALE 255
#define LDG_TERM_VT_SIXEL_FULL 100
#define LDG_TERM_VT_SIXEL_MAP 256
#define LDG_TERM_VT_SIXEL_DIM_MAX 256
#define LDG_TERM_VT_SIXEL_ID 1
#define LDG_TERM_VT_SIXEL_FINAL 'q'

static uint64_t vt_sixel_num_get(const ldg_term_vt_t *vt, uint64_t idx, uint32_t *out)
{
    uint32_t val = 0;

    while (idx < vt->osc_len && vt->osc[idx] >= LDG_TERM_VT_DIGIT_LOW && vt->osc[idx] <= LDG_TERM_VT_DIGIT_HIGH)
    {
        val = (val * LDG_TERM_VT_DECIMAL) + (uint32_t)((uint8_t)vt->osc[idx] - LDG_TERM_VT_DIGIT_LOW);
        idx++;
    }

    *out = val;

    return idx;
}

static void vt_sixel_dispatch(ldg_term_vt_t *vt)
{
    ldg_term_placement_t placement = LDG_STRUCT_ZERO_INIT;
    uint8_t map[LDG_TERM_VT_SIXEL_MAP][LDG_TERM_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t idx = 0;
    uint32_t col = 0;
    uint32_t row = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t repeat = 0;
    uint32_t slot = 0;
    uint32_t model = 0;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint32_t bit = 0;
    uint32_t span = 0;
    uint32_t cur_col = 0;
    uint32_t cur_row = 0;
    uint8_t val = 0;

    if (!vt->term->buff) { return; }

    width = LDG_TERM_VT_SIXEL_DIM_MAX;
    height = LDG_TERM_VT_SIXEL_DIM_MAX;
    span = width * height * LDG_TERM_VT_PNG_CHAN_RGB;

    if (span > vt->term->buff_cap) { return; }

    for (idx = 0; idx < span; idx++) { vt->term->buff[idx] = 0; }

    idx = 0;

    while (idx < vt->osc_len)
    {
        if (vt->osc[idx] == LDG_TERM_VT_SIXEL_RASTER)
        {
            idx++;

            while (idx < vt->osc_len && vt->osc[idx] != LDG_TERM_VT_SIXEL_COLOR && vt->osc[idx] != LDG_TERM_VT_SIXEL_LF && (vt->osc[idx] < LDG_TERM_VT_SIXEL_LOW || vt->osc[idx] > LDG_TERM_VT_SIXEL_HIGH)) { idx++; }

            continue;
        }

        if (vt->osc[idx] == LDG_TERM_VT_SIXEL_COLOR)
        {
            idx = vt_sixel_num_get(vt, idx + LDG_TERM_VT_STEP, &slot);

            if (slot >= LDG_TERM_VT_SIXEL_MAP) { slot = LDG_TERM_VT_PARAM_FIRST; }

            if (idx < vt->osc_len && vt->osc[idx] == LDG_TERM_VT_SIXEL_SEP)
            {
                idx = vt_sixel_num_get(vt, idx + LDG_TERM_VT_STEP, &model);

                if (idx < vt->osc_len && vt->osc[idx] == LDG_TERM_VT_SIXEL_SEP) { idx = vt_sixel_num_get(vt, idx + LDG_TERM_VT_STEP, &red); }

                if (idx < vt->osc_len && vt->osc[idx] == LDG_TERM_VT_SIXEL_SEP) { idx = vt_sixel_num_get(vt, idx + LDG_TERM_VT_STEP, &green); }

                if (idx < vt->osc_len && vt->osc[idx] == LDG_TERM_VT_SIXEL_SEP) { idx = vt_sixel_num_get(vt, idx + LDG_TERM_VT_STEP, &blue); }

                if (model == LDG_TERM_VT_SIXEL_MODEL_RGB && slot < LDG_TERM_VT_SIXEL_MAP)
                {
                    map[slot][LDG_TERM_VT_RED_IDX] = (uint8_t)((red * LDG_TERM_VT_SIXEL_SCALE) / LDG_TERM_VT_SIXEL_FULL);
                    map[slot][LDG_TERM_VT_GREEN_IDX] = (uint8_t)((green * LDG_TERM_VT_SIXEL_SCALE) / LDG_TERM_VT_SIXEL_FULL);
                    map[slot][LDG_TERM_VT_BLUE_IDX] = (uint8_t)((blue * LDG_TERM_VT_SIXEL_SCALE) / LDG_TERM_VT_SIXEL_FULL);
                }
            }

            continue;
        }

        if (vt->osc[idx] == LDG_TERM_VT_SIXEL_REPEAT)
        {
            idx = vt_sixel_num_get(vt, idx + LDG_TERM_VT_STEP, &repeat);
            continue;
        }

        if (vt->osc[idx] == LDG_TERM_VT_SIXEL_CR)
        {
            cur_col = 0;
            idx++;
            continue;
        }

        if (vt->osc[idx] == LDG_TERM_VT_SIXEL_LF)
        {
            cur_col = 0;
            cur_row += LDG_TERM_VT_SIXEL_BAND;
            idx++;
            continue;
        }

        if (vt->osc[idx] < LDG_TERM_VT_SIXEL_LOW || vt->osc[idx] > LDG_TERM_VT_SIXEL_HIGH)
        {
            idx++;
            continue;
        }

        val = (uint8_t)((uint8_t)vt->osc[idx] - LDG_TERM_VT_SIXEL_LOW);

        if (repeat == 0) { repeat = LDG_TERM_VT_STEP; }

        if (repeat > width) { repeat = width; }

        for (col = 0; col < repeat; col++)
        {
            for (bit = 0; bit < LDG_TERM_VT_SIXEL_BAND; bit++)
            {
                if (!(val & (LDG_TERM_VT_STEP << bit))) { continue; }

                row = cur_row + bit;

                if (cur_col >= width || row >= height) { continue; }

                vt->term->buff[((row * width) + cur_col) * LDG_TERM_VT_PNG_CHAN_RGB] = map[slot][LDG_TERM_VT_RED_IDX];
                vt->term->buff[(((row * width) + cur_col) * LDG_TERM_VT_PNG_CHAN_RGB) + LDG_TERM_VT_STEP] = map[slot][LDG_TERM_VT_GREEN_IDX];
                vt->term->buff[(((row * width) + cur_col) * LDG_TERM_VT_PNG_CHAN_RGB) + LDG_TERM_VT_PAIR_STEP] = map[slot][LDG_TERM_VT_BLUE_IDX];
            }

            cur_col++;
        }

        repeat = 0;
        idx++;
    }

    if (ldg_term_img_st(vt->term, LDG_TERM_VT_SIXEL_ID, width, height, LDG_TERM_IMG_FMT_RGB, vt->term->buff, span) != LDG_ERR_AOK) { return; }

    ldg_term_cursor_get(vt->term, &col, &row);

    placement.img_id = LDG_TERM_VT_SIXEL_ID;
    placement.col = col;
    placement.row = row;

    ldg_term_placement_add(vt->term, &placement);
}

static void vt_byte_feed(ldg_term_vt_t *vt, uint8_t byte)
{
    if (byte == LDG_TERM_VT_CAN_CH || byte == LDG_TERM_VT_SUB_CH)
    {
        vt_param_rst(vt);
        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_OSC)
    {
        if (byte == LDG_TERM_VT_BEL_CH)
        {
            vt_osc_dispatch(vt);
            vt_param_rst(vt);
            vt->state = LDG_TERM_VT_STATE_GROUND;
            return;
        }

        if (byte == LDG_TERM_VT_ESC_CH)
        {
            vt_osc_dispatch(vt);
            vt->state = LDG_TERM_VT_STATE_OSC_ESC;
            return;
        }

        vt_str_collect(vt, byte);
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_STR || vt->state == LDG_TERM_VT_STATE_DCS_PASS)
    {
        if (byte == LDG_TERM_VT_ESC_CH)
        {
            vt->state = LDG_TERM_VT_STATE_STR_ESC;
            return;
        }

        vt_str_collect(vt, byte);
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_OSC_ESC || vt->state == LDG_TERM_VT_STATE_STR_ESC)
    {
        if (byte != LDG_TERM_VT_ST_CH)
        {
            vt_param_rst(vt);
            vt->state = LDG_TERM_VT_STATE_ESC;
            vt_esc_feed(vt, byte);
            return;
        }

        if (vt->str_kind == LDG_TERM_VT_STR_APC) { vt_gfx_dispatch(vt); }
        else if (vt->str_kind == LDG_TERM_VT_STR_DCS && vt->inter_cunt == LDG_TERM_VT_STEP && vt->inter[LDG_TERM_VT_PARAM_FIRST] == LDG_TERM_VT_SIXEL_FINAL) { vt_sixel_dispatch(vt); }

        vt_param_rst(vt);
        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    if (byte == LDG_TERM_VT_ESC_CH)
    {
        vt_param_rst(vt);
        vt->state = LDG_TERM_VT_STATE_ESC;
        return;
    }

    if (byte <= LDG_TERM_VT_C0_HIGH)
    {
        if (vt->state == LDG_TERM_VT_STATE_GROUND) { vt_ground_feed(vt, byte); }
        else { vt_ctrl_exec(vt, byte); }

        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_GROUND)
    {
        vt_ground_feed(vt, byte);
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_ESC)
    {
        vt_esc_feed(vt, byte);
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_ESC_INTER)
    {
        if (byte >= LDG_TERM_VT_INTER_LOW && byte <= LDG_TERM_VT_INTER_HIGH)
        {
            vt_inter_add(vt, byte);
            return;
        }

        if (byte >= LDG_TERM_VT_ESC_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH) { vt_esc_dispatch(vt, byte); }

        vt_param_rst(vt);
        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_VT52_ROW)
    {
        vt->vt52_row = (uint32_t)byte - LDG_TERM_VT_VT52_ADDR_BIAS;
        vt->state = LDG_TERM_VT_STATE_VT52_COL;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_VT52_COL)
    {
        ldg_term_cursor_set(vt->term, (uint32_t)byte - LDG_TERM_VT_VT52_ADDR_BIAS, vt->vt52_row);
        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_CHARSET)
    {
        if (byte == LDG_TERM_VT_CHARSET_GFX) { ldg_term_charset_set(vt->term, vt->charset_slot, LDG_TERM_CHARSET_GRAPHIC); }
        else if (byte == LDG_TERM_VT_CHARSET_UK) { ldg_term_charset_set(vt->term, vt->charset_slot, LDG_TERM_CHARSET_UK); }
        else { ldg_term_charset_set(vt->term, vt->charset_slot, LDG_TERM_CHARSET_ASCII); }

        vt->state = LDG_TERM_VT_STATE_GROUND;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_CSI_ENTRY || vt->state == LDG_TERM_VT_STATE_CSI_PARAM)
    {
        vt_csi_feed(vt, byte);
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_CSI_INTER)
    {
        if (byte >= LDG_TERM_VT_INTER_LOW && byte <= LDG_TERM_VT_INTER_HIGH)
        {
            vt_inter_add(vt, byte);
            return;
        }

        if (byte >= LDG_TERM_VT_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH)
        {
            vt_csi_dispatch(vt, byte);
            vt_param_rst(vt);
            vt->state = LDG_TERM_VT_STATE_GROUND;
            return;
        }

        vt->state = LDG_TERM_VT_STATE_CSI_IGNORE;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_CSI_IGNORE)
    {
        if (byte >= LDG_TERM_VT_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH)
        {
            vt_unhandled_mark(vt, byte);
            vt_param_rst(vt);
            vt->state = LDG_TERM_VT_STATE_GROUND;
        }

        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_DCS_ENTRY || vt->state == LDG_TERM_VT_STATE_DCS_PARAM)
    {
        vt_dcs_feed(vt, byte);
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_DCS_INTER)
    {
        if (byte >= LDG_TERM_VT_INTER_LOW && byte <= LDG_TERM_VT_INTER_HIGH)
        {
            vt_inter_add(vt, byte);
            return;
        }

        if (byte >= LDG_TERM_VT_FINAL_LOW && byte <= LDG_TERM_VT_FINAL_HIGH)
        {
            vt_inter_add(vt, byte);
            vt->osc_len = 0;
            vt->state = LDG_TERM_VT_STATE_DCS_PASS;
            return;
        }

        vt->state = LDG_TERM_VT_STATE_DCS_IGNORE;
        return;
    }

    if (vt->state == LDG_TERM_VT_STATE_DCS_IGNORE) { return; }

    vt_param_rst(vt);
    vt->state = LDG_TERM_VT_STATE_GROUND;
}

uint32_t ldg_term_vt_init(ldg_term_vt_t *vt, ldg_term_t *term)
{
    if (LDG_UNLIKELY(!vt || !term)) { return LDG_ERR_FUNC_ARG_NULL; }

    vt->term = term;
    vt->state = LDG_TERM_VT_STATE_GROUND;
    vt->osc[0] = LDG_STR_TERM;
    vt->title[0] = LDG_STR_TERM;
    vt->title_len = 0;
    vt->title_depth = 0;
    vt->unhandled_mode_cunt = 0;
    vt->last_ch = LDG_TERM_VT_CH_NONE;

    vt->mode = LDG_TERM_VT_MODE_ANSI;
    vt->gfx_pending = 0;

    vt_kbd_rst(vt);

    vt_param_rst(vt);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_rst(ldg_term_vt_t *vt)
{
    if (LDG_UNLIKELY(!vt)) { return LDG_ERR_FUNC_ARG_NULL; }

    vt->state = LDG_TERM_VT_STATE_GROUND;
    vt->osc[0] = LDG_STR_TERM;
    vt->title[0] = LDG_STR_TERM;
    vt->title_len = 0;
    vt->title_depth = 0;
    vt->unhandled_mode_cunt = 0;
    vt->last_ch = LDG_TERM_VT_CH_NONE;

    vt->mode = LDG_TERM_VT_MODE_ANSI;
    vt->gfx_pending = 0;

    vt_kbd_rst(vt);

    vt_param_rst(vt);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_feed(ldg_term_vt_t *vt, const ldg_byte_t *buff, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vt || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vt->term)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < len; idx++) { vt_byte_feed(vt, buff[idx]); }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_title_get(const ldg_term_vt_t *vt, char *out, uint64_t cap)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vt || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (vt->title_len == 0) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(vt->title_len >= cap)) { return LDG_ERR_FULL; }

    for (idx = 0; idx < vt->title_len; idx++) { out[idx] = vt->title[idx]; }

    out[vt->title_len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

#define LDG_TERM_VT_ESC_BYTE 0x1B
#define LDG_TERM_VT_CSI_LEAD 0x5B
#define LDG_TERM_VT_SS3_LEAD 0x4F
#define LDG_TERM_VT_SEMI 0x3B
#define LDG_TERM_VT_TILDE 0x7E
#define LDG_TERM_VT_DIGIT_BASE 0x30
#define LDG_TERM_VT_DEL_BYTE 0x7F
#define LDG_TERM_VT_TAB_BYTE 0x09
#define LDG_TERM_VT_ENTER_BYTE 0x0D
#define LDG_TERM_VT_CTRL_MASK 0x1F
#define LDG_TERM_VT_MOD_BASE 1
#define LDG_TERM_VT_KEY_CODE_NONE 0
#define LDG_TERM_VT_KEY_CODE_TAB 9
#define LDG_TERM_VT_KEY_CODE_ENTER 13
#define LDG_TERM_VT_KEY_CODE_ESC 27
#define LDG_TERM_VT_KEY_CODE_BS 127
#define LDG_TERM_VT_NUM_RADIX 10
#define LDG_TERM_VT_NUM_CAP 8
#define LDG_TERM_VT_TILDE_F3 13
#define LDG_TERM_VT_TILDE_INSERT 2
#define LDG_TERM_VT_TILDE_DELETE 3
#define LDG_TERM_VT_TILDE_PAGE_UP 5
#define LDG_TERM_VT_TILDE_PAGE_DOWN 6
#define LDG_TERM_VT_PASTE_START 200
#define LDG_TERM_VT_PASTE_END 201

static uint32_t vt_out_byte_wr(ldg_byte_t *out, uint64_t cap, uint64_t *len, uint8_t byte)
{
    if (LDG_UNLIKELY(*len >= cap)) { return LDG_ERR_FULL; }

    out[*len] = byte;
    *len = *len + LDG_TERM_VT_STEP;

    return LDG_ERR_AOK;
}

static uint32_t vt_out_num_wr(ldg_byte_t *out, uint64_t cap, uint64_t *len, uint32_t val)
{
    uint8_t digit[LDG_TERM_VT_NUM_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t rest = val;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    do
    {
        if (LDG_UNLIKELY(cunt >= LDG_TERM_VT_NUM_CAP)) { return LDG_ERR_FULL; }

        digit[cunt] = (uint8_t)(LDG_TERM_VT_DIGIT_BASE + (rest % LDG_TERM_VT_NUM_RADIX));
        rest = rest / LDG_TERM_VT_NUM_RADIX;
        cunt++;
    }
    while (rest > 0);

    for (idx = cunt; idx > 0; idx--)
    {
        ret = vt_out_byte_wr(out, cap, len, digit[idx - LDG_TERM_VT_STEP]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t vt_mod_param_get(uint32_t mods)
{
    return LDG_TERM_VT_MOD_BASE + (mods & LDG_TERM_MOD_MASK);
}

static uint32_t vt_kbd_active_get(const ldg_term_vt_t *vt)
{
    return vt->kbd_flags[vt_kbd_screen_get(vt)];
}

static uint32_t vt_key_code_get(uint8_t key, uint32_t codepoint)
{
    if (key == LDG_TERM_KEY_NONE) { return codepoint; }

    if (key == LDG_TERM_KEY_ESCAPE) { return LDG_TERM_VT_KEY_CODE_ESC; }

    if (key == LDG_TERM_KEY_ENTER) { return LDG_TERM_VT_KEY_CODE_ENTER; }

    if (key == LDG_TERM_KEY_TAB) { return LDG_TERM_VT_KEY_CODE_TAB; }

    if (key == LDG_TERM_KEY_BACKSPACE) { return LDG_TERM_VT_KEY_CODE_BS; }

    return LDG_TERM_VT_KEY_CODE_NONE;
}

static uint32_t vt_csi_u_wr(const ldg_term_vt_t *vt, const ldg_term_key_event_t *event, uint32_t code, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint32_t flags = 0;
    uint32_t param = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint64_t len = 0;

    flags = vt_kbd_active_get(vt);
    param = vt_mod_param_get(event->mods);

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_CSI_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, &len, code);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if ((flags & LDG_TERM_VT_KBD_ALT_KEY) && (event->shifted != 0 || event->base != 0))
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_COLON_CH);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (event->shifted != 0 && (event->mods & LDG_TERM_MOD_SHIFT))
        {
            ret = vt_out_num_wr(out, cap, &len, event->shifted);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (event->base != 0)
        {
            ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_COLON_CH);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = vt_out_num_wr(out, cap, &len, event->base);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (param != LDG_TERM_VT_MOD_BASE || ((flags & LDG_TERM_VT_KBD_EVENT_TYPE) && event->kind != LDG_TERM_KEY_KIND_PRESS) || ((flags & LDG_TERM_VT_KBD_TEXT) && event->text_cunt > 0))
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_num_wr(out, cap, &len, param);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if ((flags & LDG_TERM_VT_KBD_EVENT_TYPE) && event->kind != LDG_TERM_KEY_KIND_PRESS)
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_COLON_CH);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_num_wr(out, cap, &len, event->kind);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if ((flags & LDG_TERM_VT_KBD_TEXT) && event->text_cunt > 0)
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (idx = 0; idx < event->text_cunt && idx < LDG_TERM_KEY_TEXT_MAX; idx++)
        {
            if (idx > 0)
            {
                ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_COLON_CH);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            ret = vt_out_num_wr(out, cap, &len, event->text[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ret = vt_out_byte_wr(out, cap, &len, 'u');
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = len;

    return LDG_ERR_AOK;
}

static uint32_t vt_key_final_get(ldg_term_key_t key, uint8_t *out_final, uint8_t *out_tilde, uint32_t *out_num)
{
    *out_final = 0;
    *out_tilde = LDG_TRUTH_FALSE;
    *out_num = 0;

    if (key == LDG_TERM_KEY_UP) { *out_final = 'A'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_DOWN) { *out_final = 'B'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_RHS) { *out_final = 'C'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_LHS) { *out_final = 'D'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_HOME) { *out_final = 'H'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_END) { *out_final = 'F'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F1) { *out_final = 'P'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F2) { *out_final = 'Q'; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F4) { *out_final = 'S'; return LDG_ERR_AOK; }

    *out_tilde = LDG_TRUTH_TRUE;

    if (key == LDG_TERM_KEY_F3) { *out_num = LDG_TERM_VT_TILDE_F3; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_INSERT) { *out_num = LDG_TERM_VT_TILDE_INSERT; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_DELETE) { *out_num = LDG_TERM_VT_TILDE_DELETE; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_PAGE_UP) { *out_num = LDG_TERM_VT_TILDE_PAGE_UP; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_PAGE_DOWN) { *out_num = LDG_TERM_VT_TILDE_PAGE_DOWN; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F5) { *out_num = 15; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F6) { *out_num = 17; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F7) { *out_num = 18; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F8) { *out_num = 19; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F9) { *out_num = 20; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F10) { *out_num = 21; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F11) { *out_num = 23; return LDG_ERR_AOK; }

    if (key == LDG_TERM_KEY_F12) { *out_num = 24; return LDG_ERR_AOK; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_term_vt_clipboard_get(ldg_term_vt_t *vt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vt || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (vt->clip_len == 0) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY((uint64_t)vt->clip_len > cap)) { return LDG_ERR_FULL; }

    for (idx = 0; idx < vt->clip_len; idx++) { out[idx] = vt->clip[idx]; }

    *out_len = (uint64_t)vt->clip_len;
    vt->clip_len = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_clipboard_encode(const ldg_byte_t *text, uint64_t len, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t written = 0;
    uint64_t coded = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!text || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_OSC_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_OSC_CLIP_HIGH);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_OSC_CLIP_LOW);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_OSC_SEP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_CLIP_SEL);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_OSC_SEP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_base64_encode(text, len, (char *)&out[written], cap - written, &coded);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    written += coded;

    ret = vt_out_byte_wr(out, cap, &written, LDG_TERM_VT_BEL_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = written;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_reply_drain(ldg_term_vt_t *vt, ldg_byte_t *buff, uint64_t cap, uint64_t *out_len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vt || !buff || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (vt->reply_len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY((uint64_t)vt->reply_len > cap)) { return LDG_ERR_FULL; }

    for (idx = 0; idx < vt->reply_len; idx++) { buff[idx] = vt->reply[idx]; }

    *out_len = (uint64_t)vt->reply_len;
    vt->reply_len = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_mode_get(const ldg_term_vt_t *vt, uint32_t *out)
{
    if (LDG_UNLIKELY(!vt || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = vt->mode;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_unhandled_get(const ldg_term_vt_t *vt, ldg_byte_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint64_t found = 0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!vt || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (slot = 0; slot < LDG_TERM_VT_FINAL_SPAN; slot++)
    {
        if ((vt->unhandled_lo & (LDG_TERM_VT_BIT_ONE << slot)) == 0) { continue; }

        if (found >= cap) { return LDG_ERR_FULL; }

        out[found] = (ldg_byte_t)(LDG_TERM_VT_FINAL_BASE + slot);
        found++;
    }

    for (slot = 0; slot < LDG_TERM_VT_FINAL_SPAN; slot++)
    {
        if ((vt->unhandled_hi & (LDG_TERM_VT_BIT_ONE << slot)) == 0) { continue; }

        if (found >= cap) { return LDG_ERR_FULL; }

        out[found] = (ldg_byte_t)(LDG_TERM_VT_FINAL_BASE + LDG_TERM_VT_FINAL_SPAN + slot);
        found++;
    }

    *out_cunt = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_vt_cursor_shape_get(const ldg_term_vt_t *vt, uint8_t *out)
{
    if (LDG_UNLIKELY(!vt || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = vt->cursor_shape;

    return LDG_ERR_AOK;
}

static uint32_t vt_legacy_text_wr(const ldg_term_key_event_t *event, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint32_t ret = 0;
    uint8_t byte = 0;

    byte = (uint8_t)event->codepoint;

    if (event->mods & LDG_TERM_MOD_CTRL) { byte = (uint8_t)(byte & LDG_TERM_VT_CTRL_MASK); }

    if (event->mods & LDG_TERM_MOD_ALT)
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_ESC_BYTE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = vt_out_byte_wr(out, cap, &len, byte);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = len;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_key_encode(const ldg_term_vt_t *vt, const ldg_term_key_event_t *event, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint32_t param = 0;
    uint32_t num = 0;
    uint32_t ret = 0;
    uint32_t flags = 0;
    uint32_t code = 0;
    uint32_t mods = 0;
    uint8_t key = 0;
    uint8_t final = 0;
    uint8_t tilde = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!vt || !event || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    flags = vt_kbd_active_get(vt);
    key = event->key;
    mods = event->mods;
    code = vt_key_code_get(key, event->codepoint);

    if (key == LDG_TERM_KEY_NONE)
    {
        if (flags & LDG_TERM_VT_KBD_ALL_ESC) { return vt_csi_u_wr(vt, event, code, out, cap, out_len); }

        if ((flags & LDG_TERM_VT_KBD_DISAMBIGUATE) && (mods & (LDG_TERM_MOD_CTRL | LDG_TERM_MOD_ALT))) { return vt_csi_u_wr(vt, event, code, out, cap, out_len); }

        return vt_legacy_text_wr(event, out, cap, out_len);
    }

    if (code != LDG_TERM_VT_KEY_CODE_NONE)
    {
        if (flags & LDG_TERM_VT_KBD_ALL_ESC) { return vt_csi_u_wr(vt, event, code, out, cap, out_len); }

        if ((flags & LDG_TERM_VT_KBD_DISAMBIGUATE) && key == LDG_TERM_KEY_ESCAPE) { return vt_csi_u_wr(vt, event, code, out, cap, out_len); }
    }

    if (key == LDG_TERM_KEY_BACKSPACE) { return vt_out_byte_wr(out, cap, out_len, LDG_TERM_VT_DEL_BYTE); }

    if (key == LDG_TERM_KEY_ENTER) { return vt_out_byte_wr(out, cap, out_len, LDG_TERM_VT_ENTER_BYTE); }

    if (key == LDG_TERM_KEY_ESCAPE) { return vt_out_byte_wr(out, cap, out_len, LDG_TERM_VT_ESC_BYTE); }

    if (key == LDG_TERM_KEY_TAB && !(mods & LDG_TERM_MOD_SHIFT)) { return vt_out_byte_wr(out, cap, out_len, LDG_TERM_VT_TAB_BYTE); }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (key == LDG_TERM_KEY_TAB)
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_CSI_LEAD);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_byte_wr(out, cap, &len, 'Z');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_len = len;
        return LDG_ERR_AOK;
    }

    ret = vt_key_final_get(key, &final, &tilde, &num);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    param = vt_mod_param_get(mods);

    if (tilde != LDG_TRUTH_TRUE && param == LDG_TERM_VT_MOD_BASE && (vt->mode & LDG_TERM_VT_MODE_APP_CURSOR))
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SS3_LEAD);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_byte_wr(out, cap, &len, final);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_len = len;
        return LDG_ERR_AOK;
    }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_CSI_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (tilde == LDG_TRUTH_TRUE) { ret = vt_out_num_wr(out, cap, &len, num); }
    else if (param != LDG_TERM_VT_MOD_BASE) { ret = vt_out_num_wr(out, cap, &len, LDG_TERM_VT_MOD_BASE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (param != LDG_TERM_VT_MOD_BASE)
    {
        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_num_wr(out, cap, &len, param);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (tilde == LDG_TRUTH_TRUE) { ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_TILDE); }
    else { ret = vt_out_byte_wr(out, cap, &len, final); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = len;

    return LDG_ERR_AOK;
}

#define LDG_TERM_VT_MOUSE_X10_BIAS 32
#define LDG_TERM_VT_MOUSE_X10_MAX 223
#define LDG_TERM_VT_MOUSE_MOTION_BIT 32
#define LDG_TERM_VT_MOUSE_WHEEL_BIT 64
#define LDG_TERM_VT_MOUSE_SHIFT_BIT 4
#define LDG_TERM_VT_MOUSE_ALT_BIT 8
#define LDG_TERM_VT_MOUSE_CTRL_BIT 16
#define LDG_TERM_VT_MOUSE_SGR_LEAD 0x3C

static uint32_t vt_mouse_button_get(const ldg_term_mouse_event_t *event, uint8_t sgr)
{
    uint32_t code = 0;

    if (event->button == LDG_TERM_MOUSE_BTN_WHEEL_UP) { code = LDG_TERM_VT_MOUSE_WHEEL_BIT; }
    else if (event->button == LDG_TERM_MOUSE_BTN_WHEEL_DOWN) { code = LDG_TERM_VT_MOUSE_WHEEL_BIT + LDG_TERM_VT_STEP; }
    else { code = event->button; }

    if (event->kind == LDG_TERM_MOUSE_KIND_RELEASE && sgr != LDG_TRUTH_TRUE) { code = LDG_TERM_MOUSE_BTN_NONE; }

    if (event->kind == LDG_TERM_MOUSE_KIND_MOTION) { code += LDG_TERM_VT_MOUSE_MOTION_BIT; }

    if (event->mods & LDG_TERM_MOD_SHIFT) { code += LDG_TERM_VT_MOUSE_SHIFT_BIT; }

    if (event->mods & LDG_TERM_MOD_ALT) { code += LDG_TERM_VT_MOUSE_ALT_BIT; }

    if (event->mods & LDG_TERM_MOD_CTRL) { code += LDG_TERM_VT_MOUSE_CTRL_BIT; }

    return code;
}

static uint8_t vt_mouse_wanted_is(const ldg_term_vt_t *vt, const ldg_term_mouse_event_t *event)
{
    if (event->kind != LDG_TERM_MOUSE_KIND_MOTION)
    {
        if (vt->mode & (LDG_TERM_VT_MODE_MOUSE_NORMAL | LDG_TERM_VT_MODE_MOUSE_BUTTON | LDG_TERM_VT_MODE_MOUSE_ANY)) { return LDG_TRUTH_TRUE; }

        return LDG_TRUTH_FALSE;
    }

    if (vt->mode & LDG_TERM_VT_MODE_MOUSE_ANY) { return LDG_TRUTH_TRUE; }

    if ((vt->mode & LDG_TERM_VT_MODE_MOUSE_BUTTON) && event->button != LDG_TERM_MOUSE_BTN_NONE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

#define LDG_TERM_VT_RESIZE_KIND 48

uint32_t ldg_term_resize_encode(const ldg_term_vt_t *vt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vt || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!(vt->mode & LDG_TERM_VT_MODE_RESIZE)) { return LDG_ERR_AOK; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_CSI_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, &len, LDG_TERM_VT_RESIZE_KIND);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, &len, vt->term->row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, &len, vt->term->col_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, &len, vt->term->row_cunt * LDG_TERM_VT_CELL_HEIGHT_PX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, &len, vt->term->col_cunt * LDG_TERM_VT_CELL_WIDTH_PX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, 't');
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = len;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_mouse_encode(const ldg_term_vt_t *vt, const ldg_term_mouse_event_t *event, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint32_t code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vt || !event || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (vt_mouse_wanted_is(vt, event) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_CSI_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (vt->mode & LDG_TERM_VT_MODE_MOUSE_SGR)
    {
        code = vt_mouse_button_get(event, LDG_TRUTH_TRUE);

        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_MOUSE_SGR_LEAD);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_num_wr(out, cap, &len, code);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_num_wr(out, cap, &len, event->col + LDG_TERM_VT_STEP);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_SEMI);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = vt_out_num_wr(out, cap, &len, event->row + LDG_TERM_VT_STEP);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (event->kind == LDG_TERM_MOUSE_KIND_RELEASE) { ret = vt_out_byte_wr(out, cap, &len, 'm'); }
        else { ret = vt_out_byte_wr(out, cap, &len, 'M'); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_len = len;

        return LDG_ERR_AOK;
    }

    if (event->col >= LDG_TERM_VT_MOUSE_X10_MAX || event->row >= LDG_TERM_VT_MOUSE_X10_MAX) { return LDG_ERR_BOUNDS; }

    code = vt_mouse_button_get(event, LDG_TRUTH_FALSE);

    ret = vt_out_byte_wr(out, cap, &len, 'M');
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, (uint8_t)(code + LDG_TERM_VT_MOUSE_X10_BIAS));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, (uint8_t)(event->col + LDG_TERM_VT_STEP + LDG_TERM_VT_MOUSE_X10_BIAS));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, (uint8_t)(event->row + LDG_TERM_VT_STEP + LDG_TERM_VT_MOUSE_X10_BIAS));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = len;

    return LDG_ERR_AOK;
}

static uint32_t vt_paste_marker_wr(ldg_byte_t *out, uint64_t cap, uint64_t *len, uint32_t code)
{
    uint32_t ret = 0;

    ret = vt_out_byte_wr(out, cap, len, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, len, LDG_TERM_VT_CSI_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, len, '?');
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_num_wr(out, cap, len, code);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return vt_out_byte_wr(out, cap, len, LDG_TERM_VT_TILDE);
}

uint32_t ldg_term_paste_encode(const ldg_term_vt_t *vt, const ldg_byte_t *text, uint64_t len, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t written = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vt || !text || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (vt->mode & LDG_TERM_VT_MODE_PASTE_BRACKET)
    {
        ret = vt_paste_marker_wr(out, cap, &written, LDG_TERM_VT_PASTE_START);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < len; idx++)
    {
        ret = vt_out_byte_wr(out, cap, &written, text[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (vt->mode & LDG_TERM_VT_MODE_PASTE_BRACKET)
    {
        ret = vt_paste_marker_wr(out, cap, &written, LDG_TERM_VT_PASTE_END);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out_len = written;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_encode(const ldg_term_vt_t *vt, uint8_t focused, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vt || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!(vt->mode & LDG_TERM_VT_MODE_FOCUS)) { return LDG_ERR_AOK; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_ESC_BYTE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = vt_out_byte_wr(out, cap, &len, LDG_TERM_VT_CSI_LEAD);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (focused == LDG_TRUTH_TRUE) { ret = vt_out_byte_wr(out, cap, &len, 'I'); }
    else { ret = vt_out_byte_wr(out, cap, &len, 'O'); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_len = len;

    return LDG_ERR_AOK;
}
