//------------------------------------------------------
//
// Copyright 2023 Mark Seminatore. All rights reserved.
//------------------------------------------------------
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