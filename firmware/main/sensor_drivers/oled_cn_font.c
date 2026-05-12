#include "oled_cn_font.h"

/*
 * Font data in SSD1306-native page format:
 *   glyph[0..15]:  top page (rows 0-7), 16 columns, bit0=top of page
 *   glyph[16..31]: bottom page (rows 8-15), 16 columns
 *
 * To add more characters: use PCtoLCD2002 or similar tool,
 * generate 16x16 bitmaps, and paste below as 32-byte arrays.
 * Format: column-major within each page, bit0=top of that page.
 */

const uint32_t cn_font_codepoint[] = {
    0x4E00, // 一
};

const int cn_font_count = sizeof(cn_font_codepoint) / sizeof(cn_font_codepoint[0]);

const uint8_t cn_font_16x16[][32] = {
    /* 0x4E00 一 — 2px horizontal bar at rows 7-8 */
    {
        0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
        0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    },
};

int cn_font_lookup(uint32_t cp)
{
    for (int i = 0; i < cn_font_count; i++) {
        if (cn_font_codepoint[i] == cp) return i;
    }
    return -1;
}
