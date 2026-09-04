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
#include <unistd.h>
#include <extras.h>
#include <limits.h>
#include <libgen.h>
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

    SUITE("extras.h");
    do {
        char *bits[8];
        char str[16];

        TEST(split_str("one two three   four ", ' ', bits, 8) == 4);
        TEST(strcmp(bits[0], "one") == 0);
        TEST(strcmp(bits[3], "four") == 0);
        strcpy(str, "Hi!");
        TEST(strpad(str, 16, '.') == 0);
        TEST(strcmp(str, "Hi!.............") == 0);
        TEST(midstr(str, sizeof(str), "Hello, world", 2, 4) != NULL);
        TEST(strcmp(str, "llo") == 0);
        TEST(midstr(str, sizeof(str), "Hello, world", 7, 900) != NULL);
        printf("str='%s'\n", str);
        TEST(strcmp(str, "world") == 0);
    } while (0);

    SUITE("stdlib");
    do {
        char path[PATH_MAX];

        TEST(realpath(".", path) != NULL);
        printf("path='%s'\n", path);

        TEST(realpath("/one/two/three/four/../six/./seven//eight", path) != NULL);
        TEST(strcmp(path, "/one/two/three/six/seven/eight") == 0);

        TEST(realpath("////usr//bin///gcc//", path) != NULL);
        TEST(strcmp(path, "/usr/bin/gcc") == 0);

        TEST(realpath("./subby", path) != NULL);
        TEST(strcmp(path, "/rom0/subby") == 0);
    } while (0);

    SUITE("libgen");
    do {
        TEST(strcmp(dirname("/usr/lib/gcc"), "/usr/lib") == 0);
        TEST(strcmp(dirname("/usr/lib"), "/usr") == 0);
        TEST(strcmp(dirname("/usr/"), "/") == 0);
        TEST(strcmp(dirname("usr"), ".") == 0);
        TEST(strcmp(dirname("/"), "/") == 0);
        TEST(strcmp(dirname("."), ".") == 0);
        TEST(strcmp(dirname(".."), ".") == 0);

        TEST(strcmp(basename("/usr/lib/gcc"), "gcc") == 0);
        TEST(strcmp(basename("/usr/lib"), "lib") == 0);
        TEST(strcmp(basename("/usr/"), "usr") == 0);
        TEST(strcmp(basename("usr"), "usr") == 0);
        TEST(strcmp(basename("/"), "/") == 0);
        TEST(strcmp(basename("."), ".") == 0);
        TEST(strcmp(basename(".."), "..") == 0);
    } while(0);
}