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

                section .text,code


ill_inst_vec    equ     4<<2

; Result in d0:
;    0: 68000/68008
;    1: 68010
;    2: 68020
;    3: 68030
;
detect_cpu_type::
                movem.l d1/d2/d3/a0/a2,-(sp)

                move.l  #0,a0
                move.l  ill_inst_vec(a0),d1     ; Temporarily take over the illegal
                move.l  d1,saved_vector         ; instruction handler.

                lea     illegal_inst,a2
                movea.l a2,ill_inst_vec(a0)

                move.l  sp,a2                   ; Remember the stack pointer

                move.b  #0,d3
                moveq.l #0,d0                   ; Default is 68000/68008 (0)

; Now try executing instructions for each processor in ascending order until one fails
                move    ccr,d2                  ; Needs 68010 (1)
                tst.b   d3                      ; Did an exception take place?
                bne     cpu_detected
                addi.l  #1,d0

                movec   cacr,d2                 ; Needs 68020 (2)
                tst.b   d3                      ; Did an exception take place?
                bne     cpu_detected
                addi.l  #1,d0

                pmove   tc,saved_tt0            ; Needs a 68030 (3)
                tst.b   d3                      ; Did an exception take place?
                bne     cpu_detected
                addi.l  #1,d0

cpu_detected
                move.l  saved_vector,d1         ; instruction handler
                move.l  d1,ill_inst_vec(a0)     

                movem.l (sp)+,d1/d2/d3/a0/a2
                rts

illegal_inst:
                move.b  #$ff,d3
                rte

                section .data,data

saved_vector    dc.l    0
saved_tt0       dc.l    0