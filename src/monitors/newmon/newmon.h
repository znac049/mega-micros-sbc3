#pragma once

#include <ctype.h>

#define VECTOR_TABLE_SIZE 256

// Not normally exposed by libraries
extern void pre_main(void);
extern void _init_heap(void);

// vectors.c
extern void set_default_vectors(void);