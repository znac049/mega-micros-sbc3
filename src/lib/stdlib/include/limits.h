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

/* Borrowed this from the Minix source tree */

/* The <limits.h> header defines some basic sizes, both of the language types 
 * (e.g., the number of bits in an integer), and of the operating system (e.g.
 * the number of characters in a file name.
 */

/* Definitions about chars (8 bits in MINIX, and signed). */
#define CHAR_BIT           8	/* # bits in a char */
#define CHAR_MIN        -128	/* minimum value of a char */
#define CHAR_MAX         127	/* maximum value of a char */
#define SCHAR_MIN       -128	/* minimum value of a signed char */
#define SCHAR_MAX        127	/* maximum value of a signed char */
#define UCHAR_MAX        255	/* maximum value of an unsigned char */
#define MB_LEN_MAX         1	/* maximum length of a multibyte char */

/* Definitions about shorts (16 bits in MINIX). */
#define SHRT_MIN  (-32767-1)	/* minimum value of a short */
#define SHRT_MAX       32767	/* maximum value of a short */
#define USHRT_MAX     0xFFFF	/* maximum value of unsigned short */

#define INT_MIN (-2147483647-1)	/* minimum value of a 32-bit int */
#define INT_MAX   2147483647	/* maximum value of a 32-bit int */
#define UINT_MAX  0xFFFFFFFF	/* maximum value of an unsigned 32-bit int */

/*Definitions about longs (32 bits in MINIX). */
#define LONG_MIN (-2147483647L-1)/* minimum value of a long */
#define LONG_MAX  2147483647L	/* maximum value of a long */
#define ULONG_MAX 0xFFFFFFFFL	/* maximum value of an unsigned long */

// Filesystem related defines
#define PATH_MAX 128
#define MAX_DIR_DEPTH 16
