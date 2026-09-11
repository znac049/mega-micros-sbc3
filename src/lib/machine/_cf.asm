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

cf_reg_base         equ $ae0000
cf_reg_data_byte    equ cf_reg_base
cf_reg_sector_count equ cf_reg_base + 4
cf_reg_lba0         equ cf_reg_base + 6
cf_reg_lba1         equ cf_reg_base + 8
cf_reg_lba2         equ cf_reg_base + 10
cf_reg_lba3         equ cf_reg_base + 12
cf_reg_command      equ cf_reg_base + 14
cf_reg_status       equ cf_reg_base + 14

CF_CMD_READ_SECTORS equ $20

CF_SECTOR_SIZE      equ 512

cf_status_busy      equ $80
cf_status_drq       equ $08
cf_status_error     equ $01

cf_data_mask        equ cf_status_busy | cf_status_drq

; waits for the device to not be busy
;
_cf_wait_busy::
.loop
                btst.b  #cf_status_busy,cf_reg_status
                bne     .loop
                rts


; wait for either:
;   BSY=0 and DRQ=1 (return 0)
;   ERR=1           (return -1)
;
_cf_wait_data::
.loop           move.b  cf_reg_status,d0
                btst    #cf_status_error,d0
                bne     .err
                and.b   #cf_data_mask,d0
                cmp.b   #cf_status_drq,d0
                bne     .loop
                move.l  #0,d0
                rts

.err            move.l  #-1,d0
                rts


; Assembly version of cf_read(uint8_t drive_num, uint32_t sector, uint8_t *buffer)
;
; returns (d0)
;   0 - success
;  -1 - Something went wrong (set errno)
;
_cf_read::
                movem.l d2/a0,-(sp)
                
                move.l  16(sp),d1           ; block_num
                move.b  d1,cf_reg_lba0

                lsr.l   #8,d1
                move.b  d1,cf_reg_lba1

                lsr.l   #8,d1
                move.b  d1,cf_reg_lba2

                lsr.l   #8,d1
; lba3 is special - the top 4 bits contain the drive number and
; a flag to set LBA mode
                and.b   #$0f,d1
                or.b    #$e0,d1
;                tst.b   15(sp)               ; drive 0 or 1?
;                beq     _cfr_d0
;                or.b    #$10,d1
_cfr_d0         move.b  d1,cf_reg_lba3

                move.b  #1,cf_reg_sector_count
                move.b  #CF_CMD_READ_SECTORS,cf_reg_command
;                bsr     _cf_wait_busy
;                bne     _cfr_timeout

                bsr     _cf_wait_data
                tst.l   d0
                bne     _cfr_error

                move.w  #CF_SECTOR_SIZE-1,d2  ; loop counter

_cfr_get_byte   move.b  cf_reg_data_byte,(a0)+   ; read a byte
                dbra    d2,_cfr_get_byte
                bra     _cfr_ok

_cfr_error      move.l  #2,d0
                bra     _cfr_done

_cfr_timeout    move.l  #1,d0
                bra     _cfr_done

_cfr_ok         move.l  #0,d0

_cfr_done
                movem.l (sp)+,d2/a0
                rts