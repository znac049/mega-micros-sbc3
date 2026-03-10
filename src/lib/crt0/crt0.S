	.global _start
	.global exit

	.extern __bss_start
	.extern _end
	
	.text

	.equ hex_display,0xab0000
	.equ init_sp,0x00ffff
	.equ init_fp,init_sp-512

_start:

/* Setup the stack and frame pointer */
	move.l init_sp,%sp
	move.l init_fp,%fp

/* Initialise the bss segment */
	move.l #__bss_start,%a0

_bss_next:	
	cmpa.l #+end,%a0
	bgt _bss_zeroed

	clr.b (%a0)+
	bra _bss_next

_bss_zeroed:
	
/* ...and finally invoke main() */
	jsr	main

exit:	
/* all done - back to the monitor */
	move.b #228,%d7
	trap #14

	.end