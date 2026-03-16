	.global _start
	.global exit
    .global twiddle_thumbs

	.text

	.equ init_sp,0x27fff0
	.equ init_fp,init_sp-1024

_start:

/* Setup the stack and frame pointer */
	move.l #init_sp,%sp
	move.l #init_fp,%fp

/* Init BSS */
	move.l 	#_bss_start,%a0
1:	cmp.l	#_bss_end,%a0
	beq		2f
	clr.b   (%a0)+
	bra.s   1b	
2:

/* invoke main() */
	bsr	main

exit:	
/* all done - back to the monitor */
	move.b #228,%d7
	trap #14

twiddle_thumbs:
    movem.l %d2/%d6,-(%sp)

    move.l  #1000000,%d2
    moveq.l #1,%d6
spin:
    sub.l   %d6,%d2
    bne.s   spin

    movem.l (%sp)+,%d2/%d6
    rts

	.end

