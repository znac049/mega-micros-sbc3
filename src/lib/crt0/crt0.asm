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

_start::
; Setup the stack and frame pointer
	; move.l  #_INITIAL_STACK,sp

* Init BSS
	move.l 	#_bss_start,a0
ibloop:	
    cmp.l	#_bss_end,a0
	beq		ibdone
	clr.b   (a0)+
	bra.s   ibloop

ibdone:
;	bsr		pre_main

; invoke main() 
	move.l	4(sp),d0		; pass argc, argv to main()
	move.l	8(sp),d1
	movem.l	d0-d1,-(sp)
	bsr	    main

ifd BAREMETAL
 	bra     done

exit::
 	move.l  4(sp),d0		; grab exit code
done:
	move.l  d0,-(sp)
	bsr		post_main
	move.l  (sp)+,d0

; ...and pass control to the monitor
	rts
endif

ifnd BAREMETAL
; usage: do_trap0(syscall_number, arg1, arg2, arg)
;
do_trap0::
	trap #0
	rts
endif


; usage: get_heap_start()
;
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


