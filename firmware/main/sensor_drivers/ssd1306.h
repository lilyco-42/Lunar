#pragma once

#include <stdint.h>
#include "driver/i2c.h"

/**
 * @brief Initialize the SSD1306 OLED display over I2C.
 *
 * The I2C bus (I2C_NUM_0) must already be configured and installed
 * by the caller.  Must be called before any other oled_* function.
 */
void oled_init(void);

/**
 * @brief Clear the entire display (all 8 pages, 128 columns).
 */
void oled_clear(void);

/**
 * @brief Render a null-terminated string at a given display line.
 *
 * @param line  Page index (0-7). Each line is 8 pixels tall.
 *              Characters are rendered left-to-right starting at column 0.
 * @param text  Null-terminated ASCII string (0x20-0x7F).
 *              Characters outside the font range are silently skipped.
 */
void oled_show_text(int line, const char *text);

/**
 * @brief Write raw bitmap data to consecutive pages.
 *
 * Writes up to 128 columns × N pages starting at (page, col).
 * Each page gets min(128-col, remaining_bytes) columns.
 *
 * @param page   Starting page (0-7).
 * @param col    Starting column (0-127).
 * @param data   Raw SSD1306 column data.
 * @param len    Total bytes to write (typically 32, 64, 128).
 */
void oled_write_bitmap(int page, int col, const uint8_t *data, int len);

/**
 * @brief Convenience wrapper: show text on the first two lines.
 *
 * Equivalent to:
 *   oled_show_text(0, line1);
 *   oled_show_text(1, line2);
 *
 * @param line1  Text for the top line (page 0).
 * @param line2  Text for the second line (page 1).
 */
void oled_show_two_lines(const char *line1, const char *line2);

/**
 * @brief Render a 16x16 glyph at given page/column.
 *
 * The glyph spans 2 pages (16 pixels tall) and 16 columns wide.
 * Data format: 32 bytes SSD1306-page-optimized — first 16 bytes = top page
 * (8 rows), next 16 bytes = bottom page, each byte is one column.
 *
 * @param page  Top page index (0-6). Must leave room for page+1.
 * @param col   Starting column (0-111). Must leave room for 16 cols.
 * @param glyph 32-byte glyph data.
 */
void oled_show_glyph16(int page, int col, const uint8_t glyph[32]);

/**
 * @brief Render a UTF-8 string with mixed ASCII and Chinese characters.
 *
 * ASCII chars are rendered with the built-in 5x7 font. CJK chars are
 * looked up in the Chinese font table and rendered as 16x16 glyphs.
 * Falls back to '?' for unrecognized characters.
 *
 * @param page  Page index (0-6).
 * @param text  Null-terminated UTF-8 string.
 * @param max_width  Maximum pixel width before wrapping (0 = no wrap).
 * @return  Number of pixel columns consumed.
 */
int oled_show_utf8(int page, const char *text, int max_width);
