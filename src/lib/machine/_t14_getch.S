	text

_t14_getch::
    		movem.l d1/d7/a0,-(sp)
			move.b #247,d7
			trap #14
    		movem.l (sp)+,d1/d7/a0
    		rts

			end