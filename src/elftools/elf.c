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
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#include <unistd.h>

#include "ed.h"
#include "elf.h"

static uint8_t endian_mismatch = NO;

static void dump_elf_hdr(elf32_ehdr_t *hdr) {
    static char *Classes[] = {"NONE", "32-Bit", "64-Bit"};
    static char *Endian[] = {"NONE", "LSB First", "MSB First"};

    uint8_t class = hdr->e_ident[EI_CLASS];
    uint8_t data = hdr->e_ident[EI_DATA];

    printf("e_ident: 0x%02x, '%c', '%c', '%c', ", hdr->e_ident[EI_MAG0], hdr->e_ident[EI_MAG1], hdr->e_ident[EI_MAG2], hdr->e_ident[EI_MAG3]);
    printf("%s, ", class>ELFCLASS64?"Unknown":Classes[class]);
    printf("%s, ", data>ELFDATA2MSB?"Unknown":Endian[data]);
    printf("v%d, ", hdr->e_ident[EI_VERSION]);
    
    for (int i=EI_OSABI; i<EI_NIDENT; i++) {
        printf("0x%02x", hdr->e_ident[i]);
        if (i != (EI_NIDENT-1)) {
            printf(", ");
        }
    }
    putchar('\n');

    printf("e_type:      %d\n", hdr->e_type);
    printf("e_machine:   %d\n", hdr->e_machine);
    printf("e_version:   %d\n", hdr->e_version);
    printf("e_entry:     0x%08x\n", hdr->e_entry);
    printf("e_phoff:     %d\n", hdr->e_phoff);
    printf("e_shoff:     %d\n", hdr->e_shoff);
    printf("e_flags:     %d\n", hdr->e_flags);
    printf("e_ehsize:    %d\n", hdr->e_ehsize);
    printf("e_phentsize: %d\n", hdr->e_phentsize);
    printf("e_phnum:     %d\n", hdr->e_phnum);
    printf("e_shentsize: %d\n", hdr->e_shentsize);
    printf("e_shnum:     %d\n", hdr->e_shnum);
    printf("e_shstrndx:  %d\n", hdr->e_shstrndx);
}

static void dump_elf_phdr(elf32_phdr_t *phdr) {
    printf("p_type:      0x%08x\n", phdr->p_type);
    printf("p_offset:    %d\n", phdr->p_offset);
    printf("p_vaddr:     0x%08x\n", phdr->p_vaddr);
    printf("p_paddr:     0x%08x\n", phdr->p_paddr);
    printf("p_filesz:    %d\n", phdr->p_filesz);
    printf("p_memsz:     %d\n", phdr->p_memsz);
    printf("p_flags:     %c%c%c\n", 
        (phdr->p_flags & PF_R)?'r':'-', 
        (phdr->p_flags & PF_W)?'w':'-', 
        (phdr->p_flags & PF_X)?'x':'-' );
    printf("p_align:     %d\n", phdr->p_align);
}

static void dump_elf_shdr(elf32_shdr_t *shdr, uint8_t *names) {
    static char *Types[] = {"NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH",
                            "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM",
                            "*UNDEF*", "*UNDEF*", "INIT_ARRAY", "FINI_ARRAY",
                            "PREINIT_ARRAY", "GROUP", "SYMTAB_SHNDX", "RELR"};

    char sep = ' ';

    printf("sh_name:      %s\n", &names[shdr->sh_name]);
    printf("sh_type:      %s\n", (shdr->sh_type<sizeof(Types))?Types[shdr->sh_type]:"Undefined");
    printf("sh_flags:   ");
    if (shdr->sh_flags & SH_WRITE) {
        printf("%c SH_WRITE", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_ALLOC) {
        printf("%c SH_ALLOC", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_EXECINSTR) {
        printf("%c SH_EXECINSTR", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_MERGE) {
        printf("%c SH_MERGE", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_STRINGS) {
        printf("%c SH_STRINGS", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_INFO_LINK) {
        printf("%c SH_INFO_LINK", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_LINK_ORDER) {
        printf("%c SH_LINK_ORDER", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_OS_NONCONFORMING) {
        printf("%c SH_OS_NONCONFORMING", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_GROUP) {
        printf("%c SH_GROUP", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_TLS) {
        printf("%c SH_TLS", sep);
        sep = ',';
    }
    
    if (shdr->sh_flags & SH_COMPRESSED) {
        printf("%c SH_COMPRESSED", sep);
    }

    putchar('\n');
    
    printf("sh_addr:      0x%08x\n", shdr->sh_addr);
    printf("sh_offset:    %d\n", shdr->sh_offset);
    printf("sh_size:      %d\n", shdr->sh_size);
    printf("sh_link:      %d\n", shdr->sh_link);
    printf("sh_info:      %d\n", shdr->sh_info);
    printf("sh_addralign: %d\n", shdr->sh_addralign);
    printf("sh_entsize:   %d\n", shdr->sh_entsize);
}

static void sanitize_elf_hdr(elf32_ehdr_t *hdr) {
    if ((is_little_endian() && hdr->e_ident[EI_DATA] == ELFDATA2MSB) || 
        (!is_little_endian() && hdr->e_ident[EI_DATA] == ELFDATA2LSB)) {
        endian_mismatch = YES;
    }

    if (endian_mismatch) {
        hdr->e_type = __builtin_bswap16(hdr->e_type);
        hdr->e_machine = __builtin_bswap16(hdr->e_machine);
        hdr->e_version = __builtin_bswap32(hdr->e_version);
        hdr->e_entry = __builtin_bswap32(hdr->e_entry);
        hdr->e_phoff = __builtin_bswap32(hdr->e_phoff);
        hdr->e_shoff = __builtin_bswap32(hdr->e_shoff);
        hdr->e_flags = __builtin_bswap32(hdr->e_flags);
        hdr->e_ehsize = __builtin_bswap16(hdr->e_type);
        hdr->e_phentsize = __builtin_bswap16(hdr->e_phentsize);
        hdr->e_phnum = __builtin_bswap16(hdr->e_phnum);
        hdr->e_shentsize = __builtin_bswap16(hdr->e_shentsize);
        hdr->e_shnum = __builtin_bswap16(hdr->e_shnum);
        hdr->e_shstrndx = __builtin_bswap16(hdr->e_shstrndx);
    }
}

static void sanitize_elf_phdr(elf32_phdr_t *phdr) {
    if (endian_mismatch == YES) {
        phdr->p_type = __builtin_bswap32(phdr->p_type);
        phdr->p_offset = __builtin_bswap32(phdr->p_offset);
        phdr->p_vaddr = __builtin_bswap32(phdr->p_vaddr);
        phdr->p_paddr = __builtin_bswap32(phdr->p_paddr);
        phdr->p_filesz = __builtin_bswap32(phdr->p_filesz);
        phdr->p_memsz = __builtin_bswap32(phdr->p_memsz);
        phdr->p_flags = __builtin_bswap32(phdr->p_flags);
        phdr->p_align = __builtin_bswap32(phdr->p_align);
    }
}

static void sanitize_elf_shdr(elf32_shdr_t *shdr) {
    if (endian_mismatch == YES) {
        shdr->sh_name = __builtin_bswap32(shdr->sh_name);
        shdr->sh_type = __builtin_bswap32(shdr->sh_type);
        shdr->sh_flags = __builtin_bswap32(shdr->sh_flags);
        shdr->sh_addr = __builtin_bswap32(shdr->sh_addr);
        shdr->sh_offset = __builtin_bswap32(shdr->sh_offset);
        shdr->sh_size = __builtin_bswap32(shdr->sh_size);
        shdr->sh_link = __builtin_bswap32(shdr->sh_link);
        shdr->sh_info = __builtin_bswap32(shdr->sh_info);
        shdr->sh_addralign = __builtin_bswap32(shdr->sh_addralign);
        shdr->sh_entsize = __builtin_bswap32(shdr->sh_entsize);
    }
}

static int valid_header(elf32_ehdr_t *hdr) {
    if ((hdr->e_ident[EI_MAG0] == ELFMAG0) &&
        (hdr->e_ident[EI_MAG1] == ELFMAG1) &&
        (hdr->e_ident[EI_MAG2] == ELFMAG2) &&
        (hdr->e_ident[EI_MAG3] == ELFMAG3) &&
        (hdr->e_ident[EI_VERSION] == EV_CURRENT) &&
        (hdr->e_ident[EI_PAD] == 0) &&
        (hdr->e_ident[EI_PAD+1] == 0) &&
        (hdr->e_ident[EI_PAD+2] == 0) &&
        (hdr->e_ident[EI_PAD+3] == 0) &&
        (hdr->e_ident[EI_PAD+4] == 0) &&
        (hdr->e_ident[EI_PAD+5] == 0) &&
        (hdr->e_ident[EI_PAD+6] == 0) ) {
            return YES;
        }

    return NO;
}

static int target_is_68k(elf32_ehdr_t *hdr) {
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        printf("Not a 32bit ELF header\n");
        return NO;
    }

    if (hdr->e_ident[EI_DATA] != ELFDATA2MSB) {
        printf("Wrong byte order\n");
        return NO;
    }

    if (hdr->e_type != ET_EXEC) {
        printf("Not an executable format\n");
        return NO;
    }

    if (hdr->e_machine != EM_68K) {
        printf("DEoesn't target 68k\n");
        return NO;
    }

    return YES;
}

static int read_elf_phdr(int fd, elf32_phdr_t *phdr) {
    size_t res = read(fd, phdr, sizeof(elf32_phdr_t));
    if (res != sizeof(elf32_phdr_t)) {
        return NOT_OK;
    }

    sanitize_elf_phdr(phdr);

    return OK;
}

static int read_elf_shdr(int fd, elf32_shdr_t *shdr) {
    size_t res = read(fd, shdr, sizeof(elf32_shdr_t));
    if (res != sizeof(elf32_shdr_t)) {
        return NOT_OK;
    }

    sanitize_elf_shdr(shdr);

    return OK;
}

static int read_elf_hdr(int fd, elf32_ehdr_t *hdr) {
    size_t res = read(fd, hdr, sizeof(elf32_ehdr_t));
    if (res != sizeof(elf32_ehdr_t)) {
        return NOT_OK;
    }

    sanitize_elf_hdr(hdr);

    return OK;
}

int load_elf(int fd) {
    elf32_ehdr_t hdr;
    elf32_phdr_t phdr;
    elf32_shdr_t shdr;
    elf32_shdr_t strtab;
    Elf32_Word offs;
    uint8_t *names;

    if (read_elf_hdr(fd, &hdr) != OK) {
        printf("Failed to read ELF header.\n");
        return NOT_OK;
    }

    printf("ELF Header\n"
           "----------\n");
    dump_elf_hdr(&hdr);

    if (valid_header(&hdr) == NO) {
        printf("No ELF header found\n");
        return NOT_OK;
    }

    // Check that it's m68k elf
    if (target_is_68k(&hdr) == NO) {
        printf("ELF header not compatible with target platform\n");
        return NOT_OK;
    }

    // So far so good, let's take a look at the Program header table
    if (lseek(fd, hdr.e_phoff, SEEK_SET) != hdr.e_phoff) {
        printf("Failed to move to start of program header table\n");
        return NOT_OK;
    }

    printf("\nProgram Header Table\n"
             "--------------------\n");
    for (int i=0; i<hdr.e_phnum; i++) {
        printf("\nProgram header #%d\n", i);

        if (read_elf_phdr(fd, &phdr) != OK) {
            printf("...problem reading entry\n");
            return NOT_OK;
        }

        dump_elf_phdr(&phdr);
    }

    // Read the Section that contains the names of the sections
    offs = hdr.e_shoff + (hdr.e_shentsize * hdr.e_shstrndx);
    if (lseek(fd, offs, SEEK_SET) != offs) {
        printf("Failed to read the section names strtab\n");
        return NOT_OK;
    }

    if (read_elf_shdr(fd, &strtab) != OK) {
        printf("...problem reading section names strtab\n");
        return NOT_OK;
    }

    if (lseek(fd, strtab.sh_offset, SEEK_SET) != strtab.sh_offset) {
        printf("Failed to move to start of section names strtab\n");
        return NOT_OK;
    }

    names = malloc(strtab.sh_size);
    if (read(fd, names, strtab.sh_size) != strtab.sh_size) {
        printf("Failed to read section name data\n");
        free(names);

        return NOT_OK;
    }

    // Section header table
    if (lseek(fd, hdr.e_shoff, SEEK_SET) != hdr.e_shoff) {
        printf("Failed to move to start of section header table\n");
        free(names);

        return NOT_OK;
    }

    printf("\nSection Header Table\n"
             "--------------------\n");
    for (int i=0; i<hdr.e_shnum; i++) {
        printf("\nSection header #%d\n", i);

        if (read_elf_shdr(fd, &shdr) != OK) {
            printf("...problem reading entry\n");
            free(names);

            return NOT_OK;
        }

        dump_elf_shdr(&shdr, names);
    }

    free(names);

    return OK;
}