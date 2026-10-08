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



;======================================================================================
; Boot ROM for SBC-3 system
;
; On reset, the monitor has to take careful steps before it can
; assume we have a fully working system:
;
; Immediately after reset, SP has been set but we don't know that the
; memory area it points to is ok. Nor do we know that the ROM is
; completely valid. So, as quickly as possible, we need to do the following:
;
; 1. Calculate a ROM checksum and compare it against a checksum (stored in
;    te ROM). 
; 2. Quickly perform a functional memory test between 0 and 1MB.

; Beyond this point, we can use the stack and the first MB of RAM.

; 3. Copy the vector table into RAM and make sure VBR is pointing at it.
; 4. Relocate the C data section from ROM into RAM
; 5. Initialise the C BSS. 
; 6. Probe to discover how much memory we have.

; Beyond this point, it's safe to call C code in the ROM.

; Fill in C global structure: sysinfo_t system_info;
; 7. Call main(void)

;======================================================================================


        include "machine_defs.inc"


VECTOR_BASE     equ 8                           ; Address to copy the vector table into


        include "vectable.inc"


        section .text,code

_start::
; Setup the stack and frame pointer
	move.l  #_INITIAL_STACK,sp


; Initialise PIT ports A and B as outputs, so we can use the
; LEDs for boot diagnostics - change this once we have a hex
; display board
;
        lea     PIT_BASE,a5
        move.b  #$FF,d0                 ; All bits are outputs
        move.b  d0,pit_paddr_o(a5)      ; Port A
        move.b  d0,pit_pbddr_o(a5)      ; Port B

        move.b  d0,pit_padr(a5)
        move.b  d0,pit_pbdr(a5)


; Check the ROM checksum
        lea     sum_done(pc),a6          ; Can't trust the stack yet
        jmp     rom_checksum
sum_done:
        beq     sum_ok

; Rom checksum failed :-(
        move.b  #$F0,d0
        move.b  d0,pit_pbdr_o(a5)       ; Light some LEDs

.stop:        
        stop    #$2700
        bra     .stop

sum_ok:

; Now check the first 1meg of ran
        lea     ram_test_done(pc),a6    ; Still can't trust the stack
        jmp     ram_test
ram_test_done:
        beq     ram_ok



ram_ok:

; copy the vector table into RAM at VECTOR_BASE
;
        lea     vectors+8,a0
        lea     VECTOR_BASE,a1
        move.w  #253,d0
cpvec:
        move.l  (a0)+,d1
        move.l  d1,(a1)+
        dbeq    d0,cpvec

; relocate the data section into RAM
;
        move.l  #_data_load_start,a0    ; Where to copy from
        move.l  #_d_start,a1            ; where to copy to
        move.l  #_data_length,d0        ; Number of bytes to copy
        subq.l  #1,d0


cpdata:
        move.l  (a0)+,d1
        move.l  d1,(a1)+
        dbra    d0,cpdata               



; Init BSS
;
    	move.l 	#_bss_start,a0
ibloop:	
        cmp.l	#_bss_end,a0
        beq	ibdone
        clr.l   (a0)+
        bra.s   ibloop

ibdone:


; invoke main() 
        bsr	main
        bra     done

; We're running in ROM, so exit should never get called.
exit::
    	move.l  4(sp),d0
done:
        move.b  running_in_rom,d0
        bra     done                            ; Don't know what else to do!


get_heap_start::
        move.l	a0,-(sp)
        lea	_bss_end,a0
        addq.l  #4,a0
        move.l  a0,d0
        and.l	#$fffffffc,d0
        move.l	(sp)+,a0
        rts


; default "do nothing" exception handler
_not_handled::
        movem.l d0-d7/a0-a6,-(sp)               ; Save general registers
        move.l  usp,a0
        move.l  a0,-(sp)                        ; Save usp

        move.l  sp,-(sp)                        ; Pass sp into C function
        jsr     catch_exception
        addq.l  #4,sp

        move.l  (sp)+,a0                        ; Restore registers
        move.l  a0,usp
        movem.l (sp)+,d0-d7/a0-a6

        rte

; BUS ERROR handler - set a flag
_bus_err_exception::
        movem.l a0,-(sp)
        move.b  #1,bus_error_flag
        lea.l   $af0001,a0
        move.b  #0,18(a0)
        movem.l (sp)+,a0
        rte


        section .pretext,code

romsum::
        dc.l    0
        

    	section	.data,data

running_in_rom::
    	dc.b	1

bus_error_flag::
        dc.b    0

    	end



