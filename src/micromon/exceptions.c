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

#include <stddef.h>
#include <ctype.h>
#include <duart.h>

struct common_frame {
    uint16_t sr;
    uint32_t pc;
    uint16_t format_vector;
} __attribute((packed));

typedef struct common_frame common_frame_t;


struct exception {
    uint32_t usp;
    uint32_t dregs[8];
    uint32_t aregs[7];
    common_frame_t frame;
} __attribute__((packed));

typedef struct exception exception_t;


#define SR_SUPERVISOR  0x2000
 
/* SSW bits (format $A/$B bus fault frames) */
#define SSW_FC   0x8000  /* fault on instruction stage C */
#define SSW_FB   0x4000  /* fault on instruction stage B */
#define SSW_DF   0x0100  /* data cycle fault             */
#define SSW_RW   0x0040  /* 1 = read, 0 = write          */
 
static const char *const exception_names[64] = {
    "Reset: initial SSP",
    "Reset: initial PC",
    "Bus error",
    "Address error",
    "Illegal instruction",
    "Divide by zero",
    "CHK/CHK2",
    "TRAPcc/TRAPV",
    "Privilege violation",
    "Trace",
    "Line A emulator",
    "Line F emulator",
    NULL,
    "Coprocessor protocol violation",
    "Format error",
    "Uninitialized interrupt",
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    "Spurious interrupt",
    "Autovector interrupt",
    "Autovector interrupt",
    "Autovector interrupt",
    "Autovector interrupt",
    "Autovector interrupt",
    "Autovector interrupt",
    "Autovector interrupt",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "TRAP",
    "FPU branch/set on unordered",
    "FPU inexact result",
    "FPU divide by zero",
    "FPU underflow",
    "FPU operand error",
    "FPU overflow",
    "FPU signaling NaN",
    NULL,
    "MMU configuration error",
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL
};
 
/* Size in bytes of each frame format the 68030 can generate. */
static unsigned frame_size(unsigned fmt) {
    switch (fmt) {
        case 0x0:
            return 8;   /* normal four-word           */

        case 0x1:
            return 8;   /* throwaway (interrupt, M=1) */

        case 0x2:
            return 12;  /* six-word instruction exc.  */

        case 0x9:
            return 20;  /* coprocessor mid-instruction */

        case 0xA:
            return 32;  /* short bus cycle fault      */

        case 0xB:
            return 92;  /* long bus cycle fault       */

        default:
            return 0;   /* not a 68030 format         */
    }
}
 
static inline uint16_t fr16(const common_frame_t *f, unsigned off) {
    return *(const uint16_t *)((const uint8_t *)f + off);
}
 
static inline uint32_t fr32(const common_frame_t *f, unsigned off) {
    return *(const uint32_t *)((const uint8_t *)f + off);
}
 
void catch_exception(exception_t *context) {
    const common_frame_t *f = &context->frame;
    unsigned fmt  = f->format_vector >> 12;
    unsigned vec  = (f->format_vector & 0x0FFF) >> 2;
    unsigned size = frame_size(fmt);
    const char *name;
    int i;
 
    if (vec < 64) {
        name = exception_names[vec] ? exception_names[vec] : "Unassigned/reserved";
    }
    else {
        name = "User-defined vector";
    }
 
    kprintf("\n*** EXCEPTION: %s (vector %u", name, vec);
    if (vec >= 25 && vec <= 31) {
        kprintf(", level %u", vec - 24);
    }
    else if (vec >= 32 && vec <= 47) {
        kprintf(", #%u", vec - 32);
    }
    kprintf(") ***\n");
 
    kprintf("PC=%08lx  SR=%04x (%s)  frame format $%X\n",
            (unsigned long)f->pc, f->sr,
            (f->sr & SR_SUPERVISOR) ? "supervisor" : "user", fmt);
 
    /* Format-specific extra information */
    switch (fmt) {
        case 0x2:
        case 0x9:
            kprintf("Instruction address: %08lx\n",
                    (unsigned long)fr32(f, 0x08));
            break;
    
        case 0xA:
        case 0xB: {
            uint16_t ssw = fr16(f, 0x0A);

            kprintf("SSW=%04x", ssw);
            if (ssw & SSW_DF) {
                kprintf("  data %s fault at %08lx (FC=%u)",
                        (ssw & SSW_RW) ? "read" : "write",
                        (unsigned long)fr32(f, 0x10), ssw & 7);
            }
            
            if (ssw & (SSW_FB | SSW_FC)) {
                kprintf("  instruction fetch fault");
            }

            kprintf("\n");
            break;
        }
    }
 
    /* Register dump */
    for (i = 0; i < 8; i++) {
        kprintf("D%d=%08lx%s", i, (unsigned long)context->dregs[i],
                (i & 3) == 3 ? "\n" : "  ");
    }

    for (i = 0; i < 7; i++) {
        kprintf("A%d=%08lx%s", i, (unsigned long)context->aregs[i],
                (i & 3) == 3 ? "\n" : "  ");
    }
 
    if (size) {
        kprintf("SSP=%08lx\n",
                (unsigned long)((uintptr_t)f + size));  /* SSP before exception */
    }
    else {
        kprintf("SSP=????????\n");
    }
 
    kprintf("USP=%08lx\n", (unsigned long)context->usp);
    kprintf("System halted.\n");
 
    /*
     * Halt. Remove this loop for vectors you want to resume from
     * (e.g. TRAP, trace); the wrapper will then RTE normally.
     * Don't resume bus/address errors without fixing the cause,
     * or the faulting access will just repeat.
     */
    for (;;)
        __asm__ volatile("stop #0x2700");
}
