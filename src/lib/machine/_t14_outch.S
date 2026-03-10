			text
_t14_outch::
    		movem.l d0/d1/d7/a0,-(sp)
			move.b #248,d7
			trap #14
    		movem.l (sp)+,d0/d1/d7/a0
    		rts

			end