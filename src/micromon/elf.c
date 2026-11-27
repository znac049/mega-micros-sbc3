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


#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <machine.h>

#include "micromon.h"
#include "elf.h"

// Most program headers we'll cope with
#define MAX_PHDRS       16

/*
 * Programs are loaded from _PROGRAM_BASE (memory.ld) up: the user program
 * link scripts put CODE_START there too.
 */
extern char _PROGRAM_BASE[];
#define LOAD_MIN        ((uint32_t)_PROGRAM_BASE)

/*
 * The stack size the loaded program asked for, from its PT_GNU_STACK
 * header (link with -z stack-size=<bytes>), or 0 for the default
 */
uint32_t program_stack_size;

// CACR bits
#define CACR_EI         (1<<0)      // instruction cache enabled
#define CACR_CI         (1<<3)      // clear instruction cache

//Invalidate the instruction cache, but only if it's turned on
static void flush_icache(void) {
    uint32_t cacr = get_cacr();

    if (cacr & CACR_EI) {
        set_cacr(cacr | CACR_CI);
    }
}

static int read_fully(int fd, void *dest, uint32_t num_bytes) {
    char *p = (char *)dest;

    while (num_bytes > 0) {
        int got = read(fd, p, num_bytes);

        if (got <= 0) {
            if (got == 0) {
                errno = EIO;                // file shorter than the headers say
            }

            return NOT_OK;
        }

        p += got;
        num_bytes -= got;
    }

    return OK;
}

static int read_at(int fd, uint32_t offset, void *dest, uint32_t num_bytes) {
    if (lseek(fd, offset, SEEK_SET) != (off_t)offset) {
        errno = EIO;
        return NOT_OK;
    }

    return read_fully(fd, dest, num_bytes);
}

static int check_header(const elf32_ehdr_t *eh) {
    if ((eh->e_ident[EI_MAG0] != 0x7f) || (eh->e_ident[EI_MAG0+1] != 'E') ||
        (eh->e_ident[EI_MAG0+2] != 'L') || (eh->e_ident[EI_MAG0+3] != 'F')) {
        printk("Not an ELF file\n");
        return NOT_OK;
    }

    if (eh->e_ident[EI_CLASS] != ELFCLASS32) {
        printk("Not a 32-bit ELF file\n");
        return NOT_OK;
    }

    if (eh->e_ident[EI_DATA] != ELFDATA2MSB) {
        printk("Not a big-endian ELF file\n");
        return NOT_OK;
    }

    if ((eh->e_ident[EI_VERSION] != EV_CURRENT) || (eh->e_version != EV_CURRENT)) {
        printk("Unsupported ELF version\n");
        return NOT_OK;
    }

    if (eh->e_machine != EM_68K) {
        printk("Not an m68k program (machine type %d)\n", eh->e_machine);
        return NOT_OK;
    }

    if (eh->e_type != ET_EXEC) {
        printk("Not an executable (ELF type %d)\n", eh->e_type);
        return NOT_OK;
    }

    if ((eh->e_phentsize != sizeof(elf32_phdr_t)) || (eh->e_phnum == 0) || (eh->e_phnum > MAX_PHDRS)) {
        printk("Unsupported program headers (%d of %d bytes)\n", eh->e_phnum, eh->e_phentsize);
        return NOT_OK;
    }

    return OK;
}

/*
 * Check a loadable segment fits in the file and in user memory, and
 * doesn't land on anything the monitor is using.
 */
static int check_segment(int index, const elf32_phdr_t *ph, uint32_t file_size) {
    uint32_t start = ph->p_vaddr;
    uint32_t end = start + ph->p_memsz - 1;

    if (ph->p_filesz > ph->p_memsz) {
        printk("Segment %d: file size is bigger than memory size\n", index);
        return NOT_OK;
    }

    if ((ph->p_offset > file_size) || (ph->p_filesz > (file_size - ph->p_offset))) {
        printk("Segment %d: extends past the end of the file\n", index);
        return NOT_OK;
    }

    if ((end < start) || (start < LOAD_MIN) || (end > ram_end)) {
        printk("Segment %d: $%08x-$%08x is outside user memory ($%08x-$%08x)\n",
            index, start, end, LOAD_MIN, ram_end);
        return NOT_OK;
    }

    if (heap_range_in_use(start, end)) {
        printk("Segment %d: $%08x-$%08x overlaps memory the monitor is using\n", index, start, end);
        return NOT_OK;
    }

    return OK;
}

/*
 * Load a big-endian m68k ELF executable from an open file.
 *
 * Everything is checked before anything is written to memory, so a bad
 * file leaves memory untouched. On success, *entry is the start address.
 */
int load_elf(int fd, uint32_t *entry) {
    elf32_ehdr_t eh;
    elf32_phdr_t ph[MAX_PHDRS];
    off_t file_size;
    uint32_t total = 0;
    int num_loaded = 0;
    bool_t entry_ok = NO;
    uint32_t low = 0xffffffffU;
    uint32_t high = 0;

    file_size = lseek(fd, 0, SEEK_END);
    if (file_size == NOT_OK) {
        return NOT_OK;
    }

    if ((file_size < (off_t)sizeof(eh)) || (read_at(fd, 0, &eh, sizeof(eh)) == NOT_OK)) {
        printk("Couldn't read the ELF header\n");
        return NOT_OK;
    }

    if (check_header(&eh) == NOT_OK) {
        errno = ENOEXEC;
        return NOT_OK;
    }

    if ((eh.e_phoff > (uint32_t)file_size) ||
        ((eh.e_phnum * sizeof(elf32_phdr_t)) > ((uint32_t)file_size - eh.e_phoff)) ||
        (read_at(fd, eh.e_phoff, ph, eh.e_phnum * sizeof(elf32_phdr_t)) == NOT_OK)) {
        printk("Couldn't read the program headers\n");
        errno = ENOEXEC;
        return NOT_OK;
    }

    /*
     * The program that was loaded before is about to be replaced: give
     * its memory back, so the new one can go there
     */
    clean_heap(LOADED_PROGRAM_OWNER);
    program_stack_size = 0;

    // Pass 1: check everything before touching memory
    for (int i=0; i<eh.e_phnum; i++) {
        if (ph[i].p_type == PT_GNU_STACK) {
            program_stack_size = ph[i].p_memsz;
            continue;
        }

        if ((ph[i].p_type != PT_LOAD) || (ph[i].p_memsz == 0)) {
            continue;
        }

        if (check_segment(i, &ph[i], file_size) == NOT_OK) {
            errno = ENOEXEC;
            return NOT_OK;
        }

        if ((eh.e_entry >= ph[i].p_vaddr) && (eh.e_entry - ph[i].p_vaddr < ph[i].p_memsz)) {
            entry_ok = YES;
        }

        if (ph[i].p_vaddr < low) {
            low = ph[i].p_vaddr;
        }

        if (ph[i].p_vaddr + ph[i].p_memsz - 1 > high) {
            high = ph[i].p_vaddr + ph[i].p_memsz - 1;
        }
    }

    if (entry_ok == NO) {
        printk("Entry point $%08x isn't in a loaded segment\n", eh.e_entry);
        errno = ENOEXEC;
        return NOT_OK;
    }

    /*
     * Keep the heap off the program's memory until the next program is
     * loaded: the program's own mallocs mustn't land on it, and 'go' can
     * run it again after it exits
     */
    if (heap_claim_range(low, high, LOADED_PROGRAM_OWNER) == NOT_OK) {
        printk("$%08x-$%08x isn't free for the program\n", low, high);
        errno = ENOMEM;
        return NOT_OK;
    }

    // Pass 2: load each segment and zero the rest of it (bss)
    for (int i=0; i<eh.e_phnum; i++) {
        uint8_t *dest = (uint8_t *)ph[i].p_vaddr;

        if ((ph[i].p_type != PT_LOAD) || (ph[i].p_memsz == 0)) {
            continue;
        }

        if (ph[i].p_filesz > 0) {
            if (read_at(fd, ph[i].p_offset, dest, ph[i].p_filesz) == NOT_OK) {
                printk("Segment %d: read failed\n", i);
                return NOT_OK;
            }
        }

        memset(dest + ph[i].p_filesz, 0, ph[i].p_memsz - ph[i].p_filesz);

        // printk("  $%08x-$%08x  %c%c%c  %d bytes\n",
        //     ph[i].p_vaddr, ph[i].p_vaddr + ph[i].p_memsz - 1,
        //     (ph[i].p_flags & PF_R)?'r':'-',
        //     (ph[i].p_flags & PF_W)?'w':'-',
        //     (ph[i].p_flags & PF_X)?'x':'-',
        //     ph[i].p_memsz);

        total += ph[i].p_memsz;
        num_loaded++;
    }

    /*
     * We've just written code with data writes: make sure the
     * instruction cache can't hold stale copies. (The 68030's data
     * cache is write-through, so memory itself is up to date.)
     */
    flush_icache();

    // printk("Loaded %d segment%s, %d bytes. Entry point $%08x\n",
    //     num_loaded, (num_loaded == 1)?"":"s", total, eh.e_entry);

    *entry = eh.e_entry;

    return OK;
}
