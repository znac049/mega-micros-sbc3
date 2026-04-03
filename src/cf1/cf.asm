        section .text,code

ata_reg_base    equ     $ae0000
ata_reg_status  equ     ata_reg_base+$0e

ata_status_busy equ     $80
ata_status_drq  equ     $08

_ata_wait_busy::
        btst.b  #ata_status_busy,ata_reg_status
        bne     _ata_wait_busy
        rts

_ata_wait_data::
        btst.b  #ata_status_busy,ata_reg_status
        bne     _ata_wait_data
awd_waitdrq
        btst.b  #ata_status_drq,ata_reg_status
        beq     awd_waitdrq
        rts