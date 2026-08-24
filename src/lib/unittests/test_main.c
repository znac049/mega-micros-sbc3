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

#include <stdio.h>
#include <dirent.h>
#include <nonstd.h>
#include "unittest.h"

void test_main(void) {
    MODULE("Run Time Library");

    SUITE("string.h");
    TEST(strcmp("one", "One") > 0);
    TEST(strcmp("one", "two") != 0);
    TEST(strcmp("on", "one") < 0);
    TEST(strlen("blargle") == 7);
    TEST(strlen("") == 0);

    SUITE("strings.h");
    TEST(strcasecmp("one", "One") == 0);
    TEST(strcasecmp("one", "two") != 0);
    TEST(strcasecmp("on", "one") < 0);

    SUITE("dirent.h");
    TEST(opendir(".")!=NULL);

    SUITE("nonstd.h");
    do {
        char *bits[8];
        char str[16];

        TEST(split_str("one two three   four ", ' ', bits, 8) == 4);
        TEST(strcmp(bits[0], "one") == 0);
        TEST(strcmp(bits[3], "four") == 0);
        strcpy(str, "Hi!");
        TEST(strpad(str, 16, '.') == 0);
        TEST(strcmp(str, "Hi!.............") == 0);
    } while (0);
}