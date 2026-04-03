                section .text,code

swap16::
                rts

swap32::
                rts

ill_inst_vec    equ     4<<2

; Result in d0:
;    0: 68000/68008
;    1: 68010
;    2: 68020
;    3: 68030
;
detect_cpu_type::
                movem.l d1/d2/a0,-(sp)
                move.l  #0,a0
                move.l  ill_inst_vec(a0),d1     ; Temporarily take over the illegal
                move.l  d1,saved_vector         ; instruction handler.
                move.l  illegal_inst,d1
                move.l  d1,ill_inst_vec(a0)
                move.l  sp,a2                   ; Remember the stack pointer

                moveq.l #0,d0                   ; Default is 68000/68008

; Now try executing instructions for each processor in ascending order until one fails
                move    ccr,d2                  ; Needs 68010
                addi.l  #1,d0

                movec   cacr,d2                 ; Needs 68020
                addi.l  #1,d0

                dc.l    $4E7A0807               ; MOVEC SRP,D0 - 68030+
                addi.l  #1,d0

cpu_detected
                move.l  #0,a0                   ; Restore the original illegal
                move.l  saved_vector,d1         ; instruction handler
                move.l  d1,ill_inst_vec(a0)     
                movem.l (sp)+,d1/d2/a0
                rts

illegal_inst:
                move.l  a2,sp
                bra     cpu_detected

                section .data,data

saved_vector    dc.l    0