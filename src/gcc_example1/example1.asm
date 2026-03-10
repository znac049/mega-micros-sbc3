

init_sp equ $240000

        org     $280000

_start::
	    movea.l init_sp,sp


prloop:
        move.b  #'X',d0
        bsr     putchar

        bra     prloop

putchar:
        movem.l  d1/a0,-(sp)
        movea.l duart_base,a0

putchar_wait:
        move.b  2(a0),d1
        and.b   #8,d1
        beq     putchar_wait

        move.b  d0,6(a0)

        movem.l  (sp)+,d1/a0
        rts
