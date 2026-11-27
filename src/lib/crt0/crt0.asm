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
	bsr		run_init_array	; the constructors run first
	jsr	    main			; jsr, not bsr: a big program puts main
					; more than 32K away

; Calling exit() takes care of any housekeeping: the atexit() functions
; and C++'s static destructors. main()'s return value is the exit status.
	move.l	d0,-(sp)
	jsr		exit			; passes control back to the monitor


; Call each function in the init array (see sections.ld): the C++
; compiler puts the constructors of static objects there, and C's
; __attribute__((constructor)) functions go there too
run_init_array:
	movem.l	a2-a3,-(sp)
	lea		__init_array_start,a2
	lea		__init_array_end,a3
.next:
	cmp.l	a3,a2
	beq.s	.done
	move.l	(a2)+,a0
	jsr		(a0)
	bra.s	.next
.done:
	movem.l	(sp)+,a2-a3
	rts

	


	ifnd BAREMETAL

; usage: do_trap0(syscall_number, arg1, arg2, arg3, *err_num)
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


