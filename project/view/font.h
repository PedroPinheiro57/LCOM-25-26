/**
 * @file font.h
 * @brief Bitmap font rendering for the game HUD and menus.
 *
 * Loads a monochrome bitmap font from an XPM sprite sheet and exposes
 * helpers for drawing individual characters and null-terminated strings
 * directly into the framebuffer.
 *
 * The font sheet is organised as a 16-column grid of 8×12-pixel glyphs
 * starting at ASCII code 32 (space).  Scaling is applied uniformly —
 * each logical pixel is rendered as a @p scale × @p scale block.
 */

#pragma once
#include <stdint.h>
#include <lcom/lcf.h>
#include "sprites.h"

/** @brief Width of one glyph in the source sprite sheet (pixels). */
#define FONT_CHAR_W  8
/** @brief Height of one glyph in the source sprite sheet (pixels). */
#define FONT_CHAR_H  12
/** @brief Number of glyph columns in the sprite sheet. */
#define FONT_COLS    16
/** @brief ASCII code of the first glyph in the sprite sheet. */
#define FONT_FIRST   32

/**
 * @brief Loads the font sprite sheet from XPM data into memory.
 *
 * Must be called once before any @ref draw_char or @ref draw_string call.
 * Safe to call multiple times; subsequent calls are no-ops if the font
 * is already loaded.
 */
void font_init(void);

/**
 * @brief Returns a pointer to the loaded font sprite sheet.
 *
 * The returned pointer is valid for the lifetime of the application
 * after @ref font_init has been called.
 *
 * @return Pointer to the font @ref sprite_t, or @c NULL if the font has
 *         not yet been loaded.
 */
sprite_t *font_get_sprite(void);

/**
 * @brief Returns @c true if the font sprite sheet has been successfully loaded.
 */
bool font_is_loaded(void);

/**
 * @brief Draws a single ASCII character into the framebuffer.
 *
 * Characters outside the printable ASCII range (< @ref FONT_FIRST or
 * beyond the last glyph) are silently ignored.
 *
 * @param c     Character to draw.
 * @param x     X pixel coordinate of the glyph's top-left corner.
 * @param y     Y pixel coordinate of the glyph's top-left corner.
 * @param color 24-bit RGB colour applied to non-transparent pixels.
 * @param scale Uniform integer scaling factor (1 = native size).
 */
void draw_char(char c, uint16_t x, uint16_t y, uint32_t color, uint8_t scale);

/**
 * @brief Draws a null-terminated ASCII string into the framebuffer.
 *
 * Characters are laid out left-to-right with no automatic line-wrapping.
 * Each glyph advances the pen by <tt>FONT_CHAR_W × scale</tt> pixels.
 *
 * @param s     Null-terminated string to render.
 * @param x     X pixel coordinate of the first glyph's top-left corner.
 * @param y     Y pixel coordinate of the first glyph's top-left corner.
 * @param color 24-bit RGB colour applied to non-transparent pixels.
 * @param scale Uniform integer scaling factor (1 = native size).
 */
void draw_string(const char *s, uint16_t x, uint16_t y, uint32_t color, uint8_t scale);
