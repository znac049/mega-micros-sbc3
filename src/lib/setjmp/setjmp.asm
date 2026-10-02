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

offs_d1 equ		0
offs_d2 equ		4
offs_d3 equ		8
offs_d4 equ		12
offs_d5 equ		16
offs_d6 equ		20
offs_d7 equ		24

offs_a1	equ		28
offs_a2	equ		32
offs_a3	equ		36
offs_a4	equ		40
offs_a5	equ		44
offs_a6	equ		48

offs_sp	equ		52
offs_sr	equ		56

offs_magic equ	58

offs_ra equ		62

; int setjmp(jmp_buf env)
; Stack on entry:
;   0(sp)  return address
;   4(sp)  env

setjmp::
		movea.l 4(sp),a0					; a0 = &env;

		move.l	#$deadface,offs_magic(a0)	; signature
		movem.l d1-d7/a1-a6,(a0)

		move.l  sp,offs_sp(a0)           	; stash SP
		move.w  sr,offs_sr(a0)				; stash SR
		move.l	(sp),offs_ra(a0)			; Stash the return address

		moveq   #0,d0                     	; the direct call always returns 0
		rts


; void longjmp(jmp_buf env, int val)
; Stack on entry:
;   0(sp)  return address (unused -- we never return here)
;   4(sp)  env
;   8(sp)  val

longjmp::
		move.l  8(sp),d0					; return code (val) passed to longjmp() must not be 0
		tst.l   d0
		bne     lj_ok
		moveq   #1,d0						; Special case of longjmp being called with 0:
lj_ok										; e.g. longjmp(env, 0) must make setjmp() return 1 instead

		movea.l 4(sp),a0					; a0 = &env

		movem.l	(a0),d1-d7/a1-a6			; Restoring machine state
		move.w  offs_sr(a0),sr				; Restore SR

		move.l offs_sp(a0),sp				; Restore SP
		movea.l	offs_ra(a0),a0				; Grab the saved return address
		move.l	a0,(sp)						; Fixup the return address on the stack

		rts									; Fall through hyperspace and "return" from the original
											; call to setjmp(), but with a non-zero result.