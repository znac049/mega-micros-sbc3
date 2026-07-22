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

ofs_pc	equ		44
ofs_sp	equ		48
ofs_sr	equ		52

; int setjmp(jmp_buf env)
; Stack on entry:
;   0(sp)  return address
;   4(sp)  env

setjmp::
		movea.l 4(sp),a0					; env -> a0

		movem.l d2-d7/a2-a6,(a0)

		move.l  (sp),ofs_pc(a0)				; stash return address
		lea     4(sp),a1
		move.l  a1,ofs_sp(a0)           	; stash SP as it will be right after setjmp returns
		move.w  ccr,ofs_sr(a0)

		moveq   #0,d0                     	; the direct call always returns 0
		rts


; void longjmp(jmp_buf env, int val)
; Stack on entry:
;   0(sp)  return address (unused -- we never return here)
;   4(sp)  env
;   8(sp)  val

longjmp::
		movea.l 4(sp),a0
		move.l  8(sp),d1

		tst.l   d1
		bne.s   lj_ok
		moveq   #1,d1						; Special case of longjmp being called with 0:
											; e.g. longjmp(env, 0) must make setjmp. Return 1 instead
lj_ok
		movem.l (a0),d2-d7/a2-a6     		; restore the saved state
		move.w  ofs_sr(a0),ccr
		move.l  d1,d0                     	; return value from setjmp

		movea.l ofs_sp(a0),a1
		movea.l ofs_pc(a0),a0
		movea.l a1,sp                     	; reinstate the stack as per the call to setjmp...
		jmp     (a0)                       	; ...and magic happens
