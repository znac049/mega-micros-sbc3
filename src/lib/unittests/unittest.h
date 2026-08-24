//--------------------------------------------------------
// Simple CTest CI/CD compatible C/C++ test framework
// 
// Copyright 2022 Mark Seminatore. All rights reserved.
//--------------------------------------------------------
#pragma once

#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// screen control
#define ESC "\x1b"
#define TERM_CLEAR          ESC "[2J" ESC "[H"
#define TERM_BOLD           ESC "[1m"
#define TERM_UNDERLINE      ESC "[4m"
#define TERM_BLINK          ESC "[5m"

// text colors
#define TERM_RESET          ESC "[0m"
#define TERM_BLACK          ESC "[30m"
#define TERM_RED            ESC "[31m"
#define TERM_GREEN          ESC "[32m"
#define TERM_YELLOW         ESC "[33m"
#define TERM_BLUE           ESC "[34m"
#define TERM_MAGENTA        ESC "[35m"
#define TERM_CYAN           ESC "[36m"
#define TERM_WHITE          ESC "[37m"
#define TERM_BRIGHT_BLACK   ESC "[90m"
#define TERM_BRIGHT_RED     ESC "[91m"
#define TERM_BRIGHT_GREEN   ESC "[92m"
#define TERM_BRIGHT_YELLOW  ESC "[93m"
#define TERM_BRIGHT_BLUE    ESC "[94m"
#define TERM_BRIGHT_MAGENTA ESC "[95m"
#define TERM_BRIGHT_CYAN    ESC "[96m"
#define TERM_BRIGHT_WHITE   ESC "[97m"

#define CHECK_MARK TERM_GREEN "\u2713" TERM_RESET
#define X_MARK TERM_BRIGHT_RED "\u274C" TERM_RESET

#define TEST_ASSERT(expr, f, l) \
    do { \
        if ((expr)) { \
            puts(CHECK_MARK); \
        } \
        else { \
            puts(X_MARK); test_failures++; \
            printf("\tTest %sfailed%s at [%s%s:%d%s]\n", TERM_BRIGHT_RED, TERM_RESET, TERM_BRIGHT_RED, (f), (l), TERM_RESET); \
        } \
    } while(0)

// simple test harness macros
#define TEST(s)         do { printf("\t%d test case: %s" #s "%s ", ++test_number, TERM_CYAN, TERM_RESET); TEST_ASSERT(s, __FILE__, __LINE__); } while(0)
#define TESTEX(msg,s)   do { printf("\t%d test case: %s" msg "%s ", ++test_number, TERM_CYAN, TERM_RESET); TEST_ASSERT(s, __FILE__, __LINE__); } while(0)
#define COMMENT(s)      printf("\n\t%s" s "%s\n", TERM_BRIGHT_BLUE, TERM_RESET)
#define SUITE(s)        do { printf("\nTesting suite %s" s "%s...\n", TERM_YELLOW, TERM_RESET); test_suites++; } while(0)
#define MODULE(s)       do { printf("\nModule %s" s "%s...\n", TERM_BRIGHT_MAGENTA, TERM_RESET); test_modules++; } while(0)
#define SKIP_TEST(s)  do { printf("\t%s" #s "%s %sSKIPPED%s\n", TERM_CYAN, TERM_RESET, TERM_YELLOW, TERM_RESET); test_skipped++; } while(0)

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

#ifndef EPSILON
# define EPSILON 1e-5
#endif

#ifndef EQUAL_EPSILON
# define EQUAL_EPSILON(a, b)     (fabs((a) - (b)) < EPSILON)
#endif
    
#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a)        (sizeof(a)/sizeof(a[0]))
#endif

#ifndef EQUAL_ARRAY
# define EQUAL_ARRAY(a, b)       !memcmp((a), (b), sizeof(a))
# define NOT_EQUAL_ARRAY(a, b)   !EQUAL_ARRAY(a, b)
#endif
    
extern int test_number;
extern int test_suites;
extern int test_failures;
extern int test_modules;
extern int test_skipped;

void test_main(void);

#ifdef __cplusplus
    }
#endif

#endif // #ifndef __TEST_H
