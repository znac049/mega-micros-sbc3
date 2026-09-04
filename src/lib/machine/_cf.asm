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

cf_reg_base     equ     $ae0000
cf_reg_status   equ     cf_reg_base+$0e

cf_status_busy  equ     $80
cf_status_drq   equ     $08

; returns (d0):
; 0 - good (busy gone away)
; -1 - timed out
_cf_wait_busy::
                move.l  #10000,d0
_cfwb_test
                btst.b  #cf_status_busy,cf_reg_status
                dbne     d0,_cfwb_test
                cmp.l   #-1,d0
                beq     _cfwb_timed_out         ; return -1
                move.l  #0,d0                   ; return 0
_cfwb_timed_out
                rts

_cf_wait_data::
                btst.b  #cf_status_drq,cf_reg_status
                beq     _cf_wait_data
                rts