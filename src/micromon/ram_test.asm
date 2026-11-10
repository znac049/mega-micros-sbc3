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


;==============================================================================
; ram_test.s  -  Register-only quick RAM test, 68030 boot ROM
;
; Tests:    $000008 - $0FFFFF  (first 1 MB)
; Purpose:  A fast confidence check before the stack is set up. This is not
;           an exhaustive cell test.
;
; Stages (the number is returned in D0 on failure):
;   1  Data bus  - walking 1 across all 32 data lines at $000000
;   2  Byte lanes - byte and word writes land in the right lanes
;   3  Address bus - power-of-two addresses don't alias each other
;   4  Fill/verify - every longword holds its own address
;   5  Fill/verify - every longword holds the complement of its address
;
; Calling convention (no stack available):
;       lea     .back(pc),a6        ; return address in A6
;       bra     RamTest
; .back:
;       bne     RamBad              ; Z clear -> failure
;
; On exit:
;   OK:     D0.L = 0, Z set
;   Fail:   D0.L = stage number (1-5), Z clear
;           A0.L = failing address
;           D1.L = expected value
;           D2.L = value actually read
; Clobbers: D0-D3, A0
;
; On exit, RAM holds the stage 5 pattern (not zeroed).


RAM_START       equ     $00000400
RAM_END         equ     $00100000

ram_test::

;------------------------------------------------------------------------------
; Stage 1: data bus, walking 1 at $000008.
; After each write, the complement is written to $00000C, so the bus is driven
; to the opposite state before the read. A missing or floating data line can't
; pass by "remembering" the last value on the bus.

        moveq   #1,d0                           ; Stage number

        lea     RAM_START,a0
        moveq   #1,d1                           ; Start with bottom bit
.s1_tstbit:
        move.l  d1,(a0)                         ; Write the bit

        move.l  d1,d2
        not.l   d2
;        move.l  d2,4(a0)                        ; drive the bus the other way

        cmp.l   (a0),d1                         ; Test the bit
        bne     .rt_fail

        lsl.l   #1,d1                           ; next bit
        bne     .s1_tstbit

;------------------------------------------------------------------------------
; Stage 2: byte lanes. Catches byte/word strobe (UDS/LDS style) wiring faults
; that a long-only test would never see.

        moveq   #2,d0                           ; Stage number

        clr.l   (a0)
        move.b  #$11,(a0)
        move.b  #$22,1(a0)
        move.b  #$33,2(a0)
        move.b  #$44,3(a0)
        move.l  #$11223344,d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne     .rt_fail
        move.w  #$5566,2(a0)                    ; low word only
        move.l  #$11225566,d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne     .rt_fail
        move.w  #$7788,(a0)                     ; high word only
        move.l  #$77885566,d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne     .rt_fail

;------------------------------------------------------------------------------
; Stage 3: address bus. Write ~addr to $0 and to each power-of-two address
; ($4, $8 ... $80000), then read them all back. A stuck or shorted address
; line makes two of these addresses hit the same cell, so one write
; overwrites the other and the readback shows it.

        moveq   #3,d0                           ; Stage number

        lea     RAM_START,a0

        moveq   #-1,d1                          ; ~$0
        move.l  d1,(a0)
        moveq   #4,d3
.s3write:
        movea.l d3,a0
        move.l  d3,d1
        not.l   d1
        move.l  d1,(a0)
        add.l   d3,d3
        cmp.l   #RAM_END,d3
        blo.s   .s3write

        lea     RAM_START,a0
        
        moveq   #-1,d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne     .rt_fail
        moveq   #4,d3
.s3read:
        movea.l d3,a0
        move.l  d3,d1
        not.l   d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne     .rt_fail
        add.l   d3,d3
        cmp.l   #RAM_END,d3
        blo.s   .s3read

;------------------------------------------------------------------------------
; Stage 4: fill all of RAM with address-in-address, then verify.
; Every cell gets a different value, so decode faults, missing chips and
; stuck bits all show up. The whole fill finishes before any reads, so
; bus capacitance can't fake a pass.

        moveq   #4,d0                           ; Stage number

        lea     RAM_START,a0

.s4write:
        move.l  a0,d1
        move.l  d1,(a0)+
        cmpa.l  #RAM_END,a0
        blo.s   .s4write

        movea.l a5,a0                           ; Start address
.s4read:
        move.l  a0,d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne.s   .rt_fail
        addq.l  #4,a0
        cmpa.l  #RAM_END,a0
        blo.s   .s4read

;------------------------------------------------------------------------------
; Stage 5: the same, with the complement, so every bit of every cell has now
; held both a 0 and a 1.
;------------------------------------------------------------------------------
        moveq   #5,d0                           ; Stage number

        lea     RAM_START,a0

.s5write:
        move.l  a0,d1
        not.l   d1
        move.l  d1,(a0)+
        cmpa.l  #RAM_END,a0
        blo.s   .s5write

        lea     RAM_START,a0
.s5read:
        move.l  a0,d1
        not.l   d1
        move.l  (a0),d2
        cmp.l   d1,d2
        bne.s   .rt_fail
        addq.l  #4,a0
        cmpa.l  #RAM_END,a0
        blo.s   .s5read


.rt_pass:
        moveq   #0,d0                           ; pass: D0 = 0, Z set
        jmp     (a6)

.rt_fail:
        tst.l   d0                              ; fail: Z clear, D0 = stage
        jmp     (a6)