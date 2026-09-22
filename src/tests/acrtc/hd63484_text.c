/**
 * hd63484_text.c - software text renderer for the HD63484 ACRTC
 * See hd63484_text.h for documentation.
 */
#include "hd63484_text.h"

#include <string.h>

#ifndef HD63484_TEXT_NO_PRINTF
#include <stdarg.h>
#include <stdio.h>
#endif

#define CHUNK_MAX 32u   /* max characters composed in one pass */

/* ------------------------------------------------------------------------- */
/* Pixel format helpers                                                      */
/* ------------------------------------------------------------------------- */
static unsigned bpp_of(const hd63484_surface_t *s)
{
    return 1u << (unsigned)s->gbm;
}

static uint16_t color_mask(const hd63484_surface_t *s)
{
    return (uint16_t)(((uint32_t)1u << bpp_of(s)) - 1u);
}

/* Bit position of pixel `slot` (0 = leftmost) within a 16-bit word. */
static unsigned pix_shift(const hd63484_surface_t *s, unsigned slot)
{
    unsigned bpp = bpp_of(s);

    return (s->order == HD63484_PIX_MSB_FIRST) ? 16u - bpp * (slot + 1u)
                                               : bpp * slot;
}

/* One word filled with a single colour. */
static uint16_t word_pattern(const hd63484_surface_t *s, uint16_t color)
{
    unsigned ppw = 16u / bpp_of(s), slot;
    uint32_t cm = color_mask(s), w = 0;

    for (slot = 0; slot < ppw; slot++)
        w |= ((uint32_t)color & cm) << pix_shift(s, slot);
    return (uint16_t)w;
}

static unsigned words_per_line(const hd63484_surface_t *s)
{
    return ((unsigned)s->width * bpp_of(s) + 15u) / 16u;
}

/* ------------------------------------------------------------------------- */
/* Glyph lookup                                                              */
/* ------------------------------------------------------------------------- */
static const uint8_t *glyph_for(const hd63484_font_t *f, unsigned char c)
{
    unsigned rowbytes = ((unsigned)f->width + 7u) / 8u;

    if (c < f->first || c > f->last) {
        c = '?';
        if (c < f->first || c > f->last)
            return NULL;                       /* blank */
    }
    return f->data + (size_t)(c - f->first) * f->height * rowbytes;
}

/* Longest run of characters we can compose in a single pass. */
static unsigned max_run(const hd63484_surface_t *s, const hd63484_font_t *f,
                        unsigned scale)
{
    unsigned ppw = 16u / bpp_of(s);
    unsigned cw  = (unsigned)f->width * scale;
    unsigned avail = HD63484_TEXT_BUF_WORDS * ppw;
    unsigned m;

    if (!cw || avail < ppw)
        return 0;
    m = (avail - (ppw - 1u)) / cw;
    return m > CHUNK_MAX ? CHUNK_MAX : m;
}

/* ------------------------------------------------------------------------- */
/* Core: compose and write one run of glyphs                                 */
/* ------------------------------------------------------------------------- */
/* A NULL entry in glyphs[] draws a blank cell. Runs may start at any pixel x.
 * Words touched only partly (edges) or every word (transparent mode) are read
 * first so existing pixels survive. */
static int draw_run(hd63484_text_t *t, unsigned x, unsigned y,
                    const uint8_t *const *glyphs, unsigned n)
{
    const hd63484_surface_t *s = &t->surf;
    const hd63484_font_t *f = t->font;
    const unsigned scale = t->scale;
    const unsigned cw = (unsigned)f->width * scale;
    const unsigned ch = (unsigned)f->height * scale;
    const unsigned rowbytes = ((unsigned)f->width + 7u) / 8u;
    const unsigned bpp = bpp_of(s);
    const unsigned ppw = 16u / bpp;
    const uint32_t cmask = color_mask(s);
    const uint32_t fg = t->fg & cmask;
    const uint32_t bg = t->bg & cmask;
    uint16_t buf[HD63484_TEXT_BUF_WORDS];
    unsigned end_px, first_word, lead, tail, last_word, nwords;
    unsigned gy, i, px, k;
    int rc = HD63484_OK;

    if (!n)
        return HD63484_OK;

    end_px     = x + n * cw;
    first_word = x / ppw;
    lead       = x % ppw;
    tail       = end_px % ppw;
    last_word  = (end_px - 1u) / ppw;
    nwords     = last_word - first_word + 1u;
    if (nwords > HD63484_TEXT_BUF_WORDS)
        return HD63484_ERR_PARAM;

    for (gy = 0; gy < ch; gy++) {
        unsigned yy = y + gy;
        unsigned gr = gy / scale;
        uint32_t addr;

        if (yy >= s->height)
            break;                                  /* clip at bottom */
        addr = s->base_addr + (uint32_t)yy * s->mem_width + first_word;
        memset(buf, 0, nwords * sizeof buf[0]);

        /* Fetch pixels that must be preserved */
        if (t->transparent) {
            rc = hd63484_read_words(s->dn, addr, buf, nwords);
        } else {
            if (lead)
                rc = hd63484_read_words(s->dn, addr, &buf[0], 1);
            if (rc == HD63484_OK && tail && (nwords > 1u || !lead))
                rc = hd63484_read_words(s->dn, addr + nwords - 1u,
                                        &buf[nwords - 1u], 1);
        }
        if (rc)
            return rc;

        /* Expand glyph pixels into the word buffer */
        for (i = 0; i < n; i++) {
            const uint8_t *g = glyphs[i];
            uint16_t bits = 0;

            if (g) {
                bits = (uint16_t)((unsigned)g[gr * rowbytes] << 8);
                if (rowbytes > 1u)
                    bits |= g[gr * rowbytes + 1u];
            }
            for (px = 0; px < cw; px++) {
                bool on = ((bits >> (15u - px / scale)) & 1u) != 0;
                unsigned p, wi, sh;
                uint32_t m;

                if (!on && t->transparent)
                    continue;
                p  = x + i * cw + px;
                wi = p / ppw - first_word;
                sh = pix_shift(s, p % ppw);
                m  = cmask << sh;
                buf[wi] = (uint16_t)((buf[wi] & ~m) | (((on ? fg : bg) << sh) & m));
            }
        }

        /* Write the raster back: set RWP once, then one WT per word */
        rc = hd63484_set_rwp(s->dn, addr, 0);
        for (k = 0; rc == HD63484_OK && k < nwords; k++)
            rc = hd63484_send_cmd(HD63484_CMD_WT, &buf[k], 1);
        if (rc)
            return rc;
    }
    return HD63484_OK;
}

/* ------------------------------------------------------------------------- */
/* Region helpers                                                            */
/* ------------------------------------------------------------------------- */
static int fill_rows(hd63484_text_t *t, unsigned y0, unsigned nrows)
{
    const hd63484_surface_t *s = &t->surf;
    uint16_t pat = word_pattern(s, t->bg);
    unsigned wpl = words_per_line(s), y;
    int rc = HD63484_OK;

    for (y = y0; rc == HD63484_OK && y < y0 + nrows && y < s->height; y++)
        rc = hd63484_fill_words(s->dn,
                                s->base_addr + (uint32_t)y * s->mem_width,
                                pat, wpl);
    return rc;
}

/* Scroll the text area up by one text row and clear the freed row. */
static int scroll_up(hd63484_text_t *t)
{
    const hd63484_surface_t *s = &t->surf;
    unsigned th = (unsigned)t->font->height * t->scale;
    unsigned text_h = (unsigned)t->rows * th;
    unsigned wpl = words_per_line(s), y, off, n;
    uint16_t buf[HD63484_TEXT_BUF_WORDS];
    int rc = HD63484_OK;

    for (y = th; rc == HD63484_OK && y < text_h; y++) {
        uint32_t src = s->base_addr + (uint32_t)y * s->mem_width;
        uint32_t dst = s->base_addr + (uint32_t)(y - th) * s->mem_width;

        for (off = 0; rc == HD63484_OK && off < wpl; off += n) {
            n = wpl - off;
            if (n > HD63484_TEXT_BUF_WORDS)
                n = HD63484_TEXT_BUF_WORDS;
            rc = hd63484_read_words(s->dn, src + off, buf, n);
            if (rc == HD63484_OK)
                rc = hd63484_write_words(s->dn, dst + off, buf, n);
        }
    }
    return rc ? rc : fill_rows(t, text_h - th, th);
}

static int newline(hd63484_text_t *t)
{
    t->col = 0;
    if (t->row + 1u < t->rows) {
        t->row++;
        return HD63484_OK;
    }
    if (t->scroll) {
        t->row = (uint16_t)(t->rows - 1u);
        return scroll_up(t);
    }
    t->row = 0;                                    /* wrap to top, clear line */
    return fill_rows(t, 0, (unsigned)t->font->height * t->scale);
}

/* ------------------------------------------------------------------------- */
/* Setup                                                                     */
/* ------------------------------------------------------------------------- */
int hd63484_surface_from_config(hd63484_surface_t *s,
                                const hd63484_config_t *c, unsigned screen)
{
    const hd63484_screen_t *sc;
    unsigned ppw, mw_px, vis, ppmc;

    if (!s || !c || screen > 3u)
        return HD63484_ERR_PARAM;
    sc = &c->screen[screen];
    if (!sc->enable)
        return HD63484_ERR_PARAM;
    if (sc->character)
        return HD63484_ERR_UNSUPPORTED;

    ppw   = 16u >> (unsigned)c->gbm;
    mw_px = (unsigned)sc->mem_width * ppw;
    ppmc  = hd63484_pixels_per_mcycle(c->gbm, c->gai, c->acm);
    vis   = ppmc ? (unsigned)(screen == HD63484_SCREEN_WINDOW ? c->hww : c->hdw) * ppmc
                 : mw_px;

    s->dn        = (uint8_t)screen;
    s->base_addr = sc->start_addr;
    s->mem_width = sc->mem_width;
    s->gbm       = c->gbm;
    s->order     = HD63484_PIX_LSB_FIRST;
    s->width     = (uint16_t)(vis < mw_px ? vis : mw_px);
    s->height    = (screen == HD63484_SCREEN_WINDOW) ? c->vww : sc->height;
    return HD63484_OK;
}

/* Recompute the character grid; leaves *t untouched on failure. */
static int apply_geometry(hd63484_text_t *t, const hd63484_font_t *f,
                          unsigned scale)
{
    unsigned cw, ch, cols, rows;

    if (!f || !f->data || f->width < 1 || f->width > 16 || f->height < 1 ||
        f->last < f->first || scale < 1u || scale > 32u)
        return HD63484_ERR_PARAM;
    if (max_run(&t->surf, f, scale) == 0)
        return HD63484_ERR_PARAM;                 /* glyph too wide for buffer */

    cw = (unsigned)f->width * scale;
    ch = (unsigned)f->height * scale;
    cols = t->surf.width / cw;
    rows = t->surf.height / ch;
    if (!cols || !rows)
        return HD63484_ERR_PARAM;

    t->font  = f;
    t->scale = (uint8_t)scale;
    t->cols  = (uint16_t)cols;
    t->rows  = (uint16_t)rows;
    if (t->col > t->cols) t->col = t->cols;
    if (t->row >= t->rows) t->row = (uint16_t)(t->rows - 1u);
    return HD63484_OK;
}

int hd63484_text_init(hd63484_text_t *t, const hd63484_surface_t *surf,
                      const hd63484_font_t *font)
{
    if (!t || !surf || (unsigned)surf->gbm > HD63484_GBM_16BPP ||
        !surf->width || !surf->height)
        return HD63484_ERR_PARAM;

    memset(t, 0, sizeof *t);
    t->surf   = *surf;
    t->scroll = true;
    t->fg     = color_mask(surf);
    t->bg     = 0;
    return apply_geometry(t, font ? font : &hd63484_font_8x8, 1u);
}

int hd63484_text_set_font(hd63484_text_t *t, const hd63484_font_t *font)
{
    return apply_geometry(t, font ? font : &hd63484_font_8x8, t->scale);
}

int hd63484_text_set_scale(hd63484_text_t *t, unsigned scale)
{
    return apply_geometry(t, t->font, scale);
}

void hd63484_text_set_colors(hd63484_text_t *t, uint16_t fg, uint16_t bg)
{
    uint16_t m = color_mask(&t->surf);

    t->fg = fg & m;
    t->bg = bg & m;
}

void hd63484_text_set_transparent(hd63484_text_t *t, bool v) { t->transparent = v; }
void hd63484_text_set_scroll(hd63484_text_t *t, bool v)      { t->scroll = v; }

void hd63484_text_set_cursor(hd63484_text_t *t, unsigned col, unsigned row)
{
    t->col = (uint16_t)(col >= t->cols ? t->cols - 1u : col);
    t->row = (uint16_t)(row >= t->rows ? t->rows - 1u : row);
}

static int finish(int rc)
{
    return rc ? rc : hd63484_wait_idle();
}

int hd63484_text_clear(hd63484_text_t *t)
{
    int rc = fill_rows(t, 0, t->surf.height);

    t->col = t->row = 0;
    return finish(rc);
}

/* ------------------------------------------------------------------------- */
/* Output                                                                    */
/* ------------------------------------------------------------------------- */
static bool is_ctrl(unsigned char c)
{
    return c == '\n' || c == '\r' || c == '\b' || c == '\t';
}

static int write_impl(hd63484_text_t *t, const char *s, size_t len)
{
    const unsigned cw = (unsigned)t->font->width * t->scale;
    const unsigned ch = (unsigned)t->font->height * t->scale;
    const unsigned maxrun = max_run(&t->surf, t->font, t->scale);
    size_t i = 0;
    int rc = HD63484_OK;

    while (rc == HD63484_OK && i < len) {
        unsigned char c = (unsigned char)s[i];
        const uint8_t *g[CHUNK_MAX];
        unsigned n = 0;

        switch (c) {
        case '\n':
            rc = newline(t);
            i++;
            continue;
        case '\r':
            t->col = 0;
            i++;
            continue;
        case '\b':
            if (t->col)
                t->col--;
            i++;
            continue;
        case '\t': {
            const uint8_t *blank[8] = {0};
            unsigned gap;

            if (t->col >= t->cols)
                rc = newline(t);
            if (rc == HD63484_OK) {
                gap = 8u - (t->col % 8u);
                if (gap > (unsigned)(t->cols - t->col))
                    gap = t->cols - t->col;
                rc = draw_run(t, (unsigned)t->col * cw, (unsigned)t->row * ch,
                              blank, gap);
                t->col = (uint16_t)(t->col + gap);
            }
            i++;
            continue;
        }
        default:
            break;
        }

        if (t->col >= t->cols) {                   /* lazy wrap */
            rc = newline(t);
            if (rc)
                break;
        }
        while (i + n < len && n < maxrun && t->col + n < t->cols &&
               !is_ctrl((unsigned char)s[i + n])) {
            g[n] = glyph_for(t->font, (unsigned char)s[i + n]);
            n++;
        }
        rc = draw_run(t, (unsigned)t->col * cw, (unsigned)t->row * ch, g, n);
        t->col = (uint16_t)(t->col + n);
        i += n;
    }
    return rc;
}

int hd63484_text_write(hd63484_text_t *t, const char *s, size_t len)
{
    if (!t || (!s && len))
        return HD63484_ERR_PARAM;
    return finish(write_impl(t, s, len));
}

int hd63484_text_puts(hd63484_text_t *t, const char *s)
{
    if (!t || !s)
        return HD63484_ERR_PARAM;
    return hd63484_text_write(t, s, strlen(s));
}

int hd63484_text_putc(hd63484_text_t *t, char c)
{
    return hd63484_text_write(t, &c, 1);
}

#ifndef HD63484_TEXT_NO_PRINTF
int hd63484_text_printf(hd63484_text_t *t, const char *fmt, ...)
{
    char buf[HD63484_TEXT_PRINTF_BUF];
    va_list ap;
    int n, rc;

    if (!t || !fmt)
        return HD63484_ERR_PARAM;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n < 0)
        return HD63484_ERR_PARAM;
    if ((size_t)n >= sizeof buf)
        n = (int)sizeof buf - 1;

    rc = hd63484_text_write(t, buf, (size_t)n);
    return rc ? rc : n;
}
#endif

int hd63484_text_draw(hd63484_text_t *t, unsigned x, unsigned y, const char *s)
{
    unsigned cw, maxrun;
    int rc = HD63484_OK;

    if (!t || !s)
        return HD63484_ERR_PARAM;
    cw     = (unsigned)t->font->width * t->scale;
    maxrun = max_run(&t->surf, t->font, t->scale);

    while (rc == HD63484_OK && *s && x + cw <= t->surf.width &&
           y < t->surf.height) {
        const uint8_t *g[CHUNK_MAX];
        unsigned n = 0;

        while (s[n] && n < maxrun && x + (n + 1u) * cw <= t->surf.width) {
            g[n] = glyph_for(t->font, (unsigned char)s[n]);
            n++;
        }
        rc = draw_run(t, x, y, g, n);
        s += n;
        x += n * cw;
    }
    return finish(rc);
}