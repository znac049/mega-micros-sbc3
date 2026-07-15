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

struct constant {
    const char *name;
    long value;
};

typedef struct constant constant_t;

typedef enum {
    TOK_END,
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_SHL,
    TOK_SHR,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_IDENT
} token_type_t;

struct token {
    token_type_t type;
    long number;         /* valid when type == TOK_NUMBER */
    const char *ident;     /* valid when type == TOK_IDENT  */
    size_t ident_len;
    int pos;                /* offset in source, for error reporting */
};

typedef struct token token_t;

struct parser {
    const char *src;
    size_t len;
    size_t pos;
    token_t cur;
    expr_error_t err;
    int err_pos;
};

typedef struct parser parser_t;


expr_error_t expr_evaluate(const char *expression, long *result, int *error_pos);
const char *expr_error_string(expr_error_t err);
