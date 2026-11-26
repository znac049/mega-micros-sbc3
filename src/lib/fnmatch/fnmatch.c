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

#include <fnmatch.h>
#include <stddef.h>

/*
 * Shell-style pattern matching: * matches any run of characters, ? any
 * one, [...] any one of a set (ranges a-z, negated with ! or ^), and \
 * quotes the next character unless FNM_NOESCAPE is given.
 */

// Does a leading period at s need an explicit match? (FNM_PERIOD)
static int hidden_period(const char *string, const char *s, int flags) {
    if (!(flags & FNM_PERIOD) || (*s != '.')) {
        return 0;
    }

    return (s == string) || ((flags & FNM_PATHNAME) && (s[-1] == '/'));
}

/*
 * Match one bracket expression, starting just after its '['. Returns a
 * pointer just past its ']', with *matched set, or NULL if the bracket
 * isn't closed (and so the '[' is an ordinary character).
 */
static const char *match_bracket(const char *p, unsigned char c, int flags, int *matched) {
    int negate = 0;
    int found = 0;
    int first = 1;

    if ((*p == '!') || (*p == '^')) {
        negate = 1;
        p++;
    }

    // A ']' straight after the '[' (or '[!') is a member, not the end
    while ((*p != ']') || first) {
        unsigned char lo, hi;

        first = 0;
        if (*p == '\0') {
            return NULL;
        }

        if ((*p == '\\') && !(flags & FNM_NOESCAPE) && (p[1] != '\0')) {
            p++;
        }
        lo = (unsigned char)*p++;
        hi = lo;

        if ((*p == '-') && (p[1] != ']') && (p[1] != '\0')) {
            p++;
            if ((*p == '\\') && !(flags & FNM_NOESCAPE) && (p[1] != '\0')) {
                p++;
            }
            hi = (unsigned char)*p++;
        }

        if ((c >= lo) && (c <= hi)) {
            found = 1;
        }
    }

    *matched = found ^ negate;
    return p + 1;
}

int fnmatch(const char *pattern, const char *string, int flags) {
    const char *p = pattern;
    const char *s = string;
    const char *star_p = NULL;      // just after the last '*' seen
    const char *star_s = NULL;      // where in the string that '*' got to

    for (;;) {
        if (*p == '*') {
            // Several stars in a row are the same as one
            while (*p == '*') {
                p++;
            }

            if (hidden_period(string, s, flags)) {
                return FNM_NOMATCH;
            }

            star_p = p;
            star_s = s;
            continue;
        }

        if (*s != '\0') {
            unsigned char c = (unsigned char)*s;
            const char *next = NULL;
            int matched = 0;

            if (*p == '?') {
                matched = !((flags & FNM_PATHNAME) && (c == '/')) &&
                          !hidden_period(string, s, flags);
                next = p + 1;
            }
            else if (*p == '[') {
                if ((flags & FNM_PATHNAME) && (c == '/')) {
                    matched = 0;
                    next = p + 1;
                }
                else if (hidden_period(string, s, flags)) {
                    matched = 0;
                    next = p + 1;
                }
                else {
                    next = match_bracket(p + 1, c, flags, &matched);
                    if (next == NULL) {
                        // No closing ']': the '[' is just a character
                        matched = (c == '[');
                        next = p + 1;
                    }
                }
            }
            else if (*p != '\0') {
                const char *q = p;

                if ((*q == '\\') && !(flags & FNM_NOESCAPE) && (q[1] != '\0')) {
                    q++;
                }
                matched = ((unsigned char)*q == c);
                next = q + 1;
            }

            if (matched) {
                p = next;
                s++;
                continue;
            }
        }
        else if (*p == '\0') {
            return 0;
        }

        // A mismatch: let the last '*' swallow one more character, if it can
        if ((star_p == NULL) || (*star_s == '\0') ||
            ((flags & FNM_PATHNAME) && (*star_s == '/'))) {
            return FNM_NOMATCH;
        }

        star_s++;
        p = star_p;
        s = star_s;
    }
}
