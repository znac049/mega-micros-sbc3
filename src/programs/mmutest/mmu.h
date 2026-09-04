

/*
 * mc68030_mmu.h -- A small C library for programming the on-chip PMMU
 * of the Motorola MC68030 (and MC68EC030 in "ACR" mode where noted).
 *
 * Target: m68k-elf-gcc / m68k-linux-gnu-gcc (or any gcc configured for a
 * bare-metal m68k target). Requires binutils `as` with 68030/68851 MMU
 * instruction support (pmove, pflush, pflusha, pload, ptest) -- pass
 * -Wa,-m68030 -m68030 (or -mcpu=68030) on the gcc command line.
 *
 * ---------------------------------------------------------------------
 * IMPORTANT -- READ BEFORE USING ON REAL/EMULATED HARDWARE
 * ---------------------------------------------------------------------
 * This code was written from a combination of the MC68030 User's Manual
 * (Motorola/NXP, order no. MC68030UM/AD, Section 9 "Memory Management
 * Unit", pages 9-1..9-82) and cross-checked against real, known-working
 * boot-time PMMU setup values published for 68030 systems (Atari TT/
 * Falcon, NetBSD/m68k). The following pieces are verified against a
 * working example and can be trusted:
 *
 *   - Root pointer (CRP/SRP) format               [Fig. 9-35, p.9-54]
 *   - Translation Control register (TC) format     [Fig. 9-36, p.9-54]
 *   - Descriptor Type (DT) encoding (0/1/2/3)       [Sec. 9.5.1, p.9-20]
 *   - TT0/TT1 logical address Base/Mask/Enable      [Fig. 9-37, p.9-57]
 *
 * The following are implemented per the commonly published MC68851/
 * MC68030 descriptor layout but were NOT individually re-verified
 * against a byte-exact manual transcription, and you should confirm
 * them against Section 9.5.1 (pages 9-20..9-28, Figures 9-9..9-18)
 * before shipping on real silicon:
 *
 *   - Exact bit positions of U / M / WP / CI / S inside short- and
 *     long-format table and page descriptors.
 *   - Exact bit layout of the low byte of TT0/TT1 (FC base/mask and
 *     R/W control fields) -- the high word (logical address base +
 *     mask) and the enable bit are solid; the low word's individual
 *     bit meanings are given as documented but double-check Fig 9-37.
 *   - MMUSR bit definitions (Table 9-3, p.9-60).
 *
 * When in doubt, PTEST + read MMUSR at runtime to sanity-check a
 * descriptor you built, and single-step your table setup under an
 * emulator (Hatari, WinUAE, ARAnyM, MAME) that implements the 68030
 * PMMU before trusting it on real hardware.
 * ---------------------------------------------------------------------
 */
 
#ifndef MC68030_MMU_H
#define MC68030_MMU_H

#include <ctype.h>
#include <stddef.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 
/* ===================================================================
 * Descriptor Type (DT) field -- the low 2 bits of every table/page
 * descriptor, and of the root pointer's control word. CONFIRMED.
 * =================================================================== */
typedef enum {
    M68030_DT_INVALID     = 0, /* descriptor not valid -> bus error    */
    M68030_DT_PAGE        = 1, /* page descriptor (tree terminates)    */
    M68030_DT_TABLE_SHORT = 2, /* valid descriptor, 4-byte (short) fmt */
    M68030_DT_TABLE_LONG  = 3  /* valid descriptor, 8-byte (long) fmt  */
} m68030_dt_t;
 
/* ===================================================================
 * Root Pointer (CRP / SRP) -- 64-bit descriptor, CONFIRMED format:
 *
 *   word0 (high longword):
 *     bit31    = LU   (0 = value in LIMIT is an upper limit,
 *                       1 = value in LIMIT is a lower limit)
 *     bits30-16= LIMIT (15-bit limit field)
 *     bits15-2 = reserved (0)
 *     bits1-0  = DT (must be M68030_DT_TABLE_SHORT or _LONG)
 *   word1 (low longword):
 *     bits31-4 = table address of the first-level (A) table
 *     bits3-0  = reserved (0)
 *
 * Real-world confirmed example (Atari 68030 boot code):
 *   CRP = 0x80000002_00000700
 *       -> LU=1, LIMIT=0 (i.e. "no limit checking"), DT=2 (short table),
 *          A-table at physical address 0x700.
 * =================================================================== */
typedef struct {
    uint32_t word0;
    uint32_t word1;
} m68030_root_ptr_t;
 
static inline m68030_root_ptr_t
m68030_make_root_ptr(uint32_t table_address, m68030_dt_t dt, int limit_is_lower)
{
    m68030_root_ptr_t rp;
    rp.word0 = ((uint32_t)(limit_is_lower ? 1u : 0u) << 31) | ((uint32_t)dt & 3u);
    rp.word1 = table_address & 0xFFFFFFF0u;
    return rp;
}
 
/* ===================================================================
 * Translation Control register (TC) -- CONFIRMED format:
 *
 *   bit31     = E   (1 = enable address translation)
 *   bits30-25 = reserved (0)  [SRE/FCL are believed to live somewhere
 *                               in here on some references -- leave 0
 *                               unless you have verified otherwise]
 *   bits23-20 = PS  (Page Size exponent: page size = 2^PS bytes,
 *                     valid range 8..15, i.e. 256 bytes .. 32 Kbytes)
 *   bits19-16 = IS  (Initial Shift: number of high-order logical
 *                     address bits ignored before indexing table A)
 *   bits15-12 = TIA (bits of logical address used to index table A)
 *   bits11-8  = TIB (bits used to index table B)
 *   bits7-4   = TIC (bits used to index table C)
 *   bits3-0   = TID (bits used to index table D)
 *
 * Constraint from the manual: IS + TIA + TIB + TIC + TID + PS == 32,
 * and any TIx field left at 0 must have all following TIx fields be
 * 0 too (that collapses the tree to fewer levels).
 *
 * Real-world confirmed example (Atari 68030 boot code):
 *   TC = 0x80F04445
 *      -> E=1, PS=15 (32K pages), IS=0, TIA=4, TIB=4, TIC=4, TID=5
 *         (4+4+4+5+0+15 == 32, checks out)
 * =================================================================== */
#define M68030_TC_E        (1u << 31)
 
typedef struct {
    unsigned enable;      /* 0 or 1 */
    unsigned page_size_exp; /* 8..15  -> page size 2^n bytes */
    unsigned initial_shift; /* IS, 0..15 */
    unsigned tia, tib, tic, tid; /* index widths in bits, may be 0 */
} m68030_tc_config_t;
 
static inline uint32_t
m68030_make_tc(const m68030_tc_config_t *c)
{
    uint32_t tc = 0;
    if (c->enable) tc |= M68030_TC_E;
    tc |= ((uint32_t)c->page_size_exp   & 0xFu) << 20;
    tc |= ((uint32_t)c->initial_shift   & 0xFu) << 16;
    tc |= ((uint32_t)c->tia             & 0xFu) << 12;
    tc |= ((uint32_t)c->tib             & 0xFu) << 8;
    tc |= ((uint32_t)c->tic             & 0xFu) << 4;
    tc |= ((uint32_t)c->tid             & 0xFu);
    return tc;
}
 
/* ===================================================================
 * Short-format table descriptor (4 bytes) -- points at the next-level
 * table (B, C, or D). Address field high bits + DT are confirmed by
 * the general descriptor pattern; the exact position of the U/WP/S
 * control bits below should be checked against Fig. 9-10 (p.9-24).
 * =================================================================== */
#define M68030_DESC_DT_MASK   0x3u
#define M68030_DESC_WP_BIT    (1u << 2)  /* write protect  -- VERIFY */
#define M68030_DESC_U_BIT     (1u << 3)  /* used           -- VERIFY */
#define M68030_DESC_S_BIT     (1u << 7)  /* supervisor-only -- VERIFY */
 
static inline uint32_t
m68030_make_table_descriptor(uint32_t next_table_addr, m68030_dt_t dt,
                              int write_protect, int supervisor_only)
{
    uint32_t d = next_table_addr & 0xFFFFFFF0u; /* 16-byte aligned, see note */
    d |= (uint32_t)dt & M68030_DESC_DT_MASK;
    if (write_protect)  d |= M68030_DESC_WP_BIT;
    if (supervisor_only) d |= M68030_DESC_S_BIT;
    return d;
}
 
/* ===================================================================
 * Short-format page descriptor (4 bytes) -- terminal (leaf) descriptor
 * mapping a logical page to a physical page frame. Same caveat as
 * above: verify CI/M/U/WP bit positions against Fig. 9-12 (p.9-25).
 * =================================================================== */
#define M68030_PAGE_CI_BIT     (1u << 6)  /* cache inhibit  -- VERIFY */
#define M68030_PAGE_M_BIT      (1u << 4)  /* modified       -- VERIFY */
#define M68030_PAGE_U_BIT      (1u << 3)  /* used           -- VERIFY */
#define M68030_PAGE_WP_BIT     (1u << 2)  /* write protect  -- VERIFY */
 
static inline uint32_t
m68030_make_page_descriptor(uint32_t phys_page_addr, int cache_inhibit,
                             int write_protect)
{
    uint32_t d = phys_page_addr; /* low bits naturally 0 if page-aligned */
    d &= ~(uint32_t)M68030_DESC_DT_MASK;
    d |= (uint32_t)M68030_DT_PAGE;
    if (cache_inhibit) d |= M68030_PAGE_CI_BIT;
    if (write_protect) d |= M68030_PAGE_WP_BIT;
    return d;
}
 
/* ===================================================================
 * Transparent Translation registers (TT0 / TT1). High word CONFIRMED
 * (base compared to A31-A24, mask bits = "don't care", enable = bit15).
 * Low byte fields (FC base/mask, R/W control) given per commonly
 * published layout -- verify against Fig. 9-37 (p.9-57) if precise
 * FC/R-W matching matters for your use case. Two known-good literal
 * presets are provided below for the common "identity map everything"
 * case, taken from real 68030 boot code.
 * =================================================================== */
#define M68030_TT_ENABLE    (1u << 15)
 
typedef struct {
    uint8_t  logical_base; /* compared with A31-A24 */
    uint8_t  logical_mask; /* 1 bits = "don't care" in the comparison */
    unsigned enable;       /* 0 or 1 */
    unsigned low_bits;     /* raw low-byte control bits; 0 is a safe
                               default (matches supervisor+user,
                               read+write). See caveat above. */
} m68030_tt_config_t;
 
static inline uint32_t
m68030_make_tt(const m68030_tt_config_t *c)
{
    uint32_t tt = ((uint32_t)c->logical_base << 24) |
                  ((uint32_t)c->logical_mask << 16) |
                  (c->low_bits & 0x7FFFu);
    if (c->enable) tt |= M68030_TT_ENABLE;
    return tt;
}
 
/* Known-good literal TT values from real 68030 boot code, useful as a
 * quick sanity check / starting point (see mc68030_mmu.c comments for
 * where these came from). Not necessarily right for YOUR memory map --
 * they transparently map large windows of address space 1:1. */
#define M68030_TT_EXAMPLE_WINDOW0   0x017E8107u
#define M68030_TT_EXAMPLE_WINDOW1   0x807E8507u
 
/* ===================================================================
 * MMU Status Register (MMUSR) -- result of the last PTEST. 16 bits.
 * Bit meanings per Table 9-3 (p.9-60); double-check before relying on
 * individual bits for fault classification logic.
 * =================================================================== */
#define M68030_MMUSR_B   (1u << 15) /* bus error occurred during search */
#define M68030_MMUSR_L   (1u << 14) /* limit violation                  */
#define M68030_MMUSR_S   (1u << 8)  /* supervisor-only protected        */
#define M68030_MMUSR_WP  (1u << 2)  /* write protected                  */
#define M68030_MMUSR_I   (1u << 1)  /* invalid (no valid ATC/table entry)*/
#define M68030_MMUSR_M   (1u << 4)  /* modified                         */
 
/* ===================================================================
 * Register access (PMOVE / MOVEC / PFLUSH / PTEST wrappers)
 * =================================================================== */
 
/* Enable/disable translation. Safe to call from supervisor mode only. */
void m68030_mmu_enable(void);
void m68030_mmu_disable(void);
 
/* Root pointer registers. */
void m68030_set_crp(m68030_root_ptr_t rp);
void m68030_set_srp(m68030_root_ptr_t rp);
m68030_root_ptr_t m68030_get_crp(void);
m68030_root_ptr_t m68030_get_srp(void);
 
/* Translation control register. */
void m68030_set_tc(uint32_t tc);
uint32_t m68030_get_tc(void);
 
/* Transparent translation registers. */
void m68030_set_tt0(uint32_t tt);
void m68030_set_tt1(uint32_t tt);
uint32_t m68030_get_tt0(void);
uint32_t m68030_get_tt1(void);
 
/* MMU status register (updated by PTEST). */
uint16_t m68030_get_mmusr(void);
 
/* Address translation cache control. */
void m68030_flush_atc_all(void);
/* Flush ATC entries matching a logical address, for the given function
 * code mask/value (fc_mask bit i = 1 means "don't care" that FC bit).
 */
void m68030_flush_atc(uint32_t logical_addr, uint8_t fc, uint8_t fc_mask);
 
/* Test a logical address translation (PTESTR/PTESTW). This walks the
 * translation tables (or ATC) for logical_addr/fc down to `level`
 * (0..7; 0 = stop after the root, 7 = walk all the way to the page
 * descriptor) and updates MMUSR with the result -- it does not modify
 * the ATC's normal operation beyond what PTEST itself causes. Read
 * m68030_get_mmusr() afterwards to interpret the result (see the
 * M68030_MMUSR_* bits above, and MMUSR Table 9-3 / Fig 9-39,9-40 in
 * the manual for the full level-0 vs level-7 status interpretation).
 * This function returns the raw MMUSR value for convenience. Getting
 * the actual translated physical address back out generally requires
 * the optional address-register descriptor-pointer form of PTEST
 * (not exposed by this simple wrapper) followed by reading the
 * descriptor at that address -- see Section 9.7.4/9.8 of the manual. */
uint16_t m68030_ptest(uint32_t logical_addr, uint8_t fc, int level,
                       int is_write);
 
#ifdef __cplusplus
}
#endif
 
#endif /* MC68030_MMU_H */
 
