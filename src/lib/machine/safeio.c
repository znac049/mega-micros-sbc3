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

#include <ctype.h>
#include <machine.h>

static int result;

static ISR bus_error(void) {
    result = -1;
}

int peek(volatile uint8_t *addr) {
    unsigned int old_handler = set_isr_handler(VEC_BUS_ERROR, (unsigned int)bus_error);
    uint8_t data;
    
    result = 0;
    data = *addr;

    set_isr_handler(VEC_BUS_ERROR, old_handler);
    
    return (result == -1)?result:(int)data;
}

int poke(volatile uint8_t *addr, uint8_t val) {
    unsigned int old_handler = set_isr_handler(VEC_BUS_ERROR, (unsigned int)bus_error);

    result = val;
    *addr = val;

    set_isr_handler(VEC_BUS_ERROR, old_handler);
    
    return result;
}