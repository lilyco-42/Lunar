#pragma once
#include <stdint.h>

/* 16x16 Chinese glyph, row-major: 16 rows × 2 bytes = 32 bytes */
extern const uint8_t cn_font_16x16[][32];

/* UTF-8 code units that match each glyph (one char = one glyph) */
extern const uint32_t cn_font_codepoint[];

/* Number of glyphs in the font */
extern const int cn_font_count;

/* Look up glyph index by UTF-8 code point, returns -1 if not found */
int cn_font_lookup(uint32_t cp);
