; MIT License

; Copyright (c) 2026 Bob Green

; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:

; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.

; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.

; Kernel boot trampoline.
;
; This code is never run where it was linked. netbsd.c copies it (from
; boot_trampoline to boot_trampoline_end) to the top of RAM, out of the way
; of the kernel's destination, and calls the copy. It's position-independent
; and uses only registers, so it doesn't matter what the copy overwrites -
; including this program and the monitor's stack.
;
; C prototype (arguments on the stack, as usual):
;   void trampoline(uint32_t len, const void *src, void *dest,
;                   void *entry, void *new_sp);

                section .text,code

boot_trampoline::
                move.w  #$2700,sr               ; no interrupts from here on

                ; movem always fills d0-d7 then a0-a7, so this loads
                ; d0=len, a0=src, a1=dest, a2=entry, a3=new_sp
                movem.l 4(sp),d0/a0-a3

                move.l  a3,sp                   ; stack at the top of RAM

                tst.l   d0
                beq.s   .go

.copy           move.b  (a0)+,(a1)+             ; dest is below src, so a
                subq.l  #1,d0                   ; forward copy is safe even
                bne.s   .copy                   ; if they overlap

.go             move.l  #$0808,d0               ; clear both caches and leave
                movec   d0,cacr                 ; them off for the kernel

                jmp     (a2)

boot_trampoline_end::
