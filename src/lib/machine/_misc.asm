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
                movem.l d1/d2/d3/a0/a2,-(sp)

                move.l  #0,a0
                move.l  ill_inst_vec(a0),d1     ; Temporarily take over the illegal
                move.l  d1,saved_vector         ; instruction handler.

                lea     illegal_inst,a2
                movea.l a2,ill_inst_vec(a0)

                move.l  sp,a2                   ; Remember the stack pointer

                move.b  #0,d3
                moveq.l #0,d0                   ; Default is 68000/68008 (0)

; Now try executing instructions for each processor in ascending order until one fails
                move    ccr,d2                  ; Needs 68010 (1)
                tst.b   d3                      ; Did an exception take place?
                bne     cpu_detected
                addi.l  #1,d0

                movec   cacr,d2                 ; Needs 68020 (2)
                tst.b   d3                      ; Did an exception take place?
                bne     cpu_detected
                addi.l  #1,d0

                pmove   tc,saved_tt0            ; Needs a 68030 (3)
                tst.b   d3                      ; Did an exception take place?
                bne     cpu_detected
                addi.l  #1,d0

cpu_detected
                move.l  saved_vector,d1         ; instruction handler
                move.l  d1,ill_inst_vec(a0)     

                movem.l (sp)+,d1/d2/d3/a0/a2
                rts

illegal_inst:
                move.b  #$ff,d3
                rte

                section .data,data

saved_vector    dc.l    0
saved_tt0       dc.l    0