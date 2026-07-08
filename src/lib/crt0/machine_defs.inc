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


; PI/T
;
PIT_BASE::      		equ	     $AF0001

; registers - as offsets from PIT_BASE
pit_pgcr_o::    		equ     0
pit_psrr_o::    		equ     2
pit_paddr_o::   		equ     4
pit_pbddr_o::   		equ     6
pit_pcddr_o::   		equ     8
pit_pivr_o::    		equ     10
pit_pacr_o::    		equ     12
pit_pbcr_o::    		equ     14
pit_padr_o::    		equ     16
pit_pbdr_o::    		equ     18
pit_paar_o::    		equ     20
pit_pbar_o::    		equ     22
pit_pcdr_o::    		equ     24
pit_psr_o::     		equ     26

pit_tcr_o::     		equ     32
pit_tivr_o::    		equ     34
pit_cprh_o::    		equ     38
pit_cprm_o::    		equ     40
pit_cprl_o::    		equ     42
pit_cntrh_o::   		equ     46
pit_cntrm_o::   		equ     48
pit_cntrl_o::   		equ     50
pit_tsr_o::     		equ     52

; registers - as addresses
pit_pgcr::      		equ     PIT_BASE
pit_psrr::      		equ     PIT_BASE+2
pit_paddr::     		equ     PIT_BASE+4
pit_pbddr::     		equ     PIT_BASE+6
pit_pcddr::     		equ     PIT_BASE+8
pit_pivr::      		equ     PIT_BASE+10
pit_pacr::      		equ     PIT_BASE+12
pit_pbcr::      		equ     PIT_BASE+14
pit_padr::      		equ     PIT_BASE+16
pit_pbdr::      		equ     PIT_BASE+18
pit_paar::      		equ     PIT_BASE+20
pit_pbar::      		equ     PIT_BASE+22
pit_pcdr::      		equ     PIT_BASE+24
pit_psr::       		equ     PIT_BASE+26

pit_tcr::       		equ     PIT_BASE+32
pit_tivr::      		equ     PIT_BASE+34
pit_cprh::      		equ     PIT_BASE+38
pit_cprm::      		equ     PIT_BASE+40
pit_cprl::      		equ     PIT_BASE+42
pit_cntrh::     		equ     PIT_BASE+46
pit_cntrm::     		equ     PIT_BASE+48
pit_cntrl::     		equ     PIT_BASE+50
pit_tsr::       		equ     PIT_BASE+52

pit_cnt_vec             equ     84


; xr68681 duart
;
DUART_BASE::    		equ     $AD0001

duart_mr1a_o::          equ		0
duart_mr2a_o::    		equ		0
duart_sra_o::     		equ		2
duart_csra_o::    		equ		2
duart_cra_o::     		equ		4
duart_rba_o::     		equ		6
duart_tba_o::     		equ		6
duart_ipcr_o::    		equ		8
duart_acr_o::     		equ		8
duart_isr_o::     		equ		10
duart_imr_o::     		equ		10
duart_cur_o::     		equ		12
duart_ctur_o::    		equ		12
duart_clr_o::     		equ		14
duart_ctlr_o::    		equ		14
duart_mr1b_o::    		equ		16
duart_mr2b_o::    		equ		16
duart_srb_o::     		equ		18
duart_csrb_o::    		equ		18
duart_crb_o::     		equ		20
duart_rbb_o::     		equ		22
duart_tbb_o::     		equ		22
duart_ivr_o::     		equ		24
duart_opcr_o::    		equ		26
duart_start_counter_o:: equ		28
duart_opr_set_o::       equ		28
duart_stop_counter_o::  equ		30
duart_opr_reset_o::     equ		30


duart_mr1a::            equ		DUART_BASE
duart_mr2a::    		equ		DUART_BASE
duart_sra::     		equ		DUART_BASE+2
duart_csra::    		equ		DUART_BASE+2
duart_cra::     		equ		DUART_BASE+4
duart_rba::     		equ		DUART_BASE+6
duart_tba::     		equ		DUART_BASE+6
duart_ipcr::    		equ		DUART_BASE+8
duart_acr::     		equ		DUART_BASE+8
duart_isr::     		equ		DUART_BASE+10
duart_imr::     		equ		DUART_BASE+10
duart_cur::     		equ		DUART_BASE+12
duart_ctur::    		equ		DUART_BASE+12
duart_clr::     		equ		DUART_BASE+14
duart_ctlr::    		equ		DUART_BASE+14
duart_mr1b::    		equ		DUART_BASE+16
duart_mr2b::    		equ		DUART_BASE+16
duart_srb::     		equ		DUART_BASE+18
duart_csrb::    		equ		DUART_BASE+18
duart_crb::     		equ		DUART_BASE+20
duart_rbb::     		equ		DUART_BASE+22
duart_tbb::     		equ		DUART_BASE+22
duart_ivr::     		equ		DUART_BASE+24
duart_opcr::    		equ		DUART_BASE+26
duart_start_counter::   equ		DUART_BASE+28
duart_opr_set::         equ		DUART_BASE+28
duart_stop_counter::    equ		DUART_BASE+30
duart_opr_reset::       equ		DUART_BASE+30
