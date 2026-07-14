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

/*
 * expr.h - tiny arithmetic expression parser/evaluator
 *
 * Supports:
 *   Operators : +  -  *  /  %  <<  >>   (unary + and - too)
 *   Grouping  : ( )
 *   Constants : PI, E, TAU, PHI  (case-insensitive)
 *   Numbers   : 12   3.14   .5   2e10   0x1F
 *
 * Precedence (highest to lowest), matching C's own rules:
 *   1. unary + / -
 *   2. *  /  %
 *   3. binary + -
 *   4. <<  >>
 *
 * % , << and >> operate on the truncated (long long) value of
 * their operands, same as you'd expect from integer bitwise ops.
 */

typedef enum {
    EXPR_OK = 0,
    EXPR_ERR_EMPTY_EXPRESSION,   /* "" or NULL was passed in            */
    EXPR_ERR_UNEXPECTED_CHAR,    /* lexer hit a character it can't use  */
    EXPR_ERR_SYNTAX,             /* parser expected a value/operator    */
    EXPR_ERR_UNBALANCED_PARENS,  /* missing ')' or a stray ')'          */
    EXPR_ERR_UNKNOWN_CONSTANT,   /* identifier isn't a known constant   */
    EXPR_ERR_DIVISION_BY_ZERO,   /* '/' or '%' with a zero divisor      */
    EXPR_ERR_TRAILING_TOKENS,    /* extra input after a valid expr      */
    EXPR_ERR_BAD_SHIFT           /* negative or absurd shift amount     */
} expr_error_t;

/*
 * Evaluate `expression` and store the result in *result.
 *
 * Returns EXPR_OK on success.
 * On failure, returns an error code and, if error_pos is not NULL,
 * stores the character offset into `expression` where the problem
 * was detected (useful for printing a "^" pointer under the input).
 */
expr_error_t expr_evaluate(const char *expression, long *result, int *error_pos);

/* Human-readable description of an expr_error_t value. */
const char *expr_error_string(expr_error_t err);
