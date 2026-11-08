# SBC3 glue logic (EPM7128S, IC3)

This is a Verilog replacement for the glue CPLD on the Mega-Micros 68030 SBC-3B3, written from the rev 1.0 schematic. The original CPLD design wasn't available, so this is a reconstruction. Check the assumptions below before programming a board.

| File | What it is |
|---|---|
| `sbc3_glue.v` | The design. All tunable choices are parameters at the top |
| `sbc3_glue.qsf`, `sbc3_glue.qpf` | Quartus II project, with your pin allocation |
| `sbc3_glue_tb.v` | Icarus Verilog testbench (28 checks) |

## Building

- **MAX 7000S:** use Quartus II **13.0sp1**, the last Quartus version that supports this family (Web Edition is free). Open `sbc3_glue.qpf` and compile. Set `DEVICE` in the `.qsf` to your chip's speed grade first (`EPM7128SLC84-15`, `-10`, `-7`…).
- **Programming:** program the chip in-circuit through CN6 (JTAG) with the `.pof` file and a USB-Blaster.
- **Simulation:** `iverilog -g2012 -o tb sbc3_glue_tb.v sbc3_glue.v && vvp tb`.

The design uses about 22 flip-flops plus roughly 40 output functions, well within the 128 macrocells.

## What it does

| Address | Device | Port | Ended by |
|---|---|---|---|
| $000000–$1FFFFF | RAM bank 0 (IC17–20) | 32-bit | DSACK0+1 after `RAM_WAIT` |
| $200000–$3FFFFF | RAM bank 1 (IC5–8) | 32-bit | DSACK0+1 after `RAM_WAIT` |
| $400000–$7FFFFF | none | | bus error |
| $800000–$BFFFFF | backplane I/O: CF and PI/T (via IC10), Hex Display… | 16-bit | /DTACK → DSACK1, or /VPA → 6800 cycle |
| $AD0000–$ADFFFF | also /68681_CE | 16-bit | the 68681's /DTACK (J1) |
| $C00000–$FFFFFF | ROM (27C322) | 16-bit | DSACK1 after `ROM_WAIT` (writes are ignored) |
| CPU space, IACK | backplane; level 1 → /B24 (PI/T TIACK), level 2 → /B25 (68681 /IACK) | 16-bit | /DTACK with the vector |
| CPU space, other | e.g. FPU cycles | | immediate bus error, so FPU instructions take the F-line trap |

The table shows the default 2 MB banks (`RAM_BANK_BIT = 21`); with 1Mx8 SRAMs each bank is 4 MB (`RAM_BANK_BIT = 22`).

- **Reset overlay:** after reset, reads from RAM space come from the ROM, so the 68030 fetches its reset vectors there. The first access to $C00000–$FFFFFF ends the overlay. Writes always go to RAM.
- **Unused space:** with `FULL_DECODE = 1`, any address with A31–A24 ≠ 0 bus-errors.
- **Interrupts:** the seven `/IRQ` lines are priority-encoded onto `/IPL2–0`.
- **Backplane strobes:** `/UDS` and `/LDS` assert one clock after `/AS`, from A0 and SIZ, and go away with `/AS`. `/68681_CE`, `/B24` and `/B25` are timed the same way.
- **6800 cycles:** if a device answers an I/O cycle with `/VPA`, the CPLD waits for E to go low, asserts `/VMA` for one E period, and ends reads 2 clocks before E falls and writes as E falls. E = CLK2 / `E_DIV`: 1 MHz by default at 40 MHz, high for 40% of the period.
- **Bus-error timeout:** if nothing answers within `BERR_TIMEOUT` clocks (255 = 6.4 µs), `/BE` asserts. `/BE` is open-drain, since backplane cards can drive /BERR too. `/DS0` and `/DS1` are driven both ways: the 10K pull-ups on DSACK are far too slow at 40 MHz.

## Check these against your board

1. **Two pins missing from your list: /B24 on pin 41 and /B25 on pin 39.** On the schematic they're the interrupt acknowledges for the PI/T (PC7/TIACK) and the 68681 (/IACK), through R55/R54. Without them neither chip can supply its vector, so the timer and serial interrupts would fail. They're in the `.qsf`.
2. **CLK2 (pin 83)** is assumed to be a 40 MHz MC10H640 output in phase with the CPU clock. All wait-state and timeout counts are in its cycles.
3. **RAM size.** The parts are drawn as AS6C4008 (512Kx8), which gives two 2 MB banks: 4 MB in all, matching your real board's "total memory = 4096 KB". MAME currently gives the board 8 MB (two 4 MB banks), so for 1Mx8 SRAMs set `RAM_BANK_BIT = 22`. Also check that the monitor's RAM sizing copes with a bus error at $400000.
4. **Wait states:** `RAM_WAIT = 1` suits 55 ns SRAM, and `ROM_WAIT = 3` suits a 100 ns 27C322. If the board is unreliable, raise them first. They're cheap.
5. **No autovectors.** The 68030's /AVEC only has a pull-up (RP1); it isn't wired to the CPLD. So every interrupt source must supply a vector with /DTACK. A level that nobody answers ends as a spurious interrupt. For example, CF INTRQ on IPL4, which MAME treats as autovectored, would need a vector from IC10. To support autovectors you would need a wire from /AVEC to a spare CPLD pin (pin 4 is free).
6. **Full decoding** (`FULL_DECODE = 1`) differs from MAME, which repeats the board every 16 MB. Set it to 0 to match the emulator.
7. **/SEL (pin 1, jumper J6) is unused** and available for your own option. Pin 4 (header H1) is free too.
8. **/DTACK is believed from the 2nd clock of a cycle** (`DTACK_MIN`). That gives the previous device's /DTACK time to rise through R1 (1K2). Raise it if you see cycles ending early.

## Not verified

The testbench checks the logic only: decoding, byte lanes, wait-state counts, termination and the 6800 handshake, against an idealised 68030 bus. It does not check real propagation delays, which depend on the speed grade and on Quartus's fit. I couldn't run Quartus here, so the design hasn't been fitted. Read the fitter's timing report before programming, especially the /AS → /RAM_OE and /AS → DSACK paths.
