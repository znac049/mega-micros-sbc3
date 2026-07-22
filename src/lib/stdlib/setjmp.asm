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

    	section .text,code

offs_a6	equ		56
offs_sp	equ		60
offs_ra	equ		64
offs_sr	equ		68

; int setjmp(jmp_buf env)
; Stack on entry:
;   0(sp)  return address
;   4(sp)  env

setjmp::
		movem.l	a1/a6,-(sp)		
		movea.l 12(sp),a6					; env -> a6

		movem.l d0-d7/a0-a5,(a6)			; a6 has been trashed

		move.l	4(sp),offs_a6(a6)			; original a6
		move.l  8(sp),offs_ra(a6)			; stash return address
		lea     12(sp),a1					; sp just prior to the call to setjmp()
		move.l  a1,offs_sp(a6)           	; stash SP as it will be right after setjmp returns
		move.w  ccr,offs_sr(a6)

		movem.l	(sp)+,a1/a6					; restore original a1/a6

		moveq   #0,d0                     	; the direct call always returns 0
		rts


; void longjmp(jmp_buf env, int val)
; Stack on entry:
;   0(sp)  return address (unused -- we never return here)
;   4(sp)  env
;   8(sp)  val

longjmp::
		movea.l 4(sp),a6					; env -> a6

		movem.l	d0-d7/a0-a5,(a0)			; start restoring machine state - will trash a1 -> env

		move.l  8(sp),d0					; return code passed to longjmp()

		tst.l   d0
		bne     lj_ok
		moveq   #1,d0						; Special case of longjmp being called with 0:
											; e.g. longjmp(env, 0) must make setjmp() return 1 instead
lj_ok
		movea.l offs_sp(a6),a0
		movea.l a0,sp 						; restore the stack pointer     

		movea.l ofs_ra(a6),a0
		move.l	a0,-(sp);					; push the return address

		move.w  offs_sr(a6),ccr				; Restore CCR
		
		move.l	offs_a6(a6),a6				; Finally restore a6 itself

		rts									; Fall through hyperspace...