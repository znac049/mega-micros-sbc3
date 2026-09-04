/*
 * example.c -- two worked examples of using mc68030_mmu.
 *
 * This is example/reference code for a bare-metal 68030 target -- it
 * will not link or run as-is on a hosted OS. Build it as part of your
 * own bare-metal image (see README.md / Makefile).
 *
 * Example 1 (recommended starting point): enable the MMU using ONLY
 * the transparent translation registers (TT0/TT1). This requires no
 * page tables at all -- every logical address in the window is passed
 * straight through to the same physical address. It is the lowest-
 * risk way to turn the MMU on (e.g. to get write-protection or cache
 * control on a region) without needing byte-exact page/table
 * descriptors.
 *
 * Example 2: a minimal single-level paged mapping, showing how the
 * descriptor-building helpers fit together. Double-check the
 * descriptor status bits against the manual before trusting this on
 * real hardware (see the header file's caveats).
 */

#include <stddef.h>
#include <stdio.h>
#include <ctype.h>

#include "mmu.h"

/* ---------------------------------------------------------------------
 * Example 1: transparent identity mapping of all of physical memory
 * via TT0, with the MMU otherwise enabled but no page tables set up.
 * (Root pointer left invalid -- fine, because every access we care
 * about is caught by the transparent window and never reaches the
 * table walker.)
 * --------------------------------------------------------------------- */
void
example_transparent_identity_map(void)
{
    m68030_tt_config_t tt0 = {
        .logical_base = 0x00, /* compare against A31-A24 == 0x00      */
        .logical_mask = 0xFF, /* ignore ALL address-base bits         */
        .enable       = 1,
        .low_bits     = 0,    /* default: any FC, read+write, cached  */
    };

    /* Window covering the entire 4GB space, always transparent. */
    m68030_set_tt0(m68030_make_tt(&tt0));

    /* Leave TT1 disabled. */
    m68030_set_tt1(0);

    m68030_flush_atc_all();
    m68030_mmu_enable();

    /* At this point every logical address == physical address, but
     * the MMU is formally "on" (E bit set in TC), which is what some
     * software / debuggers check for, and you get the CI/WP controls
     * available on the TT low bits if you set them. */
}

/* ---------------------------------------------------------------------
 * Example 2: a minimal single-level table mapping the low 16MB with
 * 32Kbyte pages (the confirmed-good page size value from real 68030
 * boot code), all pages resident, read-write, cacheable.
 *
 * With PS=15 (32K pages) and, say, TIA=9 (root table indexes bits
 * 31-23 of the logical address directly to a page descriptor -- i.e.
 * a single-level tree), IS=0, TIB=TIC=TID=0: 0+9+0+0+0+15 = 24, which
 * is short of 32 -- the remaining 8 bits (address bits 22-15, plus
 * the low 15 bits as page offset) need accounting for. Getting this
 * arithmetic right for YOUR desired page size/table depth is the main
 * design step; the IS+TIA+TIB+TIC+TID+PS==32 identity from the
 * manual (confirmed against the real TC=0x80F04445 example: IS=0,
 * TIA=TIB=TIC=4, TID=5, PS=15 -> 0+4+4+4+5+15=32) is your check.
 *
 * Below we reuse that exact confirmed-working configuration (a
 * 4-level tree, 32K pages) rather than inventing new numbers, and
 * build only the first couple of levels for illustration -- extend
 * the pattern for TIB/TIC/TID tables in your own allocator.
 * --------------------------------------------------------------------- */

/* Tables must be aligned; a static array gives us a fixed, known
 * address to point descriptors at. Real code will normally allocate
 * these dynamically from a physical memory pool set up by your boot
 * code. Alignment/placement below is illustrative only. */
static uint32_t a_table[16] __attribute__((aligned(64))); /* TIA=4 -> 16 entries */

void
example_paged_setup(uint32_t b_table_phys_addr)
{
    m68030_tc_config_t tc_cfg = {
        .enable         = 1,
        .page_size_exp  = 15, /* 32K pages, confirmed-valid value    */
        .initial_shift  = 0,
        .tia            = 4,
        .tib            = 4,
        .tic            = 4,
        .tid            = 5,
    };

    /* Point every A-table entry at the same B-table for this simple
     * example (i.e. treat TIA as "don't care" -- fine for a small
     * identity-style map; a real OS would fill each entry
     * differently). */
    for (int i = 0; i < 16; i++) {
        a_table[i] = m68030_make_table_descriptor(
            b_table_phys_addr, M68030_DT_TABLE_SHORT,
            /*write_protect=*/0, /*supervisor_only=*/0);
    }

    m68030_root_ptr_t crp =
        m68030_make_root_ptr((uint32_t)(uintptr_t)a_table,
                              M68030_DT_TABLE_SHORT, /*limit_is_lower=*/1);
    m68030_set_crp(crp);

    m68030_set_tc(m68030_make_tc(&tc_cfg));

    /* No transparent windows in this example -- everything must
     * resolve through the table tree. */
    m68030_set_tt0(0);
    m68030_set_tt1(0);

    m68030_flush_atc_all();
    m68030_mmu_enable();

    /* Sanity-check a translation before trusting the rest of the
     * system to use it: */
    uint16_t status = m68030_ptest(/*logical_addr=*/0x00000000,
                                    /*fc=*/5 /* supervisor data */,
                                    /*level=*/7, /*is_write=*/0);
    if (status & M68030_MMUSR_I) {
        /* Translation invalid -- table setup needs fixing before you
         * rely on it. Handle/report this in your boot code. */
    }
}

int
main(void)
{
    example_transparent_identity_map();
    /* example_paged_setup(some_b_table_physical_address); */

    printf("Hello, virtual world!\n");
    getchar();

    m68030_mmu_disable();
    return 0;
}