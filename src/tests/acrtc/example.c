/**
 * example.c - bring up the HD63484 at 0xAA0000 for 640x480, 1 bit/pixel and
 * print some text.
 *
 * Assumptions (change to suit your board):
 *   - one ACRTC, memory-mapped at HD63484_BASE_ADDR (0xAA0000), 16-bit bus
 *   - VGA-style 640x480 timing. The 2CLK input and the external shift-register
 *     clocking must be arranged so that one memory cycle outputs 16 pixels at
 *     1 bpp / GAI +1 / single access.
 *
 * 68030 note: the ACRTC's registers must NOT be cached, or status polling can
 * return stale data and the driver will time out. Make the window
 * cache-inhibited in hardware (/CIIN) or in your MMU tables. See
 * hd63484_m68030.h before using its software fallbacks.
 */
#include <stdio.h>
#include <stddef.h>
#include "hd63484.h"
#include "hd63484_text.h"

int main(void)
{
    hd63484_config_t cfg;
    hd63484_crt_timing_t crt = {
        .h_active = 640, .h_front = 16, .h_sync = 96, .h_back = 48,
        .v_active = 480, .v_front = 10, .v_sync = 2,  .v_back = 33,
    };
    hd63484_surface_t surf;
    hd63484_text_t    txt;
    int rc;

    /* --- describe the display --- */
    hd63484_config_default(&cfg);
    cfg.gbm = HD63484_GBM_1BPP;
    cfg.gai = HD63484_GAI_1;
    cfg.acm = HD63484_ACM_SINGLE;

    rc = hd63484_config_set_timing(&cfg, &crt);
    if (rc) {
        printf("timing: %s\n", hd63484_strerror(rc));
        return 1;
    }

    /* Frame buffer: base screen at word address 0, 40 words (640 px) per line */
    cfg.screen[HD63484_SCREEN_BASE].start_addr = 0;
    cfg.screen[HD63484_SCREEN_BASE].mem_width  =
            hd63484_mem_width_words(640, cfg.gbm);

    /* --- bring the chip up --- */
    rc = hd63484_init(&cfg);
    if (rc) {
        printf("init failed: %s\n", hd63484_strerror(rc));
        return 1;
    }

    /* --- text output --- */
    rc = hd63484_surface_from_config(&surf, &cfg, HD63484_SCREEN_BASE);
    if (!rc)
        rc = hd63484_text_init(&txt, &surf, NULL);   /* built-in 8x8 font */
    if (!rc)
        rc = hd63484_text_clear(&txt);               /* fills with background */
    if (rc) {
        printf("text setup failed: %s\n", hd63484_strerror(rc));
        return 1;
    }

    /* At 1 bpp: fg = 1, bg = 0. On colour modes pass pixel values instead. */
    hd63484_text_set_colors(&txt, 1, 0);
    hd63484_text_puts(&txt, "HD63484 ACRTC\n");
    hd63484_text_printf(&txt, "%ux%u, %u cols x %u rows\n",
                        surf.width, surf.height, txt.cols, txt.rows);

    /* Big text at an arbitrary pixel position; doesn't move the cursor */
    hd63484_text_set_scale(&txt, 3);
    hd63484_text_draw(&txt, 20, 100, "Scaled 3x");
    hd63484_text_set_scale(&txt, 1);

    /* Overlay with a transparent background */
    hd63484_text_set_transparent(&txt, true);
    hd63484_text_draw(&txt, 23, 103, "Scaled 3x");

    return 0;
}