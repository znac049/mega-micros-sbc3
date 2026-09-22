/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#define acrtc_base          0xaa0000
#define ramdac_base         0xa90000

#define acrtc_status        ((volatile uint16_t*) acrtc_base)
#define acrtc_address       ((volatile uint16_t*) acrtc_base)
#define acrtc_data          ((volatile uint16_t*) (acrtc_base + 2))

// ACRTC Register addresses
#define ACRTC_REG_CCR       0x02         // Command Control Register


// ACRTC Status Register bits
#define ACRTC_SR_CER        0x80    // Command Error (cleared by ABT)
#define ACRTC_SR_ARD        0x40    // Area Detect (cleared by RPR or ABT)
#define ACRTC_SR_CED        0x20    // Command End: able to accept a command
#define ACRTC_SR_LPD        0x10    // Light Pen Strobe Detect
#define ACRTC_SR_RFF        0x08    // Read FIFO Full
#define ACRTC_SR_RFR        0x04    // Read FIFO Ready (has data)
#define ACRTC_SR_WFR        0x02    // Write FIFO Ready (not full)
#define ACRTC_SR_WFE        0x01    // Write FIFO Empty


// ACRTC Commands
#define ACRTC_CCR_ABT       0x8000  // Abort (1 = abort, FIFOs cleared)

