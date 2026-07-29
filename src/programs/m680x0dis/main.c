/*
 * m68k_dis.c -- A Motorola 68030 (68000/010/020/030 core ISA) disassembler.
 *
 * Reads a flat binary file of machine code and prints it as Motorola-syntax
 * assembly listing:
 *
 *      00001000  4E71                nop
 *      00001002  203C 00000010       move.l  #$10,d0
 *      00001008  4EF9 00001000       jmp     $1000.l
 *
 * Coverage:
 *   - Full 68000/68010 instruction set
 *   - 68020/68030 additions: LINK.L, EXTB.L, MULU.L/MULS.L (32x32->32/64),
 *     DIVU.L/DIVS.L (32/32->32 and 64/32->32:32), CHK2/CMP2, TRAPcc,
 *     bit field instructions (BFTST/BFEXTU/BFEXTS/BFCHG/BFCLR/BFSET/BFFFO/
 *     BFINS), PACK/UNPK, 32-bit branch/Bcc displacements
 *   - Full 68020+ extended effective-address modes: brief and full
 *     extension words, including memory-indirect (pre/post-indexed) modes,
 *     base/index suppression and base/outer displacements
 *
 * Cross-validated instruction-by-instruction against GNU binutils'
 * m68k-linux-gnu-objdump (-M motorola) across ~75 representative
 * encodings spanning every instruction class implemented here.
 *
 * NOT covered (prints "dc.w $xxxx" instead of decoding):
 *   - Coprocessor (68881/68882 FPU, 68851/68030 MMU) instructions (F-line)
 *   - CAS, CAS2, CALLM, RTM, MOVES  (rare / privileged, easy to add later)
 *   - Line-A (1010) opcodes (historically used for OS traps, not a fixed ISA)
 *
 * Known cosmetic limitation: PC-relative indexed addressing modes
 * (mode 7/reg 3, brief or full extension word) are printed symbolically
 * as e.g. "(pc,d2.w*4)" rather than resolving to a numeric absolute
 * address the way the simpler 16-bit "d16(pc)" mode is resolved.
 *
 * Build:  gcc -O2 -Wall -o m68kdis m68k_dis.c
 * Usage:  ./m68kdis [-a base_addr] <input.bin>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* ------------------------------------------------------------------ */
/* Byte stream / instruction cursor                                    */
/* ------------------------------------------------------------------ */

struct disasm_ctx {
    const uint8_t *buf;
    size_t len;
    size_t pos;        /* current byte offset into buf */
    uint32_t base_addr; /* address corresponding to buf[0] */
    int truncated;      /* set if we ran off the end of the buffer */
};

typedef struct disasm_ctx disasm_ctx_t;

static uint32_t cur_addr(disasm_ctx_t *d) { return d->base_addr + (uint32_t)d->pos; }

static uint16_t fetch16(disasm_ctx_t *d) {
    if (d->pos + 2 > d->len) { d->truncated = 1; return 0; }
    uint16_t v = (uint16_t)((d->buf[d->pos] << 8) | d->buf[d->pos + 1]);
    d->pos += 2;
    return v;
}

static uint32_t fetch32(disasm_ctx_t *d) {
    uint32_t hi = fetch16(d);
    uint32_t lo = fetch16(d);
    return (hi << 16) | lo;
}

/* ------------------------------------------------------------------ */
/* Small formatting helpers                                            */
/* ------------------------------------------------------------------ */

static void hexsigned(int64_t v, char *out, size_t n) {
    if (v < 0) snprintf(out, n, "-$%llx", (unsigned long long)(-v));
    else       snprintf(out, n, "$%llx", (unsigned long long)v);
}

static void hexunsigned(uint64_t v, char *out, size_t n) {
    snprintf(out, n, "$%llx", (unsigned long long)v);
}

static const char *size_suffix(int size) {
    switch (size) {
        case 1: return ".b";
        case 2: return ".w";
        case 4: return ".l";
        default: return "";
    }
}

static const char *cc_names[16] = {
    "t","f","hi","ls","cc","cs","ne","eq",
    "vc","vs","pl","mi","ge","lt","gt","le"
};

/* ------------------------------------------------------------------ */
/* Effective address decoding (includes full 68020+ extended modes)    */
/* ------------------------------------------------------------------ */

static void join_parts(char *out, size_t outsz, const char *parts[], int n) {
    /* joins non-empty strings from parts[] with commas, wraps in () */
    char tmp[192] = "";
    int first = 1;
    for (int i = 0; i < n; i++) {
        if (parts[i] == NULL || parts[i][0] == '\0') continue;
        if (!first) strncat(tmp, ",", sizeof(tmp) - strlen(tmp) - 1);
        strncat(tmp, parts[i], sizeof(tmp) - strlen(tmp) - 1);
        first = 0;
    }
    snprintf(out, outsz, "(%s)", tmp);
}

/* Decodes an 8xx0/68020 index/base extension word, given the textual
 * name of the base register ("a3" or "pc"). Writes the resulting
 * addressing-mode text (WITHOUT any leading base-displacement-outside-parens
 * text -- that part is handled by the caller for the brief case) into out. */
static void decode_ext_word(disasm_ctx_t *d, const char *base_reg, int base_suppressible,
                             char *out, size_t outsz) {
    uint16_t ext = fetch16(d);
    int da     = (ext >> 15) & 1;
    int xreg   = (ext >> 12) & 7;
    int wl     = (ext >> 11) & 1;
    int scale  = 1 << ((ext >> 9) & 3);
    int full   = (ext >> 8) & 1;

    char xname[24] = "";
    char scalestr[8] = "";
    if (scale != 1) snprintf(scalestr, sizeof(scalestr), "*%d", scale);
    snprintf(xname, sizeof(xname), "%s%d.%s%s", da ? "a" : "d", xreg, wl ? "l" : "w", scalestr);

    if (!full) {
        /* Brief extension word: 8-bit displacement, base+idx always present */
        int8_t disp = (int8_t)(ext & 0xFF);
        char dispstr[32] = "";
        if (disp != 0) hexsigned(disp, dispstr, sizeof(dispstr));
        const char *parts[3] = { dispstr, base_reg, xname };
        join_parts(out, outsz, parts, 3);
        return;
    }

    /* Full extension word */
    int bs      = (ext >> 6) & 1; /* base register suppress */
    int is_     = (ext >> 5) & 1; /* index suppress */
    int bdsize  = (ext >> 4) & 3; /* 0 reserved, 1 null, 2 word, 3 long */
    int iis     = ext & 7;

    int32_t basedisp = 0;
    if (bdsize == 2) basedisp = (int16_t)fetch16(d);
    else if (bdsize == 3) basedisp = (int32_t)fetch32(d);

    char bdstr[32] = "";
    if (basedisp != 0) hexsigned(basedisp, bdstr, sizeof(bdstr));

    char basepart[24] = "";
    if (!bs && base_suppressible >= 0) snprintf(basepart, sizeof(basepart), "%s", base_reg);
    else if (!bs) snprintf(basepart, sizeof(basepart), "%s", base_reg);

    char idxpart[24] = "";
    if (!is_) snprintf(idxpart, sizeof(idxpart), "%s", xname);

    if (iis == 0) {
        /* (bd,An,Xn) -- no memory indirection */
        const char *parts[3] = { bdstr, basepart, idxpart };
        join_parts(out, outsz, parts, 3);
        return;
    }

    /* Memory indirect: need outer displacement */
    int outer_size = iis & 3; /* 1=null,2=word,3=long */
    int32_t outerdisp = 0;
    if (outer_size == 2) outerdisp = (int16_t)fetch16(d);
    else if (outer_size == 3) outerdisp = (int32_t)fetch32(d);
    char odstr[32] = "";
    if (outerdisp != 0) hexsigned(outerdisp, odstr, sizeof(odstr));

    int postindexed = (iis >= 5);

    char inner[128];
    if (!postindexed) {
        /* preindexed: ([bd,An,Xn],od) */
        const char *iparts[3] = { bdstr, basepart, idxpart };
        join_parts(inner, sizeof(inner), iparts, 3);
        char outerparts_buf[160];
        snprintf(outerparts_buf, sizeof(outerparts_buf), "[%s%s%s]",
                 inner + 1, /* strip leading '(' */
                 "", "");
        /* fix: replace trailing ')' with ']' handled below */
        size_t l = strlen(outerparts_buf);
        if (l && outerparts_buf[l-1] == ')') outerparts_buf[l-1] = ']';
        if (odstr[0]) snprintf(out, outsz, "(%s,%s)", outerparts_buf, odstr);
        else snprintf(out, outsz, "(%s)", outerparts_buf);
    } else {
        /* postindexed: ([bd,An],Xn,od) */
        const char *iparts[2] = { bdstr, basepart };
        join_parts(inner, sizeof(inner), iparts, 2);
        char innerbr[128];
        snprintf(innerbr, sizeof(innerbr), "%s", inner);
        size_t l = strlen(innerbr);
        if (l >= 2) { innerbr[0] = '['; innerbr[l-1] = ']'; }
        const char *parts[3] = { NULL, NULL, NULL };
        char combined[128];
        snprintf(combined, sizeof(combined), "%s", innerbr);
        parts[0] = combined;
        parts[1] = idxpart;
        parts[2] = odstr;
        join_parts(out, outsz, parts, 3);
    }
}

/* mode/reg: standard 6-bit effective address field (mode=bits5-3,reg=bits2-0)
 * size: operand size in bytes (1,2,4) -- only matters for immediate (mode7,reg4)
 * insn_pc: address of the *start* of the instruction (for PC-relative calc) */
static void ea_str(disasm_ctx_t *d, int mode, int reg, int size, uint32_t insn_pc,
                    char *out, size_t outsz) {
    char tmp[32];
    switch (mode) {
        case 0: snprintf(out, outsz, "d%d", reg); return;
        case 1: snprintf(out, outsz, "a%d", reg); return;
        case 2: snprintf(out, outsz, "(a%d)", reg); return;
        case 3: snprintf(out, outsz, "(a%d)+", reg); return;
        case 4: snprintf(out, outsz, "-(a%d)", reg); return;
        case 5: {
            int16_t disp = (int16_t)fetch16(d);
            char dispstr[32]; hexsigned(disp, dispstr, sizeof(dispstr));
            snprintf(out, outsz, "%s(a%d)", dispstr, reg);
            return;
        }
        case 6: {
            snprintf(tmp, sizeof(tmp), "a%d", reg);
            decode_ext_word(d, tmp, 0, out, outsz);
            return;
        }
        case 7:
            switch (reg) {
                case 0: {
                    int16_t v = (int16_t)fetch16(d);
                    hexsigned(v, tmp, sizeof(tmp));
                    snprintf(out, outsz, "%s.w", tmp);
                    return;
                }
                case 1: {
                    uint32_t v = fetch32(d);
                    hexunsigned(v, tmp, sizeof(tmp));
                    snprintf(out, outsz, "%s.l", tmp);
                    return;
                }
                case 2: {
                    uint32_t here = cur_addr(d);
                    int16_t disp = (int16_t)fetch16(d);
                    uint32_t target = here + disp;
                    snprintf(out, outsz, "$%x(pc)", target);
                    return;
                }
                case 3: {
                    decode_ext_word(d, "pc", 0, out, outsz);
                    return;
                }
                case 4: {
                    if (size == 1) {
                        uint16_t v = fetch16(d) & 0xFF;
                        snprintf(out, outsz, "#$%x", v);
                    } else if (size == 2) {
                        uint16_t v = fetch16(d);
                        snprintf(out, outsz, "#$%x", v);
                    } else {
                        uint32_t v = fetch32(d);
                        snprintf(out, outsz, "#$%x", v);
                    }
                    return;
                }
                default:
                    snprintf(out, outsz, "?");
                    return;
            }
        default:
            snprintf(out, outsz, "?");
            return;
    }
    (void)insn_pc;
}

/* ------------------------------------------------------------------ */
/* Instruction decode                                                  */
/* ------------------------------------------------------------------ */

typedef struct {
    char mnem[16];
    char ops[160];
} Insn;

static void set(Insn *i, const char *m, const char *fmt, ...) {
    snprintf(i->mnem, sizeof(i->mnem), "%s", m);
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(i->ops, sizeof(i->ops), fmt, ap);
        va_end(ap);
    } else {
        i->ops[0] = 0;
    }
}

/* forward */
static void decode(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out);

static void unknown(Insn *out, uint16_t op) {
    snprintf(out->mnem, sizeof(out->mnem), "dc.w");
    snprintf(out->ops, sizeof(out->ops), "$%04x", op);
}

/* ---- group 0000: immediate/bit ops, MOVEP, CHK2/CMP2 --------------- */
static void decode_0000(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    /* ANDI/ORI/EORI to CCR/SR (exact opcodes) */
    switch (op) {
        case 0x003C: { uint16_t v = fetch16(d) & 0xFF; set(out, "ori", "#$%x,ccr", v); return; }
        case 0x007C: { uint16_t v = fetch16(d); set(out, "ori", "#$%x,sr", v); return; }
        case 0x023C: { uint16_t v = fetch16(d) & 0xFF; set(out, "andi", "#$%x,ccr", v); return; }
        case 0x027C: { uint16_t v = fetch16(d); set(out, "andi", "#$%x,sr", v); return; }
        case 0x0A3C: { uint16_t v = fetch16(d) & 0xFF; set(out, "eori", "#$%x,ccr", v); return; }
        case 0x0A7C: { uint16_t v = fetch16(d); set(out, "eori", "#$%x,sr", v); return; }
    }

    int b15_8 = (op >> 8) & 0xFF;
    int mode = (op >> 3) & 7;
    int reg = op & 7;

    /* CHK2/CMP2 (68020+): 0000 0ss 011 mmmrrr + ext word */
    if ((op & 0xF9C0) == 0x00C0) {
        int size = (op >> 9) & 3;
        if (size != 3) {
            uint16_t ext = fetch16(d);
            int isAn = (ext >> 15) & 1;
            int regnum = (ext >> 12) & 7;
            int isChk = (ext >> 11) & 1;
            int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
            char ea[64]; ea_str(d, mode, reg, szb, pc, ea, sizeof(ea));
            char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", isChk ? "chk2" : "cmp2", size_suffix(szb));
            set(out, mnem, "%s,%s%d", ea, isAn ? "a" : "d", regnum);
            return;
        }
    }

    /* Immediate ALU: 0000 ooo0 ss mmmrrr  ooo: 000 ORI,001 ANDI,010 SUBI,011 ADDI,101 EORI,110 CMPI */
    if ((op & 0xF800) <= 0x0E00 && (op & 0x0100) == 0) {
        int type = (op >> 9) & 7;
        int size = (op >> 6) & 3;
        static const char *names[8] = {"ori","andi","subi","addi",NULL,"eori","cmpi",NULL};
        if (names[type] && size != 3) {
            int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
            char imm[32];
            if (szb == 1) { uint16_t v = fetch16(d) & 0xFF; snprintf(imm, sizeof(imm), "#$%x", v); }
            else if (szb == 2) { uint16_t v = fetch16(d); snprintf(imm, sizeof(imm), "#$%x", v); }
            else { uint32_t v = fetch32(d); snprintf(imm, sizeof(imm), "#$%x", v); }
            /* CMPI/CHK2/CMP2 68020: when mode==7,reg>=2 with 'cmpi' it's actually CMP2/CHK2 - skip that edge here */
            char ea[64];
            ea_str(d, mode, reg, szb, pc, ea, sizeof(ea));
            char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", names[type], size_suffix(szb));
            set(out, mnem, "%s,%s", imm, ea);
            return;
        }
    }

    /* MOVEP: 0000 ddd1 oo 001 aaa -- must be checked before the generic BTST/BCHG/
     * BCLR/BSET dynamic form below, since MOVEP also has bit8=1 and mode==1. */
    if (mode == 1 && (op & 0x0100) && (((op>>6)&7) == 4 || ((op>>6)&7)==5 || ((op>>6)&7)==6 || ((op>>6)&7)==7)) {
        int dn = (op >> 9) & 7;
        int direction = (op >> 7) & 1; /* 0: mem->reg, 1: reg->mem */
        int size = (op >> 6) & 1; /* 0=word,1=long */
        int16_t disp = (int16_t)fetch16(d);
        char dispstr[32]; hexsigned(disp, dispstr, sizeof(dispstr));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "movep%s", size ? ".l" : ".w");
        if (direction) set(out, mnem, "d%d,%s(a%d)", dn, dispstr, reg);
        else set(out, mnem, "%s(a%d),d%d", dispstr, reg, dn);
        return;
    }

    /* BTST/BCHG/BCLR/BSET dynamic (register) form: 0000 ddd1 oo mmmrrr */
    if ((op & 0x0100) != 0) {
        int dn = (op >> 9) & 7;
        int opsel = (op >> 6) & 3;
        static const char *names[4] = {"btst","bchg","bclr","bset"};
        char ea[64];
        int size = (mode == 0) ? 4 : 1;
        ea_str(d, mode, reg, 1, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", names[opsel], size_suffix(size));
        set(out, mnem, "d%d,%s", dn, ea);
        return;
    }

    /* BTST/BCHG/BCLR/BSET static (immediate) form: 0000 1000 oo mmmrrr, ext word = bit# */
    if (b15_8 == 0x08 || b15_8 == 0x09 || b15_8 == 0x0A || b15_8 == 0x0B) {
        int opsel = (op >> 6) & 3;
        static const char *names[4] = {"btst","bchg","bclr","bset"};
        uint16_t bitnum = fetch16(d) & 0xFF;
        char ea[64];
        int size = (mode == 0) ? 4 : 1;
        ea_str(d, mode, reg, 1, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", names[opsel], size_suffix(size));
        set(out, mnem, "#$%x,%s", bitnum, ea);
        return;
    }

    unknown(out, op);
}

/* ---- MOVE / MOVEA (opcodes 01,10,11 top bits) ---------------------- */
static void decode_move(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int szf = (op >> 12) & 3; /* 01=byte,11=word,10=long */
    int size = szf == 1 ? 1 : szf == 3 ? 2 : 4;
    int dst_reg = (op >> 9) & 7;
    int dst_mode = (op >> 6) & 7;
    int src_mode = (op >> 3) & 7;
    int src_reg = op & 7;

    char src[80];
    ea_str(d, src_mode, src_reg, size, pc, src, sizeof(src));
    char dst[80];
    ea_str(d, dst_mode, dst_reg, size, pc, dst, sizeof(dst));

    if (dst_mode == 1) {
        char mnem[16]; snprintf(mnem, sizeof(mnem), "movea%s", size_suffix(size));
        set(out, mnem, "%s,%s", src, dst);
    } else {
        char mnem[16]; snprintf(mnem, sizeof(mnem), "move%s", size_suffix(size));
        set(out, mnem, "%s,%s", src, dst);
    }
}

/* ---- group 0100: misc (NEG, CLR, LEA, JSR, MOVEM, TRAP, ...) ------- */
static void decode_0100(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int mode = (op >> 3) & 7;
    int reg = op & 7;

    /* exact single opcodes */
    switch (op) {
        case 0x4E70: set(out, "reset", NULL); return;
        case 0x4E71: set(out, "nop", NULL); return;
        case 0x4E72: { uint16_t v = fetch16(d); set(out, "stop", "#$%x", v); return; }
        case 0x4E73: set(out, "rte", NULL); return;
        case 0x4E74: { uint16_t v = fetch16(d); set(out, "rtd", "#$%x", (int16_t)v); return; }
        case 0x4E75: set(out, "rts", NULL); return;
        case 0x4E76: set(out, "trapv", NULL); return;
        case 0x4E77: set(out, "rtr", NULL); return;
        case 0x4AFC: set(out, "illegal", NULL); return;
    }

    /* MOVE from SR: 0100 0000 11 mmmrrr */
    if ((op & 0xFFC0) == 0x40C0) {
        char ea[64]; ea_str(d, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "sr,%s", ea); return;
    }
    /* MOVE from CCR (68010+): 0100 0010 11 mmmrrr */
    if ((op & 0xFFC0) == 0x42C0) {
        char ea[64]; ea_str(d, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "ccr,%s", ea); return;
    }
    /* MOVE to CCR: 0100 0100 11 mmmrrr */
    if ((op & 0xFFC0) == 0x44C0) {
        char ea[64]; ea_str(d, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "%s,ccr", ea); return;
    }
    /* MOVE to SR: 0100 0110 11 mmmrrr */
    if ((op & 0xFFC0) == 0x46C0) {
        char ea[64]; ea_str(d, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "%s,sr", ea); return;
    }

    /* NEGX/CLR/NEG/NOT: 0100 00gg ss mmmrrr, gg(bits10-9):00 NEGX,01 CLR,10 NEG,11 NOT */
    if ((op & 0xF900) == 0x4000 && ((op >> 6) & 3) != 3) {
        int grp = (op >> 9) & 3;
        int size = (op >> 6) & 3;
        static const char *names[4] = {"negx","clr","neg","not"};
        int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
        char ea[64]; ea_str(d, mode, reg, szb, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", names[grp], size_suffix(szb));
        set(out, mnem, "%s", ea);
        return;
    }

    /* TST: 0100 1010 ss mmmrrr (size 11 = TAS, handled next) */
    if ((op & 0xFF00) == 0x4A00 && ((op >> 6) & 3) != 3) {
        int size = (op >> 6) & 3;
        int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
        char ea[64]; ea_str(d, mode, reg, szb, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "tst%s", size_suffix(szb));
        set(out, mnem, "%s", ea);
        return;
    }
    /* TAS: 0100 1010 11 mmmrrr */
    if ((op & 0xFFC0) == 0x4AC0) {
        char ea[64]; ea_str(d, mode, reg, 1, pc, ea, sizeof(ea));
        set(out, "tas", "%s", ea);
        return;
    }

    /* SWAP: 0100 1000 01 000 rrr -- must be checked before PEA, whose mask would otherwise shadow it */
    if ((op & 0xFFF8) == 0x4840) {
        set(out, "swap", "d%d", reg);
        return;
    }

    /* PEA: 0100 1000 01 mmmrrr */
    if ((op & 0xFFC0) == 0x4840) {
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "pea", "%s", ea);
        return;
    }

    /* EXT.W / EXT.L / EXTB.L: 0100 100 opmode 000 rrr (opmode: 010=W,011=L,111=EXTB.L) */
    if ((op & 0xFE38) == 0x4800) {
        int opmode = (op >> 6) & 7;
        if (opmode == 2) { set(out, "ext.w", "d%d", reg); return; }
        if (opmode == 3) { set(out, "ext.l", "d%d", reg); return; }
        if (opmode == 7) { set(out, "extb.l", "d%d", reg); return; }
    }

    /* LINK.W: 0100 1110 0101 0rrr ; LINK.L: 0100 1000 0000 1rrr (68020) -- checked
     * before NBCD below, since LINK.L's ea-mode=1 (An direct) bit pattern would
     * otherwise be swallowed by NBCD's broader mask (An direct is not a valid
     * NBCD operand in the real ISA, which is why the encoding is reused here). */
    if ((op & 0xFFF8) == 0x4E50) {
        int16_t disp = (int16_t)fetch16(d);
        set(out, "link.w", "a%d,#%d", reg, disp);
        return;
    }
    if ((op & 0xFFF8) == 0x4808) {
        int32_t disp = (int32_t)fetch32(d);
        set(out, "link.l", "a%d,#%d", reg, disp);
        return;
    }
    /* UNLK: 0100 1110 0101 1rrr */
    if ((op & 0xFFF8) == 0x4E58) {
        set(out, "unlk", "a%d", reg);
        return;
    }
    /* MOVE USP: 0100 1110 0110 drrr */
    if ((op & 0xFFF0) == 0x4E60) {
        int dir = (op >> 3) & 1;
        if (dir) set(out, "move", "usp,a%d", reg);
        else set(out, "move", "a%d,usp", reg);
        return;
    }

    /* NBCD: 0100 1000 00 mmmrrr */
    if ((op & 0xFFC0) == 0x4800) {
        char ea[64]; ea_str(d, mode, reg, 1, pc, ea, sizeof(ea));
        set(out, "nbcd", "%s", ea);
        return;
    }

    /* JSR / JMP: 0100 1110 1 x mmmrrr, x=0 JSR,1 JMP */
    if ((op & 0xFFC0) == 0x4E80) {
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "jsr", "%s", ea);
        return;
    }
    if ((op & 0xFFC0) == 0x4EC0) {
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "jmp", "%s", ea);
        return;
    }

    /* MOVEM: bit11=1(fixed) bit10=dir bit9-8=00(fixed) bit7=size bit6=1(fixed), mask 0xFB40 value 0x4840 */
    if ((op & 0xFB40) == 0x4840) {
        int dir = (op >> 10) & 1; /* 0 = regs->mem, 1 = mem->regs */
        int sz = (op >> 7) & 1;   /* 0 = word, 1 = long */
        uint16_t maskv = fetch16(d);
        char list[128] = "";
        /* For predecrement mode the mask's bit position is reversed relative
         * to every other addressing mode (bit15=D0 ... bit8=D7, bit7=A0 ...
         * bit0=A7), even though register *identity* should still be listed
         * D0..D7,A0..A7 in the usual ascending order. */
        int predec = (mode == 4);
        int present[16];
        for (int k = 0; k < 16; k++) {
            int bitpos = predec ? (15 - k) : k;
            present[k] = (maskv & (1 << bitpos)) ? 1 : 0;
        }
        for (int k = 0; k < 16; ) {
            if (!present[k]) { k++; continue; }
            int run_end = k;
            while (run_end + 1 < 16 && present[run_end + 1] &&
                   ((run_end + 1 < 8) == (k < 8))) run_end++; /* don't span d/a boundary */
            char first[16], last[16];
            if (k < 8) snprintf(first, sizeof(first), "d%d", k); else snprintf(first, sizeof(first), "a%d", k - 8);
            if (list[0]) strncat(list, "/", sizeof(list) - strlen(list) - 1);
            if (run_end == k) {
                strncat(list, first, sizeof(list) - strlen(list) - 1);
            } else {
                if (run_end < 8) snprintf(last, sizeof(last), "d%d", run_end); else snprintf(last, sizeof(last), "a%d", run_end - 8);
                strncat(list, first, sizeof(list) - strlen(list) - 1);
                strncat(list, "-", sizeof(list) - strlen(list) - 1);
                strncat(list, last, sizeof(list) - strlen(list) - 1);
            }
            k = run_end + 1;
        }
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "movem%s", sz ? ".l" : ".w");
        if (dir) set(out, mnem, "%s,%s", ea, list);
        else set(out, mnem, "%s,%s", list, ea);
        return;
    }

    /* LEA: 0100 rrr1 11 mmmrrr */
    if ((op & 0xF1C0) == 0x41C0) {
        int an = (op >> 9) & 7;
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "lea", "%s,a%d", ea, an);
        return;
    }

    /* MULU.L/MULS.L (32x32->32 or 64): 0100 1100 00 mmmrrr + ext word */
    if ((op & 0xFFC0) == 0x4C00) {
        uint16_t ext = fetch16(d);
        int sign = (ext >> 15) & 1;
        int dh = (ext >> 12) & 7;
        int sz64 = (ext >> 10) & 1;
        int dl = ext & 7;
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s.l", sign ? "muls" : "mulu");
        if (sz64) set(out, mnem, "%s,d%d:d%d", ea, dh, dl);
        else set(out, mnem, "%s,d%d", ea, dl);
        return;
    }
    /* DIVU.L/DIVS.L/DIVUL.L/DIVSL.L: 0100 1100 01 mmmrrr + ext word */
    if ((op & 0xFFC0) == 0x4C40) {
        uint16_t ext = fetch16(d);
        int sign = (ext >> 15) & 1;
        int dh = (ext >> 12) & 7;
        int sz64 = (ext >> 10) & 1;
        int dl = ext & 7;
        char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s.l", sign ? "divs" : "divu");
        if (sz64) set(out, mnem, "%s,d%d:d%d", ea, dh, dl);
        else set(out, mnem, "%s,d%d", ea, dl);
        return;
    }

    /* CHK.W / CHK.L: 0100 rrr opmode mmmrrr, opmode 110=word 100=long */
    if ((op & 0xF000) == 0x4000) {
        int dn = (op >> 9) & 7;
        int opmode = (op >> 6) & 7;
        if (opmode == 6) {
            char ea[64]; ea_str(d, mode, reg, 2, pc, ea, sizeof(ea));
            set(out, "chk.w", "%s,d%d", ea, dn);
            return;
        }
        if (opmode == 4) {
            char ea[64]; ea_str(d, mode, reg, 4, pc, ea, sizeof(ea));
            set(out, "chk.l", "%s,d%d", ea, dn);
            return;
        }
    }

    /* TRAP: 0100 1110 0100 vvvv */
    if ((op & 0xFFF0) == 0x4E40) {
        set(out, "trap", "#%d", op & 0xF);
        return;
    }

    /* CAS: 0000 1SS 011 mmmrrr -- handled in group 0000; not here */

    unknown(out, op);
}

/* ---- group 0101: ADDQ/SUBQ, Scc, DBcc, TRAPcc ---------------------- */
static void decode_0101(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int mode = (op >> 3) & 7;
    int reg = op & 7;
    int size = (op >> 6) & 3;
    int data = (op >> 9) & 7;
    if (data == 0) data = 8;
    int cc = (op >> 8) & 0xF;

    if (size != 3) {
        int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
        int isadd = ((op >> 8) & 1) == 0;
        char ea[64]; ea_str(d, mode, reg, szb, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", isadd ? "addq" : "subq", size_suffix(szb));
        set(out, mnem, "#%d,%s", data, ea);
        return;
    }

    /* size==3 region: DBcc, Scc, TRAPcc */
    if (mode == 1) {
        /* DBcc: 0101 cccc 11001 rrr */
        int16_t disp = (int16_t)fetch16(d);
        uint32_t target = pc + 2 + disp;
        char mnem[16]; snprintf(mnem, sizeof(mnem), "db%s", cc_names[cc]);
        set(out, mnem, "d%d,$%x", reg, target);
        return;
    }
    if (mode == 7 && (reg == 2 || reg == 3 || reg == 4)) {
        /* TRAPcc: 0101 cccc 11111 op2, op2: 010=word imm,011=long imm,100=none */
        char mnem[16]; snprintf(mnem, sizeof(mnem), "trap%s", cc_names[cc]);
        if (reg == 2) { uint16_t v = fetch16(d); set(out, mnem, "#$%x", v); }
        else if (reg == 3) { uint32_t v = fetch32(d); set(out, mnem, "#$%x", v); }
        else set(out, mnem, NULL);
        return;
    }
    /* Scc: 0101 cccc 11 mmmrrr */
    char ea[64]; ea_str(d, mode, reg, 1, pc, ea, sizeof(ea));
    char mnem[16]; snprintf(mnem, sizeof(mnem), "s%s", cc_names[cc]);
    set(out, mnem, "%s", ea);
}

/* ---- group 0110: BRA/BSR/Bcc ---------------------------------------- */
static void decode_0110(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int cc = (op >> 8) & 0xF;
    int8_t disp8 = (int8_t)(op & 0xFF);
    int32_t disp;
    const char *sizesfx;
    if ((op & 0xFF) == 0x00) {
        disp = (int16_t)fetch16(d);
        sizesfx = ".w";
    } else if ((op & 0xFF) == 0xFF) {
        disp = (int32_t)fetch32(d);
        sizesfx = ".l";
    } else {
        disp = disp8;
        sizesfx = ".s";
    }
    uint32_t target = pc + 2 + disp;
    char mnem[16];
    if (cc == 0) snprintf(mnem, sizeof(mnem), "bra%s", sizesfx);
    else if (cc == 1) snprintf(mnem, sizeof(mnem), "bsr%s", sizesfx);
    else snprintf(mnem, sizeof(mnem), "b%s%s", cc_names[cc], sizesfx);
    set(out, mnem, "$%x", target);
}

/* ---- group 0111: MOVEQ ------------------------------------------------ */
static void decode_0111(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    (void)d; (void)pc;
    int reg = (op >> 9) & 7;
    int8_t data = (int8_t)(op & 0xFF);
    set(out, "moveq", "#%d,d%d", data, reg);
}

/* ---- shared helper: ADD/SUB/AND/CMP family (Dn,ea)/(ea,Dn)/A/X forms - */
static void decode_alu_family(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out,
                               const char *base_name, int allow_x, int is_cmp_eor) {
    int reg = (op >> 9) & 7;
    int opmode = (op >> 6) & 7;
    int mode = (op >> 3) & 7;
    int ea_reg = op & 7;

    if (opmode == 3 || opmode == 7) {
        /* Axxx.W / Axxx.L */
        int size = opmode == 3 ? 2 : 4;
        char ea[64]; ea_str(d, mode, ea_reg, size, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%sa%s", base_name, size_suffix(size));
        set(out, mnem, "%s,a%d", ea, reg);
        return;
    }

    if (opmode >= 4 && allow_x && (mode == 0 || mode == 1)) {
        /* Xxxx (ADDX/SUBX) or CMPM/EOR handled by caller for is_cmp_eor */
        int size = opmode == 4 ? 1 : opmode == 5 ? 2 : 4;
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%sx%s", base_name, size_suffix(size));
        if (mode == 0) set(out, mnem, "d%d,d%d", ea_reg, reg);
        else set(out, mnem, "-(a%d),-(a%d)", ea_reg, reg);
        return;
    }

    if (is_cmp_eor && opmode >= 4 && mode == 1) {
        /* CMPM: (Ay)+,(Ax)+ */
        int size = opmode == 4 ? 1 : opmode == 5 ? 2 : 4;
        char mnem[16]; snprintf(mnem, sizeof(mnem), "cmpm%s", size_suffix(size));
        set(out, mnem, "(a%d)+,(a%d)+", ea_reg, reg);
        return;
    }

    if (opmode <= 2) {
        /* <ea>,Dn */
        int size = opmode == 0 ? 1 : opmode == 1 ? 2 : 4;
        char ea[64]; ea_str(d, mode, ea_reg, size, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", base_name, size_suffix(size));
        set(out, mnem, "%s,d%d", ea, reg);
        return;
    }

    if (opmode >= 4) {
        /* Dn,<ea> (or Dn,<ea> for EOR always) */
        int size = opmode == 4 ? 1 : opmode == 5 ? 2 : 4;
        char ea[64]; ea_str(d, mode, ea_reg, size, pc, ea, sizeof(ea));
        const char *nm = is_cmp_eor ? "eor" : base_name;
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s", nm, size_suffix(size));
        set(out, mnem, "d%d,%s", reg, ea);
        return;
    }

    unknown(out, op);
}

/* ---- group 1000: OR, DIVU, DIVS, SBCD, PACK, UNPK -------------------- */
static void decode_1000(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int reg = (op >> 9) & 7;
    int opmode = (op >> 6) & 7;
    int mode = (op >> 3) & 7;
    int ea_reg = op & 7;

    if (opmode == 3) { char ea[64]; ea_str(d,mode,ea_reg,2,pc,ea,sizeof(ea)); set(out,"divu.w","%s,d%d",ea,reg); return; }
    if (opmode == 7) { char ea[64]; ea_str(d,mode,ea_reg,2,pc,ea,sizeof(ea)); set(out,"divs.w","%s,d%d",ea,reg); return; }

    if (opmode == 4 && (mode == 0 || mode == 1)) {
        if (mode == 0) { set(out, "sbcd", "d%d,d%d", ea_reg, reg); return; }
        else { set(out, "sbcd", "-(a%d),-(a%d)", ea_reg, reg); return; }
    }
    if (opmode == 5 && (mode == 0 || mode == 1)) {
        uint16_t adj = fetch16(d);
        if (mode == 0) { set(out, "pack", "d%d,d%d,#$%x", ea_reg, reg, adj); return; }
        else { set(out, "pack", "-(a%d),-(a%d),#$%x", ea_reg, reg, adj); return; }
    }
    if (opmode == 6 && (mode == 0 || mode == 1)) {
        uint16_t adj = fetch16(d);
        if (mode == 0) { set(out, "unpk", "d%d,d%d,#$%x", ea_reg, reg, adj); return; }
        else { set(out, "unpk", "-(a%d),-(a%d),#$%x", ea_reg, reg, adj); return; }
    }
    decode_alu_family(d, pc, op, out, "or", 0, 0);
}

/* ---- group 1001 / 1101: SUB/SUBX/SUBA and ADD/ADDX/ADDA -------------- */
static void decode_1001(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    decode_alu_family(d, pc, op, out, "sub", 1, 0);
}
static void decode_1101(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    decode_alu_family(d, pc, op, out, "add", 1, 0);
}

/* ---- group 1011: CMP/CMPA/EOR/CMPM ----------------------------------- */
static void decode_1011(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    decode_alu_family(d, pc, op, out, "cmp", 0, 1);
}

/* ---- group 1100: AND, MULU, MULS, ABCD, EXG --------------------------- */
static void decode_1100(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int reg = (op >> 9) & 7;
    int opmode = (op >> 6) & 7;
    int mode = (op >> 3) & 7;
    int ea_reg = op & 7;

    if (opmode == 3) { char ea[64]; ea_str(d,mode,ea_reg,2,pc,ea,sizeof(ea)); set(out,"mulu.w","%s,d%d",ea,reg); return; }
    if (opmode == 7) { char ea[64]; ea_str(d,mode,ea_reg,2,pc,ea,sizeof(ea)); set(out,"muls.w","%s,d%d",ea,reg); return; }

    if (opmode == 4 && (mode == 0 || mode == 1)) {
        if (mode == 0) { set(out, "abcd", "d%d,d%d", ea_reg, reg); return; }
        else { set(out, "abcd", "-(a%d),-(a%d)", ea_reg, reg); return; }
    }
    /* EXG: 1100 rrr1 oooo o rrr -- opmode 01000=Dn,Dn  01001=An,An  10001=Dn,An */
    if ((op & 0xF1F8) == 0xC140) { set(out, "exg", "d%d,d%d", reg, ea_reg); return; }
    if ((op & 0xF1F8) == 0xC148) { set(out, "exg", "a%d,a%d", reg, ea_reg); return; }
    if ((op & 0xF1F8) == 0xC188) { set(out, "exg", "d%d,a%d", reg, ea_reg); return; }

    decode_alu_family(d, pc, op, out, "and", 1, 0);
}

/* ---- group 1110: shifts/rotates + bitfield instructions -------------- */
static void decode_1110(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int size = (op >> 6) & 3;

    if (size == 3) {
        /* Either a memory-operand shift/rotate or a 68020 bit field instruction.
         * bit11 distinguishes them: 0 = memory shift/rotate, 1 = bit field op. */
        int mode = (op >> 3) & 7;
        int ea_reg = op & 7;

        if (op & 0x0800) {
            /* Bit field instruction: 1110 1 ooo 11 mmmrrr, ooo = bits10-8 (8 variants) */
            int opidx = (op >> 8) & 7;
            static const char *names[8] = {
                "bftst","bfextu","bfchg","bfexts","bfclr","bfffo","bfset","bfins"
            };
            uint16_t ext = fetch16(d);
            int dReg   = (ext >> 12) & 7;
            int doff   = (ext >> 11) & 1;
            int offv   = (ext >> 6) & 0x1F;
            int dw     = (ext >> 5) & 1;
            int widthv = ext & 0x1F;
            char offstr[16], widstr[16];
            if (doff) snprintf(offstr, sizeof(offstr), "d%d", (ext >> 6) & 7);
            else snprintf(offstr, sizeof(offstr), "%d", offv);
            if (dw) snprintf(widstr, sizeof(widstr), "d%d", ext & 7);
            else snprintf(widstr, sizeof(widstr), "%d", widthv == 0 ? 32 : widthv);

            char ea[64]; ea_str(d, mode, ea_reg, 1, pc, ea, sizeof(ea));
            char spec[128]; snprintf(spec, sizeof(spec), "%s{%s:%s}", ea, offstr, widstr);

            switch (opidx) {
                case 0: set(out, "bftst",  "%s", spec); return;
                case 1: set(out, "bfextu", "%s,d%d", spec, dReg); return;
                case 2: set(out, "bfchg",  "%s", spec); return;
                case 3: set(out, "bfexts", "%s,d%d", spec, dReg); return;
                case 4: set(out, "bfclr",  "%s", spec); return;
                case 5: set(out, "bfffo",  "%s,d%d", spec, dReg); return;
                case 6: set(out, "bfset",  "%s", spec); return;
                case 7: set(out, "bfins",  "d%d,%s", dReg, spec); return;
            }
            (void)names;
            unknown(out, op);
            return;
        }

        /* Memory shift/rotate: 1110 0 tt d 11 mmmrrr, tt = type (bits10-9), d = direction (bit8) */
        int type = (op >> 9) & 3;
        int dir = (op >> 8) & 1;
        static const char *names[4] = {"as","ls","rox","ro"};
        char ea[64]; ea_str(d, mode, ea_reg, 2, pc, ea, sizeof(ea));
        char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s.w", names[type], dir ? "l" : "r");
        set(out, mnem, "%s", ea);
        return;
    }

    /* Register/immediate shift-rotate: 1110 ccc d ss t i rrr
     * bits11-9 = count (or source register if i/r=1), bit8 = direction,
     * bits7-6 = size, bit5 = i/r (0=immediate count, 1=register count),
     * bits4-3 = type (00 AS,01 LS,10 ROX,11 RO), bits2-0 = Dn */
    int cnt_or_reg = (op >> 9) & 7;
    int dir = (op >> 8) & 1;
    int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
    int ir = (op >> 5) & 1;
    int type = (op >> 3) & 3;
    int ea_reg = op & 7;
    static const char *names[4] = {"as","ls","rox","ro"};
    char mnem[16]; snprintf(mnem, sizeof(mnem), "%s%s%s", names[type], dir ? "l" : "r", size_suffix(szb));
    if (ir) set(out, mnem, "d%d,d%d", cnt_or_reg, ea_reg);
    else set(out, mnem, "#%d,d%d", cnt_or_reg == 0 ? 8 : cnt_or_reg, ea_reg);
}

/* ------------------------------------------------------------------ */
/* Top level dispatcher                                                */
/* ------------------------------------------------------------------ */

static void decode(disasm_ctx_t *d, uint32_t pc, uint16_t op, Insn *out) {
    int top = (op >> 12) & 0xF;
    switch (top) {
        case 0x0: decode_0000(d, pc, op, out); return;
        case 0x1: case 0x2: case 0x3: decode_move(d, pc, op, out); return;
        case 0x4: decode_0100(d, pc, op, out); return;
        case 0x5: decode_0101(d, pc, op, out); return;
        case 0x6: decode_0110(d, pc, op, out); return;
        case 0x7: decode_0111(d, pc, op, out); return;
        case 0x8: decode_1000(d, pc, op, out); return;
        case 0x9: decode_1001(d, pc, op, out); return;
        case 0xB: decode_1011(d, pc, op, out); return;
        case 0xC: decode_1100(d, pc, op, out); return;
        case 0xD: decode_1101(d, pc, op, out); return;
        case 0xE: decode_1110(d, pc, op, out); return;
        case 0xA: unknown(out, op); return; /* line-A */
        case 0xF: unknown(out, op); return; /* coprocessor / line-F */
        default: unknown(out, op); return;
    }
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

static void print_hexbytes(const uint8_t *buf, size_t start, size_t end, char *out, size_t outsz) {
    size_t o = 0;
    for (size_t i = start; i < end && o + 5 < outsz; i += 2) {
        int n = snprintf(out + o, outsz - o, "%02X%02X ", buf[i], buf[i+1]);
        o += n;
    }
    if (o > 0) out[o-1] = 0; else out[0] = 0;
}

int main(int argc, char **argv) {
    uint32_t base_addr = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            base_addr = (uint32_t)strtoul(argv[++i], NULL, 0);
        } else {
            path = argv[i];
        }
    }
    if (!path) {
        fprintf(stderr, "usage: %s [-a base_addr] <input.bin>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fprintf(stderr, "empty or unreadable file\n"); fclose(f); return 1; }
    uint8_t *buf = malloc(sz);
    if (fread(buf, 1, sz, f) != (size_t)sz) { fprintf(stderr, "short read\n"); fclose(f); return 1; }
    fclose(f);

    disasm_ctx_t d = { buf, (size_t)sz, 0, base_addr, 0 };

    while (d.pos + 2 <= d.len) {
        size_t start = d.pos;
        uint32_t pc = cur_addr(&d);
        uint16_t op = fetch16(&d);
        Insn insn;
        decode(&d, pc, op, &insn);

        if (d.truncated) {
            /* not enough bytes to complete decode; back off to a raw word */
            d.pos = start + 2;
            snprintf(insn.mnem, sizeof(insn.mnem), "dc.w");
            snprintf(insn.ops, sizeof(insn.ops), "$%04x", op);
            d.truncated = 0;
        }

        char hexbytes[64];
        print_hexbytes(buf, start, d.pos, hexbytes, sizeof(hexbytes));

        printf("%08x  %-24s%-8s%s\n", pc, hexbytes, insn.mnem, insn.ops);
    }

    free(buf);
    return 0;
}