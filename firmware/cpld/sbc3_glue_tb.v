// sbc3_glue_tb.v - Icarus Verilog testbench for sbc3_glue.v
//   iverilog -g2012 -o tb sbc3_glue_tb.v sbc3_glue.v && vvp tb
// Runs 68030-style bus cycles against the glue logic and checks the
// chip selects, strobes, termination and wait states.
`timescale 1ns/1ps
module sbc3_glue_tb;
    reg         clk2 = 0;
    always #12.5 clk2 = ~clk2;               // 40 MHz

    reg         reset_n = 0;
    reg         as1_n = 1, rw3 = 1;
    reg  [1:0]  siz = 0;
    reg  [2:0]  fc = 3'b101;
    reg  [31:0] addr = 0;
    reg  [7:1]  irq_n = 7'h7f;
    reg         sel_n = 1;
    wire        dtack_n, vpa_n;
    wire        ds1_n, ds0_n, be_n, uds_n, lds_n, e, vma_n;
    wire [2:0]  ipl_n;
    wire        ram0_oe_n, ram1_oe_n, rom_ce_n, duart_ce_n, b24_n, b25_n;
    wire [3:0]  ram0_we_n, ram1_we_n;

    pullup (be_n);

    sbc3_glue dut (
        .clk2(clk2), .reset_n(reset_n), .as1_n(as1_n), .rw3(rw3),
        .siz(siz), .fc(fc), .a_hi(addr[31:16]), .a_lo(addr[3:0]),
        .sel_n(sel_n), .irq_n(irq_n), .dtack_n(dtack_n), .vpa_n(vpa_n),
        .ds1_n(ds1_n), .ds0_n(ds0_n), .be_n(be_n), .ipl_n(ipl_n),
        .uds_n(uds_n), .lds_n(lds_n), .e(e), .vma_n(vma_n),
        .ram0_oe_n(ram0_oe_n), .ram0_we_n(ram0_we_n),
        .ram1_oe_n(ram1_oe_n), .ram1_we_n(ram1_we_n),
        .rom_ce_n(rom_ce_n), .duart_ce_n(duart_ce_n),
        .b24_n(b24_n), .b25_n(b25_n));

    // ---- backplane devices ------------------------------------------
    // 68681: /DTACK 3 clocks after /68681_CE.  "Hex display" at $AB0000:
    // /DTACK 2 clocks after /UDS or /LDS.  PI/T: answers /B24.  A 6800
    // device at $A00000 answers with /VPA.
    reg dt_duart = 1, dt_card = 1, dt_pit = 1, vpa_dev = 1;
    assign dtack_n = dt_duart & dt_card & dt_pit;
    assign vpa_n   = vpa_dev;
    always @(posedge clk2) begin
        dt_duart <= duart_ce_n ? 1'b1 : (dut_ce_clks >= 3 ? 1'b0 : 1'b1);
        dt_card  <= (!(uds_n && lds_n) && addr[23:16] == 8'hAB) ? 1'b0 : 1'b1;
        dt_pit   <= b24_n;
        vpa_dev  <= (!(uds_n && lds_n) && addr[23:16] == 8'hA0) ? 1'b0 : 1'b1;
    end
    integer dut_ce_clks = 0;
    always @(posedge clk2) dut_ce_clks <= duart_ce_n ? 0 : dut_ce_clks + 1;

    // ---- 68030 bus cycle ---------------------------------------------
    // S0 at a rising edge; /AS asserted at the next falling edge (S1);
    // /DSACKx and /BERR sampled at falling edges ("clocks" = falling
    // edges up to termination: 1 + wait states); /AS negated half a
    // clock after termination is seen.  Records what the glue did.
    integer errors = 0;
    integer clocks;
    reg [3:0] we_seen0, we_seen1;
    reg oe0_seen, oe1_seen, rom_seen, duart_seen, uds_seen, lds_seen;
    reg b24_seen, b25_seen, vma_seen, e_at_end;
    reg [1:0] term;      // 3 = 32-bit, 2 = 16-bit, 1 = bus error

    task automatic cycle(input [31:0] a, input [2:0] f, input [1:0] s,
                         input r);
        begin
            @(posedge clk2);
            addr = a; fc = f; siz = s; rw3 = r;
            we_seen0 = 0; we_seen1 = 0; oe0_seen = 0; oe1_seen = 0;
            rom_seen = 0; duart_seen = 0; uds_seen = 0; lds_seen = 0;
            b24_seen = 0; b25_seen = 0; vma_seen = 0;
            @(negedge clk2); as1_n = 0; clocks = 0;
            term = 0;
            while (term == 0) begin
                @(negedge clk2);
                clocks = clocks + 1;
                we_seen0 = we_seen0 | ~ram0_we_n;
                we_seen1 = we_seen1 | ~ram1_we_n;
                oe0_seen = oe0_seen | ~ram0_oe_n;
                oe1_seen = oe1_seen | ~ram1_oe_n;
                rom_seen = rom_seen | ~rom_ce_n;
                duart_seen = duart_seen | ~duart_ce_n;
                uds_seen = uds_seen | ~uds_n;
                lds_seen = lds_seen | ~lds_n;
                b24_seen = b24_seen | ~b24_n;
                b25_seen = b25_seen | ~b25_n;
                vma_seen = vma_seen | ~vma_n;
                if (be_n === 1'b0)                 term = 1;
                else if (!ds0_n && !ds1_n)         term = 3;
                else if (!ds1_n)                   term = 2;
                if (clocks > 400) begin
                    $display("FAIL: cycle at %h never ended", a);
                    errors = errors + 1; term = 1;
                end
            end
            e_at_end = e;
            @(posedge clk2); @(negedge clk2);   // S4/S5: data latched
            as1_n = 1;
            @(posedge clk2);
            if (!ds1_n || !ds0_n || be_n === 1'b0) begin
                $display("FAIL: termination still asserted after /AS at %h", a);
                errors = errors + 1;
            end
        end
    endtask

    task check(input cond, input [639:0] what);
        if (!cond) begin
            $display("FAIL: %0s", what);
            errors = errors + 1;
        end else
            $display("ok:   %0s", what);
    endtask

    initial begin
        repeat (3) @(posedge clk2);
        reset_n = 1;

        // Reset overlay: vector fetch from 0 reads the ROM.
        cycle(32'h00000000, 3'b110, 2'b00, 1);
        check(rom_seen && !oe0_seen && term == 2, "overlay: read at 0 goes to ROM, 16-bit");
        cycle(32'h00000004, 3'b110, 2'b00, 1);
        check(rom_seen && term == 2, "overlay: read at 4 goes to ROM");
        // First ROM-space access clears it.
        cycle(32'h00C00008, 3'b110, 2'b10, 1);
        check(rom_seen && term == 2 && clocks == 1 + 3,
              "ROM at $C00008: 16-bit, 3 wait states (6-clock cycle)");
        cycle(32'h00000000, 3'b101, 2'b00, 1);
        check(oe0_seen && !rom_seen && term == 3, "after ROM access: RAM at 0, 32-bit");
        check(clocks == 1 + 1, "RAM: 1 wait state (4-clock cycle)");

        // RAM byte lanes for writes.
        cycle(32'h00001000, 3'b101, 2'b00, 0);
        check(we_seen0 == 4'b1111 && we_seen1 == 0, "long write: all four WE, bank 0");
        cycle(32'h00001001, 3'b101, 2'b01, 0);
        check(we_seen0 == 4'b0100, "byte write at +1: WE2 only");
        cycle(32'h00001003, 3'b101, 2'b01, 0);
        check(we_seen0 == 4'b0001, "byte write at +3: WE0 only");
        cycle(32'h00001002, 3'b101, 2'b10, 0);
        check(we_seen0 == 4'b0011, "word write at +2: WE1, WE0");
        cycle(32'h00001001, 3'b101, 2'b00, 0);
        check(we_seen0 == 4'b0111, "misaligned long at +1: WE2-WE0");
        cycle(32'h00001003, 3'b101, 2'b10, 0);
        check(we_seen0 == 4'b0001, "misaligned word at +3: WE0");
        cycle(32'h00200000, 3'b101, 2'b00, 0);
        check(we_seen1 == 4'b1111 && we_seen0 == 0, "$200000: bank 1 (2 MB banks)");
        cycle(32'h00400000, 3'b101, 2'b00, 1);
        check(term == 1, "$400000: bus error (above 4 MB)");
        cycle(32'h01000000, 3'b101, 2'b00, 1);
        check(term == 1, "$01000000: bus error (full decode)");

        // ROM writes are ignored but acknowledged.
        cycle(32'h00C00000, 3'b101, 2'b10, 0);
        check(!rom_seen && term == 2, "ROM write: no /ROM_CE, acknowledged");

        // 68681 at $AD0001 (byte read).
        cycle(32'h00AD0001, 3'b101, 2'b01, 1);
        check(duart_seen && lds_seen && !uds_seen && term == 2,
              "68681: /68681_CE, /LDS, ended by /DTACK, 16-bit");
        // Card on the backplane, word access.
        cycle(32'h00AB0000, 3'b101, 2'b10, 0);
        check(uds_seen && lds_seen && !duart_seen && term == 2,
              "backplane word write: /UDS+/LDS, /DTACK");
        // Nothing there: bus error after the timeout.
        cycle(32'h00A80000, 3'b101, 2'b01, 1);
        check(term == 1 && clocks >= 255, "empty I/O slot: bus error after timeout");

        // 6800-style device: /VPA, synchronised to E.
        cycle(32'h00A00001, 3'b101, 2'b01, 1);
        check(vma_seen && term == 2 && e_at_end == 1, "VPA read: /VMA, ends while E high");
        cycle(32'h00A00001, 3'b101, 2'b01, 0);
        check(vma_seen && term == 2, "VPA write: /VMA, ends as E falls");

        // CPU space.
        cycle(32'hFFFFFFF3, 3'b111, 2'b01, 1);        // IACK level 1
        check(b24_seen && !b25_seen && term == 2, "IACK level 1: /B24, vector by /DTACK");
        cycle(32'hFFFFFFF5, 3'b111, 2'b01, 1);        // IACK level 2
        check(b25_seen && !b24_seen, "IACK level 2: /B25");
        cycle(32'hFFFFFFF9, 3'b111, 2'b01, 1);        // IACK level 4, nobody
        check(term == 1, "IACK level 4 with no device: bus error (spurious)");
        cycle(32'h00022000, 3'b111, 2'b00, 1);        // coprocessor (FPU)
        check(term == 1 && clocks <= 2, "coprocessor cycle: immediate bus error");

        // Interrupt encoder.
        irq_n = 7'b1111110; #1 check(ipl_n == 3'b110, "IRQ1 -> IPL 1");
        irq_n = 7'b1101101; #1 check(ipl_n == 3'b010, "IRQ2+IRQ5 -> IPL 5");
        irq_n = 7'b0111111; #1 check(ipl_n == 3'b000, "IRQ7 -> IPL 7");
        irq_n = 7'h7f;      #1 check(ipl_n == 3'b111, "none -> IPL 0");

        $display("%0d failure(s)", errors);
        $finish;
    end
endmodule
