// sbc3_glue.v - glue logic for the Mega-Micros 68030 SBC3 (rev 1.0),
// for the Altera EPM7128SLC84 (IC3).
//
// Written from the SBC-3B3 schematic (22/01/2025).  It is a replacement
// for the original CPLD design, which isn't available, so treat it as a
// starting point: the PARAMETERS section below collects every timing and
// memory-map choice, and the notes in README.md list what to check.
//
// What it does
//   * Address decoding (A31-A24 must be 0 unless FULL_DECODE = 0):
//       RAM: two 32-bit banks of four byte-wide SRAMs (RAM_MB):
//         2 MB banks:    bank 0 $000000-$1FFFFF, bank 1 $200000-$3FFFFF
//         8 MB banks:    bank 0 $000000-$7FFFFF, bank 1 $10000000-$107FFFFF
//       $800000-$BFFFFF  I/O: 68000-style cycle on the backplane, ended
//                        by /DTACK (or /VPA for a 6800-style cycle);
//                        $AD0000-$ADFFFF also selects the on-board 68681,
//                        with DUART_RECOVERY clocks between accesses
//       $C00000-$FFFFFF  ROM: 27C322, 16-bit port on D16-D31
//     Until the first access to ROM space after reset, reads from RAM
//     space come from the ROM, so the reset vectors are fetched from it.
//   * Dynamic bus sizing: DSACK0+DSACK1 for RAM (32-bit), DSACK1 for ROM
//     and all I/O (16-bit, D16-D31).
//   * Wait states for RAM and ROM, counted in CLK2 cycles.
//   * Bus error on: no /DTACK within BERR_TIMEOUT clocks, CPU-space
//     cycles other than interrupt acknowledge (so FPU instructions take
//     the F-line trap, as there is no FPU), and unused address space.
//   * Interrupts: /IRQ7-/IRQ1 priority-encoded onto /IPL2-/IPL0.
//     Interrupt acknowledge cycles run on the backplane like I/O cycles;
//     level 1 also asserts /B24 (PI/T PC7/TIACK) and level 2 /B25 (68681
//     /IACK).  The 68030's /AVEC is not connected to the CPLD, so there
//     are no autovectors: a level with no vectoring device bus-errors and
//     the CPU takes the spurious interrupt exception.
//   * 6800-style peripherals: E (CLK2 / E_DIV) and /VMA, used when a
//     device answers an I/O cycle with /VPA instead of /DTACK.
//   * /UDS and /LDS for the 16-bit backplane bus.
//
// Clock: CLK2 is assumed to be the 40 MHz CPU clock (one of the MC10H640
// outputs, in phase with the 68030's CLK).  All registers use its rising
// edge; the CPU's /AS is sampled there.  Strobes and acknowledges are
// gated with /AS, so they all go away as soon as the CPU ends the cycle.

module sbc3_glue #(
    // ---------------------------------------------------------------
    // PARAMETERS
    // ---------------------------------------------------------------
    // Extra CLK2 cycles before DSACK for RAM and ROM.  0 gives the
    // 68030's minimum 3-clock cycle.  At 40 MHz: 55 ns SRAM needs 1,
    // a 100 ns 27C322 needs 3.
    parameter RAM_WAIT     = 1,
    parameter ROM_WAIT     = 5,
    // RAM bank size in MB: 2 or 8.
    //   2: 512Kx8 SRAMs (AS6C4008, as drawn).  Bank 0 $000000-$1FFFFF,
    //      bank 1 $200000-$3FFFFF; $400000-$7FFFFF bus-errors.
    //   8: 2Mx8 SRAMs (AS6C16008).  Bank 0 $000000-$7FFFFF, bank 1
    //      $10000000-$107FFFFF; $10800000-$10FFFFFF bus-errors.
    parameter RAM_MB       = 2,
    // 1: only $00xxxxxx (and $10xxxxxx for 8 MB bank 1) is decoded; any
    //    other A31-A24 bus-errors.
    // 0: A31-A24 are ignored, so the 16 MB map repeats (as in MAME),
    //    except at $10xxxxxx when that holds 8 MB bank 1.
    parameter FULL_DECODE  = 1,
    // Minimum CLK2 cycles between the end of one 68681 access (/68681_CE
    // negated) and the start of the next.  The 68681 needs three edges
    // of its X1 clock between consecutive writes to the same command
    // register; at 7.3728 MHz that's about 410 ns, so 20 clocks (500 ns)
    // at 40 MHz.  Without it, back-to-back CR writes (reset TX, enable TX
    // ...) are lost and the port never transmits.
    // Counted down from this value, so any value up to 63 costs the same
    // logic.
    parameter DUART_RECOVERY = 20,
    // Minimum CLK2 cycles /68681_CE is asserted before the 68681's /DTACK
    // is believed (up to 15).  The 68681 has read data valid up to 175 ns
    // after /CS, so 8 clocks (200 ns) also guards against another device
    // pulling /DTACK low early.
    parameter DUART_SETUP = 8,
    // 1: end 68681 cycles with its /DTACK (through J1).  0: ignore /DTACK
    // and end them after DUART_ACCESS clocks of /68681_CE (up to 15;
    // 12 = 300 ns), for testing whether /DTACK is the problem.
    parameter DUART_DTACK = 1,
    parameter DUART_ACCESS = 12,
    // What pin 4 (pads P4 / H1) shows, for debugging:
    //   0: goes low and stays low after a 68681 access or interrupt
    //      acknowledge ends in a bus error
    //   1: low during each 68681 read (a copy of /68681_CE for reads)
    //   2: low during each 68681 write
    //   3: toggles at the end of each 68681 read: a steady 0 V or 5 V
    //      means no reads; about 2.5 V on a meter means it's polling
    //   4: goes low and stays low after any bus error at all
    //   5: goes low and stays low after any interrupt acknowledge
    //   6: goes low and stays low after an I/O read outside the 68681,
    //      CF, PI/T and Hex Display ($AD, $AE, $AF, $AB) is answered
    //      with /DTACK, i.e. a board the monitor probes for "exists"
    parameter DEBUG_MODE = 0,
    // Clocks to wait for /DTACK or /VPA before asserting /BERR (max 255).
    parameter BERR_TIMEOUT = 255,
    // Earliest clock (counted from the first clock edge of the cycle) at
    // which /DTACK is believed.  Lets the previous cycle's /DTACK, pulled
    // up by R1 (1K2), finish rising first.
    parameter DTACK_MIN    = 2,
    // E = CLK2 / E_DIV, low for 60% and high for 40% of the period, like
    // the 68000's.  40 gives 1 MHz at 40 MHz (6800-family parts are 1-2
    // MHz).  Must be at least 10.
    parameter E_DIV        = 40,
    // 1: a device answering an I/O cycle with /VPA gets a 6800-style
    // E/VMA cycle.  0: /VPA is ignored.  /VPA has no pull-up on the SBC3
    // (only /DTACK has one, R1), so with no backplane card driving it the
    // CPLD input floats, usually low.  That made every I/O cycle look
    // answered, so the monitor's probes for absent boards "found" them.
    // Only set this to 1 with a 6800-style card fitted, or a pull-up
    // (e.g. 4K7) added to /VPA.
    parameter USE_VPA      = 0
) (
    input  wire        clk2,       // 83  CLK2 (40 MHz)
    input  wire        reset_n,    // 28  /RESET
    input  wire        as1_n,      // 61  /AS1  (68030 /AS)
    input  wire        rw3,        // 31  R/W3  (1 = read)
    input  wire [1:0]  siz,        // 58 SIZ1, 57 SIZ0
    input  wire [2:0]  fc,         // 65 FC2, 67 FC1, 68 FC0
    input  wire [31:16] a_hi,      // A31-A16
    input  wire [3:0]  a_lo,       // A3-A0
    input  wire        sel_n,      // 1   /SEL  (jumper J6; spare, unused)
    input  wire [7:1]  irq_n,      // /IRQ7-/IRQ1
    input  wire        dtack_n,    // 34  /DTACK (backplane)
    input  wire        vpa_n,      // 84  /VPA   (backplane)

    output wire        ds1_n,      // 63  /DS1 -> J3 -> /DSACK1
    output wire        ds0_n,      // 64  /DS0 -> J8 -> /DSACK0
    output wire        be_n,       // 60  /BE  -> J2 -> /BERR (see below)
    output wire [2:0]  ipl_n,      // 17 /IPL2, 18 /IPL1, 16 /IPL0
    output wire        uds_n,      // 56  /UDS (also /UDS2 to IC10)
    output wire        lds_n,      // 55  /LDS
    output wire        e,          // 52  E (held low unless USE_VPA = 1)
    output wire        vma_n,      // 54  /VMA
    output wire        ram0_oe_n,  // 33
    output wire [3:0]  ram0_we_n,  // 36 WE3, 35 WE2, 37 WE1, 40 WE0
    output wire        ram1_oe_n,  // 29
    output wire [3:0]  ram1_we_n,  // 25 WE3, 27 WE2, 24 WE1, 22 WE0
    output wire        rom_ce_n,   // 20  /ROM_CE (27C322 /G)
    output wire        duart_ce_n, // 30  /68681_CE
    output wire        b24_n,      // 41  /B24: level 1 IACK (PI/T TIACK)
    output wire        b25_n,      // 39  /B25: level 2 IACK (68681 /IACK)
    // Debug (pads P4 / H1): see DEBUG_MODE.
    output wire        dbg_n       // 4
);

    // ---------------------------------------------------------------
    // Address decode (inputs are stable while /AS is asserted)
    // ---------------------------------------------------------------
    wire        rd       = rw3;
    wire        cpuspace = (fc == 3'b111);
    wire        iack     = cpuspace && (a_hi[19:16] == 4'hF);
    wire [2:0]  level    = a_lo[3:1];
    localparam  BIG      = (RAM_MB == 8);
    // $10xxxxxx: 8 MB bank 1 (A23 must be 0).
    wire        hi_bank1 = BIG && (a_hi[31:24] == 8'h10);
    wire        hi_ok    = FULL_DECODE ? (a_hi[31:24] == 8'h00) : !hi_bank1;
    wire [1:0]  region   = a_hi[23:22];      // 0,1 RAM  2 I/O  3 ROM

    reg         overlay;                     // ROM at 0 after reset
    reg         dbg_berr_n;

    wire ram_space = hi_ok && !cpuspace && !region[1];
    wire bank1_8mb = hi_bank1 && !cpuspace && !a_hi[23];
    wire ram_hole  = BIG ? 1'b0 : a_hi[22];
    wire ram_bank  = BIG ? bank1_8mb : a_hi[21];

    wire t_rom   = hi_ok && !cpuspace && ((region == 2'b11) ||
                                          (overlay && !region[1] && rd));
    wire t_ram   = ((ram_space && !ram_hole) || bank1_8mb) && !t_rom;
    wire t_duart = hi_ok && !cpuspace && (region == 2'b10) &&
                   (a_hi[21:16] == 6'h2D);          // $AD0000-$ADFFFF
    wire t_bus   = (hi_ok && !cpuspace && (region == 2'b10)) || iack;
    // Anything else - unused space, the RAM hole, CPU space other than
    // IACK (coprocessor, breakpoint, MMU) - ends in a bus error.
    wire t_none  = !t_rom && !t_ram && !t_bus;

    // ---------------------------------------------------------------
    // Byte lanes
    // ---------------------------------------------------------------
    // 32-bit port: lane 3 = D31-D24 (WE3) ... lane 0 = D7-D0 (WE0).
    // The transfer starts at byte A1:A0 and is SIZ bytes long
    // (SIZ 00 = 4), clipped at the end of the long word.
    reg [3:0] lane;
    always @* begin : lanes
        integer i, start, len;
        start = a_lo[1:0];
        len   = (siz == 2'b00) ? 4 : siz;
        for (i = 0; i < 4; i = i + 1)
            lane[3 - i] = (i >= start) && (i < start + len);
    end
    // 16-bit port on D31-D16: /UDS = D31-D24 (even byte), /LDS = D23-D16.
    wire uds_en = !a_lo[0];
    wire lds_en =  a_lo[0] || (siz != 2'b01);

    // ---------------------------------------------------------------
    // Bus cycle sequencer
    // ---------------------------------------------------------------
    reg        active;       // /AS has been seen at a clock edge
    reg  [7:0] cnt;          // clock edges of this cycle so far (saturating):
                             // 0 at the first edge with /AS asserted
    reg        ack32, ack16; // terminate: 32-bit / 16-bit port
    reg        berr;
    reg        dtack_s, vpa_s;
    reg        vma;
    reg  [5:0] duart_rec;    // recovery left before /68681_CE may assert
    reg  [3:0] duart_set;    // setup left before the 68681's /DTACK counts
    reg        duart_go;     // /68681_CE asserted for this cycle

    // n: which clock edge of the cycle this is (0 = the first).  It's just
    // cnt, which is cleared while /AS is negated; the name is kept for the
    // conditions below.  (Comparing the register itself, not cnt + 1,
    // keeps the logic small enough for the EPM7128S.)
    wire [7:0] n = cnt;

    // E clock: free-running divider.  ecnt 0 .. E_DIV-1; E is low for the
    // first 60% of the count.
    // Only built with USE_VPA = 1; otherwise E stays low and the divider
    // costs nothing.
    localparam E_LOW = (E_DIV * 6) / 10;
    reg [5:0] ecnt;
    reg       e_r;
    assign e = USE_VPA ? e_r : 1'b0;
    always @(posedge clk2 or negedge reset_n)
        if (!reset_n) begin
            ecnt <= 6'd0;
            e_r  <= 1'b0;
        end else if (USE_VPA) begin
            ecnt <= (ecnt == E_DIV - 1) ? 6'd0 : ecnt + 6'd1;
            e_r  <= (ecnt >= E_LOW - 1) && (ecnt != E_DIV - 1);
        end
    // With e registered, E is high while ecnt is E_LOW .. E_DIV-1,
    // and falls on the edge where ecnt wraps to 0.

    always @(posedge clk2 or negedge reset_n)
        if (!reset_n) begin
            overlay <= 1'b1;
            dbg_berr_n <= 1'b1;
            active  <= 1'b0;
            cnt     <= 8'd0;
            ack32   <= 1'b0;
            ack16   <= 1'b0;
            berr    <= 1'b0;
            dtack_s <= 1'b0;
            vpa_s   <= 1'b0;
            vma     <= 1'b0;
            duart_rec <= 6'd0;
            duart_set <= DUART_DTACK ? DUART_SETUP : DUART_ACCESS;
            duart_go  <= 1'b0;
        end else begin
            dtack_s <= !dtack_n;

            // 68681 recovery and setup, as down-counters: while /68681_CE
            // is asserted, reload the recovery count and count down the
            // setup; while it's negated, the reverse.
            if (!duart_ce_n) begin
                duart_rec <= DUART_RECOVERY;
                if (duart_set != 4'd0)
                    duart_set <= duart_set - 4'd1;
            end else begin
                duart_set <= DUART_DTACK ? DUART_SETUP : DUART_ACCESS;
                if (duart_rec != 6'd0)
                    duart_rec <= duart_rec - 6'd1;
            end
            if (as1_n || !t_duart)
                duart_go <= 1'b0;
            else if (active && duart_rec == 6'd0)
                duart_go <= 1'b1;
            vpa_s   <= USE_VPA ? !vpa_n : 1'b0;

            if (as1_n) begin
                active <= 1'b0;
                cnt    <= 8'd0;
                ack32  <= 1'b0;
                ack16  <= 1'b0;
                berr   <= 1'b0;
                vma    <= 1'b0;
            end else begin
                active <= 1'b1;
                if (cnt != 8'hFF)
                    cnt <= cnt + 8'd1;

                // The first ROM-space access ends the reset overlay.
                if (hi_ok && !cpuspace && region == 2'b11)
                    overlay <= 1'b0;

                if (t_none)
                    berr <= 1'b1;
                else if (t_ram)
                    ack32 <= (n >= RAM_WAIT);
                else if (t_rom)
                    ack16 <= (n >= ROM_WAIT);
                else begin
                    // I/O or interrupt acknowledge, on the backplane.
                    if (vma) begin
                        // 6800-style cycle: end a read 2 clocks before E
                        // falls; end a write as it falls, so the data is
                        // still driven when the peripheral latches it.
                        if (rd ? (ecnt == E_DIV - 3) : (ecnt == E_DIV - 1))
                            ack16 <= 1'b1;
                    end else if (t_duart ? (duart_go && duart_set == 4'd0 &&
                                            (dtack_s || !DUART_DTACK))
                                         : (dtack_s && n >= DTACK_MIN))
                        ack16 <= 1'b1;
                    else if (vpa_s && n >= DTACK_MIN && !iack) begin
                        // Start VMA with the next E low phase.
                        if (ecnt == E_DIV - 1)
                            vma <= 1'b1;
                    end

                    if (n >= BERR_TIMEOUT && !ack16 && !vma) begin
                        berr <= 1'b1;
                        if (t_duart || iack)
                            dbg_berr_n <= 1'b0;
                    end
                    // /VPA on an IACK would mean an autovector, which this
                    // board can't do (/AVEC isn't driven): bus error.
                    if (iack && vpa_s && n >= DTACK_MIN)
                        berr <= 1'b1;
                end
            end
        end

    // ---------------------------------------------------------------
    // Outputs (all gated with /AS)
    // ---------------------------------------------------------------
    wire cyc = !as1_n && active;      // from the first clock edge

    assign ds1_n = !(!as1_n && (ack16 || ack32));
    assign ds0_n = !(!as1_n && ack32);
    // /BERR is shared with the backplane, so it's only driven while
    // needed: low during the bus error, then actively high for one to two
    // clocks after /AS negates, then released.  The only pull-up is 10K
    // (RP2), which would take about half a microsecond to bring the line
    // back up.  Meanwhile the 68030 would see /BERR still asserted on the
    // first cycles of its bus error exception, take a double bus fault
    // and halt.
    reg berr_rel;
    always @(posedge clk2 or negedge reset_n)
        if (!reset_n) berr_rel <= 1'b0;
        else          berr_rel <= berr && as1_n;
    assign be_n = (!as1_n && berr)              ? 1'b0 :
                  ((as1_n && berr) || berr_rel) ? 1'b1 : 1'bz;

    wire ram0 = cyc && t_ram && !ram_bank;
    wire ram1 = cyc && t_ram &&  ram_bank;
    assign ram0_oe_n = !(ram0 && rd);
    assign ram1_oe_n = !(ram1 && rd);
    assign ram0_we_n = ~({4{ram0 && !rd}} & lane);
    assign ram1_we_n = ~({4{ram1 && !rd}} & lane);
    assign rom_ce_n  = !(cyc && t_rom && rd);

    wire busy = cyc && t_bus;
    assign duart_ce_n = !(busy && t_duart && duart_go);
    // No backplane strobes for the on-board 68681: it doesn't use them,
    // and a card that half-decodes the address mustn't see a cycle.
    assign uds_n = !(busy && !t_duart && uds_en);
    assign lds_n = !(busy && !t_duart && lds_en);
    assign vma_n = !(vma && !as1_n);
    assign b24_n = !(busy && iack && level == 3'd1);
    assign b25_n = !(busy && iack && level == 3'd2);

    // Debug pin.
    reg dbg_any_berr_n, dbg_iack_n, dbg_probe_n;
    always @(posedge clk2 or negedge reset_n)
        if (!reset_n) begin
            dbg_any_berr_n <= 1'b1;
            dbg_iack_n     <= 1'b1;
            dbg_probe_n    <= 1'b1;
        end else begin
            if (!as1_n && berr)
                dbg_any_berr_n <= 1'b0;
            if (cyc && iack)
                dbg_iack_n <= 1'b0;
            if (!as1_n && ack16 && rd && t_bus && !iack && !t_duart &&
                a_hi[23:16] != 8'hAB && a_hi[23:16] != 8'hAE &&
                a_hi[23:16] != 8'hAF)
                dbg_probe_n <= 1'b0;
        end
    wire rd_end = ack16 && t_duart && rd && !as1_n;
    reg  rd_end_q, dbg_tog;
    always @(posedge clk2 or negedge reset_n)
        if (!reset_n) begin
            rd_end_q <= 1'b0;
            dbg_tog  <= 1'b0;
        end else begin
            rd_end_q <= rd_end;
            if (rd_end && !rd_end_q)
                dbg_tog <= !dbg_tog;
        end
    assign dbg_n = (DEBUG_MODE == 1) ? !(!duart_ce_n &&  rd) :
                   (DEBUG_MODE == 2) ? !(!duart_ce_n && !rd) :
                   (DEBUG_MODE == 3) ? dbg_tog :
                   (DEBUG_MODE == 4) ? dbg_any_berr_n :
                   (DEBUG_MODE == 5) ? dbg_iack_n :
                   (DEBUG_MODE == 6) ? dbg_probe_n :
                                       dbg_berr_n;

    // Interrupt priority encoder.
    reg [2:0] ipl;
    always @* begin
        casez (irq_n)
            7'b0??????: ipl = 3'd7;
            7'b10?????: ipl = 3'd6;
            7'b110????: ipl = 3'd5;
            7'b1110???: ipl = 3'd4;
            7'b11110??: ipl = 3'd3;
            7'b111110?: ipl = 3'd2;
            7'b1111110: ipl = 3'd1;
            default:    ipl = 3'd0;
        endcase
    end
    assign ipl_n = ~ipl;

endmodule
