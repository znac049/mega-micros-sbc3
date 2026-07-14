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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expr.h"

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

static const constant_t constants[] = {
    {"CF_BASE",    0xAD0001},
    {"DUART_BASE", 0xAD0001},
    {"PIT_BASE",   0xAF0001},
};

#define NUM_CONSTANTS (sizeof(constants) / sizeof(constant_t))

// forward decl
static long parse_shift(parser_t *p);


static int ci_streq_n(const char *a, size_t alen, const char *b) {
    size_t blen = strlen(b);
    if (alen != blen) return 0;
    for (size_t i = 0; i < alen; i++) {
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return 0;
    }
    return 1;
}

static int lookup_constant(const char *name, size_t len, long *out) {
    for (size_t i = 0; i < NUM_CONSTANTS; i++) {
        if (ci_streq_n(name, len, constants[i].name)) {
            *out = constants[i].value;
            return 1;
        }
    }
    return 0;
}

static void set_error(parser_t *p, expr_error_t err, int pos) {
    if (p->err == EXPR_OK) {
        p->err = err;
        p->err_pos = pos;
    }
}

static void skip_whitespace(parser_t *p) {
    while ((p->pos < p->len) && isspace((unsigned char)p->src[p->pos])) {
        p->pos++;
    }
}

static void lex_number(parser_t *p, token_t *tok) {
    size_t start = p->pos;

    tok->pos = (int)start;

    if ((p->src[p->pos] == '$') ||
        ((p->src[p->pos] == '0') && ((p->pos + 1 < p->len) && (tolower((p->src[p->pos + 1]) == 'x'))))
    ) {
        // Hex number
        size_t hex_start;
        size_t n;
        char buf[64];

        p->pos++;
        if (p->src[p->pos] == '0') {
            p->pos++;
        }

        hex_start = p->pos;
        while (p->pos < p->len && isxdigit((unsigned char)p->src[p->pos])) {
            p->pos++;
        }

        if (p->pos == hex_start) {
            set_error(p, EXPR_ERR_UNEXPECTED_CHAR, (int)start);
            tok->type = TOK_NUMBER;
            tok->number = 0;
            return;
        }

        n = p->pos - start;

        if (n >= sizeof(buf)) {
            n = sizeof(buf) - 1;
        }

        memcpy(buf, p->src + start, n);
        buf[n] = EOS;

        tok->type = TOK_NUMBER;
        tok->number = strtol(buf, NULL, 16);
    }
    else {
        // Decimal number
        size_t n;
        char buf[64];

        while (p->pos < p->len && isdigit((unsigned char)p->src[p->pos])) {
            p->pos++;
        }

        n = p->pos - start;

        if (n >= sizeof(buf)) {
            n = sizeof(buf) - 1;
        }

        memcpy(buf, p->src + start, n);
        buf[n] = EOS;

        tok->type = TOK_NUMBER;
        tok->number = strtol(buf, NULL, 10);
    }
}

static void next_token(parser_t *p) {
    char c;
    int start;

    skip_whitespace(p);

    if (p->pos >= p->len) {
        p->cur.type = TOK_END;
        p->cur.pos = (int)p->pos;

        return;
    }

    c = p->src[p->pos];
    start = (int)p->pos;

    if (isdigit((unsigned char)c) || ((c == '$') && (p->pos + 1 < p->len) && (isxdigit((unsigned char)p->src[p->pos + 1])))) {
        lex_number(p, &p->cur);
        return;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        size_t s = p->pos;

        while ((p->pos < p->len) &&  ((isalnum((unsigned char)p->src[p->pos])) || (p->src[p->pos] == '_'))) {
            p->pos++;
        }

        p->cur.type = TOK_IDENT;
        p->cur.ident = p->src + s;
        p->cur.ident_len = p->pos - s;
        p->cur.pos = start;
        return;
    }

    switch (c) {
        case '+':
             p->pos++;
             p->cur.type = TOK_PLUS;
             p->cur.pos = start;
             return;

        case '-':
            p->pos++;
            p->cur.type = TOK_MINUS;
            p->cur.pos = start;
            return;

        case '*':
            p->pos++;
            p->cur.type = TOK_STAR;
            p->cur.pos = start;
            return;

        case '/':
            p->pos++;
            p->cur.type = TOK_SLASH;
            p->cur.pos = start;
            return;

        case '%':
            p->pos++;
            p->cur.type = TOK_PERCENT;
            p->cur.pos = start;
            return;

        case '(':
            p->pos++;
            p->cur.type = TOK_LPAREN;
            p->cur.pos = start;
            return;

        case ')':
            p->pos++;
            p->cur.type = TOK_RPAREN;
            p->cur.pos = start;
            return;

        case '<':
            if ((p->pos + 1 < p->len) && (p->src[p->pos + 1] == '<')) {
                p->pos += 2;
                p->cur.type = TOK_SHL;
                p->cur.pos = start;
                return;
            }
            break;

        case '>':
            if ((p->pos + 1 < p->len) && (p->src[p->pos + 1] == '>')) {
                p->pos += 2;
                p->cur.type = TOK_SHR;
                p->cur.pos = start;
                return;
            }
            break;

        default:
            break;
    }

    set_error(p, EXPR_ERR_UNEXPECTED_CHAR, start);
    p->pos++;
    p->cur.type = TOK_END; /* stop parsing further, error already set */
    p->cur.pos = start;
}

static long parse_primary(parser_t *p) {
    if (p->err) {
        return 0;
    }

    switch (p->cur.type) {
        case TOK_NUMBER: {
            long v = p->cur.number;

            next_token(p);

            return v;
        }
        break;

        case TOK_IDENT: {
            long v;

            if (!lookup_constant(p->cur.ident, p->cur.ident_len, &v)) {
                set_error(p, EXPR_ERR_UNKNOWN_CONSTANT, p->cur.pos);
                return 0;
            }

            next_token(p);

            return v;
        }
        break;

        case TOK_LPAREN: {
            int open_pos = p->cur.pos;
            long v;

            next_token(p);
            v = parse_shift(p);

            if (p->err) {
                return 0;
            }

            if (p->cur.type != TOK_RPAREN) {
                set_error(p, EXPR_ERR_UNBALANCED_PARENS, open_pos);
                return 0;
            }

            next_token(p);

            return v;
        }
        break;

        default:
            set_error(p, EXPR_ERR_SYNTAX, p->cur.pos);
            return 0;
    }
}

static long parse_unary(parser_t *p) {
    if (p->err) {
        return 0;
    }

    if (p->cur.type == TOK_MINUS) {
        next_token(p);

        return -parse_unary(p);
    }
    else if (p->cur.type == TOK_PLUS) {
        next_token(p);

        return parse_unary(p);
    }

    return parse_primary(p);
}

static long parse_term(parser_t *p) {
    long lhs = parse_unary(p);

    while ((!p->err) &&
           ((p->cur.type == TOK_STAR || p->cur.type == TOK_SLASH || p->cur.type == TOK_PERCENT))) {
        token_type_t op = p->cur.type;
        int op_pos = p->cur.pos;
        long rhs;

        next_token(p);
        rhs = parse_unary(p);
        if (p->err) {
            return 0;
        }

        if (op == TOK_STAR) {
            lhs = lhs * rhs;
        } else if (op == TOK_SLASH) {
            if (rhs == 0) {
                set_error(p, EXPR_ERR_DIVISION_BY_ZERO, op_pos);
                return 0;
            }

            lhs = lhs / rhs;
        } else { /* TOK_PERCENT */
            long long li = (long long)lhs;
            long long ri = (long long)rhs;

            if (ri == 0) {
                set_error(p, EXPR_ERR_DIVISION_BY_ZERO, op_pos);
                return 0;
            }

            lhs = (long)(li % ri);
        }
    }
    return lhs;
}

static long parse_additive(parser_t *p) {
    long lhs = parse_term(p);

    while ((!p->err && ((p->cur.type == TOK_PLUS) || (p->cur.type == TOK_MINUS)))) {
        token_type_t op = p->cur.type;
        long rhs;

        next_token(p);
        rhs = parse_term(p);

        if (p->err) {
            return 0;
        }

        lhs = (op == TOK_PLUS) ? lhs + rhs : lhs - rhs;
    }

    return lhs;
}

static long parse_shift(parser_t *p) {
    long lhs = parse_additive(p);

    while ((!p->err && ((p->cur.type == TOK_SHL) || (p->cur.type == TOK_SHR)))) {
        token_type_t op = p->cur.type;
        int op_pos = p->cur.pos;
        long rhs;
        long long li;
        long long shift;

        next_token(p);
        rhs = parse_additive(p);
        if (p->err) {
            return 0;
        }

        li = (long long)lhs;
        shift = (long long)rhs;
        if ((shift < 0) || (shift >= 64)) {
            set_error(p, EXPR_ERR_BAD_SHIFT, op_pos);
            return 0;
        }

        lhs = (op == TOK_SHL) ? (long)(li << shift) : (long)(li >> shift);
    }

    return lhs;
}

expr_error_t expr_evaluate(const char *expression, long *result, int *error_pos) {
    parser_t p;
    long val;

    if (error_pos != NULL) {
        *error_pos = 0;
    }

    if (!expression || (expression[0] == EOS)) {
        return EXPR_ERR_EMPTY_EXPRESSION;
    }

    p.src = expression;
    p.len = strlen(expression);
    p.pos = 0;
    p.err = EXPR_OK;
    p.err_pos = -1;

    next_token(&p); // prime the pump

    if ((p.cur.type == TOK_END) && (p.err == EXPR_OK)) {
        // Empty expression
        return EXPR_ERR_EMPTY_EXPRESSION;
    }

    val = parse_shift(&p);

    if (p.err != EXPR_OK) {
        if (error_pos != NULL) {
            *error_pos = p.err_pos;
        }

        return p.err;
    }

    if (p.cur.type != TOK_END) {
        if (error_pos != NULL) {
            *error_pos = p.cur.pos;
        }

        return EXPR_ERR_TRAILING_TOKENS;
    }

    if (result != NULL) {
        *result = val;
    }

    return EXPR_OK;
}

const char *expr_error_string(expr_error_t err) {
    switch (err) {
        case EXPR_OK:
            return "no error";

        case EXPR_ERR_EMPTY_EXPRESSION:
            return "empty expression";

        case EXPR_ERR_UNEXPECTED_CHAR:
            return "unexpected character";

        case EXPR_ERR_SYNTAX:
            return "syntax error, expected a value";

        case EXPR_ERR_UNBALANCED_PARENS:
            return "unbalanced parentheses";

        case EXPR_ERR_UNKNOWN_CONSTANT:
            return "unknown constant";

        case EXPR_ERR_DIVISION_BY_ZERO:
            return "division by zero";

        case EXPR_ERR_TRAILING_TOKENS:
            return "unexpected trailing input";

        case EXPR_ERR_BAD_SHIFT:
            return "shift amount must be between 0 and 63";

        default:
            return "unknown error";
    }
}