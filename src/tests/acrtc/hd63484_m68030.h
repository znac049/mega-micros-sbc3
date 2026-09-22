/**
 * hd63484_m68030.h - optional Motorola 68030 helpers for the HD63484 driver
 *
 * WHY THIS EXISTS
 * The 68030 has a 256-byte on-chip data cache. If the ACRTC's registers are
 * cacheable, a status-register poll can be served from the cache instead of
 * the chip, so flags such as WFR/CED never appear to change and the driver
 * ends up in HD63484_ERR_TIMEOUT. `volatile` does not help: it constrains the
 * compiler, not the CPU cache.
 *
 * The right fix is to make the I/O window cache-inhibited, in this order of
 * preference:
 *   1. Hardware: address decode asserts /CIIN for the ACRTC's range.
 *   2. Your OS/MMU tables map the range with the CI bit set.
 *   3. Bare metal: use a transparent-translation register (hd63484_m68030_
 *      tt_inhibit below).
 *   4. Last resort: turn the data cache off altogether
 *      (hd63484_m68030_dcache_off) - this slows the whole program down.
 *
 * WARNING for the default base address 0x00AA0000: the transparent-translation
 * registers compare only address bits 31-24, so hd63484_m68030_tt_inhibit()
 * would mark the ENTIRE range 0x000000-0xFFFFFF non-cacheable - almost
 * certainly including your RAM and ROM, which cripples performance. Use it
 * only if the whole 16 MB block is I/O. Otherwise prefer /CIIN from the
 * address decoder or page-level cache inhibit in your MMU tables.
 *
 * All functions here execute privileged instructions (movec / pmove), so they
 * must run in supervisor mode. Do NOT use the TT helper if an OS is managing
 * the MMU; ask the OS to map the region cache-inhibited instead.
 *
 * Only compiled for GCC targeting the 68030 (-m68030, defines __mc68030__).
 */
#ifndef HD63484_M68030_H
#define HD63484_M68030_H

#include <stdint.h>

#if defined(__GNUC__) && defined(__mc68030__)

/* 68030 Cache Control Register bits */
#define HD63484_CACR_EI   (1u << 0)   /* enable instruction cache          */
#define HD63484_CACR_CI   (1u << 3)   /* clear instruction cache (strobe)  */
#define HD63484_CACR_ED   (1u << 8)   /* enable data cache                 */
#define HD63484_CACR_CED  (1u << 10)  /* clear data cache entry (strobe)   */
#define HD63484_CACR_CD   (1u << 11)  /* clear data cache (strobe)         */

static inline uint32_t hd63484_m68030_get_cacr(void)
{
    uint32_t v;
    __asm__ volatile ("movec %%cacr,%0" : "=d"(v));
    return v;
}

static inline void hd63484_m68030_set_cacr(uint32_t v)
{
    __asm__ volatile ("movec %0,%%cacr" : : "d"(v) : "memory");
}

/** Flush the data cache (invalidates all entries; write-through, so no data
 *  is lost). */
static inline void hd63484_m68030_dcache_flush(void)
{
    hd63484_m68030_set_cacr(hd63484_m68030_get_cacr() | HD63484_CACR_CD);
}

/** Disable and flush the data cache. Returns the previous CACR value for
 *  hd63484_m68030_cacr_restore(). */
static inline uint32_t hd63484_m68030_dcache_off(void)
{
    uint32_t old = hd63484_m68030_get_cacr();

    hd63484_m68030_set_cacr((old & ~HD63484_CACR_ED) | HD63484_CACR_CD);
    return old;
}

static inline void hd63484_m68030_cacr_restore(uint32_t old)
{
    hd63484_m68030_set_cacr(old & ~(HD63484_CACR_CI | HD63484_CACR_CED |
                                    HD63484_CACR_CD));
}

/**
 * Mark the 16 MB block containing `addr` as cache-inhibited using
 * transparent-translation register TT0 or TT1 (`tt` = 0 or 1), matching all
 * function codes and both read and write. The 68030 TT registers compare only
 * address bits 31-24, so the whole 16 MB block is affected: put the ACRTC in
 * an I/O window, not in the middle of RAM. The data cache is flushed so stale
 * copies of the region are dropped.
 */
static inline void hd63484_m68030_tt_inhibit(unsigned tt, uint32_t addr)
{
    /* base[31:24] | mask[23:16]=0 | E | CI | RWM (ignore R/W) | FC mask = 7 */
    uint32_t v = (addr & 0xFF000000u) | 0x8000u | 0x0400u | 0x0100u | 0x0007u;

    if (tt == 0u)
        __asm__ volatile ("pmove %0,%%tt0" : : "m"(v) : "memory");
    else
        __asm__ volatile ("pmove %0,%%tt1" : : "m"(v) : "memory");
    hd63484_m68030_dcache_flush();
}

#endif /* __GNUC__ && __mc68030__ */
#endif /* HD63484_M68030_H */