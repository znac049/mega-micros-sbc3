                section .text,code

cf_reg_base     equ     $ae0000
cf_reg_status   equ     cf_reg_base+$0e

cf_status_busy  equ     $80
cf_status_drq   equ     $08

_cf_wait_busy::
                btst.b  #cf_status_busy,cf_reg_status
                bne     _cf_wait_busy
                rts

_cf_wait_data::
                btst.b  #cf_status_drq,cf_reg_status
                beq     _cf_wait_data
                rts