#pragma once
#include <stdint.h>

/* Max runtime-registered glyphs (in addition to built-in) */
#define CN_FONT_DYNAMIC_MAX  16

/* 16x16 glyph in SSD1306-native page format (32 bytes) */
typedef const uint8_t glyph16_t[32];

/* Built-in glyph table */
extern const uint32_t cn_font_codepoint[];
extern const uint8_t cn_font_16x16[][32];
extern const int cn_font_count;

/* Look up glyph index by Unicode code point. Searches built-in table
   first, then dynamic table. Returns -1 if not found. */
int cn_font_lookup(uint32_t cp);

/**
 * Register a 16x16 glyph at runtime.
 *
 * @param cp     Unicode code point (e.g. 0x2665 for ♥).
 * @param glyph  32-byte SSD1306-page-format bitmap.
 *               glyph[0..15]:  top page (rows 0-7), 16 columns, bit0=top
 *               glyph[16..31]: bottom page (rows 8-15), 16 columns
 * @return 0 on success, -1 if the dynamic table is full.
 */
int oled_register_glyph(uint32_t cp, const uint8_t glyph[32]);
