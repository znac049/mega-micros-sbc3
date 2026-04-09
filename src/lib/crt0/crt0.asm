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
; Setup system timer
	bsr		_claim_pit;

; Take control of the duart
	bsr		_claim_duart

; Initialise the heap
	bsr		_init_heap

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

	end
