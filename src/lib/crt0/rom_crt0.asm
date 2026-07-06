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

        include "machine_defs.inc"
        
VECTOR_BASE equ 0                       ; Address to copy the vetor table into

       section .pretext,code

; Machine vectors
vectors::
        dc.l    _INITIAL_STACK      	;   0
        dc.l    _start              	;   1
        dc.l    _not_handled        	;   2 - Bus Error
        dc.l    _not_handled        	;   3 - Address Error
        dc.l    _not_handled        	;   4 - Illegal instruction
        dc.l    _not_handled        	;   5 - Divide by 0
        dc.l    _not_handled        	;   6 - CHK, CHK2 instruction
        dc.l    _not_handled        	;   7 - cpTRAPcc, TRAPcc, TRAPV instructions
        dc.l    _not_handled        	;   8 - Privilege Violation
        dc.l    _not_handled        	;   9 - Trace
        dc.l    _not_handled        	;  10 - LINE 1010 Emulator
        dc.l    _not_handled        	;  11 - LINE 1111 Emulator
        dc.l    _not_handled        	;  12 - Reserved
        dc.l    _not_handled        	;  13 - Coprocessor Violation
        dc.l    _not_handled        	;  14 - Format error
        dc.l    _not_handled        	;  15  -Unititialised interrupt

; 16-23 - Reserved
        dc.l    _not_handled        	;  16
        dc.l    _not_handled        	;  17
        dc.l    _not_handled        	;  18
        dc.l    _not_handled        	;  19
        dc.l    _not_handled        	;  20
        dc.l    _not_handled        	;  21
        dc.l    _not_handled        	;  22
        dc.l    _not_handled        	;  23

        dc.l    _not_handled        	;  24 - Spurious interrupt
        dc.l    _not_handled        	;  25 - Level 1 interrupt autovector
        dc.l    _not_handled        	;  26 - Level 1 interrupt autovector
        dc.l    _not_handled        	;  27 - Level 1 interrupt autovector
        dc.l    _not_handled        	;  28 - Level 1 interrupt autovector
        dc.l    _not_handled        	;  29 - Level 1 interrupt autovector
        dc.l    _not_handled        	;  30 - Level 1 interrupt autovector
        dc.l    _not_handled        	;  31 - Level 1 interrupt autovector

        dc.l    _not_handled        	;  32 - TRAP #0 instruction
        dc.l    _not_handled        	;  33 - TRAP #1 instruction
        dc.l    _not_handled        	;  34 - TRAP #2 instruction
        dc.l    _not_handled        	;  35 - TRAP #3 instruction
        dc.l    _not_handled        	;  36 - TRAP #4 instruction
        dc.l    _not_handled        	;  37 - TRAP #5 instruction
        dc.l    _not_handled        	;  38 - TRAP #6 instruction
        dc.l    _not_handled        	;  39 - TRAP #7 instruction
        dc.l    _not_handled        	;  40 - TRAP #8 instruction
        dc.l    _not_handled        	;  41 - TRAP #9 instruction
        dc.l    _not_handled        	;  42 - TRAP #10 instruction
        dc.l    _not_handled        	;  43 - TRAP #11 instruction
        dc.l    _not_handled        	;  44 - TRAP #12 instruction
        dc.l    _not_handled        	;  45 - TRAP #13 instruction
        dc.l    _not_handled        	;  46 - TRAP #14 instruction
        dc.l    _not_handled        	;  47 - TRAP #15 instruction

        dc.l    _not_handled        	;  48 - FPCP Branch or Set on Unordered Condition
        dc.l    _not_handled        	;  49 - FPCP Inexact Result
        dc.l    _not_handled        	;  50 - FPCP Divide by Zero
        dc.l    _not_handled        	;  51 - FPCP Underflow
        dc.l    _not_handled        	;  52 - FPCP Operand Error
        dc.l    _not_handled        	;  53 - FPCP Overflow
        dc.l    _not_handled        	;  54 - FPCP Signaling NAN
        dc.l    _not_handled        	;  55 - Reserved
        dc.l    _not_handled        	;  56 - MMU Configuration Error
        dc.l    _not_handled        	;  57 - Defined for MC68851 not used by MC68030
        dc.l    _not_handled        	;  58 - Defined for MC68851 not used by MC68030

        dc.l    _not_handled        	;  59 - Reserved
        dc.l    _not_handled        	;  60 - Reserved
        dc.l    _not_handled        	;  61 - Reserved
        dc.l    _not_handled        	;  62 - Reserved
        dc.l    _not_handled        	;  63 - Reserved

; The remianing vectors are user defined
        dc.l    _not_handled        	;  64
        dc.l    _not_handled        	;  65
        dc.l    _not_handled        	;  66
        dc.l    _not_handled        	;  67
        dc.l    _not_handled        	;  68
        dc.l    _not_handled        	;  69
        dc.l    _not_handled        	;  70
        dc.l    _not_handled        	;  71
        dc.l    _not_handled        	;  72
        dc.l    _not_handled        	;  73
        dc.l    _not_handled        	;  74
        dc.l    _not_handled        	;  75
        dc.l    _not_handled        	;  76
        dc.l    _not_handled        	;  77
        dc.l    _not_handled        	;  78
        dc.l    _not_handled        	;  79
        dc.l    _not_handled        	;  80
        dc.l    _not_handled        	;  81
        dc.l    _not_handled        	;  82
        dc.l    _not_handled        	;  83
        dc.l    _not_handled        	;  84
        dc.l    _not_handled        	;  85
        dc.l    _not_handled        	;  86
        dc.l    _not_handled        	;  87
        dc.l    _not_handled        	;  88
        dc.l    _not_handled        	;  89
        dc.l    _not_handled        	;  90
        dc.l    _not_handled        	;  91
        dc.l    _not_handled        	;  92
        dc.l    _not_handled        	;  93
        dc.l    _not_handled        	;  94
        dc.l    _not_handled        	;  95
        dc.l    _not_handled        	;  96
        dc.l    _not_handled        	;  97
        dc.l    _not_handled        	;  98
        dc.l    _not_handled        	;  99
        dc.l    _not_handled        	; 100
        dc.l    _not_handled        	; 101
        dc.l    _not_handled        	; 102
        dc.l    _not_handled        	; 103
        dc.l    _not_handled        	; 104
        dc.l    _not_handled        	; 105
        dc.l    _not_handled        	; 106
        dc.l    _not_handled        	; 107
        dc.l    _not_handled        	; 108
        dc.l    _not_handled        	; 109
        dc.l    _not_handled        	; 110
        dc.l    _not_handled        	; 111
        dc.l    _not_handled        	; 112
        dc.l    _not_handled        	; 113
        dc.l    _not_handled        	; 114
        dc.l    _not_handled        	; 115
        dc.l    _not_handled        	; 116
        dc.l    _not_handled        	; 117
        dc.l    _not_handled        	; 118
        dc.l    _not_handled        	; 119
        dc.l    _not_handled        	; 120
        dc.l    _not_handled        	; 121
        dc.l    _not_handled        	; 122
        dc.l    _not_handled        	; 123
        dc.l    _not_handled        	; 124
        dc.l    _not_handled        	; 125
        dc.l    _not_handled        	; 126
        dc.l    _not_handled        	; 127
        dc.l    _not_handled        	; 128
        dc.l    _not_handled        	; 129
        dc.l    _not_handled        	; 130
        dc.l    _not_handled        	; 131
        dc.l    _not_handled        	; 132
        dc.l    _not_handled        	; 133
        dc.l    _not_handled        	; 134
        dc.l    _not_handled        	; 135
        dc.l    _not_handled        	; 136
        dc.l    _not_handled        	; 137
        dc.l    _not_handled        	; 138
        dc.l    _not_handled        	; 139
        dc.l    _not_handled        	; 140
        dc.l    _not_handled        	; 141
        dc.l    _not_handled        	; 142
        dc.l    _not_handled        	; 143
        dc.l    _not_handled        	; 144
        dc.l    _not_handled        	; 145
        dc.l    _not_handled        	; 146
        dc.l    _not_handled        	; 147
        dc.l    _not_handled        	; 148
        dc.l    _not_handled        	; 149
        dc.l    _not_handled        	; 150
        dc.l    _not_handled        	; 151
        dc.l    _not_handled        	; 152
        dc.l    _not_handled        	; 153
        dc.l    _not_handled        	; 154
        dc.l    _not_handled        	; 155
        dc.l    _not_handled        	; 156
        dc.l    _not_handled        	; 157
        dc.l    _not_handled        	; 158
        dc.l    _not_handled        	; 159
        dc.l    _not_handled        	; 160
        dc.l    _not_handled        	; 161
        dc.l    _not_handled        	; 162
        dc.l    _not_handled        	; 163
        dc.l    _not_handled        	; 164
        dc.l    _not_handled        	; 165
        dc.l    _not_handled        	; 166
        dc.l    _not_handled        	; 167
        dc.l    _not_handled        	; 168
        dc.l    _not_handled        	; 169
        dc.l    _not_handled        	; 170
        dc.l    _not_handled        	; 171
        dc.l    _not_handled        	; 172
        dc.l    _not_handled        	; 173
        dc.l    _not_handled        	; 174
        dc.l    _not_handled        	; 175
        dc.l    _not_handled        	; 176
        dc.l    _not_handled        	; 177
        dc.l    _not_handled        	; 178
        dc.l    _not_handled        	; 179
        dc.l    _not_handled        	; 180
        dc.l    _not_handled        	; 181
        dc.l    _not_handled        	; 182
        dc.l    _not_handled        	; 183
        dc.l    _not_handled        	; 184
        dc.l    _not_handled        	; 185
        dc.l    _not_handled        	; 186
        dc.l    _not_handled        	; 187
        dc.l    _not_handled        	; 188
        dc.l    _not_handled        	; 189
        dc.l    _not_handled        	; 190
        dc.l    _not_handled        	; 191
        dc.l    _not_handled        	; 192
        dc.l    _not_handled        	; 193
        dc.l    _not_handled        	; 194
        dc.l    _not_handled        	; 195
        dc.l    _not_handled        	; 196
        dc.l    _not_handled        	; 197
        dc.l    _not_handled        	; 198
        dc.l    _not_handled        	; 199
        dc.l    _not_handled        	; 200
        dc.l    _not_handled        	; 201
        dc.l    _not_handled        	; 202
        dc.l    _not_handled        	; 203
        dc.l    _not_handled        	; 204
        dc.l    _not_handled        	; 205
        dc.l    _not_handled        	; 206
        dc.l    _not_handled        	; 207
        dc.l    _not_handled        	; 208
        dc.l    _not_handled        	; 209
        dc.l    _not_handled        	; 210
        dc.l    _not_handled        	; 211
        dc.l    _not_handled        	; 212
        dc.l    _not_handled        	; 213
        dc.l    _not_handled        	; 214
        dc.l    _not_handled        	; 215
        dc.l    _not_handled        	; 216
        dc.l    _not_handled        	; 217
        dc.l    _not_handled        	; 218
        dc.l    _not_handled        	; 219
        dc.l    _not_handled        	; 220
        dc.l    _not_handled        	; 221
        dc.l    _not_handled        	; 222
        dc.l    _not_handled        	; 223
        dc.l    _not_handled        	; 224
        dc.l    _not_handled        	; 225
        dc.l    _not_handled        	; 226
        dc.l    _not_handled        	; 227
        dc.l    _not_handled        	; 228
        dc.l    _not_handled        	; 229
        dc.l    _not_handled        	; 230
        dc.l    _not_handled        	; 231
        dc.l    _not_handled        	; 232
        dc.l    _not_handled        	; 233
        dc.l    _not_handled        	; 234
        dc.l    _not_handled        	; 235
        dc.l    _not_handled        	; 236
        dc.l    _not_handled        	; 237
        dc.l    _not_handled        	; 238
        dc.l    _not_handled        	; 239
        dc.l    _not_handled        	; 240
        dc.l    _not_handled        	; 241
        dc.l    _not_handled        	; 242
        dc.l    _not_handled        	; 243
        dc.l    _not_handled        	; 244
        dc.l    _not_handled        	; 245
        dc.l    _not_handled        	; 246
        dc.l    _not_handled        	; 247
        dc.l    _not_handled        	; 248
        dc.l    _not_handled        	; 249
        dc.l    _not_handled        	; 250
        dc.l    _not_handled        	; 251
        dc.l    _not_handled        	; 252
        dc.l    _not_handled        	; 253
        dc.l    _not_handled        	; 254
        dc.l    _not_handled            ; 255


    section .text,code

_start::
; Setup the stack and frame pointer
	move.l  #_INITIAL_STACK,sp

; Initialise PIT ports A and B as outputs
        lea.l   PIT_BASE,a5
        move.b  #$FF,d0                 ; All bits are outputs
        move.b  d0,pit_paddr_o(a5)      ; Port A
        move.b  d0,pit_pbddr_o(a5)      ; Port B

; Light a single LED on port A
        move.b  #$FE,d0
        move.b  d0,pit_padr_o(a5)

; copy the vector table into RAM at VECTOR_BASE
        lea.l   vectors,a0
        move.l  #VECTOR_BASE,a1
        move.w  #255,d0
cpvec:
        move.l  (a0)+,d1
        move.l  d1,(a1)+
        dbra    d0,cpvec

; Light a single LED on port A
        move.b  #$FD,d0
        move.b  d0,pit_padr_o(a5)


; relocate the data section into RAM
        move.l  #_data_load_start,a0
        move.l  #_data_start,a1
cpdata:
        move.l  (a0)+,d1
        move.l  d1,(a1)+
        cmp.l   #_data_end,a0
        blt     cpdata

; Light a single LED on port A
        move.b  #$FB,d0
        move.b  d0,pit_padr_o(a5)

* Init BSS
    	move.l 	#_bss_start,a0
ibloop:	
        cmp.l	#_bss_end,a0
        beq		ibdone
        clr.l   (a0)+
        bra.s   ibloop

ibdone:
; Light a single LED on port A
        move.b  #$F7,d0
        move.b  d0,pit_padr_o(a5)


; invoke main() 
        bsr	    main
        bra     done

; We're running in ROM, so exit should never get called.
exit::
    	move.l  4(sp),d0
done:
        move.b  running_in_rom,d0
        bra     done                       ; Don't know what else to do!

get_heap_start::
        move.l	a0,-(sp)
        lea		_bss_end,a0
        addq.l  #4,a0
        move.l  a0,d0
        and.l	#$fffffffc,d0
        move.l	(sp)+,a0
        rts


; default "do nothing" exception handler
_not_handled::
       rte

    	section	.data,data

running_in_rom::
    	dc.b	1

    	end



