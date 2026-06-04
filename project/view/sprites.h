#pragma once

/**
 * @file sprites.h
 * @brief Sprite loading, drawing, cursor management, and animated sprites.
 *
 * Provides a thin abstraction over raw XPM pixel data.  A @ref sprite_t
 * holds a decoded RGBA colour buffer that can be blitted to the
 * framebuffer with optional 90° rotation.
 *
 * The @ref animated_sprite_t type wraps a sequence of up to eight
 * @ref sprite_t frames and advances through them one at a time on each
 * call to @ref anim_sprite_update.
 *
 * The cursor subsystem maintains two pre-loaded cursor sprites (normal
 * and hover) and renders the active one at an arbitrary pixel position.
 */
 
/**
 * @brief Sentinel colour value treated as fully transparent.
 *
 * Any pixel in a sprite whose colour equals this value is skipped during
 * blitting so the background shows through.
 */
#define TRANSPARENT 0xFFFFFE
 
/**
 * @brief A decoded, heap-allocated sprite ready for blitting.
 */
typedef struct {
    uint32_t *colors; /**< Flat array of @c width × @c height 24-bit RGB values. */
    uint16_t  width;  /**< Sprite width in pixels. */
    uint16_t  height; /**< Sprite height in pixels. */
} sprite_t;
 
/**
 * @brief Decodes an XPM pixel map into a heap-allocated @ref sprite_t.
 *
 * @param xpm XPM map to decode (as returned by the LCF layer).
 * @return Pointer to the new sprite, or @c NULL on allocation failure.
 *         The caller owns the returned object and must free it with
 *         @ref sprite_destroy when done.
 */
sprite_t *sprite_load(xpm_map_t xpm);
 
/**
 * @brief Blits a sprite to the framebuffer at the given pixel coordinates.
 *
 * Pixels whose colour equals @ref TRANSPARENT are not written.
 *
 * @param sp Sprite to draw.
 * @param x  Pixel X of the sprite's top-left corner.
 * @param y  Pixel Y of the sprite's top-left corner.
 */
void sprite_draw(sprite_t *sp, uint16_t x, uint16_t y);
 
/**
 * @brief Frees the colour buffer and the @ref sprite_t struct itself.
 *
 * @param sp Sprite to destroy.  Passing @c NULL is safe (no-op).
 */
void sprite_destroy(sprite_t *sp);
 
/**
 * @brief Blits a sprite, optionally applying a 90° clockwise rotation.
 *
 * Used when drawing vertically-oriented ships whose sprites are stored
 * horizontally.
 *
 * @param sp     Sprite to draw.
 * @param x      Pixel X of the (post-rotation) top-left corner.
 * @param y      Pixel Y of the (post-rotation) top-left corner.
 * @param rotate @c true to rotate 90° clockwise before blitting.
 */
void sprite_draw_rotated(sprite_t *sp, uint16_t x, uint16_t y, bool rotate);
 
/**
 * @brief Cursor appearance modes.
 */
typedef enum {
    CURSOR_NORMAL, /**< Default arrow cursor. */
    CURSOR_HOVER,  /**< Highlighted cursor shown over interactive elements. */
} cursor_mode_t;
 
/**
 * @brief Loads the normal and hover cursor sprites into memory.
 *
 * Must be called once before @ref cursor_draw.
 */
void cursor_init(void);
 
/**
 * @brief Switches the active cursor sprite.
 *
 * @param mode @ref CURSOR_NORMAL or @ref CURSOR_HOVER.
 */
void cursor_set_mode(cursor_mode_t mode);
 
/**
 * @brief Draws the active cursor sprite centred at the given pixel position.
 *
 * @param x Cursor hotspot X in screen pixels.
 * @param y Cursor hotspot Y in screen pixels.
 */
void cursor_draw(uint16_t x, uint16_t y);
 
/**
 * @brief A sequence of sprite frames that animate in a loop.
 *
 * Frames are stored as an array of pointers to individually decoded
 * @ref sprite_t objects.  Call @ref anim_sprite_update each tick to
 * advance @c cur_pixmap and @ref anim_sprite_draw to blit the current
 * frame.
 */
typedef struct {
    uint8_t   no_pixmaps;    /**< Total number of frames in the sequence (max 8). */
    uint8_t   cur_pixmap;    /**< Index of the frame to draw next.                */
    sprite_t *pixmaps[8];    /**< Per-frame sprite pointers.                      */
    uint16_t  x;             /**< Pixel X of the animation's top-left corner.     */
    uint16_t  y;             /**< Pixel Y of the animation's top-left corner.     */
} animated_sprite_t;
 
/**
 * @brief Allocates and initialises an @ref animated_sprite_t with no frames.
 *
 * Frames must be loaded into @c pixmaps[] manually after creation.
 *
 * @param x          Initial pixel X position.
 * @param y          Initial pixel Y position.
 * @param no_pixmaps Number of frames to allocate space for (max 8).
 * @return Pointer to the new animated sprite, or @c NULL on failure.
 */
animated_sprite_t *anim_sprite_create(uint16_t x, uint16_t y, uint8_t no_pixmaps);
 
/**
 * @brief Advances the animated sprite to the next frame.
 *
 * Increments @c cur_pixmap, wrapping around to 0 after the last frame.
 *
 * @param anim Animated sprite to update.
 */
void anim_sprite_update(animated_sprite_t *anim);
 
/**
 * @brief Draws the current frame of the animated sprite.
 *
 * Blits @c pixmaps[cur_pixmap] at the sprite's stored @c x / @c y
 * position.
 *
 * @param anim Animated sprite to draw.
 */
void anim_sprite_draw(animated_sprite_t *anim);
 
/**
 * @brief Frees all frame sprites and the @ref animated_sprite_t itself.
 *
 * @param anim Animated sprite to destroy.
 */
void anim_sprite_destroy(animated_sprite_t *anim);
 
/**
 * @brief Frees all game sprite objects allocated by @ref init_game_sprites.
 *
 * Should be called during game shutdown to avoid memory leaks.
 */
void destroy_game_sprites(void);
