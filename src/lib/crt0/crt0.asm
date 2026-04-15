init_sp equ $27fffe
init_fp equ init_sp-4096

    section .text,code

_start::
; Setup the stack and frame pointer
	move.l 	#init_sp,sp
	move.l 	#init_fp,fp

* Init BSS
	move.l 	#_bss_start,a0
ibloop:	
    cmp.l	#_bss_end,a0
	beq		ibdone
	clr.b   (a0)+
	bra.s   ibloop

ibdone:
; Check if we're running in ROM or RAM
	clr.b 	running_in_rom
	lea		(pc),a0
	move.l  a0,d0
	cmp.l	#$c00000,d0
	blt		in_ram
	move.b  #1,running_in_rom
in_ram:

; Setup system timer
	bsr		_claim_pit;

; Take control of the duart
	bsr		_claim_duart

; Initialise the heap
	bsr		_init_heap

	bsr		pre_main

; invoke main() 
	bsr	main


exit::
; all done - relinquish control of the serial ports
	bsr		_release_duart

; ...and the PIT
	bsr		_release_pit

; ...and pass control to the monitor
	move.b #228,d7
	trap #14


twiddle_thumbs::
    movem.l d2/d6,-(sp)

    move.l  #1000000,d2
    moveq.l #1,d6
spin:
    sub.l   d6,d2
    bne.s   spin

    movem.l (sp)+,d2/d6
	rts


get_heap_start::
	move.l	a0,-(sp)
	lea		_bss_end,a0
	addq.l  #4,a0
	move.l  a0,d0
	and.l	#$fffffffc,d0
	move.l	(sp)+,a0
	rts


	section	.data,data

running_in_rom::
	dc.b	0

	end

