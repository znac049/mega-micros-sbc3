/*
 * mc68030_mmu.c -- implementation of the MC68030 PMMU register access
 * layer declared in mc68030_mmu.h.
 *
 * All MMU registers are accessed with the PMOVE instruction, which
 * (in GNU as / Motorola-syntax mode) takes the form:
 *
 *     pmove  <ea>, %mmu_register      (load the register)
 *     pmove  %mmu_register, <ea>      (store the register)
 *
 * <ea> must be a memory operand for the 64-bit CRP/SRP registers (no
 * 64-bit general register pair addressing mode exists for them), so
 * we route everything through memory operands ("m" constraints) for
 * uniformity. This requires the assembler to be invoked with 68030 or
 * 68851 MMU support enabled, e.g.:
 *
 *     m68k-elf-gcc -m68030 -Wa,-m68030 -c mc68030_mmu.c
 *
 * NOTE ON MMUSR REGISTER NAME: older/most binutils releases spell the
 * MMU status register operand "%psr" in PMOVE (a holdover from the
 * MC68851 name for the same register slot; the MC68030 renames it
 * MMUSR but reuses the encoding). If your assembler instead wants
 * "%mmusr", change PSR_REG_NAME below.
 */

#include "mmu.h"

#define PSR_REG_NAME "%%psr"

void
m68030_mmu_enable(void)
{
    uint32_t tc = m68030_get_tc();
    tc |= M68030_TC_E;
    m68030_set_tc(tc);
}

void
m68030_mmu_disable(void)
{
    uint32_t tc = m68030_get_tc();
    tc &= ~M68030_TC_E;
    m68030_set_tc(tc);
}

void
m68030_set_crp(m68030_root_ptr_t rp)
{
    __asm__ volatile ("pmove %0,%%crp" : : "m"(rp) : "memory");
}

void
m68030_set_srp(m68030_root_ptr_t rp)
{
    __asm__ volatile ("pmove %0,%%srp" : : "m"(rp) : "memory");
}

m68030_root_ptr_t
m68030_get_crp(void)
{
    m68030_root_ptr_t rp;
    __asm__ volatile ("pmove %%crp,%0" : "=m"(rp) : : "memory");
    return rp;
}

m68030_root_ptr_t
m68030_get_srp(void)
{
    m68030_root_ptr_t rp;
    __asm__ volatile ("pmove %%srp,%0" : "=m"(rp) : : "memory");
    return rp;
}

void
m68030_set_tc(uint32_t tc)
{
    __asm__ volatile ("pmove %0,%%tc" : : "m"(tc) : "memory");
}

uint32_t
m68030_get_tc(void)
{
    uint32_t tc;
    __asm__ volatile ("pmove %%tc,%0" : "=m"(tc) : : "memory");
    return tc;
}

void
m68030_set_tt0(uint32_t tt)
{
    __asm__ volatile ("pmove %0,%%tt0" : : "m"(tt) : "memory");
}

void
m68030_set_tt1(uint32_t tt)
{
    __asm__ volatile ("pmove %0,%%tt1" : : "m"(tt) : "memory");
}

uint32_t
m68030_get_tt0(void)
{
    uint32_t tt;
    __asm__ volatile ("pmove %%tt0,%0" : "=m"(tt) : : "memory");
    return tt;
}

uint32_t
m68030_get_tt1(void)
{
    uint32_t tt;
    __asm__ volatile ("pmove %%tt1,%0" : "=m"(tt) : : "memory");
    return tt;
}

uint16_t
m68030_get_mmusr(void)
{
    uint16_t sr;
    __asm__ volatile ("pmove " PSR_REG_NAME ",%0" : "=m"(sr) : : "memory");
    return sr;
}

void
m68030_flush_atc_all(void)
{
    __asm__ volatile ("pflusha" ::: "memory");
}

/*
 * PFLUSH and PTEST encode their "mask" (PFLUSH) / "level" (PTEST)
 * operand as a 3-bit *immediate* field only -- the assembler (tested
 * against GNU as 2.42, m68k-linux-gnu, -m68030) rejects a data
 * register in that slot, even though it accepts one for FC. Since
 * these are true machine-level immediates and not just an assembler
 * preference, there is no way to pass an arbitrary runtime value
 * there other than picking the matching immediate at compile time.
 * We do that with a small dispatch over the only 8 possible values
 * (mask/level are both 3-bit fields, 0..7). FC, by contrast, is
 * accepted as a data register, so it is passed through normally.
 */
#define M68030_PFLUSH_CASE(fcv, addr, m) \
    case (m): __asm__ volatile ("pflush %0,#" #m ",(%1)" \
                                 : : "d"(fcv), "a"(addr) : "memory"); break

void
m68030_flush_atc(uint32_t logical_addr, uint8_t fc, uint8_t fc_mask)
{
    unsigned fcv = fc & 7u;
    switch (fc_mask & 7u) {
        M68030_PFLUSH_CASE(fcv, logical_addr, 0);
        M68030_PFLUSH_CASE(fcv, logical_addr, 1);
        M68030_PFLUSH_CASE(fcv, logical_addr, 2);
        M68030_PFLUSH_CASE(fcv, logical_addr, 3);
        M68030_PFLUSH_CASE(fcv, logical_addr, 4);
        M68030_PFLUSH_CASE(fcv, logical_addr, 5);
        M68030_PFLUSH_CASE(fcv, logical_addr, 6);
        M68030_PFLUSH_CASE(fcv, logical_addr, 7);
    }
}
#undef M68030_PFLUSH_CASE

#define M68030_PTESTR_CASE(fcv, addr, l) \
    case (l): __asm__ volatile ("ptestr %0,(%1),#" #l \
                                 : : "d"(fcv), "a"(addr) : "memory"); break
#define M68030_PTESTW_CASE(fcv, addr, l) \
    case (l): __asm__ volatile ("ptestw %0,(%1),#" #l \
                                 : : "d"(fcv), "a"(addr) : "memory"); break

uint16_t
m68030_ptest(uint32_t logical_addr, uint8_t fc, int level, int is_write)
{
    unsigned fcv = fc & 7u;
    if (is_write) {
        switch ((unsigned)level & 7u) {
            M68030_PTESTW_CASE(fcv, logical_addr, 0);
            M68030_PTESTW_CASE(fcv, logical_addr, 1);
            M68030_PTESTW_CASE(fcv, logical_addr, 2);
            M68030_PTESTW_CASE(fcv, logical_addr, 3);
            M68030_PTESTW_CASE(fcv, logical_addr, 4);
            M68030_PTESTW_CASE(fcv, logical_addr, 5);
            M68030_PTESTW_CASE(fcv, logical_addr, 6);
            M68030_PTESTW_CASE(fcv, logical_addr, 7);
        }
    } else {
        switch ((unsigned)level & 7u) {
            M68030_PTESTR_CASE(fcv, logical_addr, 0);
            M68030_PTESTR_CASE(fcv, logical_addr, 1);
            M68030_PTESTR_CASE(fcv, logical_addr, 2);
            M68030_PTESTR_CASE(fcv, logical_addr, 3);
            M68030_PTESTR_CASE(fcv, logical_addr, 4);
            M68030_PTESTR_CASE(fcv, logical_addr, 5);
            M68030_PTESTR_CASE(fcv, logical_addr, 6);
            M68030_PTESTR_CASE(fcv, logical_addr, 7);
        }
    }
    return m68030_get_mmusr();
}
#undef M68030_PTESTR_CASE
#undef M68030_PTESTW_CASE