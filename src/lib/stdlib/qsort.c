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

#include <stdlib.h>

static void swap(char *a, char *b, size_t size) {
    while (size--) {
        char t = *a;

        *a++ = *b;
        *b++ = t;
    }
}

// Push element i down the heap of n elements until it's in place
static void sift_down(char *base, size_t i, size_t n, size_t size,
                      int (*compar)(const void *, const void *)) {
    for (;;) {
        size_t child = 2 * i + 1;

        if (child >= n) {
            return;
        }

        if ((child + 1 < n) && (compar(base + child * size, base + (child + 1) * size) < 0)) {
            child++;
        }

        if (compar(base + i * size, base + child * size) >= 0) {
            return;
        }

        swap(base + i * size, base + child * size, size);
        i = child;
    }
}

// Heapsort: in place, no recursion, but doesn't keep equal elements in order
static void heap_sort(char *b, size_t nmemb, size_t size,
                      int (*compar)(const void *, const void *)) {
    for (size_t i = nmemb / 2; i-- > 0; ) {
        sift_down(b, i, nmemb, size, compar);
    }

    for (size_t n = nmemb - 1; n > 0; n--) {
        swap(b, b + n * size, size);
        sift_down(b, 0, n, size, compar);
    }
}

static void copy(char *d, const char *s, size_t n) {
    while (n--) {
        *d++ = *s++;
    }
}

/*
 * Bottom-up merge sort through tmp (as big as the array): stable, so
 * equal elements keep their order, as with glibc's qsort().
 */
static void merge_sort(char *b, char *tmp, size_t nmemb, size_t size,
                       int (*compar)(const void *, const void *)) {
    char *from = b, *to = tmp;

    for (size_t width = 1; width < nmemb; width *= 2) {
        for (size_t lo = 0; lo < nmemb; lo += 2 * width) {
            size_t mid = (lo + width < nmemb)?(lo + width):nmemb;
            size_t hi = (lo + 2 * width < nmemb)?(lo + 2 * width):nmemb;
            size_t i = lo, j = mid, k = lo;

            while ((i < mid) && (j < hi)) {
                // <= takes from the left run first: that's what keeps it stable
                if (compar(from + i * size, from + j * size) <= 0) {
                    copy(to + k++ * size, from + i++ * size, size);
                }
                else {
                    copy(to + k++ * size, from + j++ * size, size);
                }
            }

            while (i < mid) {
                copy(to + k++ * size, from + i++ * size, size);
            }

            while (j < hi) {
                copy(to + k++ * size, from + j++ * size, size);
            }
        }

        // Swap roles for the next pass
        {
            char *t = from;

            from = to;
            to = t;
        }
    }

    if (from != b) {
        copy(b, from, nmemb * size);
    }
}

/*
 * A stable merge sort if there's memory for a copy of the array, otherwise
 * heapsort (which isn't stable). Neither recurses, so the stack is safe
 * whatever the input.
 */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    char *tmp;

    if ((nmemb < 2) || (size == 0)) {
        return;
    }

    tmp = malloc(nmemb * size);
    if (tmp != NULL) {
        merge_sort(base, tmp, nmemb, size, compar);
        free(tmp);
    }
    else {
        heap_sort(base, nmemb, size, compar);
    }
}
