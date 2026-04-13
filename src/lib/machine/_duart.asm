        section .text,code

duart_base          equ     $00ad0001

serial_vector       equ 80

duart_mr1a          equ 0*2             /* mode register a */
duart_mr2a          equ 0*2             /* mode register a */
duart_sra           equ 1*2             /* status register a */
duart_csra          equ 1*2             /* clock-select register a */
duart_cra           equ 2*2             /* command register a */
duart_rba           equ 3*2             /* receiver buffer a */
duart_tba           equ 3*2             /* transmitter buffer a */
duart_ipcr          equ 4*2             /* input port change register */
duart_acr           equ 4*2             /* auxiliary control register */
duart_isr           equ 5*2             /* interrupt status register */
duart_imr           equ 5*2             /* interrupt mask register */
duart_cur           equ 6*2             /* counter mode: current msb of counter */
duart_ctur          equ 6*2             /* counter/timer upper register */
duart_clr           equ 7*2             /* counter mode: current lsb of counter */
duart_ctlr          equ 7*2             /* counter/timer lower register */
duart_mr1b          equ 8*2             /* mode register b */
duart_mr2b          equ 8*2             /* mode register b */
duart_srb           equ 9*2             /* status register b */
duart_csrb          equ 9*2             /* clock-select register b */
duart_crb           equ 10*2            /* command register b */
duart_rbb           equ 11*2            /* receiver buffer b */
duart_tbb           equ 11*2            /* transmitter buffer b */
duart_ivr           equ 12*2            /* interrupt-vector register */
duart_opcr          equ 13*2            /* output port configuration register */
duart_start_counter equ 14*2            /* start-counter command */
duart_opr_set       equ 14*2            /* bit set command */
duart_stop_counter  equ 15*2            /* stop-counter command */
duart_opr_reset     equ 15*2            /* bit reset command */

duart_rxrdy         equ 0               /* receiver ready */
duart_txrdy         equ 2               /* transmitter ready */


_polled_putchar::
                move.l  #duart_base,a1
                btst.b  #3,duart_sra(a1)
                beq     _polled_putchar
                move.l  4(sp),d0
                move.b  d0,duart_tba(a1)
                rts

_polled_char_available::
                move.l  #duart_base,a1
                moveq.l #0,d0
                move.b  duart_sra(a1),d0
                and.b   #1,d0
                rts

_polled_getchar::
                move.l  #duart_base,a1
                btst.b  #0,duart_sra(a1)
                beq     _polled_getchar
                move.b  duart_rba(a1),d0
                rts
