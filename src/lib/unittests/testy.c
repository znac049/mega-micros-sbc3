/*
The original unit test framework can be found at 
 
 https://github.com/mseminatore/testy
 
and is releasedf under the MIT licence by
Mark Seminatore.
 
 
My changes to Mark's code are similarly released under the MIT License:
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
#include "unittest.h"

int test_number		= 0;
int test_failures 	= 0;
int test_suites 	= 0;
int test_modules 	= 0;
int test_skipped    = 0;


static void begin_tests(void) {
    printf("%sBegin test pass%s...\n", TERM_BRIGHT_MAGENTA, TERM_RESET);
}

static void end_tests(void) {
    printf("\n%sTest pass completed%s.\n", TERM_BRIGHT_MAGENTA, TERM_RESET);
    printf("Evaluated %s%d%s module%s. ", TERM_GREEN, test_modules, TERM_RESET, (test_modules!=1)?"s":"");
    printf("%s%d%s suite%s, and ", TERM_GREEN, test_suites, TERM_RESET, (test_suites!=1)?"s":"");
    printf("%s%d%s test%s passed with ", TERM_GREEN, test_number, TERM_RESET, (test_number!=1)?"s":"");
    printf("%s%d%s failed and ", test_failures?TERM_BRIGHT_RED:TERM_GREEN, test_failures, TERM_RESET, (test_failures!=1)?"s":"");
    printf("%s%d%s skipped test case(s).\n", TERM_YELLOW, test_skipped, TERM_RESET);

    printf("one ");
    printf("two ");
    printf("three\n");

    exit(test_failures);
}

int main(int argc, char *argv[])
{
    // Shut the compiler up!
    (void)argc;
    (void)argv;

    begin_tests();

    // User supplied code
    test_main();

	end_tests();
}