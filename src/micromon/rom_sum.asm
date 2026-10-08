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
; rom_checksum.s  -  Register-only ROM checksum for 68030 boot ROM
;
; ROM:       $C00000 - $FFFFFF  (4 MB = $100000 longwords)
; Algorithm: 32-bit ones'-complement sum (add with end-around carry) over
;            EVERY longword in the ROM, including the embedded checksum.
;            The build tool stores a checksum longword chosen so that the
;            total comes out to $FFFFFFFF. Because the stored value is part of
;            the summed data, the loop needs no special case for skipping it,
;            and the checksum can live anywhere in the ROM (by convention, the
;            last longword at $FFFFFC - see romsum.py).
;
; No RAM is touched: no stack, no BSR/JSR/RTS, no variables.
;
; Calling convention (no stack available):
;       lea     .back(pc),a6        ; return address in A6
;       bra     RomChecksum
; .back:
;       bne     RomBad              ; Z clear -> checksum failure
;
; On exit:
;       D0.L = 0 if the checksum is good, nonzero if it is bad
;       CCR  Z set = good, Z clear = bad
; Clobbers: D0, D1, D2, A0   (A6 preserved)
;==============================================================================

ROM_BASE        equ     $00C00000
ROM_SIZE        equ     $00400000               ; 4 MB
ROM_LONGS       equ     ROM_SIZE/4              ; $100000 longwords
UNROLL          equ     16                      ; longwords per loop pass
ROM_PASSES      equ     ROM_LONGS/UNROLL        ; $10000 = 65536 passes

rom_checksum::
        lea     ROM_BASE,a0                     ; A0 = read pointer
        moveq   #0,d0                           ; D0 = running sum
        moveq   #0,d2                           ; D2 = constant 0 for ADDX
        move.w  #ROM_PASSES-1,d1                ; $FFFF -> DBRA runs 65536x

.loop:
        rept    UNROLL
        add.l   (a0)+,d0                        ; sum += longword, X = carry
        addx.l  d2,d0                           ; fold carry back in
        endr                                    ; (can't re-carry: max $FFFFFFFE+1)
        dbra    d1,.loop                        ; DBRA leaves CCR/X untouched

        not.l   d0                              ; $FFFFFFFF -> 0 if good
        jmp     (a6)                            ; flags set by NOT.L