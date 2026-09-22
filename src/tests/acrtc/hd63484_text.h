/**
 * hd63484_text.h - text output for the HD63484 ACRTC (graphic screens)
 *
 * The ACRTC has no built-in font, so text is rendered in software: glyph
 * bitmaps are expanded to the frame buffer's pixel format (1/2/4/8/16 bpp)
 * and written through the Read/Write Pointer with WT commands. Text can be
 * placed at any pixel position; partial words at the edges of a run are
 * handled with read-modify-write, so neighbouring pixels are preserved.
 *
 * Features: built-in 8x8 ASCII font (or your own), integer scaling, opaque or
 * transparent background, cursor with \n \r \b \t handling, line wrap,
 * scrolling (software copy) or wrap-to-top, printf.
 *
 * Requires hd63484.h (single-device version). Character-mode (CHR) screens
 * are not supported: those need an external character generator instead.
 *
 * Typical use:
 *     hd63484_surface_t surf;
 *     hd63484_text_t    txt;
 *     hd63484_surface_from_config(&surf, &cfg, HD63484_SCREEN_BASE);
 *     hd63484_text_init(&txt, &surf, NULL);              // NULL = 8x8 font
 *     hd63484_text_clear(&txt);
 *     hd63484_text_printf(&txt, "Hello %d\n", 42);
 */
#ifndef HD63484_TEXT_H
#define HD63484_TEXT_H

#include <ctype.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include <stdint.h>

#include "hd63484.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Words of stack used as a scratch row buffer. Larger = longer runs per
 *  pass at high bit depths. Must be >= 2. */
#ifndef HD63484_TEXT_BUF_WORDS
#define HD63484_TEXT_BUF_WORDS 128
#endif

/** Buffer size used by hd63484_text_printf(). Output beyond it is truncated.
 *  Define HD63484_TEXT_NO_PRINTF to drop printf support (no <stdio.h>). */
#ifndef HD63484_TEXT_PRINTF_BUF
#define HD63484_TEXT_PRINTF_BUF 128
#endif

/* ------------------------------------------------------------------------- */
/* Fonts                                                                     */
/* ------------------------------------------------------------------------- */
typedef struct {
    uint8_t        width;   /* glyph width in pixels, 1..16                   */
    uint8_t        height;  /* glyph height in pixels                         */
    uint8_t        first;   /* first character code covered                   */
    uint8_t        last;    /* last character code covered (inclusive)        */
    /** (last-first+1) glyphs, each `height` rows of ceil(width/8) bytes.
     *  MSB of the first byte of a row is the leftmost pixel; set bit = ink. */
    const uint8_t *data;
} hd63484_font_t;

/** Built-in 8x8 font, ASCII 0x20..0x7E (public domain, see .c file). */
extern const hd63484_font_t hd63484_font_8x8;

/* ------------------------------------------------------------------------- */
/* Drawing surface                                                           */
/* ------------------------------------------------------------------------- */
/** Order of pixels inside a 16-bit frame buffer word. */
typedef enum {
    /** Pixel 0 (leftmost) in the least significant bits. This matches how the
     *  ACRTC's own drawing commands place dots (per MAME's model) [VERIFY].
     *  If text appears left/right mirrored inside each word, switch to
     *  HD63484_PIX_MSB_FIRST. */
    HD63484_PIX_LSB_FIRST = 0,
    /** Pixel 0 in the most significant bits. */
    HD63484_PIX_MSB_FIRST = 1
} hd63484_pixel_order_t;

typedef struct {
    uint8_t               dn;         /* screen number 0-3 (goes into RWP)    */
    uint32_t              base_addr;  /* word address of pixel (0,0)          */
    uint16_t              mem_width;  /* words per raster                     */
    hd63484_gbm_t         gbm;        /* bits per pixel                       */
    hd63484_pixel_order_t order;
    uint16_t              width;      /* usable pixels per raster             */
    uint16_t              height;     /* usable rasters                       */
} hd63484_surface_t;

/**
 * Derive a surface from the config that was passed to hd63484_init().
 * Width is the visible width (display width x pixels per memory cycle) capped
 * at the memory width; height is the screen height (window: VWW).
 * Start dot (horizontal scroll offset) is ignored.
 */
int hd63484_surface_from_config(hd63484_surface_t *surf,
                                const hd63484_config_t *cfg, unsigned screen);

/* ------------------------------------------------------------------------- */
/* Text context                                                              */
/* ------------------------------------------------------------------------- */
typedef struct {
    hd63484_surface_t       surf;
    const hd63484_font_t   *font;
    uint16_t                fg, bg;        /* pixel values (low `bpp` bits)   */
    bool                    transparent;   /* don't paint background pixels   */
    bool                    scroll;        /* true: scroll; false: wrap to top */
    uint8_t                 scale;         /* integer magnification, >= 1     */
    uint16_t                col, row;      /* cursor, in character cells      */
    uint16_t                cols, rows;    /* grid size for current font/scale */
} hd63484_text_t;

/**
 * Initialise a text context. `font` may be NULL for the built-in 8x8 font.
 * Defaults: foreground = all ones, background = 0, opaque, scale 1,
 * scrolling on, cursor at (0,0). Nothing is drawn.
 */
int hd63484_text_init(hd63484_text_t *t, const hd63484_surface_t *surf,
                      const hd63484_font_t *font);

/* Settings (cell grid is recomputed; the cursor is clamped) */
int  hd63484_text_set_font(hd63484_text_t *t, const hd63484_font_t *font);
int  hd63484_text_set_scale(hd63484_text_t *t, unsigned scale);
void hd63484_text_set_colors(hd63484_text_t *t, uint16_t fg, uint16_t bg);
void hd63484_text_set_transparent(hd63484_text_t *t, bool transparent);
void hd63484_text_set_scroll(hd63484_text_t *t, bool scroll);
void hd63484_text_set_cursor(hd63484_text_t *t, unsigned col, unsigned row);

/** Fill the surface with the background colour and home the cursor. */
int hd63484_text_clear(hd63484_text_t *t);

/**
 * Character output at the cursor. Control characters: '\n' (CR+LF),
 * '\r', '\b' (cursor left, no erase), '\t' (advance to next multiple of 8,
 * painting the gap). Characters missing from the font show as '?'.
 * Lines wrap at the right edge. Past the last row the screen scrolls by one
 * text line, or wraps to the top and clears that line if scrolling is off.
 * All functions wait for the ACRTC to finish before returning.
 */
int hd63484_text_putc(hd63484_text_t *t, char c);
int hd63484_text_puts(hd63484_text_t *t, const char *s);
int hd63484_text_write(hd63484_text_t *t, const char *s, size_t len);

#ifndef HD63484_TEXT_NO_PRINTF
/** printf to the cursor. Returns characters output (>= 0) or a negative
 *  hd63484_err_t. Output longer than HD63484_TEXT_PRINTF_BUF-1 is truncated. */
#if defined(__GNUC__)
__attribute__((format(printf, 2, 3)))
#endif
int hd63484_text_printf(hd63484_text_t *t, const char *fmt, ...);
#endif

/**
 * Draw a string with its top-left corner at pixel (x, y). Does not use or move
 * the cursor, does not interpret control characters, and stops at the right
 * edge of the surface (a glyph that would not fit completely is dropped).
 */
int hd63484_text_draw(hd63484_text_t *t, unsigned x, unsigned y,
                      const char *s);

#ifdef __cplusplus
}
#endif
#endif /* HD63484_TEXT_H */