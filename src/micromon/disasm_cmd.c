/*
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <nonstd.h>
#include <machine.h>

#include "micromon.h"
#include "expr.h"
#include "disasm.h"

static disasm_ctx_t disasm_context = {0, 0, NO};
static int num_instructions_to_list = 20;

static const char *cc_names[16] = {
    "t","f","hi","ls","cc","cs","ne","eq",
    "vc","vs","pl","mi","ge","lt","gt","le"
};


static uint32_t cur_addr(disasm_ctx_t *ctx) {
    return ctx->pos;
}

static int fetch_byte(uint32_t addr) {
    uint8_t *ptr = (uint8_t *)addr;

    return *ptr;
}

static uint16_t fetch16(disasm_ctx_t *ctx) {
    uint16_t val;

    if (ctx->pos == 0xffffffff) {
        ctx->truncated = YES;
        return 0;

    }
    val = (uint16_t)((fetch_byte(ctx->pos) << 8) | fetch_byte(ctx->pos + 1));
    ctx->pos += 2;

    return val;
}

static uint32_t fetch32(disasm_ctx_t *ctx) {
    uint32_t high = fetch16(ctx);
    uint32_t low = fetch16(ctx);

    return (high << 16) | low;
}

static void hexsigned(int64_t val, char *out, size_t n) {
    if (val < 0) {
        snprintf(out, n, "-$%llx", (unsigned long long)(-val));
    }
    else {
        snprintf(out, n, "$%llx", (unsigned long long)val);
    }
}

static void hexunsigned(uint64_t val, char *out, size_t n) {
    snprintf(out, n, "$%llx", (unsigned long long)val);
}

static const char *size_suffix(int size) {
    switch (size) {
        case 1:
            return ".b";

        case 2:
            return ".w";

        case 4:
            return ".l";

        default:
            return "";
    }
}

static void join_parts(char *out, size_t outsz, const char *parts[], int n) {
    /* joins non-empty strings from parts[] with commas, wraps in () */
    char tmp[192];
    int first = 1;

    tmp[0] = EOS;

    for (int i = 0; i < n; i++) {
        if (parts[i] == NULL || parts[i][0] == '\0')
            continue;

        if (!first) {
            strncat(tmp, ",", sizeof(tmp) - strlen(tmp) - 1);
        }

        strncat(tmp, parts[i], sizeof(tmp) - strlen(tmp) - 1);
        first = 0;
    }

    snprintf(out, outsz, "(%s)", tmp);
}

/* Decodes an 8xx0/68020 index/base extension word, given the textual
 * name of the base register ("a3" or "pc"). Writes the resulting
 * addressing-mode text (WITHOUT any leading base-displacement-outside-parens
 * text -- that part is handled by the caller for the brief case) into out. */
static void decode_ext_word(disasm_ctx_t *ctx, const char *base_reg, int base_suppressible,
                             char *out, size_t outsz) {
    uint16_t ext = fetch16(ctx);
    int da     = (ext >> 15) & 1;
    int xreg   = (ext >> 12) & 7;
    int wl     = (ext >> 11) & 1;
    int scale  = 1 << ((ext >> 9) & 3);
    int full   = (ext >> 8) & 1;
    char xname[24];
    char scalestr[8];

    xname[0] = EOS;
    scalestr[0] = EOS;

    if (scale != 1) {
        snprintf(scalestr, sizeof(scalestr), "*%d", scale);
    }

    snprintf(xname, sizeof(xname), "%s%d.%s%s", da ? "a" : "d", xreg, wl ? "l" : "w", scalestr);

    if (!full) {
        /* Brief extension word: 8-bit displacement, base+idx always present */
        int8_t disp = (int8_t)(ext & 0xFF);
        char dispstr[32] = "";
        const char *parts[3] = {dispstr, base_reg, xname};

        dispstr[0] = EOS;

        if (disp != 0) {
            hexsigned(disp, dispstr, sizeof(dispstr));
        }

        join_parts(out, outsz, parts, 3);

        return;
    }

    /* Full extension word */
    int bs      = (ext >> 6) & 1; /* base register suppress */
    int is_     = (ext >> 5) & 1; /* index suppress */
    int bdsize  = (ext >> 4) & 3; /* 0 reserved, 1 null, 2 word, 3 long */
    int iis     = ext & 7;
    int32_t basedisp = 0;
    char bdstr[32];
    char basepart[24];
    char idxpart[24];

    bdstr[0] = EOS;
    basepart[0] = EOS;
    idxpart[0] = EOS;

    if (bdsize == 2) {
        basedisp = (int16_t)fetch16(ctx);
    }
    else if (bdsize == 3) {
        basedisp = (int32_t)fetch32(ctx);
    }

    if (basedisp != 0) {
        hexsigned(basedisp, bdstr, sizeof(bdstr));
    }

    if (!bs && base_suppressible >= 0) {
        snprintf(basepart, sizeof(basepart), "%s", base_reg);
    }
    else if (!bs) {
        snprintf(basepart, sizeof(basepart), "%s", base_reg);
    }

    if (!is_) {
        snprintf(idxpart, sizeof(idxpart), "%s", xname);
    }

    if (iis == 0) {
        /* (bd,An,Xn) -- no memory indirection */
        const char *parts[3] = {bdstr, basepart, idxpart};

        join_parts(out, outsz, parts, 3);
        return;
    }

    /* Memory indirect: need outer displacement */
    int outer_size = iis & 3; /* 1=null,2=word,3=long */
    int32_t outerdisp = 0;
    char odstr[32];
    int postindexed;
    char inner[128];

    odstr[0] = EOS;

    if (outer_size == 2) {
        outerdisp = (int16_t)fetch16(ctx);
    }
    else if (outer_size == 3) {
        outerdisp = (int32_t)fetch32(ctx);
    }

    if (outerdisp != 0) {
        hexsigned(outerdisp, odstr, sizeof(odstr));
    }

    postindexed = (iis >= 5);

    if (!postindexed) {
        /* preindexed: ([bd,An,Xn],od) */
        const char *iparts[3] = {bdstr, basepart, idxpart};
        char outerparts_buf[160];
        size_t len;

        join_parts(inner, sizeof(inner), iparts, 3);
        snprintf(outerparts_buf, sizeof(outerparts_buf), "[%s%s%s]",
                 inner + 1, /* strip leading '(' */
                 "", "");

        /* fix: replace trailing ')' with ']' handled below */
        len = strlen(outerparts_buf);
        if (len && outerparts_buf[len-1] == ')') {
            outerparts_buf[len-1] = ']';
        }

        if (odstr[0]) {
            snprintf(out, outsz, "(%s,%s)", outerparts_buf, odstr);
        }
        else {
            snprintf(out, outsz, "(%s)", outerparts_buf);
        }
    }
    else {
        /* postindexed: ([bd,An],Xn,od) */
        const char *iparts[2] = {bdstr, basepart};
        char innerbr[128];
        size_t len;
        char combined[128];
        const char *parts[3] = {combined, idxpart, odstr};

        join_parts(inner, sizeof(inner), iparts, 2);
        snprintf(innerbr, sizeof(innerbr), "%s", inner);
        len = strlen(innerbr);
        if (len >= 2) {
            innerbr[0] = '[';
            innerbr[len-1] = ']';
        }

        snprintf(combined, sizeof(combined), "%s", innerbr);
        join_parts(out, outsz, parts, 3);
    }
}

/* mode/reg: standard 6-bit effective address field (mode=bits5-3,reg=bits2-0)
 * size: operand size in bytes (1,2,4) -- only matters for immediate (mode7,reg4)
 * insn_pc: address of the *start* of the instruction (for PC-relative calc) */
static void ea_str(disasm_ctx_t *ctx, int mode, int reg, int size, uint32_t insn_pc,
                    char *out, size_t outsz) {
    char tmp[32];

    switch (mode) {
        case 0:
            snprintf(out, outsz, "d%d", reg);
            return;

        case 1:
            snprintf(out, outsz, "a%d", reg);
            return;

        case 2:
            snprintf(out, outsz, "(a%d)", reg);
            return;

        case 3:
            snprintf(out, outsz, "(a%d)+", reg);
            return;

        case 4:
            snprintf(out, outsz, "-(a%d)", reg);
            return;

        case 5:
        {
            int16_t disp = (int16_t)fetch16(ctx);
            char dispstr[32];

            hexsigned(disp, dispstr, sizeof(dispstr));
            snprintf(out, outsz, "%s(a%d)", dispstr, reg);

            return;
        }
        case 6:
            snprintf(tmp, sizeof(tmp), "a%d", reg);
            decode_ext_word(ctx, tmp, 0, out, outsz);
            return;

        case 7:
        {
            switch (reg) {
                case 0:
                {
                    int16_t val = (int16_t)fetch16(ctx);

                    hexsigned(val, tmp, sizeof(tmp));
                    snprintf(out, outsz, "%s.w", tmp);

                    return;
                }
                case 1: {
                    uint32_t val = fetch32(ctx);

                    hexunsigned(val, tmp, sizeof(tmp));
                    snprintf(out, outsz, "%s.l", tmp);

                    return;
                }
                case 2: {
                    uint32_t here = cur_addr(ctx);
                    int16_t disp = (int16_t)fetch16(ctx);
                    uint32_t target = here + disp;

                    snprintf(out, outsz, "$%x(pc)", target);

                    return;
                }
                case 3:
                    decode_ext_word(ctx, "pc", 0, out, outsz);
                    return;

                case 4:
                {
                    if (size == 1) {
                        uint16_t val = fetch16(ctx) & 0xFF;

                        snprintf(out, outsz, "#$%x", val);
                    }
                    else if (size == 2) {
                        uint16_t val = fetch16(ctx);

                        snprintf(out, outsz, "#$%x", val);
                    }
                    else {
                        uint32_t val = fetch32(ctx);
                        snprintf(out, outsz, "#$%x", val);
                    }

                    return;
                }

                default:
                    snprintf(out, outsz, "?");
                    return;
            }
        }

        default:
            snprintf(out, outsz, "?");
            return;
    }

    (void)insn_pc;
}


static void set(instruction_t *insn, const char *m, const char *fmt, ...) {
    snprintf(insn->mnem, sizeof(insn->mnem), "%s", m);
    if (fmt) {
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(insn->ops, sizeof(insn->ops), fmt, ap);
        va_end(ap);
    } else {
        insn->ops[0] = EOS;
    }
}

static void unknown(instruction_t *insn, uint16_t op) {
    snprintf(insn->mnem, sizeof(insn->mnem), "dc.w");
    snprintf(insn->ops, sizeof(insn->ops), "$%04x", op);
}

/* ---- group 0000: immediate/bit ops, MOVEP, CHK2/CMP2 --------------- */
static void decode_0000(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    /* ANDI/ORI/EORI to CCR/SR (exact opcodes) */
    int b15_8;
    int mode;
    int reg;

    switch (op) {
        case 0x003C:
        {
            uint16_t val = fetch16(ctx) & 0xFF;

            set(out, "ori", "#$%x,ccr", val);
            return;
        }

        case 0x007C:
        {
            uint16_t val = fetch16(ctx);

            set(out, "ori", "#$%x,sr", val);
            return;
        }

        case 0x023C:
        {
            uint16_t val = fetch16(ctx) & 0xFF;

            set(out, "andi", "#$%x,ccr", val);
            return;
        }

        case 0x027C:
        {
            uint16_t val = fetch16(ctx);

            set(out, "andi", "#$%x,sr", val);
            return;
        }

        case 0x0A3C:
        {
            uint16_t val = fetch16(ctx) & 0xFF;

            set(out, "eori", "#$%x,ccr", val);
            return;
        }

        case 0x0A7C:
        {
            uint16_t val = fetch16(ctx);

            set(out, "eori", "#$%x,sr", val);
            return;
        }
    }

    b15_8 = (op >> 8) & 0xFF;
    mode = (op >> 3) & 7;
    reg = op & 7;

    /* CHK2/CMP2 (68020+): 0000 0ss 011 mmmrrr + ext word */
    if ((op & 0xF9C0) == 0x00C0) {
        int size = (op >> 9) & 3;

        if (size != 3) {
            uint16_t ext = fetch16(ctx);
            int isAn = (ext >> 15) & 1;
            int regnum = (ext >> 12) & 7;
            int isChk = (ext >> 11) & 1;
            int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
            char ea[64];
            char mnem[16];

            ea_str(ctx, mode, reg, szb, pc, ea, sizeof(ea));
            snprintf(mnem, sizeof(mnem), "%s%s", isChk ? "chk2" : "cmp2", size_suffix(szb));

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
            char ea[64];
            char mnem[16];

            if (szb == 1) {
                uint16_t val = fetch16(ctx) & 0xFF;

                snprintf(imm, sizeof(imm), "#$%x", val);
            }
            else if (szb == 2) {
                uint16_t val = fetch16(ctx);

                snprintf(imm, sizeof(imm), "#$%x", val);
            }
            else {
                uint32_t val = fetch32(ctx);

                snprintf(imm, sizeof(imm), "#$%x", val);
            }

            /* CMPI/CHK2/CMP2 68020: when mode==7,reg>=2 with 'cmpi' it's actually CMP2/CHK2 - skip that edge here */
            ea_str(ctx, mode, reg, szb, pc, ea, sizeof(ea));
            snprintf(mnem, sizeof(mnem), "%s%s", names[type], size_suffix(szb));
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
        int16_t disp = (int16_t)fetch16(ctx);
        char dispstr[32];
        char mnem[16];

        hexsigned(disp, dispstr, sizeof(dispstr));
        snprintf(mnem, sizeof(mnem), "movep%s", size ? ".l" : ".w");

        if (direction) {
            set(out, mnem, "d%d,%s(a%d)", dn, dispstr, reg);
        }
        else {
            set(out, mnem, "%s(a%d),d%d", dispstr, reg, dn);
        }

        return;
    }

    /* BTST/BCHG/BCLR/BSET dynamic (register) form: 0000 ddd1 oo mmmrrr */
    if ((op & 0x0100) != 0) {
        static const char *names[4] = {"btst","bchg","bclr","bset"};
        int dn = (op >> 9) & 7;
        int opsel = (op >> 6) & 3;
        char ea[64];
        int size = (mode == 0) ? 4 : 1;
        char mnem[16];

        ea_str(ctx, mode, reg, 1, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s", names[opsel], size_suffix(size));
        set(out, mnem, "d%d,%s", dn, ea);

        return;
    }

    /* BTST/BCHG/BCLR/BSET static (immediate) form: 0000 1000 oo mmmrrr, ext word = bit# */
    if (b15_8 == 0x08 || b15_8 == 0x09 || b15_8 == 0x0A || b15_8 == 0x0B) {
        static const char *names[4] = {"btst","bchg","bclr","bset"};
        int opsel = (op >> 6) & 3;
        uint16_t bitnum = fetch16(ctx) & 0xFF;
        char ea[64];
        int size = (mode == 0) ? 4 : 1;
        char mnem[16];

        ea_str(ctx, mode, reg, 1, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s", names[opsel], size_suffix(size));
        set(out, mnem, "#$%x,%s", bitnum, ea);
        return;
    }

    unknown(out, op);
}

/* ---- MOVE / MOVEA (opcodes 01,10,11 top bits) ---------------------- */
static void decode_move(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int szf = (op >> 12) & 3; /* 01=byte,11=word,10=long */
    int size = szf == 1 ? 1 : szf == 3 ? 2 : 4;
    int dst_reg = (op >> 9) & 7;
    int dst_mode = (op >> 6) & 7;
    int src_mode = (op >> 3) & 7;
    int src_reg = op & 7;
    char src[80];
    char dst[80];

    ea_str(ctx, src_mode, src_reg, size, pc, src, sizeof(src));
    ea_str(ctx, dst_mode, dst_reg, size, pc, dst, sizeof(dst));

    if (dst_mode == 1) {
        char mnem[16];

        snprintf(mnem, sizeof(mnem), "movea%s", size_suffix(size));
        set(out, mnem, "%s,%s", src, dst);
    }
    else {
        char mnem[16];

        snprintf(mnem, sizeof(mnem), "move%s", size_suffix(size));
        set(out, mnem, "%s,%s", src, dst);
    }
}

/* ---- group 0100: misc (NEG, CLR, LEA, JSR, MOVEM, TRAP, ...) ------- */
static void decode_0100(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int mode = (op >> 3) & 7;
    int reg = op & 7;

    /* exact single opcodes */
    switch (op) {
        case 0x4E70:
            set(out, "reset", NULL);
            return;

        case 0x4E71:
            set(out, "nop", NULL);
            return;

        case 0x4E72:
        {
            uint16_t val = fetch16(ctx);
            set(out, "stop", "#$%x", val);
            return;
        }

        case 0x4E73:
            set(out, "rte", NULL);
            return;

        case 0x4E74:
        {
            uint16_t val = fetch16(ctx);

            set(out, "rtd", "#$%x", (int16_t)val);
            return;
        }

        case 0x4E75:
            set(out, "rts", NULL);
            return;

        case 0x4E76:
            set(out, "trapv", NULL);
            return;

        case 0x4E77:
            set(out, "rtr", NULL);
            return;

        case 0x4AFC:
            set(out, "illegal", NULL);
            return;
    }

    /* MOVE from SR: 0100 0000 11 mmmrrr */
    if ((op & 0xFFC0) == 0x40C0) {
        char ea[64];

        ea_str(ctx, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "sr,%s", ea);
        return;
    }

    /* MOVE from CCR (68010+): 0100 0010 11 mmmrrr */
    if ((op & 0xFFC0) == 0x42C0) {
        char ea[64];

        ea_str(ctx, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "ccr,%s", ea);
        return;
    }

    /* MOVE to CCR: 0100 0100 11 mmmrrr */
    if ((op & 0xFFC0) == 0x44C0) {
        char ea[64];

        ea_str(ctx, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "%s,ccr", ea);
        return;
    }

    /* MOVE to SR: 0100 0110 11 mmmrrr */
    if ((op & 0xFFC0) == 0x46C0) {
        char ea[64];

        ea_str(ctx, mode, reg, 2, pc, ea, sizeof(ea));
        set(out, "move", "%s,sr", ea);
        return;
    }

    /* NEGX/CLR/NEG/NOT: 0100 00gg ss mmmrrr, gg(bits10-9):00 NEGX,01 CLR,10 NEG,11 NOT */
    if ((op & 0xF900) == 0x4000 && ((op >> 6) & 3) != 3) {
        static const char *names[4] = {"negx","clr","neg","not"};
        int grp = (op >> 9) & 3;
        int size = (op >> 6) & 3;
        int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
        char ea[64];
        char mnem[16];

        ea_str(ctx, mode, reg, szb, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s", names[grp], size_suffix(szb));
        set(out, mnem, "%s", ea);
        return;
    }

    /* TST: 0100 1010 ss mmmrrr (size 11 = TAS, handled next) */
    if ((op & 0xFF00) == 0x4A00 && ((op >> 6) & 3) != 3) {
        int size = (op >> 6) & 3;
        int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
        char ea[64];
        char mnem[16];

        ea_str(ctx, mode, reg, szb, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "tst%s", size_suffix(szb));
        set(out, mnem, "%s", ea);
        return;
    }

    /* TAS: 0100 1010 11 mmmrrr */
    if ((op & 0xFFC0) == 0x4AC0) {
        char ea[64];

        ea_str(ctx, mode, reg, 1, pc, ea, sizeof(ea));
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
        char ea[64];

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "pea", "%s", ea);
        return;
    }

    /* EXT.W / EXT.L / EXTB.L: 0100 100 opmode 000 rrr (opmode: 010=W,011=L,111=EXTB.L) */
    if ((op & 0xFE38) == 0x4800) {
        int opmode = (op >> 6) & 7;

        if (opmode == 2) {
            set(out, "ext.w", "d%d", reg);
            return;
        }

        if (opmode == 3) {
            set(out, "ext.l", "d%d", reg);
            return;
        }

        if (opmode == 7) {
            set(out, "extb.l", "d%d", reg);
            return;
        }
    }

    /* LINK.W: 0100 1110 0101 0rrr ; LINK.L: 0100 1000 0000 1rrr (68020) -- checked
     * before NBCD below, since LINK.L's ea-mode=1 (An direct) bit pattern would
     * otherwise be swallowed by NBCD's broader mask (An direct is not a valid
     * NBCD operand in the real ISA, which is why the encoding is reused here). */
    if ((op & 0xFFF8) == 0x4E50) {
        int16_t disp = (int16_t)fetch16(ctx);

        set(out, "link.w", "a%d,#%d", reg, disp);
        return;
    }

    if ((op & 0xFFF8) == 0x4808) {
        int32_t disp = (int32_t)fetch32(ctx);

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

        if (dir) {
            set(out, "move", "usp,a%d", reg);
        }
        else {
            set(out, "move", "a%d,usp", reg);
        }

        return;
    }

    /* NBCD: 0100 1000 00 mmmrrr */
    if ((op & 0xFFC0) == 0x4800) {
        char ea[64];

        ea_str(ctx, mode, reg, 1, pc, ea, sizeof(ea));
        set(out, "nbcd", "%s", ea);

        return;
    }

    /* JSR / JMP: 0100 1110 1 x mmmrrr, x=0 JSR,1 JMP */
    if ((op & 0xFFC0) == 0x4E80) {
        char ea[64];

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "jsr", "%s", ea);

        return;
    }

    if ((op & 0xFFC0) == 0x4EC0) {
        char ea[64];

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "jmp", "%s", ea);

        return;
    }

    /* MOVEM: bit11=1(fixed) bit10=dir bit9-8=00(fixed) bit7=size bit6=1(fixed), mask 0xFB40 value 0x4840 */
    if ((op & 0xFB40) == 0x4840) {
        int dir = (op >> 10) & 1; /* 0 = regs->mem, 1 = mem->regs */
        int sz = (op >> 7) & 1;   /* 0 = word, 1 = long */
        uint16_t maskv = fetch16(ctx);
        char list[128] = "";
        /* For predecrement mode the mask's bit position is reversed relative
         * to every other addressing mode (bit15=D0 ... bit8=D7, bit7=A0 ...
         * bit0=A7), even though register *identity* should still be listed
         * D0..D7,A0..A7 in the usual ascending order. */
        int predec = (mode == 4);
        int present[16];
        char ea[64];
        char mnem[16];

        for (int k = 0; k < 16; k++) {
            int bitpos = predec ? (15 - k) : k;

            present[k] = (maskv & (1 << bitpos)) ? 1 : 0;
        }

        for (int k = 0; k < 16; ) {
            int run_end;
            char first[16], last[16];

            if (!present[k]) {
                k++;
                continue;
            }

            run_end = k;

            while (run_end + 1 < 16 && present[run_end + 1] &&
                   ((run_end + 1 < 8) == (k < 8))) {
                    run_end++; /* don't span d/a boundary */
            }

            if (k < 8) {
                snprintf(first, sizeof(first), "d%d", k);
            }
            else {
                snprintf(first, sizeof(first), "a%d", k - 8);
            }

            if (list[0]) {
                strncat(list, "/", sizeof(list) - strlen(list) - 1);
            }

            if (run_end == k) {
                strncat(list, first, sizeof(list) - strlen(list) - 1);
            } else {
                if (run_end < 8) {
                    snprintf(last, sizeof(last), "d%d", run_end);
                }
                else {
                    snprintf(last, sizeof(last), "a%d", run_end - 8);
                }

                strncat(list, first, sizeof(list) - strlen(list) - 1);
                strncat(list, "-", sizeof(list) - strlen(list) - 1);
                strncat(list, last, sizeof(list) - strlen(list) - 1);
            }
            k = run_end + 1;
        }

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "movem%s", sz ? ".l" : ".w");

        if (dir) {
            set(out, mnem, "%s,%s", ea, list);
        }
        else {
            set(out, mnem, "%s,%s", list, ea);
        }

        return;
    }

    /* LEA: 0100 rrr1 11 mmmrrr */
    if ((op & 0xF1C0) == 0x41C0) {
        int an = (op >> 9) & 7;
        char ea[64];

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        set(out, "lea", "%s,a%d", ea, an);

        return;
    }

    /* MULU.L/MULS.L (32x32->32 or 64): 0100 1100 00 mmmrrr + ext word */
    if ((op & 0xFFC0) == 0x4C00) {
        uint16_t ext = fetch16(ctx);
        int sign = (ext >> 15) & 1;
        int dh = (ext >> 12) & 7;
        int sz64 = (ext >> 10) & 1;
        int dl = ext & 7;
        char ea[64];
        char mnem[16];

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s.l", sign ? "muls" : "mulu");

        if (sz64) {
            set(out, mnem, "%s,d%d:d%d", ea, dh, dl);
        }
        else {
            set(out, mnem, "%s,d%d", ea, dl);
        }

        return;
    }

    /* DIVU.L/DIVS.L/DIVUL.L/DIVSL.L: 0100 1100 01 mmmrrr + ext word */
    if ((op & 0xFFC0) == 0x4C40) {
        uint16_t ext = fetch16(ctx);
        int sign = (ext >> 15) & 1;
        int dh = (ext >> 12) & 7;
        int sz64 = (ext >> 10) & 1;
        int dl = ext & 7;
        char ea[64];
        char mnem[16];

        ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s.l", sign ? "divs" : "divu");

        if (sz64) {
            set(out, mnem, "%s,d%d:d%d", ea, dh, dl);
        }
        else {
            set(out, mnem, "%s,d%d", ea, dl);
        }

        return;
    }

    /* CHK.W / CHK.L: 0100 rrr opmode mmmrrr, opmode 110=word 100=long */
    if ((op & 0xF000) == 0x4000) {
        int dn = (op >> 9) & 7;
        int opmode = (op >> 6) & 7;

        if (opmode == 6) {
            char ea[64];

            ea_str(ctx, mode, reg, 2, pc, ea, sizeof(ea));
            set(out, "chk.w", "%s,d%d", ea, dn);

            return;
        }
        if (opmode == 4) {
            char ea[64];

            ea_str(ctx, mode, reg, 4, pc, ea, sizeof(ea));
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
static void decode_0101(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int mode = (op >> 3) & 7;
    int reg = op & 7;
    int size = (op >> 6) & 3;
    int data = (op >> 9) & 7;
    int cc = (op >> 8) & 0xF;
    char ea[64];
    char mnem[16];

    if (data == 0) {
        data = 8;
    }

    if (size != 3) {
        int szb = size == 0 ? 1 : size == 1 ? 2 : 4;
        int isadd = ((op >> 8) & 1) == 0;

        ea_str(ctx, mode, reg, szb, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s", isadd ? "addq" : "subq", size_suffix(szb));
        set(out, mnem, "#%d,%s", data, ea);

        return;
    }

    /* size==3 region: DBcc, Scc, TRAPcc */
    if (mode == 1) {
        /* DBcc: 0101 cccc 11001 rrr */
        int16_t disp = (int16_t)fetch16(ctx);
        uint32_t target = pc + 2 + disp;

        snprintf(mnem, sizeof(mnem), "db%s", cc_names[cc]);
        set(out, mnem, "d%d,$%x", reg, target);

        return;
    }

    if (mode == 7 && (reg == 2 || reg == 3 || reg == 4)) {
        /* TRAPcc: 0101 cccc 11111 op2, op2: 010=word imm,011=long imm,100=none */
        snprintf(mnem, sizeof(mnem), "trap%s", cc_names[cc]);

        if (reg == 2) {
            uint16_t val = fetch16(ctx);

            set(out, mnem, "#$%x", val);
        }
        else if (reg == 3) {
            uint32_t val = fetch32(ctx);

            set(out, mnem, "#$%x", val);
        }
        else {
            set(out, mnem, NULL);
        }

        return;
    }

    /* Scc: 0101 cccc 11 mmmrrr */
    ea_str(ctx, mode, reg, 1, pc, ea, sizeof(ea));
    snprintf(mnem, sizeof(mnem), "s%s", cc_names[cc]);
    set(out, mnem, "%s", ea);
}

/* ---- group 0110: BRA/BSR/Bcc ---------------------------------------- */
static void decode_0110(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int cc = (op >> 8) & 0xF;
    int8_t disp8 = (int8_t)(op & 0xFF);
    int32_t disp;
    const char *sizesfx;
    uint32_t target;
    char mnem[16];

    if ((op & 0xFF) == 0x00) {
        disp = (int16_t)fetch16(ctx);
        sizesfx = ".w";
    } else if ((op & 0xFF) == 0xFF) {
        disp = (int32_t)fetch32(ctx);
        sizesfx = ".l";
    } else {
        disp = disp8;
        sizesfx = ".s";
    }

    target = pc + 2 + disp;

    if (cc == 0) {
        snprintf(mnem, sizeof(mnem), "bra%s", sizesfx);
    }
    else if (cc == 1) {
        snprintf(mnem, sizeof(mnem), "bsr%s", sizesfx);
    }
    else {
        snprintf(mnem, sizeof(mnem), "b%s%s", cc_names[cc], sizesfx);
    }

    set(out, mnem, "$%x", target);
}

/* ---- group 0111: MOVEQ ------------------------------------------------ */
static void decode_0111(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int reg = (op >> 9) & 7;
    int8_t data = (int8_t)(op & 0xFF);

    (void)ctx;
    (void)pc;

    set(out, "moveq", "#%d,d%d", data, reg);
}

/* ---- shared helper: ADD/SUB/AND/CMP family (Dn,ea)/(ea,Dn)/A/X forms - */
static void decode_alu_family(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out,
                               const char *base_name, int allow_x, int is_cmp_eor) {
    int reg = (op >> 9) & 7;
    int opmode = (op >> 6) & 7;
    int mode = (op >> 3) & 7;
    int ea_reg = op & 7;

    if (opmode == 3 || opmode == 7) {
        /* Axxx.W / Axxx.L */
        int size = opmode == 3 ? 2 : 4;
        char ea[64];
        char mnem[16];

        ea_str(ctx, mode, ea_reg, size, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%sa%s", base_name, size_suffix(size));
        set(out, mnem, "%s,a%d", ea, reg);

        return;
    }

    if (opmode >= 4 && allow_x && (mode == 0 || mode == 1)) {
        /* Xxxx (ADDX/SUBX) or CMPM/EOR handled by caller for is_cmp_eor */
        int size = opmode == 4 ? 1 : opmode == 5 ? 2 : 4;
        char mnem[16];

        snprintf(mnem, sizeof(mnem), "%sx%s", base_name, size_suffix(size));

        if (mode == 0) {
            set(out, mnem, "d%d,d%d", ea_reg, reg);
        }
        else {
            set(out, mnem, "-(a%d),-(a%d)", ea_reg, reg);
        }

        return;
    }

    if (is_cmp_eor && opmode >= 4 && mode == 1) {
        /* CMPM: (Ay)+,(Ax)+ */
        int size = opmode == 4 ? 1 : opmode == 5 ? 2 : 4;
        char mnem[16];

        snprintf(mnem, sizeof(mnem), "cmpm%s", size_suffix(size));
        set(out, mnem, "(a%d)+,(a%d)+", ea_reg, reg);

        return;
    }

    if (opmode <= 2) {
        /* <ea>,Dn */
        int size = opmode == 0 ? 1 : opmode == 1 ? 2 : 4;
        char ea[64];
        char mnem[16];

        ea_str(ctx, mode, ea_reg, size, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s", base_name, size_suffix(size));
        set(out, mnem, "%s,d%d", ea, reg);

        return;
    }

    if (opmode >= 4) {
        /* Dn,<ea> (or Dn,<ea> for EOR always) */
        int size = opmode == 4 ? 1 : opmode == 5 ? 2 : 4;
        char ea[64];
        const char *nm = is_cmp_eor ? "eor" : base_name;
        char mnem[16];

        ea_str(ctx, mode, ea_reg, size, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s", nm, size_suffix(size));
        set(out, mnem, "d%d,%s", reg, ea);

        return;
    }

    unknown(out, op);
}

/* ---- group 1000: OR, DIVU, DIVS, SBCD, PACK, UNPK -------------------- */
static void decode_1000(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int reg = (op >> 9) & 7;
    int opmode = (op >> 6) & 7;
    int mode = (op >> 3) & 7;
    int ea_reg = op & 7;

    if (opmode == 3) {
        char ea[64];

        ea_str(ctx, mode, ea_reg, 2, pc, ea, sizeof(ea));
        set(out,"divu.w","%s,d%d",ea,reg);

        return;
    }

    if (opmode == 7) {
        char ea[64];

        ea_str(ctx, mode, ea_reg, 2, pc, ea, sizeof(ea));
        set(out,"divs.w","%s,d%d",ea,reg);

        return;
    }

    if (opmode == 4 && (mode == 0 || mode == 1)) {
        if (mode == 0) {
            set(out, "sbcd", "d%d,d%d", ea_reg, reg);
        }
        else {
            set(out, "sbcd", "-(a%d),-(a%d)", ea_reg, reg);
        }

        return;
    }

    if (opmode == 5 && (mode == 0 || mode == 1)) {
        uint16_t adj = fetch16(ctx);

        if (mode == 0) {
            set(out, "pack", "d%d,d%d,#$%x", ea_reg, reg, adj);
        }
        else {
            set(out, "pack", "-(a%d),-(a%d),#$%x", ea_reg, reg, adj);
        }

        return;
    }

    if (opmode == 6 && (mode == 0 || mode == 1)) {
        uint16_t adj = fetch16(ctx);

        if (mode == 0) {
            set(out, "unpk", "d%d,d%d,#$%x", ea_reg, reg, adj);
        }
        else {
            set(out, "unpk", "-(a%d),-(a%d),#$%x", ea_reg, reg, adj);
        }

        return;
    }

    decode_alu_family(ctx, pc, op, out, "or", 0, 0);
}

/* ---- group 1001 / 1101: SUB/SUBX/SUBA and ADD/ADDX/ADDA -------------- */
static void decode_1001(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    decode_alu_family(ctx, pc, op, out, "sub", 1, 0);
}
static void decode_1101(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    decode_alu_family(ctx, pc, op, out, "add", 1, 0);
}

/* ---- group 1011: CMP/CMPA/EOR/CMPM ----------------------------------- */
static void decode_1011(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    decode_alu_family(ctx, pc, op, out, "cmp", 0, 1);
}

/* ---- group 1100: AND, MULU, MULS, ABCD, EXG --------------------------- */
static void decode_1100(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int reg = (op >> 9) & 7;
    int opmode = (op >> 6) & 7;
    int mode = (op >> 3) & 7;
    int ea_reg = op & 7;

    if (opmode == 3) {
        char ea[64];

        ea_str(ctx, mode, ea_reg, 2, pc, ea, sizeof(ea));
        set(out, "mulu.w", "%s,d%d", ea,reg);

        return;
    }

    if (opmode == 7) {
        char ea[64];

        ea_str(ctx, mode, ea_reg, 2, pc, ea, sizeof(ea));
        set(out, "muls.w", "%s,d%d", ea,reg);

        return;
    }

    if (opmode == 4 && (mode == 0 || mode == 1)) {
        if (mode == 0) {
            set(out, "abcd", "d%d,d%d", ea_reg, reg);
        }
        else {
            set(out, "abcd", "-(a%d),-(a%d)", ea_reg, reg);
        }

        return;
    }

    /* EXG: 1100 rrr1 oooo o rrr -- opmode 01000=Dn,Dn  01001=An,An  10001=Dn,An */
    if ((op & 0xF1F8) == 0xC140) {
        set(out, "exg", "d%d,d%d", reg, ea_reg);
        return;
    }

    if ((op & 0xF1F8) == 0xC148) {
        set(out, "exg", "a%d,a%d", reg, ea_reg);
        return;
    }

    if ((op & 0xF1F8) == 0xC188) {
        set(out, "exg", "d%d,a%d", reg, ea_reg);
        return;
    }

    decode_alu_family(ctx, pc, op, out, "and", 1, 0);
}

/* ---- group 1110: shifts/rotates + bitfield instructions -------------- */
static void decode_1110(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    static const char *shift_names[4] = {"as","ls","rox","ro"};
    int size = (op >> 6) & 3;
    int cnt_or_reg;
    int dir;
    int szb;
    int ir;
    int type;
    int ea_reg;
    char mnem[16];

    if (size == 3) {
        /* Either a memory-operand shift/rotate or a 68020 bit field instruction.
         * bit11 distinguishes them: 0 = memory shift/rotate, 1 = bit field op. */
        int mode = (op >> 3) & 7;
        char ea[64];
        char mnem[16];

        ea_reg = op & 7;

        if (op & 0x0800) {
            /* Bit field instruction: 1110 1 ooo 11 mmmrrr, ooo = bits10-8 (8 variants) */
            int opidx = (op >> 8) & 7;
            uint16_t ext = fetch16(ctx);
            int dReg   = (ext >> 12) & 7;
            int doff   = (ext >> 11) & 1;
            int offv   = (ext >> 6) & 0x1F;
            int dw     = (ext >> 5) & 1;
            int widthv = ext & 0x1F;
            char offstr[16], widstr[16];
            char spec[128];

            if (doff) {
                snprintf(offstr, sizeof(offstr), "d%d", (ext >> 6) & 7);
            }
            else {
                snprintf(offstr, sizeof(offstr), "%d", offv);
            }

            if (dw) {
                snprintf(widstr, sizeof(widstr), "d%d", ext & 7);
            }
            else {
                snprintf(widstr, sizeof(widstr), "%d", widthv == 0 ? 32 : widthv);
            }

            ea_str(ctx, mode, ea_reg, 1, pc, ea, sizeof(ea));
            snprintf(spec, sizeof(spec), "%s{%s:%s}", ea, offstr, widstr);

            switch (opidx) {
                case 0:
                    set(out, "bftst",  "%s", spec);
                    return;

                case 1:
                    set(out, "bfextu", "%s,d%d", spec, dReg);
                    return;

                case 2:
                    set(out, "bfchg",  "%s", spec);
                    return;

                case 3:
                    set(out, "bfexts", "%s,d%d", spec, dReg);
                    return;

                case 4:
                    set(out, "bfclr",  "%s", spec);
                    return;

                case 5:
                    set(out, "bfffo",  "%s,d%d", spec, dReg);
                    return;

                case 6:
                    set(out, "bfset",  "%s", spec);
                    return;

                case 7:
                    set(out, "bfins",  "d%d,%s", dReg, spec);
                    return;
            }

            unknown(out, op);

            return;
        }

        /* Memory shift/rotate: 1110 0 tt d 11 mmmrrr, tt = type (bits10-9), d = direction (bit8) */
        type = (op >> 9) & 3;
        dir = (op >> 8) & 1;

        ea_str(ctx, mode, ea_reg, 2, pc, ea, sizeof(ea));
        snprintf(mnem, sizeof(mnem), "%s%s.w", shift_names[type], dir ? "l" : "r");
        set(out, mnem, "%s", ea);

        return;
    }

    /* Register/immediate shift-rotate: 1110 ccc d ss t i rrr
     * bits11-9 = count (or source register if i/r=1), bit8 = direction,
     * bits7-6 = size, bit5 = i/r (0=immediate count, 1=register count),
     * bits4-3 = type (00 AS,01 LS,10 ROX,11 RO), bits2-0 = Dn */
    cnt_or_reg = (op >> 9) & 7;
    dir = (op >> 8) & 1;
    szb = size == 0 ? 1 : size == 1 ? 2 : 4;
    ir = (op >> 5) & 1;
    type = (op >> 3) & 3;
    ea_reg = op & 7;

    snprintf(mnem, sizeof(mnem), "%s%s%s", shift_names[type], dir ? "l" : "r", size_suffix(szb));

    if (ir) {
        set(out, mnem, "d%d,d%d", cnt_or_reg, ea_reg);
    }
    else {
        set(out, mnem, "#%d,d%d", cnt_or_reg == 0 ? 8 : cnt_or_reg, ea_reg);
    }
}

static void decode(disasm_ctx_t *ctx, uint32_t pc, uint16_t op, instruction_t *out) {
    int top_nibble = (op >> 12) & 0xF;

    switch (top_nibble) {
        case 0x0:
            decode_0000(ctx, pc, op, out);
            return;

        case 0x1:
        case 0x2:
        case 0x3:
            decode_move(ctx, pc, op, out);
            return;

        case 0x4:
            decode_0100(ctx, pc, op, out);
            return;

        case 0x5:
            decode_0101(ctx, pc, op, out);
            return;

        case 0x6:
            decode_0110(ctx, pc, op, out);
            return;

        case 0x7:
            decode_0111(ctx, pc, op, out);
            return;

        case 0x8:
            decode_1000(ctx, pc, op, out);
            return;

        case 0x9:
            decode_1001(ctx, pc, op, out);
            return;

        case 0xB:
            decode_1011(ctx, pc, op, out);
            return;

        case 0xC:
            decode_1100(ctx, pc, op, out);
            return;

        case 0xD:
            decode_1101(ctx, pc, op, out);
            return;

        case 0xE:
            decode_1110(ctx, pc, op, out);
            return;

        case 0xA:
            unknown(out, op);
            return; /* line-A */

        case 0xF:
            unknown(out, op);
            return; /* coprocessor / line-F */

        default:
            unknown(out, op);
            return;
    }
}

static void print_hexbytes(size_t start, size_t end, char *out, size_t out_len) {
    size_t offset = 0;

    for (size_t i = start; (i < end) && ((offset + 5) < out_len); i += 2) {
        int n = snprintf(out + offset, out_len - offset, "%02x%02x ", fetch_byte(i), fetch_byte(i+1));

        offset += n;
    }

    if (offset > 0) {
        out[offset-1] = EOS;
    }
    else {
        out[0] = EOS;
    }
}

static int disasm_single_instruction(void) {
    size_t start = disasm_context.pos;
    uint32_t pc = cur_addr(&disasm_context);
    uint16_t op = fetch16(&disasm_context);
    instruction_t insn;
    char hex_bytes[64];

    decode(&disasm_context, pc, op, &insn);

    // known instruction?
    if (disasm_context.truncated == YES) {
        disasm_context.pos = start + 2;
        snprintf(insn.mnem, sizeof(insn.mnem), "dc.w");
        snprintf(insn.ops, sizeof(insn.ops), "$%04x", op);

        disasm_context.truncated = NO;
    }

    print_hexbytes(start, disasm_context.pos, hex_bytes, sizeof(hex_bytes));

    kprintf("%06x  %-24s%-8s%s\n", pc, hex_bytes, insn.mnem, insn.ops);
    // kprintf("$%08x  %s %s%s\n", pc, hex_bytes, insn.mnem, insn.ops);

    return OK;
}

void handle_disasm_command(int argc, char *argv[]) {
    long val;
    expr_error_t res;
    int error_pos;

    if ((argc < 0) || (argc > 3)) {
        kprintf("Bad args.\n");
        return;
    }

    if (argc >= 2) {
        res = expr_evaluate(argv[1], &val, &error_pos);
        if (res == EXPR_OK) {
            disasm_context.pos = val;

            if (argc >= 3) {
                res = expr_evaluate(argv[2], &val, &error_pos);
                if (res == EXPR_OK) {
                    num_instructions_to_list = (int) val;
                }
                else {
                    kprintf("Couldn't evaluate expression: '%s'\n", argv[2]);
                    return;
                }
            }
        }
        else {
            kprintf("Couldn't evaluate expression: '%s'\n", argv[1]);
            return;
        }

    }

    for (int i=0; i<num_instructions_to_list; i++) {
        disasm_single_instruction();
    }
}