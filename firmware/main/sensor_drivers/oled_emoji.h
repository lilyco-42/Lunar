#pragma once
#include <stdint.h>

/* Built-in emoji expressions */
typedef enum {
    EMOJI_SMILE     = 0,  /* :) 微笑 */
    EMOJI_BIGSMILE  = 1,  /* :D 大笑 */
    EMOJI_SURPRISED = 2,  /* :O 惊讶 */
    EMOJI_WINK      = 3,  /* ;) 眨眼 */
    EMOJI_SAD       = 4,  /* :( 难过 */
    EMOJI_ANGRY     = 5,  /* >:( 生气 */
    EMOJI_COOL      = 6,  /* B) 酷 */
    EMOJI_HEART     = 7,  /* <3 爱心 (非脸型) */
    EMOJI_COUNT
} emoji_t;

/**
 * Show an emoji centered in the top 32px (pages 0-3) of the OLED.
 * The emoji is 32x32 pixels (4 pages tall × 32 columns wide).
 *
 * @param e  Emoji expression to display.
 */
void oled_emoji_show(emoji_t e);

/**
 * Clear the emoji area (pages 0-3).
 */
void oled_emoji_clear(void);

/**
 * Register a custom 32x32 emoji at runtime.
 *
 * @param id    Index to assign (0..15). Overwrites if already exists.
 * @param data  128 bytes: 4 pages × 32 columns, SSD1306 page format.
 *              data[0..31]:   page 0 (rows 0-7)
 *              data[32..63]:  page 1 (rows 8-15)
 *              data[64..95]:  page 2 (rows 16-23)
 *              data[96..127]: page 3 (rows 24-31)
 */
void oled_emoji_register(int id, const uint8_t data[128]);

/**
 * Show a custom emoji by id.
 */
void oled_emoji_show_custom(int id);
