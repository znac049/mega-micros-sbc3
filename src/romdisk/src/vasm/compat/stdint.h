/*
 * The library has no <stdint.h>: its fixed-size types are in <ctype.h>.
 * vasm includes <stdint.h>, and gcc's own conflicts with <ctype.h>'s, so
 * provide one that uses the library's types.
 */
#pragma once

#include <ctype.h>

typedef int                 intptr_t;
typedef long long           intmax_t;
typedef unsigned long long  uintmax_t;

#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-32767-1)
#define INT16_MAX   32767
#define UINT16_MAX  65535
#define INT32_MIN   (-2147483647-1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U
#define INT64_MIN   (-9223372036854775807LL-1)
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX  18446744073709551615ULL
