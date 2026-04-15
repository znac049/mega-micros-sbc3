;********************************************************
;*                                                      *
;*  Mon68K - Monitor program for SBC68k ver 3.0         *
;*                                                      *
;*      Custom version for Inmos FEPs 12/6/87           *
;*                                      Bob Green       *
;********************************************************
;* Change history:                                      *
;*   18-Apr-90	bg	Created from Pete's version	*
;*   19-Apr-90	bg	Added I)nitialise store command	*
;*   30-Apr-90	bg	Added P)rom command		*
;*   01-May-90	bg	Added flick code to INITSTORE	*
;*   22-May-90	bg	Added C)all 'C' command		*
;*   24-May-90	bg	Added autoinitialise of store	*
;*			and default USS,USP changed	*
;*   03-Jun-90	bg	Moved breakpoint functions into	*
;*			main menu.			*
;*   18-Jun-90	bg	Fixed store init		*
;*   26-Jun-90	bg	Improved init store code	*
;********************************************************
;

;*
;* Memory definitions
;*
RamStart        equ     $000000
RamEnd          equ     $3F0000
RomStart        equ     $C00000

;*
;* I/O  locations
;*
TtyUart::	equ	$Ad0001
HostUart::	equ	TtyUart+$10

;*
;* Constants
;*
CR              equ     $0D
LF              equ     $0A
BELL            equ     $07
NULL            equ     $00
BACKSP          equ     $08
ESC             equ     $1B
SPACE           equ     $20
DOT             equ     $2E
MINUS           equ     $2D
PLUS            equ     $2B
COMMA           equ     $2C
QUOTE           equ     $27

;*
;* System parameters
;*
;USS             equ     $FFFFFE		; user SYSTEM STACK
;USP             equ     $FFFFFE         ; user STACK POINTER
;STACK           equ     $4FF0		; monitor STACK FOR WORD ALIGNMENT

MAXLEN          equ     128             ; MAX LENGTH OF STRING (+3)
NUMBRK          equ     8               ; NUMBER OF BREAKPOINTS ALLOWED
BRKSIZ          equ     NUMBRK*12       ; space TO RESERVE

INTSOFF		equ	$2700
INTSON		equ	$2000

BUSERRVEC	equ	$8		; The address of the Bus Error vector

;
; Vector definitions
;
VEC_SPACE	equ	$400		; Amount of space used by vectors
NMI_INT         equ     64              ; refresh interrupt vector

;*
;* System storage locations (Global variables)
;*
REGS            equ     RamStart+VEC_SPACE  ; register DUMP AREA
OldSSP          equ     REGS+88
InStr           equ     OldSSP+4        ; WORKSPACE
OutStr          equ     InStr+MAXLEN    ; WORKSPACE
TempStack       equ     OutStr+MAXLEN+128 ; temporary STACK OF 128 BYTES
NUMSTR          equ     TempStack+2     ; WORKSPACE
TempBuf1        equ     NUMSTR+MAXLEN   ; workspace     
ODV             equ     TempBuf1+$10    ; old DUMP VALUE WORKSPACE
BrkTab          equ     ODV+4           
ErrNum          equ     BrkTab+BRKSIZ   ; trap VECTOR NUMBER LOC
TRCNT           equ     ErrNum+2        ; trace COUNT STORAGE
Status          equ     TRCNT+4         ; flag FOR MONITOR/USER STATUS
TRCFLG          equ     Status+2        ; flag FOR 
BRKCNT          equ     TRCFLG+2        ; breakpoint COUNT LOC
OldPC           equ     BRKCNT+2        ; location FOR A PC DURING A JSR 
CurrentIn	equ	OldPC+4		; Current input device
CurrentOut	equ	CurrentIn+4	; Current output device
RomBase		equ	CurrentOut+4	; Base of data to send to XP-640
TopOfStore	equ	RomBase+4	; Top of memory (end)
BotOfStore	equ	TopOfStore+4	; Bottom of memory (start)
FlickPtr	equ	BotOfStore+4	; Address of next flick character

;*
;* xr68c681 Duart
;*
mr_off          equ     0
sr_off          equ     2
csr_off         equ     2
cr_off          equ     4
rhr_off         equ     6
thr_off         equ     6
acr_off         equ     8
imr_off         equ     10
opcr_off        equ     26
sopbc_off       equ     28
copbc_off       equ     30


Stack           equ     FlickPtr+4096

        if TargetIsROM
                org     $c00000
        else
		org	Stack+4
        endif

;*
                dc.l    Stack           ; INITIAL SSP
                dc.l    _start          ; JUMP TO THIS LOCATION
;*
_start::
                ori.w   #INTSOFF,sr

;* Initialise the tty uart
		lea     TtyUart,a0	; Address of uart
                move.b  #0,imr_off(a0)  ; No interrupts from anything

                move.b  #$a0,cr_off(a0) ; CRA: x=1 on Tx
                move.b  #$80,cr_off(a0) ; CRA: x=1 on Rx

                move.b  #$70,acr_off(a0); ACR: BRG set #1, CLK/16 
                move.b  #$88,csr_off(a0); CSRA: 230400 (115200 x 2)

                move.b  #$93,mr_off(a0) ; MR1A: Rx RTS,8N
                move.b  #$17,mr_off(a0) ; MR2A: No echo, CTS, 1 stop bit
                move.b  #$05,cr_off(a0) ; CRA: enable tx/rx
                move.b  #$ff,sopbc_off(a0)  ;
;
;* Initialise the host uart
		lea	HostUart,a0	; Address of uart

                move.b  #$a0,cr_off(a0) ; CRB: x=1 on Tx
                move.b  #$80,cr_off(a0) ; CRB: x=1 on Rx

                move.b  #$88,csr_off(a0); CSRB: 230400 (115200 x 2)

                move.b  #$93,mr_off(a0) ; MR1B: Rx RTS,8N
                move.b  #$17,mr_off(a0) ; MR2B: No echo, CTS, 1 stop bit
                move.b  #$05,cr_off(a0) ; CRB: enable tx/rx
;;
		move.l	#TtyUart,CurrentIn  ; Setup tty port
		move.l	#TtyUart,CurrentOut ; as default io device
;
		move.l	#Flicks,FlickPtr
;
		move.l	#REGS,a0	; Register dump area
		moveq	#19,d0		; Number of regs -1 
IO		move.l	#0,(a0)+	; init location
		dbf	d0,IO

		move.l	#0,ODV		; Initialise dump start
		move.l	#$100000,REGS	; Set pc to default start addr
;               move.l  #USS,-4(a0)     ; set A7 TO STACK
;               move.l  #USS,-72(a0)    ; set SSP
;               move.l  #USP,-76(a0)    ; set USP
                move.l  #$2000,-68(a0)  ; set STATUS REGISTER
                move.l  #1,TRCNT        ; default 1 TRACE STEP
                move    #0,BRKCNT       ; CLEAR NUMBER OF BREAKPOINTS

;* SETUP SYSTEM TRAPS <2-11>
                move.l  #8,a0           ; start OF EXCEPTION TABLE
                move.l  #EXTAB,A1       ; base OF EXCEPTION TABLE
                moveq   #9,d0           ; 10 VECTORS (DBCC -1)
I1              move.l  (A1)+,(a0)+     ; insert VECTOR
                dbf     d0,I1

;* SETUP MOTOROLA RESERVED TRAPS <1LO2-23>
                moveq   #11,d0          ; no RESERVED TRAPS HERE
I2              move.l  #RES,(a0)+      ; put VECTOR IN LOC
                dbf     d0,I2

;* SETUP VECTOR 24
                move.l  #SPU,(a0)+      ; spurious TRAP

;* SETUP AUTOVECTORED INTERRUPT TRAPS <25-31>
                moveq   #6,d0           ; 7 AUTO VECTORED TRAPS
I3              move.l  #AUT,(a0)+
                dbf     d0,I3

;* SETUP USER TRAPS <32-47>
                moveq   #15,d0          ; 16 USER TRAPS
I4              move.l  #TRP,(a0)+
                dbf     d0,I4

;* SETUP RESERVED TRAPS <48-63>
                moveq   #15,d0          ; no RESERVED TRAPS @ $C0
I5              move.l  #RES,(a0)+
                dbf     d0,I5

;* SETUP VECTORED INTERRUPTS <64-255>
                move.l  #191,d0         ; no OF VECTORED INTERRUPTS
I6              move.l  #INT,(a0)+
                dbf     d0,I6

;* SETUP USER TRAPS THAT ARE USED BY MONITOR
                move.l  #EXTRACODE,$B4        ; setup EXTRACODE HANDLER IN TRAP #13
                move.l  #RESTART,$B8    ; setup SUPERVISOR RETURN TRAP IN #14
                move.l  #BRKPT,$BC      ; setup BREAKPOINT HANDLER IN TRAP #15

;* CLEAR BREAKPOINT TABLE
                move    #BRKSIZ-1,d0
                move.l  #BrkTab,a0
I7              move.b  #0,(a0)+        ; clear BREAKPOINT
                dbf     d0,I7

;* NOW ANNOUNCE SYSTEM MESSAGE TO INDICATE ALL IS WELL

                move.l  #MONSTR,a0
                bsr     P_STR

		bsr	CheckMem	; Do we need to init store
		beq	SetStacks
        
		;bsr	INITSTORE	; Initialise store
                move.l  #MonEnd+4,BotOfStore
                move.l  #$3fffff,TopOfStore

SetStacks	move.l	TopOfStore,d0	; Get top of store
		move.l	d0,REGS+4	; Set USP
		sub.l	#$4000,d0	; USP is $4000 bytes long
		move.l	d0,REGS+8	; Set SSP...
		move.l	d0,REGS+76	; ...and A7
		move.l	BotOfStore,d0	; ...and base of store
		move.l	d0,REGS+4
		
                ;move.w	#INTSON,SR	; Enable interrupts

START           move    #1,Status       ; MARK FLAG AS IN MONITOR 
                move.l  #PROMPT,a0
                bsr     P_STR
                bsr     GetCh           ; GET CHARACTER
                bsr     TO_UPPER
                cmpi.b  #CR,d0
                beq     START           ; IGNORE
                bsr     SCAN_COM        ; look FOR COMMAND
                cmpi.b  #$FF,D7         ; command NOT FOUND ?
                beq     GLER
                cmpi.b  #1,D7           ; illegal VALUE RETURN
                ble     START
                move.l  #ERR,a0
                bsr     P_STR
                bra     START
GLER            move.l  #ERRMESS,a0
                bsr     P_STR		; COMPLAIN
                bra     START

;*
;*
;* ext          Extracode function handler performs an effective JSR to a 
;*              monitor subroutine for you.
;*
;*      Code to invoke monitor subroutine is :-
;*                      .......
;*                      trap    #13     ; monitor extracode trap (EXT)
;*                      word    num     ; perform subroutine num where num
;*                                      ; is in the range 0 - 20
;*      number     function
;*
;*      0       P_STR                   ; print null terminated string 
;*      1       G_STR                   ; get an edited string 
;*      2       PUTCH                   ; output a character to port 0
;*      3       GetCh                   ; get a character from port 0 (no top bit)
;*      4       EVAL                    ; evaluate a string of hex characters
;*      5       GET_NUMS                ; get numbers from input line
;*      6       TOUP_STR                ; uppercase a string
;*      7       ASC_TO_HEX              ; convert ascii to hex number
;*      8       DEC_TO_HEX              ; convert decimal input to hex
;*      9       PRINT_HEX               ; print hex number in variable field width
;*      10      HEX_TO_ASC              ; convert hex number to ascii string
;*      11      PRINT_DEC               ; print signed 32 bit number in decimal
;*      12      H_T_D           ; convert 32 signed number to string
;*      13      IS_HEX                  ; checks if valid ascii hex character
;*      14      IS_DIGIT                ; checks if valid ascii digit
;*      15      IS_LOWER                ; checks for lowercase letter
;*      16      IS_UPPER                ; checks for uppercase letter
;*      17      IS_ALPHA                ; checks for upper/lower case letter
;*      18      IS_ALNUM                ; checks for alphanumeric character
;*      19      TO_UPPER                ; converts character to uppercase
;*      20      CRLF                    ; prints carriage return line feed pair
;*
EXTRACODE       subq.l  #8,A7           ; MAKE TEMP SPACE ON STACK
                movem.l d0/a0,-(SP)     ; save REG
                move.l  18(SP),a0       ; get PC
                addq.L  #2,18(A7)       ; move PC ON TO NEXT InStrUCTION
                move    (a0),d0         ; GET FUNCTION TYPE
                move.l  #EXF,a0         ; base OF FUNCTION TABLE
                cmpi    #20,d0
                bgt     EXR             ; NOT A VALID FUNCTION CALL 
                lsl     #2,d0           ; FUNC NUMBER * 4
                move.l  0(a0,d0),8(A7)  ; save FUNCTION ADDRESS ON STACK
                move.l  #EX,12(SP)      ; return ADDRESS FOR THIS ROUTINE
                movem.l (SP)+,d0/a0     ; restore REG
                rts                     ; GO TO FUNCTION
EX              rte                     ; THEN RETURN TO USER
;*
EXR             move.l  #ERRIF,a0
                bsr     P_STR           ; say ILLEGAL FUNCTION CALL MADE
                moveq   #10,D6
                move.l  18(SP),d0       ; get PC + 4
                subq.l  #4,d0           ; CORRECT
                bsr     PRINT_HEX       ; say WHERE ILLEGAL CALL WAS MADE FROM
                bsr     CRLF
                movem.l (SP)+,d0/a0
                addq.L  #8,A7           ; deallocate STACK STORAGE
                rte                     ; AND RETURN COMPLAINING
;
;
;
;*              
;*
;*
;* read         Routine to read a Motorola S format record into memory
;*              ends on either an esc CR sequence or an S9 record.
;*              All errors reported after end of download.
;*
;*              Record type & action :-
;*                      s0      ignored totally
;*                      S1,2,3  data is loaded into memory at address in rec
;*                      S4-8    not supported, gives an unsuported rec type err
;*                      s9      ignored but used to finish download
;*                      other   gives an illegal record data error
;*
;
READ            movem.l d0-D6/a0-A6,-(SP)
R_INIT          lea     RSTR,a0
                bsr     P_STR           ; print READ MOTOROLA ....
                clr.l   d0              ; stop dump error
                clr.l   D6
R_S             clr.l   D3              ; CLEAR CHECKSUM
R_S0            bsr     GetCh
                cmpi.b  #ESC,d0
                beq     R_SEND          ; ESCAPE FROM LOADER
                cmpi.b  #'S',d0
                beq     R_S1
                bra     R_S             ; ANY OTHER CHARACTER OK
;
R_ILL           moveq   #1,D6
R_DITCH         bsr     R_SWAITCR       ; ditch RESTOF LINE
                bra     R_S             ; ILLEGAL DATA
;
R_S1            bsr     GetCh
                bsr     IS_DIGIT
                tst.b   D7
                bne     R_ILL           ; illegal RECORD TYPE
                tst.b   d0              ; S0 RECORD
                bne     R_S11           ; no SO CARRY ON
                bra     R_DITCH         ; DITCH REST OF LINE
R_S11           cmpi.b  #9,d0           ; s9 ?
                bne     R_S12
                bsr     R_SWAITCR       ; yes SO READ REST OF LINE
                bra     R_SEND          ; AND END LOAD
R_S12           cmpi.b  #3,d0
                ble     R_S2
                moveq   #2,D6           ; illegal RECORD TYPE
                bra     R_DITCH         ; DITCH REST OF RECORD
R_S2            move    d0,D5           ; COPY RECORD TYPE
                moveq   #1,d0           ; one PAIR OF CHARACTERS
                bsr     R_GETPAIR       ; get BYTE COUNT
                tst.b   D7
                bne     R_ILL           ; illegal DATA
                subq    #2,d0           ; CORRECT FOR CHECKSUM , RECORD LENGTH
                sub     D5,d0           ; AND ADDRESS LENGTH
                move    d0,D4           ; COPY DATA COUNT
;
; kludge for null record problem ! ditches rest of line no checknsum test
;
                beq     R_DITCH         ; ditch rest of line
                subq    #1,D4           ; CORRECT FOR DBCC
                move    D5,d0           ; GET RECORD TYPE
                addq    #1,d0           ; number OF PAIRS TO ADDRESS
                bsr     R_GETPAIR       ; get ADDRESS
                tst.b   D7
                bne     R_ILL           ; illegal DATA
                move.l  d0,A2           ; copy LOAD ADDRESS
R_S3            moveq   #1,d0           ; one PAIR OF CHARACTERS
                bsr     R_GETPAIR
                tst.b   D7
                bne     R_ILL
                move.b  d0,(A2)+        ; store DATA BYTE
                dbf     D4,R_S3
R_S4            moveq   #1,d0           ; checksum PAIR
                move    D3,D5           ; COPY CHECKSUM
                clr     D3
                bsr     R_GETPAIR
                tst.b   D7
                bne     R_ILL           ; illegal DATA
                eori.b  #$FF,D5         ; INVERT CHECKSUM (1'S COMPLEMEMNT)
                cmp.b   D3,D5           ; CHECKSUMS SAME ?
                beq     R_S             ; YES
                moveq   #3,D6           ; checksum ERROR
                bra     R_S
;
R_SEND          tst     D6              ; ANY ERRORS ?
                beq     R_FIN           ; NO
                subq    #1,D6
                lsl     #2,D6           ; MAKE STRING OFFSET
                move.l  #RS1,A1         ; base OF ERROR STRING TABLE
                move.l  0(A1,D6),a0     ; copy POINTER
                bsr     P_STR           ; say ... ERROR
R_FIN           moveq   #0,D7
                movem.l (SP)+,d0-D6/a0-A6       ; restore REG
                RTS
;
; R_GETPAIR     Subroutine to take <n> pairs of hex digits from input 
; On entry :-
;               d0      number of pairs to read
;               d3      current checksum
; On exit  :-
;               d0      value
;               d3      incremented checksum by pairs
;               d7.b    status :-
;                               0)      no error
;                               1)      illegal non hex character found in string
;
R_GETPAIR       movem.l D1/D2/D4-D6/a0-A6,-(SP)
                move    d0,D2           ; COPY NUMBER OF PAIRS
                subq    #1,D2           ; CORRECT FOR DBCC
                clr.l   d0              ; CLEAR TOTAL

R_G1            lsl.l   #8,d0           ; GET READY
                bsr     EVAL2           ; GET PAIR
                tst.b   D7
                bne     R_GEND          ; illegal DATA
                add.l   D1,d0           ; ADD TOTAL
                dbf     D2,R_G1         ; keep GETTING
                
R_GEND          movem.l (SP)+,D1/D2/D4-D6/a0-A6
                RTS
;
;
;
; eval2         Get a pair of hex characters from the input;
; On entry :-
;               d3      Current Checksum
; On exit  :-
;               d1      Value of pair
;               d7.b    status :-
;                               0)      no error
;                               1)      illegal non hex character found in string
;
EVAL2           movem.l d0/D2/D4-D6/A1-A6,-(SP) ; save REG
                clr.l   D1
                moveq   #1,D2           ; 2 CHARACTERS
EV2_0           lsl.l   #4,D1           ; SHIFT READY FOR ADD
                bsr     GetCh
                bsr     IS_HEX
                tst.b   D7
                bne     EV2_ERR         ; non HEX CHAR FOUND
                add.b   d0,D1           ; ADD TO TOTAL
                dbf     D2,EV2_0
                clr     D7              ; CLEAR ERROR STATUS
                add     D1,D3           ; INCREMENT CHECKSUM
EV2_ERR         movem.l (SP)+,d0/D2/D4-D6/A1-A6 ; restore REG
                RTS
;
;
; R_SWAITCR     Waits till a carraige return or lf or escape is entered
;
R_SWAITCR       move.l  d0,-(SP)

R_SW0           bsr     GetCh
                cmpi.b  #CR,d0
                beq     R_SWEND
                cmpi.b  #LF,d0
                beq     R_SWEND
                cmpi.b  #ESC,d0
                bne     R_SW0
R_SWEND         move.l  (SP)+,d0
                RTS
;*
;*
;* eval         Subroutine which converts a series of hex characters into a 
;*              number.
;* On entry :-
;*              d0.b    Number of characters to convert
;*              a0.l    Pointer into string to convert from
;* On exit  :-
;*              d0.l    Number converted (if applic)
;*              a0.l    Pointer to next character after converted number
;*              d7.b    status :-
;*                              0)      no error
;*                              1)      illegal non hex character found in string
;*
EVAL            movem.l D1-D6/A1-A6,-(SP)       ; save REG
                clr     D2              ; CLEAR FOR DBCC
                move.b  d0,D2           ; copy COUNT
                subq.b  #1,D2           ; CORRECT FOR DBCC
                clr.l   D1              ; CLEAR TOTAL

EV_0            lsl.l   #4,D1           ; SHIFT READY FOR ADD
                move.b  (a0)+,d0        ; get CHARACTER
                bsr     IS_HEX
                tst.b   D7
                bne     EV_ERR          ; non HEX CHAR FOUND
                add.b   d0,D1           ; ADD TO TOTAL
                dbf     D2,EV_0
                move.l  D1,d0           ; move TOTAL
                clr     D7

EV_ERR          movem.l (SP)+,D1-D6/A1-A6       ; restore REG
                RTS
;*
;*
;* help         Prints help message on terminal
;*
HELP            move.l  #HSTR,a0
                bsr     P_STR
                RTS
;*
;*
;* doboot	Boot up from disk
;*
DOBOOT		move.l	#BOOTSTR,a0
		bsr	P_STR
		RTS
;*
;*
;* dorom	Send ROM images to XP-640 connected to HOST port
;*
DOROM		move.l	#PSTR,a0
		bsr	P_STR
		move.w	#1,d0		; One number to get
		bsr	GET_NUMS
		move.l	#0,RomBase	; Default address
		cmp.b	#0,d0		; Did we supply an address?
		beq	DRnoadd
		move.l	(A3),RomBase	; Use address supplied
DRnoadd
		
DoLB0		lea	RomDo,a0	; Do LB0
		bsr	P_STR
		lea	LB0,a0
		bsr	P_STR
		bsr	GetCh		; Wait for key press
		lea	RomDoing,a0
		bsr	P_STR
		move.l	RomBase,A1	; Address of data
		move.l	#4096,D1	; No of bytes
		bsr	DoBlock

DoUB0		lea	RomDo,a0	; Do LB0
		bsr	P_STR
		lea	UB0,a0
		bsr	P_STR
		bsr	GetCh		; Wait for key press
		lea	RomDoing,a0
		bsr	P_STR
		move.l	RomBase,d0	; Address of data
		add.l	#1,d0		; Odd addresses
		move.l	d0,A1
		move.l	#4096,D1	; No of bytes
		bsr	DoBlock

DDoLB1		lea	RomDo,a0	; Do LB0
		bsr	P_STR
		lea	LB1,a0
		bsr	P_STR
		bsr	GetCh		; Wait for key press
		lea	RomDoing,a0
		bsr	P_STR
		move.l	RomBase,d0	; Address of data
		add.l	#$2000,d0	; Byte 1
		move.l	d0,A1
		move.l	#4096,D1	; No of bytes
		bsr	DoBlock

DoUB1		lea	RomDo,a0	; Do LB0
		bsr	P_STR
		lea	UB1,a0
		bsr	P_STR
		bsr	GetCh		; Wait for key press
		lea	RomDoing,a0
		bsr	P_STR
		move.l	RomBase,d0	; Address of data
		add.l	#$2001,d0	; Byte 1 odd addresses
		move.l	d0,A1
		move.l	#4096,D1	; No of bytes
		bsr	DoBlock

		RTS

;
; DoBlock -- Send a block of data to the XP-640
;
DoBlock		move.l	D1,D2		; Use dbf, so -1
		sub.l	#1,D2		; D2 is counter
		move.l	A1,A2
		moveq.L	#0,D3		; Xsum = 0
DBxsum		move.b	(A2)+,d0	; get memory
		and.w	#$00FF,d0
		add.w	d0,D3		; Xsum += (int) mem[A2]
		adda.l	#1,A2		; Skip every other byte
		dbf	D2,DBxsum	; Loop until done

; Okay, we've got both a length and a checksum so let's boogie...

		move.l	CurrentOut,D4	; Remember output device
		move.l	#HostUart,CurrentOut

		move.b	D1,d0		; lsb of length
		bsr	PUTCH
		move.w	D1,d0		; MSB of length
		lsr.l	#8,d0
		bsr	PUTCH

		move.b	D3,d0		; lsb of Xsum
		bsr	PUTCH
		move.w	D3,d0		; MSB of Xsum
		lsr.l	#8,d0
		bsr	PUTCH

; Now the data...

		sub.l	#1,D1
DBsend		move.b	(A1)+,d0	; Get a byte...
		bsr	PUTCH		; ...send a byte...
		adda.l	#1,A1		; ...skip a byte
		dbf	D1,DBsend	; Loop until done

		move.l	D4,CurrentOut	; Restore output device

		RTS

;*
;* flick	Rotate the flicker
;*
FLICK		movem.l	d0/a0,-(SP)	; Save regs
		move.l	FlickPtr,a0	; Get address of character
		cmp.b	#NULL,(a0)	; EOS?
		bne	FLok
		lea.l	Flicks,a0	; Wrap around
FLok		move.b	(a0)+,d0	; Get character
		move.l	a0,FlickPtr	; Save ptr
		bsr	PUTCH		; Print it
		move.b	#BACKSP,d0	; Backspace over it
		bsr	PUTCH
		movem.l	(SP)+,d0/a0	; Restore regs
		RTS

;
;
; CheckMem	See if store has been initialised
;
;   Returns:
;   z set	Store has been initialised
;   z clear	Store needs to be initialised
;
CheckMem	move.l	TopOfStore,d0	; Check to see if its been done
		eori.w	#$FFFC,d0
		cmp.w	#0,d0
		beq	CMok
		andi	#$FFFB,SR	; Clear Z
		bra	CMret
CMok		ori	#4,SR		; Set Z
CMret		RTS

;
; findbase	Find base and size of memory
;
FINDBASE	move.l	SP,A1		; Remember our stack pointer
		move.l	#FBBUSERR,d0	; Plug in the BusErr vector...
		move.l	#BUSERRVEC,a0
		move.l	d0,(a0)		; Plug in our vector

        if TargetIsROM
                move.l  #$1000,a0
        else
                move.l  #MonEnd,d0      ; Figure out a safe place to start looking
                andi.l  #$fffff000,d0
                add.l   #$1000,d0
                move.l  d0,a0
        endif

		move.l	#-1,d0		; Data to try writing
		move.l	#0,BotOfStore

FBLOOP		move.l	d0,(a0)		; Try writing

; If we get here, the write succeeded. Now try to find
; the amount of memory...

		move.l	a0,BotOfStore
FBRET		move.l	A1,SP		; Restore stack pointer
		RTS

FBBUSERR	move.l	A1,SP		; restore old stack pointer
		RESET
		move.l	a0,D2
		cmp.l	#RomStart,D2	        ; End of RAM?
		beq	FBRET
		add.l	#$001000,D2
		move.l	D2,a0
		bra	FBLOOP		; Try next bank

;*
;*
;* initstore	Initialise store.
;*
INITSTORE
                bsr	FINDBASE
		move.l	#ISTR,a0
		bsr	P_STR

		move.l	SP,A1		; Save stack ptr...
		move.l	#ISBUSERR,d0	; Our replacement vector
		move.l	#BUSERRVEC,a0
		move.l	d0,(a0)		; Plug it in
		move.l	BotOfStore,a0	; Start of store to initialise

                move.l  #$F800,d0       ; Initialise to all 1s
                moveq.L #-1,D1          ; Initialise to all 1s
		moveq.L	#-1,D2		; Initialise to all 1s
		moveq.L	#-1,D3		; Initialise to all 1s
		moveq.L	#-1,D4		; Initialise to all 1s
		moveq.L	#-1,D5		; Initialise to all 1s
		moveq.L	#-1,D6		; Initialise to all 1s
		moveq.L	#-1,D7		; Initialise to all 1s

ISloop		movem.l	d0-D7,(a0)
		movem.l	d0-D7,32(a0)
		movem.l	d0-D7,64(a0)
		movem.l	d0-D7,96(a0)
		movem.l	d0-D7,128(a0)
		movem.l	d0-D7,160(a0)
		movem.l	d0-D7,192(a0)
		movem.l	d0-D7,224(a0)

		cmp.l	#$01000000,a0	; End of memory?
		bge	ISBUSERR
		lea.l	256(a0),a0	; Next block please

		addq.W	#1,d0		; Simple timer so flicks look okay
		bne	ISloop
		bsr	FLICK		; Rotate that wotsit
                move.l  #$F800,d0
		bra	ISloop

ISBUSERR	move.w	#INTSOFF,SR	; Become non-interruptable
		move.l	a0,d0		; Set top of store
		sub.l	#4,d0
		move.l	d0,TopOfStore	; Save it
		reset			; Turn those lights off
		move.l	#EXTAB,a0	; Restore real BusErr vector
		move.l	(a0),d0
		move.l	#BUSERRVEC,a0	; Get address of BusErr vector
		move.l	d0,(a0)		; Plug it in.
		move.l	A1,SP		; restore original stack ptr
		move.l	#20,D4		; No of bits to shift
		move.l	BotOfStore,D1	; Work out memory size
		move.l	TopOfStore,d0	; 
		add.l	#4,d0		; 'cos of prvious SUB.L #4
		sub.l	D1,d0		; Top - Bot
		moveq	#0,D1		; 0 -> No half meg
		ror.l	D4,d0		; Convert to MBytes
		bcc	NOHALF		; Is there a half meg?
		moveq	#1,D1		; 1 -> Half meg present
NOHALF		move.l	d0,D2		; Remember size in D2

		move.b	#CR,d0
		bsr	PUTCH
		move.l	D2,d0		; Get ready to print it
		and.l	#$0F,d0
		bsr	PRINT_DEC	; Print it
		cmp.b	#0,D1		; Half meg?
		beq	NOHSTR
		move.l	#HALFSTR,a0	; Print 'and a half' message
		bsr	P_STR		;
NOHSTR		move.l	#MBSTR,a0	; rest of message
		bsr	P_STR		;
		move.l	BotOfStore,d0	; Get store base
		move.b	#6,D6		; No of places
		bsr	PRINT_HEX	; Print it
		move.b	#CR,d0
		bsr	PUTCH
		move.b	#LF,d0
		bsr	PUTCH

		RTS

;*
;*
;*
;* go           Routine to jump to a user routine, loads registers from dump, 
;*              Prompts go <,> <address> if no args then location to jump to
;*              is taken as current dumped pc in table, else <address> 
;*              specifies to which location to jump to. If a comma is present
;*              then a temporary breakpoint is set at the following address and
;*              routine run from dumped pc value
;*              Address must be HEX
;*
GO		bsr	CheckMem	; Make sure store is valid
		beq	GOsok
		move.l	#NSSTR,a0	; Complain
		bsr	P_STR
		jmp	G_RET

GOsok		move.l  #BrkTab,A2      ; base OF BREAKPOINT TABLE
                clr     D4
                moveq   #NUMBRK-1,D1    ; number OF BREAKPOINTS -1 (DBCC)
G_C             cmpi.b  #2,5(A2,D4)     ; temp BREAKPOINT ?
                bne     G_C0            ; NO
                move.l  0(A2,D4),A1     ; get BREAKPOINT ADDRESS
                move    6(A2,D4),(A1)   ; RESTORE InStrUCTION
                clr.l   0(A2,D4)        ; NOW ZERO BREAKPOINT
                clr.l   4(A2,D4)        ; TYPE & InStrUCTION
                clr.l   8(A2,D4)        ; DATA FIELD
G_C0            add.l   #12,D4          ; MOVE TO NEXT BREAKPOINT ENTRY
                dbf     D1,G_C
                clr     D4              ; MARK FLAG TO DEFAULT NO BREAKPOINT
                move.l  #GSTR,a0        ; having CLEARED ALL TEMP BREAKPOINTS
                bsr     P_STR
                move.l  #InStr,a0
                bsr     G_STR   ; get REPLY
                cmpi.b  #COMMA,(a0)     ; first CHAR A ',' ?
                bne     G_0
                moveq   #1,D4           ; mark AS BREAKPOINT ADDRESS
                addq.L  #1,a0           ; skip THE ,
G_0             bsr     ASC_TO_HEX      ; get HEX ADDRESS
                cmpi.b  #1,D7           ; no NUMBER INPUT ?
                beq     G_5
                tst.b   D7              ; ERROR ?
                bne     G_RET           ; YES
                tst     D4
                beq     G_4             ; NOT A BREAKPOINT
                btst    #0,d0
                beq     G_1
                move.l  #ERROA,a0
                bsr     P_STR   ; say ODD BREAKPOINT ADDRESS
                clr     D7
                bra     G_RET
G_1             cmpi.l  #8,d0
                blt     G_2             ; TOO LOW
                cmpi.l  #RamEnd,d0
                ble     G_3             ; NOT TOO HIGH
G_2             move.l  #ERRCR,a0
                bsr     P_STR   ; say CHANGE ROM ?
                bra     G_RET
G_3             moveq   #2,D1           ; mark AS TEMPORARY BREAKPOINT
                bsr     BRAD            ; INSERT TEMPORARY BREAKPOINT
                tst.b   D7
                bne     G_RET           ; error STATUS SO DONT GO
                bra     G_5
G_4             move.l  d0,REGS         ; set NEW START ADDRESS TO GO FROM
;*
G_5             move.l  #BrkTab,A2
                move.l  REGS,d0         ; get START ADDRESS
                clr     D4
                moveq   #NUMBRK-1,D1
G_6             cmp.l   0(A2,D4),d0     ; current PC IS BREAKPOINT ?
                beq     G_FB            ; HANDLE GOING FROM A BREAKPOINT
                add     #12,D4          ; POINT TO NEXT ENTRY
                dbf     D1,G_6
                clr     d0              ; MARK AS ACTIVATE BREAKPOINTS
                bsr     BRACT           ; ACTIVATE ALL BREAKPOINTS
                bra     JSTART          ; GO TO USER PROGRAM
;*
G_RET           rts                     ; NORMAL RETURN 
;*
G_FB            ori     #$8000,REGS+14  ; SET TRACE FLAG IN STORED SR
                move.l  #GOTRC,$24      ; set TRACE VECTOR TO GO TRACE ROUTINE
                bra     JSTART          ; GO TO USER PROGRAM
;*
;* gotrc                Routine executed when trace happens after supposedly breakpointed
;*              InStruction, reset trace flag install all breakpoints exit back
;*              to user routine
;*
GOTRC           andi    #$7FFF,(SP)     ; SWITCH OFF TRACE BIT
                move    (SP)+,REGS+14   ; SAVE STATUS REG
                move.l  (SP)+,REGS      ; save PROGRAM COUNTER
                move.l  A7,REGS+8       ; save SYSTEM STACK
                movem.l d0-D7/a0-A7,REGS+16     ; save REGISTERS
                move    USP,a0
                move.l  a0,REGS+4       ; save USER STACK
                move.l  OldSSP,A7       ; get BACK MONITOR STACK
                clr     d0              ; MARK AS ACTIVATE
                bsr     BRACT           ; ACTIVATE ALL BREAKPOINTS
                bsr     JSTART          ; NOTE bsr SO AS TO KEEP STACK CORRECT
;*
;*
;*  brkpt       routine executed when a breakpoint is reached via TRAP
;*
BRKPT           move    #1,Status       ; MARK AS IN MONITOR MODE
                move    (SP)+,REGS+14   ; SAVE STATUS REGISTER
                subq.l  #2,(SP)         ; BACKSPACE PC
                move.l  (SP)+,REGS      ; save PROGRAM COUNTER
                move.l  A7,REGS+8       ; save USER SYSTEM STACK
                movem.l d0-D7/a0-A7,REGS+16     ; save REGISTERS TO DUMP
                move    USP,a0
                move.l  a0,REGS+4       ; save USER STACK
                move.l  OldSSP,A7       ; get MONITOR STACK BACK
                moveq   #1,d0
                bsr     BRACT           ; DEACTIVATE ALL BREAKPOINTS
                move.l  #BrkTab,a0      ; get BASE OF BREAKPOINT TABLE
                clr     D4
                moveq   #NUMBRK-1,D1    ; number BREAKPOINTS -1 (DBCC)
                move.l  REGS,d0         ; get PROGRAM COUNTER
BP_0            cmp.l   0(a0,D4),d0     ; current PC IN BREAKPOINT TABLE ?
                beq     BP_1
                add     #12,D4          ; POINT TO NEXT ELEMENT
                dbf     D1,BP_0
                move.l  #ERRNBR,a0
                bsr     P_STR   ; say BREAKPOINT NOT FOUND IN TABLE
                bra     BP_RET
BP_1            move.b  5(a0,D4),D1     ; get BREAKPOINT TYPE
                cmpi.b  #2,D1           ; temp BREAKPOINT ?
                bne     BP_2            ; NO
                bsr     BRAR            ; REMOVE TEMP BREAKPOINT
                bsr     DREG            ; PRINT REGISTERS
                bra     BP_RET          ; AND RETURN
BP_2            cmpi.b  #1,D1           ; ordinary BREAKPOINT ?
                bne     BP_3            ; NO
                move.l  #BRSTR,a0
                bsr     CRLF
                bsr     P_STR   ; say BREAKPOINT 
                bsr     CRLF
                bsr     DREG            ; PRINT REGISTERS
                bra     BP_RET
BP_3            cmpi.b  #3,D1           ; mark BREAKPOINT ?
                bne     BP_4            ; must BE PASS TYPE BREAKPOINT
                move.b  #'P',d0
                bsr     PUTCH
                move.b  #'C',d0
                bsr     PUTCH
                move.l  #EQUAL,a0
                bsr     P_STR           ; say PC = 
                move.l  REGS,d0
                moveq   #8,D6           ; field WIDTH 8
                bsr     PRINT_HEX       ; print PROGRAM COUNTER
                bsr     CRLF
                bsr     G_5             ; note bsr TO KEEP STACK, BACK TO USER PROG
BP_4            move.l  8(a0,D4),d0     ; get COUNT VALUE 
                beq     BP_5            ; REACHED 0 SO STOP
                subi.l  #1,8(a0,D4)     ; DECREMENT COUNT
                bsr     G_5             ; not REACHED 0 SO GO BACK (NOTE bsr)
BP_5            move.l  REGS,d0         ; get BREAKPOINT ADDRESS
                bsr     BRAR            ; REMOVE PASS BREAKPOINT WHEN 0
                move.l  #PASTR,a0
                bsr     CRLF
                bsr     P_STR           ; say PASS BREAKPOINT
                bsr     CRLF
                bsr     DREG
BP_RET          jmp     START           ; JUMP BACK TO COMMAND MODE

;*
;*
;* Add/Remove.list breakpoints
;*

B_0
BPadd		move.l  #BTSTR,a0
                bsr     P_STR   ; prompt FOR TYPE OF BREAKPOINT
                bsr     GetCh
                bsr     PUTCH
                move.b  d0,-(SP)        ; save CHARACTER
                move.b  #SPACE,d0
                bsr     PUTCH           ; PRINT SPACE AFTER
                move.b  (SP)+,d0        ; return CHARACTER
                bsr     TO_UPPER
                clr.l   D2              ; CLEAR DATA FIELD
                moveq   #1,D1           ; set TYPE TO ORDINARY
                clr     D7              ; SET STATUS TO NO ERROR TO START
                cmpi.b  #CR,d0
                beq     B_RET
                cmpi.b  #'O',d0
                beq     B_ADD
                moveq   #3,D1           ; set TYPE TO MARK
                cmpi.b  #'M',d0
                beq     B_ADD
                cmpi.b  #'P',d0
                bne     B_ERR
                move.l  #BPSTR,a0
                bsr     P_STR   ; prompt FOR PASS COUNT
                moveq   #1,d0           ; expect ONE NUMBER
                bsr     GET_NUMS
                tst     D7              ; ERROR ?
                bne     B_RET           ; YES
                move.l  (A3)+,D2        ; save PASS COUNT
                moveq   #4,D1           ; type IS PASS
B_ADD           clr     D3              ; MARK AS ADD BREAKPOINT
                bra     B_BRK

B_ERR           moveq   #4,D7           ; mark AS ERROR
                bra     B_RET

;*

B_1
BPremove	moveq   #1,D3           ; mark AS REmove.bREAKPOINT
;*
B_BRK           move.l  #BRSTR,a0
                bsr     P_STR   ; prompt FOR BREAKPOINT 
                moveq   #1,d0           ; expect 1 NUMBER
                bsr     GET_NUMS
                tst.b   D7
                bne     B_RET           ; ERROR
                move.l  (A3)+,d0        ; get BREAKPOINT ADDRESS
                btst    #0,d0
                beq     B_01
                move.l  #ERROA,a0
                bsr     P_STR   ; say ODD ADDRESS
                clr     D7
                bra     B_RET
B_01            cmpi.l  #8,d0
                blt     B_03            ; < 8 ERROR IN ROM (MUST BE)
                cmpi.l  #RamEnd,d0
                bge     B_03            ; > RamEnd THEREFORE ERROR
                tst     D3
                bne     B_02            ; not INSERT BREAKPOINT
                bsr     BRAD            ; INSERT BREAKPOINT
                clr     D7              ; ERRORS HANDLED IN BRAD
                bra     B_RET
B_02            bsr     BRAR            ; REmove.bREAKPOINT
                clr     D7              ; ERRORS HANDLED
                bra     B_RET
B_03            move.l  #ERRCR,a0
                bsr     P_STR   ; say ERROR BREAKPOINT IN ROM ETC
                clr     D7
                bra     B_RET
;*
B_2
BPlist		bsr     CRLF
                move.l  #ORSTR,a0
                bsr     P_STR   ; say ORDINARY ->
                move.l  #BrkTab,A1      ; base OF BREAKPOINT TABLE
                moveq   #10,D6          ; set HEX PRINT FIELD WIDTH
                move    #NUMBRK-1,D1
                clr     D4
B_21            cmpi.b  #1,5(A1,D4)
                bne     B_22
                move.l  0(A1,D4),d0
                bsr     PRINT_HEX       ; print BREAKPOINT
B_22            add     #12,D4          ; POINT TO NEXT BREAKPOINT ENTRY
                dbf     D1,B_21
                move.l  #MRSTR,a0
                bsr     P_STR   ; say MARK 
                move    #NUMBRK-1,D1
                clr     D4
B_23            cmpi.b  #3,5(A1,D4)
                bne     B_24
                move.l  0(A1,D4),d0
                bsr     PRINT_HEX       ; print BREAKPOINT
B_24            add     #12,D4
                dbf     D1,B_23
                move.l  #PASTR,a0
                bsr     P_STR   ; say PASS
                moveq   #NUMBRK-1,D1
                clr     D4
                clr     D5              ; FLAG FOR PASS PRINT COUNT
B_25            cmpi.b  #4,5(A1,D4)
                bne     B_28
                tst     D5              ; NOT PRINTED OUT A PASS BRKPT YET ?
                beq     B_27
                moveq   #23,D2
                move.b  #SPACE,d0
B_26            bsr     PUTCH
                dbf     D2,B_26         ; print SPACES
B_27            moveq   #10,D6          ; set PRINT HEX FIELD WIDTH 
                move.l  0(A1,D4),d0
                bsr     PRINT_HEX       ; print BREAKPOINT
                move.l  #CNSTR,a0
                bsr     P_STR   ; say COUNT
                moveq   #4,D6           ; field WIDTH OF 4 (16 BIT)
                move.l  8(A1,D4),d0     ; get COUNT
                bsr     PRINT_HEX
                bsr     CRLF
                addq    #1,D5           ; mark AS PRINTED OUT A PASS BREAKPOINT
B_28            add     #12,D4          ; POINT TO NEXT BREAKPOINT
                dbf     D1,B_25 
                moveq   #0,D7
B_CR            bsr     CRLF
B_RET           RTS
;*
;*
;* brad         Add a breakpoint to breakpoint table
;* On entry :-
;*              d0.l    breakpoint address
;*              d1.b    breakpoint type :-
;*                              1)      normal (remains until removed)
;*                              2)      temporary (set by G,<address>)
;*                              3)      Mark point (displays pc when passed)
;*                              4)      Pass point (counts down stops when 0)
;*              d2.l    Data value to store in breakpoint
;* On exit  :-
;*              d0.l    unchanged
;*              d1.b    unchanged
;*              d7.b    status :-
;*                              0)      success 
;*                              1)      breakpoint already present in table
;*                              2)      table full
;*
BRAD            movem.l d0-D6/a0-A6,-(SP)       ; save REG
                move.l  d0,a0           ; copy BREAKPOINT
                move.l  #BrkTab,A1      ; point TO TABLE
                move    BRKCNT,D3       ; GET CURRENT NO. OF BREAKPOINTS
                cmpi    #NUMBRK,D3
                blt     BA_0            ; < MAX NO
                move.l  #ERRTF,a0
                bsr     P_STR   ; say TABLE FULL
                moveq   #2,D7
                bra     BA_RET
BA_0            moveq   #NUMBRK-1,D5    ; number OF BREAKPOINTS -1 (DBCC)
                clr     D4
BA_1            cmp.l   0(A1,D4),d0     ; breakpoint THERE ?
                bne     BA_2
                move.l  #ERRBP,a0
                bsr     P_STR   ; say BREAKPOINT ALREADY THERE
                bra     BA_RET
BA_2            add     #12,D4          ; POINT TO NEXT ENTRY
                dbf     D5,BA_1
                moveq   #NUMBRK-1,D5
                clr     D4
BA_3            cmpi.l  #0,0(A1,D4)     ; EMPTY ?
                beq     BA_4
                add     #12,D4          ; MOVE TO NEXT
                dbf     D5,BA_3
BA_4            move.l  d0,0(A1,D4)     ; insert BREAKPOINT ADDRESS
                move    (a0),6(A1,D4)   ; SAVE InStrUCTION
                move.b  D1,5(A1,D4)     ; save BREAKPOINT TYPE
                move.l  D2,8(A1,D4)     ; set DATA FIELD 
                moveq   #0,D7           ; mark AS SUCCESSFULL
                addq    #1,D3           ; inc NUMBER OF BREAKPOINTS
                move    D3,BRKCNT
BA_RET          movem.l (SP)+,d0-D6/a0-A6       ; restore REG
                RTS
;*
;*
;* brar         Remove a breakpoint from breakpoint table
;* On entry :-
;*              d0.l    breakpoint address
;* On exit  :-
;*              d0.l    unchanged
;*              d7.b    status :-
;*                              0)      success 
;*                              1)      breakpoint not present in table
;*                              2)      table empty
;*
BRAR            movem.l d0-D6/a0-A6,-(SP)       ; save REG
                move.l  #BrkTab,A1      ; get BASE OF TABLE
                move    BRKCNT,D2       ; GET CURRENT NUMBER OF BREAKPOINTS
                bne     BR_0
                move.l  #ERRTE,a0
                bsr     P_STR   ; say TABLE EMPTY
                moveq   #2,D7
                bra     BR_RET
BR_0            moveq   #NUMBRK-1,D1    ; no. BREAKPOINTS
                clr     D4
BR_1            cmp.l   0(A1,D4),d0     ; breakpoint THERE ?
                beq     BR_2
                add     #12,D4          ; POINT TO NEXT BREAKPOINT
                dbf     D1,BR_1
                move.l  #ERRBN,a0
                bsr     P_STR   ; say BREAKPOINT NOT FOUND
                moveq   #1,D7
                bra     BR_RET
BR_2            move.l  d0,a0           ; copy BREAKPOINT ADDRESS
                move    6(A1,D4),a0     ; RESTORE InStrUCTION
                move.l  #0,0(A1,D4)     ; zero BREAKPOINT
                move.l  #0,4(A1,D4)     ; clear TYPE & InStrUC
                move.l  #0,8(A1,D4)     ; zero DATA FIELD
                moveq   #0,D7
                subq    #1,D2
                move    D2,BRKCNT       ; SAVE NEW NUMBER OF BREAKPOINTS 
BR_RET          movem.l (SP)+,d0-D6/a0-A6       ; restore REG
                RTS
;*
;*
;* bract                Routine to activate / deactivate all breakpoints in table
;* On entry :-
;*              d0.b    status
;*                              0)      activate breakpoints
;*                              1)      deactivate breakpoints
;* On exit  :-   No change
;*
BRACT           movem.l D1-D6/a0-A6,-(SP) ; save REG
                move.l  #BrkTab,a0      ; base OF BREAKPOINT TABLE
                clr     D4
                moveq   #NUMBRK-1,D1    ; number BREAKPOINTS -1 (DBCC)
BA              cmpi.l  #0,0(a0,D4)     ; ACTIVE BREAKPOINT ?
                beq     Ba0
                move.l  0(a0,D4),A1     ; get BREAKPOINT ADDRESS
                tst     d0              ; ACTIVATE ?
                bne     BN
                move    (A1),6(a0,D4)   ; SAVE InStrUCTION
                move    #$4E4F,(A1)     ; INSERT TRAP #15 InStrUCTION
                bra     Ba0
BN              move    6(a0,D4),(A1)   ; RESTORE InStrUCTION
Ba0             add     #12,D4          ; POINT TO NEXT ENTRY
                dbf     D1,BA
                movem.l (SP)+,D1-D6/a0-A6 ; restore REG
                RTS

;*
;*
;* docallc	Call 'C' program
;*
DOCALLC		bsr	CheckMem	; Make sure store is valid
		beq	DCsok
		move.l	#NSSTR,a0	; Complain
		bsr	P_STR
		RTS

DCsok		move.l	#CSTR,a0
		bsr	P_STR

		move.l	BotOfStore,d0	; Base of 'C' program
		jmp	J_1

;*
;*
;*
;* jump         Routine to jump to a subroutine address.
;*              Prompts for <address> and performs a jsr to this location with
;*              registers from dump
;*
JUMP		bsr	CheckMem	; Make sure store is valid
		beq	Jsok
		move.l	#NSSTR,a0	; Complain
		bsr	P_STR
		jmp	JRET

Jsok		move.l  #JSTR,a0
                bsr     P_STR
                moveq   #1,d0
                bsr     GET_NUMS
                tst.b   D7              ; GOT NUMBER ?
                bne     JRET
                move.l  (A3)+,d0        ; get ADDRESS
                btst    #0,d0
                beq     J_1
;*
                move.l  #ERROA,a0       ; odd ADDRESS ERROR
                bsr     P_STR
JRET            rts                     ; RETURN TO COMMAND HANDLER
;*
J_1             move.l  REGS,OldPC      ; save OLD PROGRAM COUNTER
                move.l  d0,REGS         ; set JUMP LOCATION
                move.l  REGS+8,a0       ; get USS
                move.l  #JEND,-(a0)     ; put 'RETURN' ADDRESS ONTO STACK
                move.l  a0,REGS+8       ; and RETURN POINTER
;*
;* jstart       this routine performs a jump to the user routine at the location
;*              held in the dumped Program counter 
;*
JSTART          addq.L  #4,A7           ; set MONITOR SYSTEM STACK CORRECT 
                move.l  A7,OldSSP       ; save MONITOR STACK
                move.l  REGS+8,a0       ; get USER SYSTEM STACK
                move.l  REGS,-(a0)      ; push JUMP ADDRESS ONTO CURRENT STACK
                move    REGS+14,-(a0)   ; PUSH STATUS REGISTER ONTO STACK
                move.l  a0,REGS+8       ; and RETURN IT
                move.l  REGS+4,a0       ; get USER STACK POINTER
                move    a0,USP
                move.l  REGS+8,REGS+76  ; put USER SYSTEM STACK INTO A7 LOC.
                clr     Status          ; MARK FLAG AS USER ROUTINE
                movem.l REGS+16,d0-D7/a0-A7     ; restore REGISTERS FROM DUMP
                rte                     ; AND JUMP TO ROUTINE
;*
;*
;* jend         returned to by a user subroutine call
;*              saves registers and then initiates a system trap to guarantee
;*              being in supervisor state before accessing stacks
;*
JEND            movem.l d0-D7/a0-A7,REGS+16     ; save REGISTERS
                move    #1,Status       ; MARK AS MONITOR MODE
                trap    #14             ; SUBROUTINE TO SUPERVISOR MONITOR
;* system MODE IS GUARANTEED FROM HERE
                move.l  A7,REGS+8       ; save USER SYSTEM STACK
                move.l  OldSSP,A7       ; get BACK MONITOR STACK
                move.l  OldPC,REGS      ; restore OLD PROGRAM COUNTER 
                move    USP,a0          ; GET USER STACK POINTER
                move.l  a0,REGS+4       ; save USER STACK
                bsr     DREG
                jmp     START           ; JUMP TO COMMAND MODE
;*
;*
;* restart      Jumped here from JEND, this ensures that the
;*              monitor is always executed in Supervisor state to allow use
;*              of reserved InStructions etc.
;*
RESTART         move    SR,REGS+14      ; SAVE STATUS REGISTER
                move    USP,a0          ; GET USER STACK (IN SYSTEM MODE)
                move.l  a0,REGS+4       ; and SAVE USP TO DUMP
                ori     #2000,(SP)      ; ENSURE SYSTEM BIT SET ON RETURN
                rte                     ; RETURN BACK TO JEND
;*
;*
;*
;* trc          Trace routine 
;*              Prompts for optional trace count, if none specified default
;*              is one step, each step dumps registers, traces from dumped PC
;*
TRC             move.l  #TRACE,$24      ; set TRACE VECTOR UP
                move.l  #TSTR,a0
                bsr     P_STR
                moveq   #1,d0           ; one NUMBER EXPECTED
                bsr     GET_NUMS
                cmpi.b  #1,D7           ; no NUMBER ?
                beq     T_2
                tst.b   D7              ; ERROR
                bne     TRET            ; YES
                move.l  (A3)+,d0        ; get COUNT
                tst.l   d0
                beq     T_ILL           ; CANT TRACE 0 STEPS !
                move.l  d0,TRCNT        ; save TRACE COUNT
T_1             ori     #$8000,REGS+14  ; SET TRACE FLAG
                bra     JSTART          ; AND EXIT TO USER ROUTINE
;*
T_2             move.l  #1,TRCNT        ; trace ONE STEP
                bra     T_1
T_ILL           move.l  #ERRIV,a0
                bsr     P_STR   ; say ILLEGAL VALUE ETC
TRET            rts                     ; RETURN TO CALLING ROUTINE
;*
;* trace                routine executed on trace exception
;*              saves registers, prints them checks step count for -1
;*              if so returns to command level
;*
TRACE           andi    #$7FFF,(SP)     ; SWITCH OFF TRACE BIT
                move    (SP)+,REGS+14   ; STORE IN STATUS AREA IN REG DUMP
                move.l  (SP)+,REGS      ; store NEW PROGRAM COUNTER
                move.l  A7,REGS+8       ; store SYSTEM STACK
                movem.l d0-D7/a0-A7,REGS+16     ; dump ALL REGISTERS
                move    USP,a0
                move.l  a0,REGS+4       ; save USER STACK
                move.l  OldSSP,A7       ; restore SYSTEM STACK POINTER
                bsr     DREG            ; PRINT REGISTERS
                bsr     CRLF
                subq.l  #1,TRCNT        ; DECREMENT COUNT
                beq     START           ; FINISHED
                bsr     T_1             ; note bsr SO AS TO KEEP STACK CORRECT
;*
;* buserr       jumps here on a bus error
;*
BUSERR          move    #2,ErrNum       ; MARK AS WHICH ERROR (BUS ERROR)
                bra     BIGERR
;*
;* aderr                jumps here on address error
;*
ADERR           move    #3,ErrNum       ; ADDRESS ERROR
BIGERR          tst     Status          ; IN MONITOR MODE ?
                bne     E0              ; YES SO DONT SAVE REGS TO DUMP
                movem.l d0-D7/a0-A7,REGS+16     ; save REGISTERS
                move    USP,a0
                move.l  a0,REGS+4       ; save USER STACK
                move.l  A7,A6           ; copy STACK FOR REMOVING DATA
                move.l  OldSSP,A7       ; get BACK MONITOR STACK
                bra     E1
;*
E0              move.l  A7,A6           ; copy MONITOR STACK SINCE INVALID
E1              moveq   #1,d0
                bsr     BRACT           ; DEACTIVATE ALL BREAKPOINTS
                move.l  EXSTR,a0
                cmpi    #2,ErrNum       ; WHAT ERROR TYPE ?
                beq     E2
                move.l  EXSTR+4,a0
E2              bsr     P_STR   ; say ERROR ETC
                move.l  #ACSTR,a0
                bsr     P_STR   ; say ACCESS TYPE
                move    (A6)+,d0        ; GET ACCESS NUMBER
                andi    #31,d0          ; MASK LOWER 5 BITS
                moveq   #2,D6
                bsr     PRINT_HEX       ; print ACCESS TYPE
                bsr     CRLF
                move.l  #CCSTR,a0
                bsr     P_STR
                moveq   #8,D6
                move.l  (A6)+,d0
                bsr     PRINT_HEX       ; print CYCLE ADDRESS
                bsr     CRLF
                move.l  #IRSTR,a0
                bsr     P_STR
                move    (A6)+,d0
                moveq   #4,D6
                bsr     PRINT_HEX       ; print InStrUCTION REGISTER CONTENTS
                bsr     CRLF
                tst     Status          ; MONITOR / USER ?
                bne     EX0             ; NOT USER
                move    (A6)+,REGS+14   ; SAVE STATUS REGISTER
                move.l  (A6)+,REGS      ; get PC ADDRESS AND SAVE
                move.l  A6,REGS+8       ; store SYSTEM STACK
                move.l  A6,REGS+76      ; store CURRENT STACK POINTER
                bsr     CRLF
                bsr     DREG            ; PRINT REGISTERS
                bra     START           ; AND GO BACK TO COMMAND
;*
EX0             move.l  #SRSTR,a0
                bsr     P_STR
                move    (A6)+,d0        ; GET STATUS
                bsr     PRINT_HEX       ; print STATUS REGISTER
                bsr     CRLF
                move.l  #PCSTR,a0
                bsr     P_STR
                moveq   #8,D6
                move.l  (A6)+,d0
                bsr     PRINT_HEX       ; print PROGRAM COUNTER
                bsr     CRLF
                move.l  #Stack,A7       ; since UNRECOVERABLE ERR RESET STACK
                bra     START           ; AND RETURN SINCE ERROR IN MONITOR
;*
;* exc1         exception handler, saves all registers to dump area
;*              prints out appropriate error message for other exceptions
;*
EXC1            movem.l d0-D7/a0-A7,REGS+16     ; save REGISTERS
                move    (SP)+,REGS+14   ; SAVE STATUS REGISTER
                move.l  (SP)+,REGS      ; get PC ADDRESS AND SAVE
                move.l  A7,REGS+8       ; store SYSTEM STACK
                move.l  A7,REGS+76      ; store CURRENT STACK POINTER
                move.l  OldSSP,A7       ; get BACK MONITOR STACK
;*
                moveq   #1,d0
                bsr     BRACT           ; DEACTIVATE ALL BREAKPOINTS
                move    ErrNum,d0       ; GET VECTOR NUMBER
                move.l  #EXSTR,a0       ; base OF EXCEPTION MESSAGE TABLE
                subq    #2,d0
                lsl     #2,d0           ; GET POSITION OF STRING POINTER IN TABLE
                move.l  #EXSTR,A1       ; get STRING POINTER TABLE BASE
                move.l  0(A1,d0),a0     ; get EXCEPTION MESSAGE START
                bsr     P_STR
                bsr     CRLF
                bsr     DREG            ; PRINT REGISTERS
                bra     START           ; RETURN TO COMMAND MODE
;*
;*
;* ill          jump here on illegal InStruction
;*
ILL             move    #4,ErrNum       ; VECTOR 4
                bra     EXC1
;*
;* div          jump here on divide by 0 
;*
DIV             move    #5,ErrNum
                bra     EXC1
;*
;* chk          jump here on check exception
;*
CHKX            move    #6,ErrNum
                bra     EXC1
;*
;* ovl          jump here on division overflow
;*
OVL             move    #7,ErrNum
                bra     EXC1
;*
;* prv          jump here on privilage violation 
;*
PRV             move    #8,ErrNum
                bra     EXC1
;*
;* trace                Defined elsewhere 
;*

;*
;* emu1         jump here on emulation 1010 opcode
;*
EMU1            move    #10,ErrNum
                bra     EXC1
;*
;* emu2         jump here on emulation 1111 opcode
;*
EMU2            move    #11,ErrNum
                bra     EXC1
;*
;* res          jump here on reserved trap opcode
;*
RES             move    #12,ErrNum      ; FROM HERE ON ErrNum HOLDS STRING OFFSET
                bra     EXC1            ; NOT VECTOR NUMBER
;*
;* spu          jump here on spurious interrupt
;*
SPU             move    #13,ErrNum
                bra     EXC1
;*
;* aut          jump here on autovectored interrupt
;*
AUT             move    #14,ErrNum
                bra     EXC1
;*
;* trp          jump here on uninitialised InStruction trap
;*
TRP             move    #15,ErrNum
                bra     EXC1
;*
;* int          jump here on uninitialised vectored interrupt
;*
INT             move    #16,ErrNum
                bra     EXC1
;*
;*
;* xam          examine and/or change registers
;*              registers may be a for all address, D for all data, or
;*              A<0-6>, or D<0-7>, or PC, SR, US, SS
;*
XAM             move.l  #XSTR,a0
                bsr     P_STR
                move.l  #InStr,a0
                bsr     G_STR
                bsr     TOUP_STR        ; uppercase STRING
                bsr     CRLF
                move.l  #REGSTR,A1      ; a1 POINTS TO REGISTER LIST (ASCII)
                move.l  a0,A2           ; a2 POINTS TO INPUT STRING
                move.l  #REGS,A4        ; point TO BASE OF REGISTER DUMP AREA
;*
                moveq   #0,D7           ; mark STATUS AS SUCCESSFUL (HOPEFUL!)
                cmpi.b  #2,D1           ; no CHARACTERS INPUT (CR,LF EXCEPTED)
                beq     DREG            ; NO INPUT SO DUMP REGISTERS
                moveq   #2,D7           ; status = ERROR
                cmpi.b  #3,D1           ; one CHARACTER ?
                bne     X_2
                cmpi.b  #'A',(A2)       ; only CHAR AN 'A' ?
                bne     X_1
                moveq   #48,D4          ; offset IN REG DUMP FOR ADDRESS REG
                moveq   #6,D5           ; count -1
                bra     X_6             ; GOTO DUMP & CHANGE
X_1             cmpi.b  #'D',(A2)       ; char A 'D'
                bne     X_RET           ; error AS NO A|D AS ONLY CHARACTER
                moveq   #16,D4          ; offset IN REG DUMP FOR DATA REGISTERS
                moveq   #7,D5           ; count -1
                bra     X_6
X_2             moveq   #0,D4           ; ready FOR FIRST CHARACTER PAIR
X_3             move.b  (A2),d0
                cmp.b   0(A1,D4),d0     ; CHECK FIRST CHAR OF PAIR
                bne     X_4
                move.b  1(A2),d0
                cmp.b   1(A1,D4),d0     ; AND SECOND
                beq     X_5
X_4             addq.L  #2,D4           ; point TO NEXT CHARACTER PAIR
                cmpi.b  #$26,D4         ; end OF LIST ?
                bne     X_3
                bra     X_RET           ; ERROR AS UNIDENTIFIABLE REGISTER
X_5             lsl.l   #1,D4           ; NOW POINTS TO WHERE IN REG DUMP
                moveq   #0,D5           ; only ONE REGISTER FOUND
                addq.L  #2,A2           ; now POINT TO WHERE ANY NUMBER IS AT
                move.l  A2,a0
                bsr     ASC_TO_HEX      ; look FOR A NUMBER
                tst.b   D7              ; ERROR ?
                bne     X_6
                move.l  d0,D2           ; number GOT FROM STRING
                moveq   #0,D7           ; no ERROR SO FAR !
                bra     X_9             ; JUMP TO INSERT & END
X_6             lsr.l   #1,D4           ; GET POSITION IN STRING
                move.b  0(A1,D4),d0     ; first CHAR
                bsr     PUTCH
                move.b  1(A1,D4),d0     ; second CHAR
                bsr     PUTCH
                move.l  #EQUAL,a0
                bsr     P_STR   ; print = 
                lsl.l   #1,D4           ; GET POSITION WITHIN REG DUMP AREA
                move.l  0(A4,D4),d0     ; get REGISTER VALUE
                cmpi.b  #12,D4          ; status REGISTER ?
                bne     X_7
                moveq   #4,D6           ; status REGISTER IS 16 BITS (FIELD 4)
                bra     X_8
X_7             moveq   #8,D6           ; field WIDTH OF 8
X_8             bsr     PRINT_HEX       ; print REG CONTENTS
                move.l  D2,d0           ; copy NUMBER GOT FROM INPUT
                move.l  #PROM,a0
                bsr     P_STR   ; print A PROMPT
                moveq   #1,d0           ; one NUMBER
                bsr     GET_NUMS        ; get IT
                move.l  (A3)+,d0        ; get NUMBER
                tst.b   D7              ; GOT IT ?
                beq     X_9
                cmpi.b  #1,D7           ; nonumber ?
                bne     X_RET           ; error IN INPUT
                bra     X_11
X_9             cmpi.b  #12,D4          ; status REGISTER ?
                bne     X_10            ; if NOT GO AHEAD AND PUT INTO DUMP
                moveq   #2,D7           ; number TOO BIG STATUS
                cmpi.l  #$FFFF,d0       ; VALUE TOO BIG ?
                bgt     X_RET           ; YES SO RETURN WITH ERROR
X_10            move.l  d0,0(A4,D4)     ; put NEW VALUE BACK INTO DUMP AREA
X_11            addq.L  #4,D4           ; point TO NEXT REGISTER LOCATION
                moveq   #1,D7           ; next TIME ROUND ASK FOR NUMBER (8)
                dbf     D5,X_6          ; print ANOTHER REGISTER
                moveq   #0,D7           ; no ERROR
X_RET           rts                     ; ERROR STATUS IN D7.B
;*
;*
;* dreg         dump all registers from save area to screen
;*
DREG            move.l  #RDSTR,a0
                bsr     P_STR
                move.l  #REGS,A1        ; pointer TO DUMP AREA
                move.l  #REGSTR,A2      ; pointer TO REGISTER STRING
                clr.l   D2
DR_0            move.b  (A2)+,d0        ; get FIRST CHAR
                bsr     PUTCH           ; PRINT IT
                move.b  (A2)+,d0        ; SECOND
                bsr     PUTCH
                move.l  #EQUAL,a0
                bsr     P_STR           ; print DELIMETER
                move.l  (A1)+,d0        ; get REGISTER
                moveq   #8,D6           ; field WIDTH OF 8 CHAR
                cmpi.b  #19,D2          ; a7 ?
                bne     DR_01           ; not STACK POINTER
                move.l  REGS+8,d0       ; get SYSTEM STACK
                btst    #13,REGS+14     ; CHECK SYSTEM / USER BIT
                bne     DR_1            ; system SO PRINT SYSTEM STACK
                move.l  REGS+4,d0       ; put USER STACK IN d0
DR_01           cmpi.b  #3,D2           ; status REGISTER ?
                bne     DR_1
                bsr     print_sr
                bra     dr_sr
;               moveq   #4,D6           ; yes SO PRINT IN FIELD WIDTH OF 4
DR_1            bsr     PRINT_HEX       ; print IT
dr_sr           move.l  #SEP,a0
                bsr     P_STR           ; delimit REGISTERS
                move.b  D2,D3
                andi.b  #3,D3
                cmpi.b  #3,D3           ; eol ?
                bne     DR_4
                bsr     CRLF            ; YES SO SEPERATE LINES
DR_4            addq.B  #1,D2
                cmpi.b  #20,D2          ; finished ?
                bne     DR_0
DR_RET          moveq   #62,D1
                move.b  #MINUS,d0
DR_5            bsr     PUTCH
                dbf     D1,DR_5         ; print LINE ------
                bsr     CRLF
                RTS

;
; print status register in flag format
;
; sr = nTSXNZVC         where n is interrupt mask level
;
print_sr        movem.l d0-d3/a0,-(sp)
                move.l  d0,d1           ; copy sr
                andi    #%0000011100000000,d0
                lsr     #8,d0           ; get interrupt level mask
                bsr     PRINT_DEC       ; print it
                move.l  d1,d0
                andi    #%1010000000011111,d0
                btst    #15,d0
                beq     x_tsb
                bset    #6,d0
x_tsb           btst    #13,d0
                beq     x_psr
                bset    #5,d0           ; d0 now holds 000000000TSXNZVC
x_psr           move    d0,d1
                lea     srflagstr,a0
                moveq   #6,d3           ; start at T
x_tl            btst    d3,d1
                beq     x_pm
                move.b  (a0),d0
                bsr     PUTCH
x_nc            add.l   #1,a0           ; point to next char            
                dbeq    d3,x_tl
                movem.l (sp)+,d0-d3/a0
                rts
;
x_pm            moveq   #'-',d0
                bsr     PUTCH
                bra     x_nc
;*
;*
;*
;* change       Routine prompts for start address, then displays contents
;*              of address, prompts for new value (hex byte), if no change
;*              occurs then says no CHANGE, else moves to next address.
;*              Input may be a hex byte or a string delimeted by '
;*
CHANGE          move.l  #CSTR,a0
                bsr     P_STR
                moveq   #1,d0           ; expected NUMBER OF ARGUMENTS
                bsr     GET_NUMS
                tst.b   D7
                bne     CEND            ; ERROR
                move.l  (A3)+,A1        ; get ADDRESS
                bsr     CRLF
C_0             move.l  A1,d0
                moveq   #9,D6           ; field WIDTH
                bsr     PRINT_HEX
                move.l  #EQUAL,a0
                bsr     P_STR   ; mark AS CONTAINS ' = '
                move.b  (A1),d0         ; get MEMORY CONTENTS
                moveq   #2,D6           ; field WIDTH TO PRINT NUMBER
                bsr     PRINT_HEX       ; print CONTENTS
                move.l  #PROM,a0
                bsr     P_STR   ; prompt FOR ENTRY
                move.l  #InStr,a0
                bsr     G_STR   ; input A LINE
                moveq   #0,D7           ; ready FOR NO ERROR RETURN
                move.l  a0,A2           ; copy INPUT STRING POINTER
                cmpi.b  #CR,(A2)
                beq     C_2             ; MOVE TO NEXT LOCATION
                cmpi.b  #'.',(A2)
                beq     CEND            ; TERMINATOR
                cmpi.b  #39,(A2)        ; a ' ?
                beq     C_1
                bsr     ASC_TO_HEX      ; convert NUMBER IF ANY
                tst.b   D7
                bne     CEND            ; ERROR
                cmpi.l  #$FF,d0         ; CHECK IF NUMBER TOO BIG
                bgt     C_BIG
                move.b  d0,(A1)         ; no SO PUT IN MEMORY
                cmp.b   (A1),d0         ; SAME NUMBERS ? (IN ROM?)
                bne     C_ROM
                addq.L  #1,A1           ; ***TEST
                bra     C_0
C_1             addq.L  #1,A2           ; point TO NEXT CHARACTER
                move.b  (A2),d0         ; get NEXT CHAR 
                cmpi.b  #QUOTE,d0       ; another ' ?
                beq     C_0
                cmpi.b  #CR,d0          ; eol ?
                beq     C_0
                move.b  d0,(A1)         ; put CHAR INTO MEMORY
                cmp.b   (A1)+,d0        ; IN ROM ?
                bne     C_ROM           ; yes SO COMPLAIN
                bra     C_1
C_2             addq.L  #1,A1           ; increment MEMORY POINTER
                bra     C_0
C_BIG           move.l  #ERRTB,a0
                jsr     P_STR   ; COMPLAIN ABOUT NUMBER TOO BIG
                bra     C_0
C_ROM           move.l  #ERRCR,a0
                jsr     P_STR   ; COMPLAIN THAT TRIED TO CHANGE ROM
                moveq   #10,D7          ; mark AS INCOMPLETED COMMAND
CEND            RTS

;*
;*
;* move         moves a block of memory a byte at a time
;*              prompts for start, length, destination (in hex)
;*
;*
MOVECMD         movem.l d0-D3/a0-A6,-(SP)       ; save REGS
                move.l  #MSTR,a0
                bsr     P_STR
                moveq   #3,d0           ; number OF NUMBERS REQUIRED
                bsr     GET_NUMS        ; get NUMBERS
                tst.b   D7              ; ERROR ?
                bne     MVCHK           ; YES SO COMPLAIN
                move.l  (A3)+,A2        ; DESTINATION
                move.l  (A3)+,d0        ; length (IN BYTES)
                beq     MVIV            ; NOT ALLOWED 0
                move.l  (A3)+,A1        ; SOURCE
MVLOOP          move.b  (A1)+,(A2)+
                subq.l  #1,d0
                bne     MVLOOP
MVEND           movem.l (SP)+,d0-D3/a0-A6       ; restore REGS
                RTS
;*
MVIV            move.l  #ERRIV,a0
                bsr     P_STR   ; say ILLEGAL VALUE
                bra     MVEND
;*
MVCHK           cmpi.b  #1,D7           ; eos ?
                bne     MVEND           ; NO TYPO ERROR
                cmpi.b  #0,d0           ; no NUMBERS CONVERTED
                beq     MVEND           ; YES
                moveq   #10,D7          ; mark AS UNFINISHED (INSUFFICIENT NO)
                bra     MVEND
;*
;*
;* fill         fills a block of memory a byte at a time
;*              prompts for start, length, value <0-ff> (all in hex)
;*
;*
FILL            movem.l d0-D6/a0-A6,-(SP)       ; save REGS
                move.l  #FSTR,a0
                bsr     P_STR
                moveq   #3,d0           ; number OF NUMBERS REQUIRED
                bsr     GET_NUMS        ; get NUMBERS
                tst.b   D7              ; ERROR ?
                bne     FCHK            ; YES SO RETURN STATUS
                move.l  (A3)+,D1        ; fill VALUE
                move.l  (A3)+,d0        ; length (IN BYTES)
                beq     FIV             ; CANT HAVE 0
                move.l  (A3)+,A2        ; start ADDRESS
                cmpi.l  #$FF,D1         ; NUMBER TO BIG ?
                bhi     FERR
FLOOP           move.b  D1,(A2)+
                subq.l  #1,d0
                bne     FLOOP
FEND            movem.l (SP)+,d0-D6/a0-A6       ; restore REGS
                RTS
FCHK            cmpi.b  #1,D7           ; eos ?
                bne     FEND            ; NO TYPO ERROR
                cmpi.b  #0,d0           ; no NUMBERS CONVERTED
                beq     FEND            ; YES
                moveq   #10,D7          ; mark AS UNFINISHED (INSUFFICIENT NO)
                bra     FEND
FERR            move.l  #FEMESS,a0
                bsr     P_STR   ; print ILLEGAL FILL VALUE
                bra     FEND
FIV             move.l  #ERRIV,a0
                bsr     P_STR
                bra     FEND
;*
;*
;*
;* dump         dumps memory to port 1
;*              if no args then dumps 128 bytes from current dump location
;*              (initially 0), 1 arg dumps 80 from this address, 2 args
;*              dumps number of characters from this address
;*              Prompts for - <start>, <length> (bytes)
;*
DUMP            move.l  #DSTR,a0
                bsr     P_STR
                moveq   #2,d0           ; max NUMBER OF ARGUMENTS
                move.l  #128,D4         ; default SIZE OF DUMP
                bsr     GET_NUMS        ; get ARGS
                cmpi.b  #1,D7
                bgt     DRET            ; ERROR IN INPUT SO LEAVE
                tst.b   d0              ; NO ARGS ?
                beq     D_0             ; SO DUMP WITH DEFAULTS
                cmpi.b  #1,d0           ; 1 ARG ?
                beq     DU0
                move.l  (A3)+,D4        ; size OF DUMP FROM ARGUMENT
DU0             move.l  (A3)+,ODV       ; set NEW START OF DUMP
D_0             move.l  ODV,A2          ; where TO START DUMP FROM
                bsr     CRLF
DUM             move.l  #OutStr,a0      ; start OF STRING
                move.l  #OutStr+60,A1   ; point TO WHERE CHARACTERS GO
                moveq   #78,d0
D_1             move.b  #SPACE,0(a0,d0) ; insert SPACE
                dbf     d0,D_1
                move.b  #CR,78(a0)
                move.b  #LF,79(a0)
                move.b  #NULL,80(a0)    ; terminate STRING
                move.l  A2,d0
                bsr     HEX_TO_ASC
                addq.L  #2,a0           ; skip A SPACE
D_2             move.b  (A2)+,d0        ; get BYTE
                move.l  a0,A3
                move.l  #TempBuf1,a0
                bsr     HEX_TO_ASC      ; convert NUMBER (8 DIGITS)
                move.b  -2(a0),(A3)+    ; copy MSB CHARACTER
                move.b  -1(a0),(A3)+    ; copy LSB CHARACTER
                move.l  A3,a0           ; RESTORE
                addq    #1,a0           ; skip A SPACE
                cmpi.b  #31,d0
                ble     D_3             ; UNPRINTABLE
                cmpi.b  #126,d0
                bgt     D_3             ; UNPRINTABLE
                move.b  d0,(A1)+        ; put CHAR IN STRING
                bra     D_4
D_3             move.b  #DOT,(A1)+      ; else PUT DOT IN STRING
D_4             subq.l  #1,D4
                tst     D4
                beq     D_E             ; NO MORE CHARS TO DUMP
                move.l  A2,d0
                andi.l  #15,d0
                bne     D_2
D_EOL           move.l  #OutStr,a0
                bsr     P_STR
                bra     DUM             ; DO ANOTHER LINE
D_E             move.l  #OutStr,a0
                bsr     P_STR
DA              move.l  A2,ODV          ; save LAST DUMP LOCATION
                moveq   #0,D7           ; make SURE NO ERROR
DRET            RTS
;*
;*
;* SCAN_COM     routine checks command list & jumps to appropriate routine
;*              if found
;* On entry :-
;*              d0.b    character to search on
;* On exit  :-
;*              if routine not found then :-
;*              d7.b    error status if non zero
;*              a0.L, d0, d1 scrambled
;*
SCAN_COM        move.l  #COMTABLE,a0    ; command LIST
SCNEXT          move.l  (a0)+,D1        ; get COMMAND
                cmpi.b  #NULL,D1        ; end OF LIST ?
                beq     SCERR
                addq.L  #4,a0           ; point TO NEXT ENTRY
                cmp.b   d0,D1           ; COMMAND FOUND ?
                bne     SCNEXT
                move.l  -4(a0),-(SP)    ; set UP ADDRESS OF ROUTINE
                rts                     ; PERFORM A 'CALL TO ROUTINE'
;*
                rts                     ; AND NOW RETURN PROPER (STATUS IN D7)
SCERR           move.b  #$FF,D7         ; command NOT FOUND IN LIST
                rts                     ; AND RETURN
;*
;*
;* GET_NUMS     returns numbers obtained from string input on a stack (reverse)
;*
;* On entry :-
;*              d0.w    number of numbers (long words) to get
;* On exit  :-
;*              d0.l    number of numbers actually got
;*              a3.l    stack pointer
;*              d7.b    status (non zero is an error as ASC_TO_HEX)
;*
GET_NUMS        movem.l D1/D3/D4/a0,-(SP)       ; save REGS
                move.l  #TempStack,A3      ; set UP TEMPORARY STACK
                move    d0,D3           ; NUMBER OF NUMBERS EXPECTED
                subq.b  #1,D3           ; DONT FORGET DBcc UNTIL -1
                clr.l   D4
                move.l  #NUMSTR,a0      ; number INPUT STRING
                bsr     G_STR
GNUM            bsr     ASC_TO_HEX      ; get NUMBER
                tst.b   D7              ; ERROR ?
                bne     GETERR
                addq.L  #1,D4           ; inc NUMBER COUNT
                move.l  d0,-(A3)        ; put ON STACK
                dbf     D3,GNUM         ; and GET ANOTHER
GETERR          move.l  D4,d0           ; copy NUMBER CONVERSION COUNT
                movem.l (SP)+,D1/D3/D4/a0       ; restore REGS
                RTS
;*
;*
;* P_STR        prints characters in a string upto a NULL
;*
;*      On entry :-
;*                      a0      points to string
;*      On exit  :-
;*                      a0      points to end of string
;*
P_STR           move.l  d0,-(SP)        ; save REGS
PS1             move.b  (a0)+,d0
                cmpi.b  #NULL,d0        ; end OF STRING ?
                beq     PSEND           ; YES
                bsr     PUTCH           ; OUTPUT CHARACTER
                bra     PS1
PSEND           move.l  (SP)+,d0        ; restore REGS
                RTS
;*
;* G_STR  gets a string from serial port 1 terminated by a CR
;*            line can be edited before entry by backspace chars,
;*            string terminated by a null after CR
;*
;*      On entry :-
;*                      a0.l    points to buffer start (min length MAXLEN)
;*      On exit  :-
;*                      a0.l    unchanged
;*                      d1.b    length (max MAXLEN)
;*
G_STR           move.l  d0,-(SP)        ; save REG
                clr.l   D1              ; ZERO LENGTH
GETLOOP         bsr     GetCh
                cmpi.b  #BACKSP,d0
                beq     BCKSP           ; HANDLE BACKSPACES
                cmpi.b  #MAXLEN-3,D1    ; string TO LONG ?
                beq     TOOLONG
                cmpi.b  #CR,d0
                beq     CREND           ; END OF LINE
                move.b  d0,0(a0,D1)     ; put CHAR IN BUFFER
                addq.B  #1,D1           ; point TO NEXT LOC
                bsr     PUTCH           ; SHOW WE HAVE CHARACTER
                bra     GETLOOP
BCKSP           cmpi.b  #0,D1           ; no CHARCATERS TO BACKSPACE OVER ?
                beq     GETLOOP         ; YES SO IGNORE
                cmpi.b  #32,-1(a0,D1)   ; unprintable ?
                blt     UNPRT
                bsr     PUTCH           ; BACKSPACE
                move.b  #SPACE,d0
                bsr     PUTCH
                move.b  #BACKSP,d0
                bsr     PUTCH           ; REMOVE CHARACTER
UNPRT           subq.b  #1,D1           ; MOVE POINTER BACK
                bra     GETLOOP
TOOLONG         cmpi.b  #CR,d0
                bne     GETLOOP         ; NO SO IGNORE CHARACTER
CREND           move.b  d0,0(a0,D1)     ; put CR IN BUFFER
                move.b  #LF,1(a0,D1)    ; put IN A LF
                move.b  #NULL,2(a0,D1)  ; put A TERMINATING NULL
                addq.B  #2,D1           ; correct LENGTH
                bsr     CRLF            ; XXXXX
                move.l  (SP)+,d0        ; retore REGS
                RTS
;
;
; SPEC_GET      gets a line without echo no editing no nothing !
;               cr or LF will terminate the line
;
; on entry a0 points to start on exit length in D1.B
;
SG_STR          move.l  d0,-(SP)        ; save REG
                clr.l   D1              ; ZERO LENGTH
SGETLOOP        bsr     GetCh
                cmpi.b  #MAXLEN-3,D1    ; string TO LONG ?
                beq     STOOLONG
                cmpi.b  #CR,d0
                beq     SCREND          ; END OF LINE
                cmpi.b  #LF,d0
                bne     SG1
                move.b  #CR,d0
                bra     SCREND          ; SIMULATE A CR TYPED
SG1             move.b  d0,0(a0,D1)     ; put CHAR IN BUFFER
                addq.B  #1,D1           ; point TO NEXT LOC
                bra     SGETLOOP
STOOLONG        cmpi.b  #CR,d0
                bne     SGETLOOP        ; NO SO IGNORE CHARACTER
SCREND          move.b  d0,0(a0,D1)     ; put CR IN BUFFER
                move.b  #LF,1(a0,D1)    ; put IN A LF
                move.b  #NULL,2(a0,D1)  ; put A TERMINATING NULL
                addq.B  #2,D1           ; correct LENGTH
                move.l  (SP)+,d0        ; retore REGS
                RTS
;               
;*
;*
;* TOUP_STR     uppercase a string
;*
;* On entry :-
;*              a0.l    pointer to start of string
;* On exit :-
;*              no change
;*
TOUP_STR        movem.l d0/D7/a0,-(SP)  ; save REG
TS_0            move.b  (a0),d0         ; get CHAR
                bsr     TO_UPPER
                move.b  d0,(a0)+        ; put CONVERTED CHAR BACK
                bne     TS_0
                movem.l (SP)+,d0/D7/a0  ; restore REG
                RTS
;*
;*
;* ASC_TO_HEX   returns hex number converted from ascii string
;*
;* On entry :-
;*              a0.l    points to start of number (will accept leading nonhex)
;*
;* On exit  :-
;*              a0.l    points to terminating character after number (if any)
;*              d0.l    converted hex number
;*              d1.b    number of characters converted
;*              d7.b    status :-
;*                              0)      no error
;*                              1)      no hex number found before EOS
;*                              2)      more than 8 hex digits found in number
;*                              3)      non hex character found in number
;*
ASC_TO_HEX      move.l  D4,-(SP)        ; save REGS
                clr.l   D4              ; ZERO NUMBER
                clr.b   D1              ; ZERO CHARACTER COUNT
ATH_LOOP        move.b  (a0)+,d0        ; get CHARACTER
                bsr     TO_UPPER        ; make SURE
                bsr     IS_HEX
                tst.b   D7              ; IS IT HEX ?
                bne     NOTHEX
                addq.B  #1,D1           ; inc CHARACTER COUNT
                cmpi.b  #9,D1
                beq     ATH_TOBIG       ; MORE THAN 8 HEX CHARS IN A ROW
                lsl.l   #4,D4           ; GET READY FOR NEXT CHARACTER
ATH_ADD         add.b   d0,D4           ; ADD TO TOTAL
                bra     ATH_LOOP        ; GO GET ANOTHER CHARACTER
NOTHEX          cmpi.b  #SPACE,d0
                bne     ATH_WS
                cmpi.b  #0,D1           ; read HEX CHAR YET ?
                beq     ATH_LOOP        ; NO SO LEADING SPACE, IGNORE
ATH_ENDNUM      moveq   #0,D7           ; else MARK AS SUCCESSFUL CONVERTION
                subq.l  #1,a0           ; POINT TO END CHAR
                bra     ATH_END
ATH_WS          cmpi.b  #CR,d0
                beq     ATH_CR
                moveq   #3,D7           ; mark AS NON HEX CHARACTER FOUND
                subq.l  #1,a0           ; POINT TO OFFENDING CHARACTER
                bra     ATH_END
ATH_CR          cmpi.b  #0,D1
                bne     ATH_ENDNUM      ; cr TERMINATING NUMBER
                moveq   #1,D7           ; mark AS NO NUMBER FOUND BEFORE EOS
                bra     ATH_END
ATH_TOBIG       moveq   #2,D7           ; mark ERROR AS TOO BIG A NUMBER FOUND
                subq.l  #1,a0           ; BACKSPACE CHARACTER POINTER
ATH_END         move.l  D4,d0           ; copy RESULT
                move.l  (SP)+,D4        ; restore REGS
                RTS
;*
;* DEC_TO_HEX   returns hex number converted from ascii string in decimal form
;*
;* On entry :-
;*              a0.l    points to start of number (will accept leading spaces)
;*
;* On exit  :-
;*              a0.l    points to terminating character after number (if any)
;*              d0.l    converted hex number
;*              d1.b    number of characters converted
;*              d7.b    error code (if any)
;*                              0)      no error
;*                              1)      no number found before EOS
;*                              2)      more than 12 digits found in number
;*                              3)      non digit character found in number
;*
DEC_TO_HEX      movem.l D3/D4,-(SP)     ; save REGS
                clr.l   D4              ; ZERO NUMBER
                clr.b   D1              ; ZERO CHARACTER COUNT
DTH_LOOP        move.b  (a0)+,d0        ; get CHARACTER
                bsr     IS_DIGIT
                tst.b   D7              ; DECIMAL ?
                bne     NOTDEC
                addq.B  #1,D1           ; inc CHARACTER COUNT
                cmpi.b  #11,D1
                beq     DTH_TOBIG       ; MORE THAN 12 DIGITS IN A ROW
DTH_ADD         move.l  D4,D3
                lsl.l   #3,D4           ; D4 * 8
                add.l   D3,D4
                add.l   D3,D4           ; D4 = D4*8 +2*D4
                add.b   d0,D4           ; ADD TO TOTAL
                bra     DTH_LOOP        ; GO GET ANOTHER CHARACTER
NOTDEC          cmpi.b  #SPACE,d0
                bne     DTH_WS
                cmpi.b  #0,D1           ; read HEX CHAR YET ?
                beq     DTH_LOOP        ; NO SO LEADING SPACE, IGNORE
DTH_ENDNUM      moveq   #0,D7           ; else MARK AS SUCCESSFUL CONVERTION
                subq.l  #1,a0           ; POINT TO END CHAR
                bra     DTH_END
DTH_WS          cmpi.b  #CR,d0
                beq     DTH_CR
                moveq   #3,D7           ; mark AS NON HEX CHARACTER FOUND
                subq.l  #1,a0           ; POINT TO OFFENDING CHARACTER
                bra     DTH_END
DTH_CR          cmpi.b  #0,D1
                bne     DTH_ENDNUM      ; cr TERMINATING NUMBER
                moveq   #1,D7           ; mark AS NO NUMBER FOUND BEFORE EOS
                bra     DTH_END
DTH_TOBIG       moveq   #2,D7           ; mark ERROR AS TOO BIG A NUMBER FOUND
                subq.l  #1,a0           ; BACKSPACE CHARACTER POINTER
                bra     DTH_END
DTH_END         move.l  D4,d0           ; copy ANSWER
                movem.l (SP)+,D3/D4     ; restore REGS
                RTS
;*
;*
;* PRINT_HEX    prints hex digits
;*
;* On entry :-
;*              d0.l    hex number to be printed
;*              d6.b    field width to print in (starting from lsb)
;*                      if > 8 print leading spaces (MAX is 15 characters)
;* On exit  :-
;*              d6.b -> D6.L
;*
PRINT_HEX       movem.l d0/D5/D6/a0,-(SP)       ; save REGS
                andi.l  #15,D6          ; MAKE SURE IS 0-15 FOR ADDRESS ADD
                move.l  D6,D5
                move.l  #TempBuf1,a0    ; point TO TEMPORARY STRING
PH_1            cmpi.b  #8,D6           ; too MANY CHARS ?
                ble     PH_2
                move.b  #SPACE,(a0)+    ; put IN LEADING SPACE
                subq.b  #1,D6
                bra     PH_1
PH_2            bsr     HEX_TO_ASC
                move.b  #NULL,(a0)      ; terminate STRING
                suba.l  D5,a0           ; MAKE SURE POINTER IS AT RIGHT PLACE
                bsr     P_STR
PH_RET          movem.l (SP)+,d0/D5/D6/a0       ; restore REGS
                RTS
;*
;*
;*
;* HEX_TO_ASC   converts hex number to ascii (always 8 chars)
;*
;* On entry :-
;*              d0.l    hex number to be converted
;*              a0.l    string to put converted number
;* On exit  :-
;*              d0.l    unchanged
;*              a0.l    points to next location in string after digit
;*
HEX_TO_ASC      movem.l d0/D1,-(SP)     ; save REGS
                moveq   #7,D1           ; number OF CHARS TO ROTATE
PHROT           rol.l   #4,d0           ; MOVE NUMBER ROUND
                move.l  d0,-(SP)        ; save TEMP VALUE
                andi.b  #$0F,d0         ; GET LOWER CHARACTER
                cmpi.b  #09,d0
                bgt     HIGHHEX         ; CHARACTER 'A' - 'F'
                addi.b  #48,d0          ; ELSE MAKE '0' - '9' ASCII
                bra     PHPUT
HIGHHEX         addi.b  #55,d0          ; MAKE A-F ASCII
PHPUT           move.b  d0,(a0)+        ; store CHARACTER IN STRING
                move.l  (SP)+,d0        ; return TEMP VALUE
                dbf     D1,PHROT
                movem.l (SP)+,d0/D1     ; restore REGS
                RTS
;*
;* PRINT_DEC    print signed decimal number in 10 character field
;*
;*      On entry :-
;*                      d0.l    number to be printed
;*      On exit  :-
;*                      No registers destroyed
;*
PRINT_DEC       movem.l D1/a0/A1,-(SP)  ; save REG
                move.l  #TempBuf1,A1    ; point TO AREA TO STORE STRING
                bsr     H_T_D
                move.l  A1,a0
                move.b  #0,0(A1,D1)     ; initialise END OF STRING
                bsr     P_STR
                movem.l (SP)+,D1/a0/A1  ; RESTORE
                RTS
;*
;*  H_T_D  Convert 32 bit signed binary number to ascii string
;*
;*      Called values :-  a1    pointer to 12 byte buffer to
;*                              hold string
;*                        d0    32 bit signed number
;*
;*      Returned values   d0    unchanged
;*                        d1    digits in string
;*                        a1    address of first character in
;*                              string
;*
H_T_D   movem.l d0/D3/D4,-(SP)  ; save TEMP REGS
                moveq   #11-1,D1        ; tempbuf1 LENGTH MINUS dbf
INTBUF          move.b  #' ',(A1)+      ; space INTO BUFFER
                dbf     D1,INTBUF
                moveq   #0,D1           ; clear CHARACTER COUNT
                tst.l   d0              ; IS IT
                bpl     DIV_L           ; NEGATIVE ?
                neg.l   d0              ; MAKE IT POSITIVE FOR NOW
DIV_L           divu    #10,d0          ; STRIP DIGIT FROM RIGHT
;*
                bvs     OVRFL           ; OVERFLOW FLAG SET ?
                move.l  d0,D3           ; d3 -> REMAINDER/XXX
                and.l   #$0000FFFF,d0   ; ERASE REMAINDER
                bra     MKASC           ; SKIP OVERFLOW STUFF
OVRFL           move.w  d0,D3           ; PREPARE FOR DOUBLE DIVISION
                clr.w   d0              ; ZERO LOW WORD
                swap    d0              ; HIGH WORD INTO LOW WORD
                divu    #10,d0          ; DIVIDE HIGH WORD
                move.w  d0,D4           ; SAVE QUOTIENT
                move.w  D3,d0           ; LOW WORD INTO LOW
                divu    #10,d0          ; DIVIDE LOW WORD
                move.l  d0,D3           ; d3 -> REMAINDER
                swap    d0              ; R/Q -> Q/R
                move.w  D4,d0           ; d0 -> L/H
                swap    d0              ; d0 -> H/L
;*
MKASC           swap    D3              ; REMAINDER INTO LOW WORD
                add.b   #'0',D3         ; MAKE ASCII
                move.b  D3,-(A1)        ; fill BUFFER FROM RIGHT
                addq.B  #1,D1           ; inc COUNT
                tst.l   d0              ; GONE TO ZERO ?
                bne     DIV_L           ; no ..
                movem.l (SP)+,d0/D3/D4  ; restore CALL VALUES
                tst.l   d0              ; WAS IT NEGATIVE
                bpl     HDFIN
                move.b  #'-',-(A1)      ; yes SO ADD THE '-'
                addq.B  #1,D1           ; inc COUNT
HDFIN           RTS
;*
;* IS_HEX       checks if character is hex if so then converts to number
;*              and returns convertion status
;* On entry :-
;*              d0.b    character 
;* On exit  :-
;*              d0.b    convertion (if any)
;*              d7.b    status :-
;*                              0)      is hex
;*                              1)      not hex
IS_HEX          cmpi.b  #'0',d0         ; hex CHECKER
                blt     IHN
                cmpi.b  #'F',d0
                bgt     IHN
                cmpi.b  #'9',d0
                ble     IHT
                cmpi.b  #'A',d0
                blt     IHN
IHT             sub.b   #'0',d0
                cmpi.b  #9,d0
                ble     IHNT
                subi.b  #7,d0           ; MAKE 'A'-'F' > 10-15
IHNT            moveq   #0,D7           ; mark AS HEX
                RTS
IHN             moveq   #1,D7           ; mark AS NOT HEX
                RTS
;*
;*
;* IS_DIGIT     checks if character is a digit and converts it if so
;* On entry :-
;*              d0.b    character
;* On exit  :-
;*              d0.b    convertion (if any)
;*              d7.b    status :-
;*                              0)      is a digit
;*                              1)      not a digit
IS_DIGIT        cmpi.b  #'0',d0
                blt     IDN
                cmpi.b  #'9',d0
                bgt     IDN
                subi.b  #'0',d0         ; MAKE DIGIT A NUMBER
                moveq   #0,D7           ; mark AS A DIGIT
                RTS
IDN             moveq   #1,D7           ; not SO
                RTS
;*
;*
;* IS_LOWER     checks if character is lowercase returns status
;* On exit  :-
;*              d7.b    status :-
;*                              0)      is lowercase
;*                              1)      not lowercase
IS_LOWER        cmpi.b  #'a',d0
                blt     ILN
                cmpi.b  #'z',d0
                bgt     ILN
                moveq   #0,D7           ; mark AS LOWERCASE
                RTS
ILN             moveq   #1,D7           ; not LOWERCASE
                RTS
;*
;*
;* IS_UPPER     checks if character is uppercase returns status
;* On exit  :-
;*              d7.b    status :-
;*                              0)      is uppercase
;*                              1)      not uppercase
IS_UPPER        cmpi.b  #'A',d0
                blt     IUN
                cmpi.b  #'Z',d0
                bgt     IUN
                moveq   #0,D7           ; mark AS UPPERCASE
                RTS
IUN             moveq   #1,D7           ; not UPPERCASE
                RTS
;*
;*
;* IS_ALPHA     checks if character is alphabetic
;* On exit  :-
;*              d7.b    status :-
;*                              0)      is alphabetic
;*                              1)      not alphabetic
IS_ALPHA        bsr     IS_LOWER
                tst.b   D7
                beq     IAT
                bsr     IS_UPPER
IAT             rts                     ; RETURN STATUS IN D7
;*
;*
;* IS_ALNUM     checks if character is alphanumeric
;* On exit  :-
;*              d7.b    status :-
;*                              0)      is alpanumeric
;*                              1)      not alphanumeric
IS_ALNUM        bsr     IS_DIGIT
                tst.b   D7
                beq     IALT
                bsr     IS_ALPHA
IALT            RTS
;*
;*
;* TO_UPPER     converts lowercase to uppercase if lowercase otherwise leaves
;*              character unchanged
;* On entry :-
;*              d0.b    character to be converted
;* On exit  :-
;*              d0.b    uppercase version of character (if appropriate)
;*              d7.b    status :-
;*                              0)      was converted to uppercase
;*                              1)      was not a lowercase character to start
TO_UPPER        bsr     IS_LOWER
                tst.b   D7              ; LOWERCASE ?
                bne     TUR
                subi.b  #'a'-'A',d0     ; YES SO CHANGE
TUR             RTS
;*
;*
;*
;*
;* crlf outputs carriage return line feed pair to console
;*
CRLF            move.l  d0,-(SP)        ; save REGS
                move.b  #CR,d0
                bsr     PUTCH
                move.b  #LF,d0          ; output CR,LF
                bsr     PUTCH
                move.l  (SP)+,d0        ; restore REGS
                RTS
;*
;*
;* putch :- Output character to serial port 1
;*      On entry :-
;*                      d0.b    character to be printed
;*      On exit  :-     
;*                      no registers changed
;*
PUTCH           move.l  a1,-(sp)
                move.l  CurrentOut,a1
pcPoll          btst.b  #3,sr_off(a1)
                beq     pcPoll
                move.b  d0,thr_off(a1)
                move.l  (sp)+,a1
                rts

;*
;*
;* getch :- Gets a character from serial port 1 (no error check)
;*      On exit  :-
;*                      d0.b    character top bit stripped
;*
GetCh		move.l  a1,-(sp)
                move.l  CurrentIn,a1
gcPoll          btst.b  #0,sr_off(a1)
                beq     gcPoll
                move.b  rhr_off(a1),d0
                and.b   #$7f,d0
                move.l  (sp)+,a1
                rts

;*
;*
COMTABLE        dc.b    0,0,0,'M'
                dc.l    MOVECMD
                dc.b    0,0,0,'F'
                dc.l    FILL
                dc.b    0,0,0,'D'
                dc.l    DUMP
                dc.b    0,0,0,'E'
                dc.l    CHANGE
                dc.b    0,0,0,'X'
                dc.l    XAM
;                dc.b    0,0,0,'B'
;                dc.l    BREAK
                dc.b    0,0,0,'G'
                dc.l    GO
                dc.b    0,0,0,'J'
                dc.l    JUMP
                dc.b    0,0,0,'T'
                dc.l    TRC
                dc.b    0,0,0,'R'
                dc.l    READ
		dc.b	0,0,0,'I'
		dc.l	INITSTORE
		dc.b	0,0,0,'P'
		dc.l	DOROM
		dc.b	0,0,0,'B'
		dc.l	DOBOOT
		dc.b	0,0,0,'C'
		dc.l	DOCALLC
		dc.b	0,0,0,'A'
		dc.l	BPadd
		dc.b	0,0,0,'K'
		dc.l	BPremove
		dc.b	0,0,0,'L'
		dc.l	BPlist
                dc.b    0,0,0,'H'
                dc.l    HELP
                dc.l    NULL                    ; END OF TABLE
;*
EXTAB           dc.l    BUSERR
                dc.l    ADERR
                dc.l    ILL
                dc.l    DIV
                dc.l    CHKX
                dc.l    OVL
                dc.l    PRV
                dc.l    TRACE
                dc.l    EMU1
                dc.l    EMU2
;*
EXSTR           dc.l    S2,S3,S4,S5,S6,S7,S8,S9,S10,S11,S12,S13,S14,S15,S16
;*
RS1             dc.l    R1,R2,R3
;*
EXF             dc.l    P_STR
                dc.l    G_STR 
                dc.l    PUTCH
                dc.l    GetCh
                dc.l    EVAL
                dc.l    GET_NUMS
                dc.l    TOUP_STR
                dc.l    ASC_TO_HEX
                dc.l    DEC_TO_HEX
                dc.l    PRINT_HEX
                dc.l    HEX_TO_ASC
                dc.l    PRINT_DEC
                dc.l    H_T_D
                dc.l    IS_HEX
                dc.l    IS_DIGIT
                dc.l    IS_LOWER
                dc.l    IS_UPPER
                dc.l    IS_ALPHA
                dc.l    IS_ALNUM
                dc.l    TO_UPPER
                dc.l    CRLF
                dc.l    DUMP
;*
;*
REGSTR          dc.b    "PCUSSSSRd0D1D2D3D4D5D6D7a0A1A2A3A4A5A6A7"
srflagstr       dc.b    "TSXNZVC!"
;*
SEP             dc.b    " | ",NULL
EQUAL           dc.b    " = ",NULL
PROM            dc.b    " -> ",NULL
SPSTR           dc.b    "   ",NULL
PROMPT          dc.b    CR,LF,"Command -> ",NULL
ERRMESS         dc.b    "Command not recognised (H for help)",CR,LF,NULL
FEMESS          dc.b    "Illegal fill value, must be (0-FF)",CR,LF,NULL
ERRCR           dc.b    "NO change | location in ROM?",CR,LF,NULL
ERRTB           dc.b    "Value too big",CR,LF,NULL
ERR             dc.b    CR,LF,"      error !",CR,LF,NULL
ERROA           dc.b    "ODD address !",CR,LF,NULL
ERRTF           dc.b    "Breakpoint table full!",CR,LF,NULL
ERRBP           dc.b    "Error - breakpoint already in table!",CR,LF,NULL
ERRBN           dc.b    "Error - breakpoint not in table!",CR,LF,NULL
ERRTE           dc.b    "Breakpoint table empty!",CR,LF,NULL
ERRIV           dc.b    "Illegal value !",CR,LF,NULL
ERRNBR          dc.b    "Breakpoint trap encountered at loc with no breakpoint!",CR,LF,NULL
ERRIF           dc.b    "Illegal function call value passed from loc. -> ",NULL
;*
S2              dc.b    "Bus error",CR,LF,NULL
S3              dc.b    "Address error",CR,LF,NULL
S4              dc.b    "Illegal InStruction",CR,LF,NULL
S5              dc.b    "Division by 0",CR,LF,NULL
S6              dc.b    "CHK trap",CR,LF,NULL
S7              dc.b    "Overflow trap",CR,LF,NULL
S8              dc.b    "Privilage violation trap",CR,LF,NULL
S9              dc.b    "Trace",CR,LF,NULL
S10             dc.b    "Emulation 1010 trap",CR,LF,NULL
S11             dc.b    "Emulation 1111 trap",CR,LF,NULL
S12             dc.b    "Motorola reserved trap <12-23/48-63>",CR,LF,NULL
S13             dc.b    "Spurious interrupt",CR,LF,NULL
S14             dc.b    "Uninitialised Autovectored interrupt",CR,LF,NULL
S15             dc.b    "Uninitialised trap InStruction",CR,LF,NULL
S16             dc.b    "Uninitialised Vectored interrupt <64-255>",CR,LF,NULL
;*
R1              dc.b    "ERROR - Illegal data encountered in record",CR,LF,NULL
R2              dc.b    "ERROR - Record type not supported",CR,LF,NULL
R3              dc.b    "ERROR - Checksum error in file",CR,LF,NULL
;*
;*
MONSTR          dc.b    CR,LF,"FEP Monitor Ver 1.05, 26th June 1990",CR,LF,NULL
MSTR            dc.b    CR,"MOVE - Source, Length (bytes), Destination -> ",NULL
FSTR            dc.b    CR,"FILL - Start, Length (bytes), Fill value (0-FF) -> ",NULL
CSTR            dc.b    CR,"Calling main()",CR,LF,NULL
DSTR            dc.b    CR,"DUMP - <start> <length (bytes)> -> ",NULL
ESTR		dc.b	CR,"CHANGE - Start address -> ",NULL
XSTR            dc.b    CR,"EXamine | change <reg> <value> -> ",NULL
GSTR            dc.b    CR,"GO - <,> <address>  -> ",NULL
JSTR            dc.b    CR,"JUMP to subroutine <address>  -> ",NULL
TSTR            dc.b    CR,"TRACE - <count>  -> ",NULL
RSTR            dc.b    CR,"READ S-records from tty port - waiting",CR,LF,NULL
ISTR		dc.b	CR,"Initialising store ",NULL
PSTR		dc.b	CR,"ROM - <base address> -> ",NULL
BOOTSTR		dc.b	CR,"Boot - <device name> -> ",NULL
RDSTR           dc.b    "Register dump ->",CR,LF,NULL
BRSTR           dc.b    "Breakpoint address  -> ",NULL
ACSTR           dc.b    "Access type           -> ",NULL
CCSTR           dc.b    "Current cycle address -> ",NULL
IRSTR           dc.b    "InStruction register  -> ",NULL
SRSTR           dc.b    "Status register       -> ",NULL
PCSTR           dc.b    "Program counter was   -> ",NULL
BPSTR           dc.b    CR,LF,"Pass count -> ",NULL
BTSTR           dc.b    CR,LF,"Breakpoint type  O)rdinary, M)ark, P)ass  -> ",NULL
ORSTR           dc.b    CR,LF,"Ordinary breakpoints -> ",NULL
PASTR           dc.b    CR,LF,"Pass breakpoint      -> ",NULL
MRSTR           dc.b    CR,LF,"Mark breakpoint      -> ",NULL
CNSTR           dc.b    "  Count  -> ",NULL
HALFSTR		dc.b	".5",NULL
MBSTR		dc.b	" MBytes of memory found starting at $",NULL
NSSTR		dc.b	CR,LF,"Top of store has not been set!",CR,LF,NULL

HSTR            dc.b    CR,LF,"Command available:",CR,LF,CR,LF
		dc.b	" A) - Add breakpoint    "
		dc.b	" B) - Boot from disk    "
		dc.b	" C) - Call c program    "
		dc.b	CR,LF
		dc.b	" D) - Dump memory       "
		dc.b	" E) - Enter in memory   "
		dc.b	" F) - Fill memory       "
		dc.b	CR,LF
		dc.b	" G) - go to a routine   "
		dc.b	" I) - Initialise store  "
		dc.b	" J) - Jump to subroutine"
		dc.b	CR,LF
		dc.b	" K) - Remove.breakpoint "
		dc.b	" L) - List breakpoints  "
		dc.b	" M) - Move memory       "
		dc.b	CR,LF
		dc.b	" P) - Upload roms       "
		dc.b	" R) - Download program  "
		dc.b	" T) - Trace InStructions"
		dc.b	CR,LF
		dc.b	" X) - Examine registers "
		dc.b	CR,LF,CR,LF,NULL

RomDo		dc.b	CR,LF,"Press any key to send ",NULL
LB0		dc.b	"LB0",NULL
UB0		dc.b	"UB0",NULL
LB1		dc.b	"LB1",NULL
UB1		dc.b	"UB1",NULL
RomDoing	dc.b	CR,LF,NULL

Flicks		dc.b	"/-\|",NULL

MonEnd          
