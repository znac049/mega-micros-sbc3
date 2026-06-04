    section .text,code

_start::
; Setup the stack and frame pointer
	move.l  #_INITIAL_STACK,sp

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

	bsr		pre_main

; invoke main() 
	bsr	    main
	bra     done

exit::
	move.l  4(sp),d0
done:
	move.l  d0,-(sp)
	bsr		post_main
	move.l  (sp)+,d0

; ...and pass control to the monitor
	move.b #228,d7
	trap #14


; twiddle_thumbs::
;     movem.l d2/d6,-(sp)

;     move.l  #1000000,d2
;     moveq.l #1,d6
; spin:
;     sub.l   d6,d2
;     bne.s   spin

;     movem.l (sp)+,d2/d6
; 	rts


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

