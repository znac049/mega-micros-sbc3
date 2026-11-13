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

extern volatile uint8_t bus_error_flag;

uint8_t peek(volatile uint8_t *addr, int *err) {
    uint8_t data;
    
    bus_error_flag = 0;
    data = *addr;

    *err = bus_error_flag;

    return data;
}

uint8_t poke(volatile uint8_t *addr, uint8_t val, int *err) {
    bus_error_flag = 0;
    *addr = val;

    *err = bus_error_flag;

    return val;
}

uint16_t peekw(volatile uint16_t *addr, int *err) {
    uint16_t data;
    
    bus_error_flag = 0;
    data = *addr;

    *err = bus_error_flag;

    return data;
}

uint16_t pokew(volatile uint16_t *addr, uint16_t val, int *err) {
    bus_error_flag = 0;
    *addr = val;

    *err = bus_error_flag;

    return val;
}

uint32_t peekl(volatile uint32_t *addr, int *err) {
    uint32_t data;
    
    bus_error_flag = 0;
    data = *addr;

    *err = bus_error_flag;

    return data;
}

uint32_t pokel(volatile uint32_t *addr, uint32_t val, int *err) {
    bus_error_flag = 0;
    *addr = val;

    *err = bus_error_flag;

    return val;
}