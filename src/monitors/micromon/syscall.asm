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

num_syscalls equ     (t0ct_end-trap0_call_table)/4


        section .text,code

trap0_handler::
        movem.l d1-d7/a0-a6,-(sp)
        move.l  d0,d1
        cmp.l   #num_syscalls,d1
        bhs     t0h_bad_num
        lsl.l   #2,d1
        lea     trap0_call_table,a0
        move.l  (a0,d1.w),a0
        jsr     (a0)
        bra     t0h_done

t0h_bad_num
        move.l  #-1,d0

t0h_done
        movem.l (sp)+,d1-d7/a0-a6
        rte


	section	.data,data

trap0_call_table::
        dc.l    0
t0ct_end     

        end