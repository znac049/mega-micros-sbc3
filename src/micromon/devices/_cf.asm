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
cf_reg_sector_count equ cf_reg_base+4
cf_reg_lba0         equ cf_reg_base+6
cf_reg_lba1         equ cf_reg_base+8
cf_reg_lba2         equ cf_reg_base+10
cf_reg_lba3         equ cf_reg_base+12
cf_reg_command      equ cf_reg_base+14
cf_reg_status       equ cf_reg_base+14

CF_CMD_READ_SECTORS equ $20

CF_SECTOR_SIZE      equ 512

; Status register bit masks (for and/cmp)
cf_status_busy      equ $80
cf_status_drq       equ $08
cf_status_error     equ $01

; Status register bit NUMBERS (for btst - which takes a bit
; number, not a mask)
cf_status_busy_bit  equ 7
cf_status_error_bit equ 0


cf_data_mask        equ cf_status_busy|cf_status_drq

; How many times to poll before giving up
CF_TIMEOUT          equ 1000000

; waits for the device to not be busy
;
; returns (d0)
;   0 - BSY clear
;  -1 - timed out (BSY never cleared - card missing or hung)
;
_cf_wait_busy::
                move.l  #CF_TIMEOUT,d1
.loop
                btst.b  #cf_status_busy_bit,cf_reg_status
                beq     .ready
                subq.l  #1,d1
                bne     .loop

                moveq   #-1,d0
                rts

.ready          moveq   #0,d0
                rts


; wait for either:
;   BSY=0 and DRQ=1 (return 0)
;   ERR=1           (return -1)
;
_cf_wait_data::
.loop           move.b  cf_reg_status,d0
                btst    #cf_status_error_bit,d0
                bne     .err
                and.b   #cf_data_mask,d0
                cmp.b   #cf_status_drq,d0
                bne     .loop
                move.l  #0,d0
                rts

.err            move.l  #-1,d0
                rts

