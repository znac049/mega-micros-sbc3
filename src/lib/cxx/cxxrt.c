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

/*
 * The run-time support the C++ compiler expects, for programs built with
 * -fno-exceptions -fno-rtti (see stdenv.mk): new and delete, and what
 * the compiler calls for pure virtual functions and function-local
 * statics. It's written in C so the library doesn't need a C++ compiler;
 * the functions are given the names g++ gives them (size_t is unsigned
 * int on the 68k, "j" in a mangled name).
 *
 * Without exceptions, a new that runs out of memory can't throw
 * std::bad_alloc, so it says so and ends the program instead, as
 * g++'s own library does when exceptions are turned off.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define CXX_NAME(name)  __asm__(name)

static void out_of_memory(size_t size) {
    fprintf(stderr, "operator new: out of memory (%u bytes)\n", (unsigned)size);
    abort();
}

static void *new_or_die(size_t size) {
    void *p;

    // new must return a distinct pointer, even for 0 bytes
    if (size == 0) {
        size = 1;
    }

    p = malloc(size);
    if (p == NULL) {
        out_of_memory(size);
    }

    return p;
}

static void *new_or_null(size_t size) {
    return malloc((size == 0)?1:size);
}

// std::nothrow, the tag for new that returns NULL (see <new>)
const char cxx_nothrow CXX_NAME("_ZSt7nothrow") = 0;

// operator new(size_t), new[](size_t)
void *cxx_new(size_t size) CXX_NAME("_Znwj");
void *cxx_new_array(size_t size) CXX_NAME("_Znaj");
void *cxx_new(size_t size) { return new_or_die(size); }
void *cxx_new_array(size_t size) { return new_or_die(size); }

// operator new(size_t, const std::nothrow_t &), and new[]
void *cxx_new_nothrow(size_t size, const void *tag) CXX_NAME("_ZnwjRKSt9nothrow_t");
void *cxx_new_array_nothrow(size_t size, const void *tag) CXX_NAME("_ZnajRKSt9nothrow_t");
void *cxx_new_nothrow(size_t size, const void *tag) { (void)tag; return new_or_null(size); }
void *cxx_new_array_nothrow(size_t size, const void *tag) { (void)tag; return new_or_null(size); }

// operator delete(void *), delete[], their sized (C++14) and nothrow forms
void cxx_delete(void *p) CXX_NAME("_ZdlPv");
void cxx_delete_array(void *p) CXX_NAME("_ZdaPv");
void cxx_delete_sized(void *p, size_t size) CXX_NAME("_ZdlPvj");
void cxx_delete_array_sized(void *p, size_t size) CXX_NAME("_ZdaPvj");
void cxx_delete_nothrow(void *p, const void *tag) CXX_NAME("_ZdlPvRKSt9nothrow_t");
void cxx_delete_array_nothrow(void *p, const void *tag) CXX_NAME("_ZdaPvRKSt9nothrow_t");
void cxx_delete(void *p) { free(p); }
void cxx_delete_array(void *p) { free(p); }
void cxx_delete_sized(void *p, size_t size) { (void)size; free(p); }
void cxx_delete_array_sized(void *p, size_t size) { (void)size; free(p); }
void cxx_delete_nothrow(void *p, const void *tag) { (void)tag; free(p); }
void cxx_delete_array_nothrow(void *p, const void *tag) { (void)tag; free(p); }

// Called if a pure virtual function, or a deleted one, is ever called
// (from a constructor or destructor of the abstract class)
void __cxa_pure_virtual(void);
void __cxa_deleted_virtual(void);

void __cxa_pure_virtual(void) {
    fprintf(stderr, "pure virtual function called\n");
    abort();
}

void __cxa_deleted_virtual(void) {
    fprintf(stderr, "deleted virtual function called\n");
    abort();
}

/*
 * Function-local statics: the compiler calls __cxa_guard_acquire() before
 * running a static's constructor, which it does if this returns non-zero,
 * then __cxa_guard_release(). There is only one thread, so the guard's
 * first byte is all that's needed: it's set once the static is made.
 */
int __cxa_guard_acquire(char *guard);
void __cxa_guard_release(char *guard);
void __cxa_guard_abort(char *guard);

int __cxa_guard_acquire(char *guard) {
    return *guard == 0;
}

void __cxa_guard_release(char *guard) {
    *guard = 1;
}

void __cxa_guard_abort(char *guard) {
    (void)guard;
}
