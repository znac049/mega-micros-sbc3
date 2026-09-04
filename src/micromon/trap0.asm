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
        movem.l d1-d7/a0-a6,-(sp)       ; 56 Bytes
                                        ;  +8 bytes for exception frame
                                        ;  +4 bytes for RA
                                        ; =68 bytes total

        move.l  68(sp),d0               ; arguments to the trap0 C function - system call number
        move.l  72(sp),d1               ; param1
        move.l  76(sp),d2               ; param 2
        move.l  80(sp),d3               ; param3

        move.l  d0,d5                   ; Check bounds
        cmp.l   #num_syscalls,d5
        bhs     t0h_bad_num
        lsl.l   #2,d5                   ; Calculate offset into table

        lea     trap0_call_table,a1     ; Base of jumnp table
        move.l  (a1,d5.w),a1            ; Address of our vector

        move.l  d3,-(sp)                ; Pass args to the function
        move.l  d2,-(sp)
        move.l  d1,-(sp)
        
        jsr     (a1)
        lea     12(sp),sp               ; fix the stack

        bra     t0h_done

t0h_bad_num
        move.l  #-1,d0

t0h_done
        movem.l (sp)+,d1-d7/a0-a6
        rte


	section	.data,data

trap0_call_table::
        dc.l    bios_putchar
        dc.l    bios_getchar
        dc.l    bios_char_available
        dc.l    bios_exit
        dc.l    ticks
        dc.l    bios_open
        dc.l    bios_close
        dc.l    bios_creat
        dc.l    bios_read
        dc.l    bios_write
        dc.l    reset_ticks
        dc.l    bios_chdir
        dc.l    bios_getcwd
        dc.l    bios_malloc
        dc.l    bios_free
        dc.l    bios_getdents
t0ct_end     

        end