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


#pragma once

#include <ctype.h>

#define EI_NIDENT       16

#define EI_MAG0         0
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6

#define ELFCLASS32      1
#define ELFDATA2MSB     2           // big-endian
#define EV_CURRENT      1

#define ET_EXEC         2
#define EM_68K          4

#define PT_LOAD         1
#define PT_GNU_STACK    0x6474e551      // its p_memsz: the stack the program wants (ld -z stack-size=)

#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

struct elf32_ehdr {
    uint8_t     e_ident[EI_NIDENT];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
};

typedef struct elf32_ehdr elf32_ehdr_t;

struct elf32_phdr {
    uint32_t    p_type;
    uint32_t    p_offset;
    uint32_t    p_vaddr;
    uint32_t    p_paddr;
    uint32_t    p_filesz;
    uint32_t    p_memsz;
    uint32_t    p_flags;
    uint32_t    p_align;
};

typedef struct elf32_phdr elf32_phdr_t;

// The structs must match the on-disk layout exactly
_Static_assert(sizeof(elf32_ehdr_t) == 52, "elf32_ehdr_t must be 52 bytes");
_Static_assert(sizeof(elf32_phdr_t) == 32, "elf32_phdr_t must be 32 bytes");
