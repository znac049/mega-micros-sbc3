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
 * netbsd [<kernel> [<device tree blob>]]
 *
 * Loads a big-endian m68k ELF kernel, and optionally a device tree blob,
 * and jumps to the kernel's entry point.
 *
 * The kernel's loadable segments are placed at their physical addresses
 * (p_paddr), with the gaps and the BSS zeroed. hb68k's fdt_bootstrap1()
 * expects the device tree to start at end[], the end of the kernel's BSS,
 * so that's where the blob goes. (A kernel built with
 * CONFIG_STATIC_DEVICE_TREE ignores it.)
 *
 * Whichever device tree the kernel will use - the one given here, or the
 * one built into the kernel (found through its dt_blob_start symbol) - has
 * its /memory "reg" entries patched to match the RAM the monitor found at
 * boot: each entry whose base is a bank's base gets that bank's size, and
 * any other entry (or one for a bank that's missing or failed its test) is
 * removed. Build the device tree with the largest RAM you might fit.
 *
 * Everything is staged in one block from the monitor's heap, so none of it
 * can land on memory the monitor is still using (its open files and mount
 * tables live at the top of the heap, which is the top of bank 0 when
 * bank 1 isn't fitted). The heap hands out memory from the top down, so the
 * block normally sits well clear of this program and of the kernel's load
 * address; both are checked before anything is written. The block is:
 *
 *      image           <- kernel (with its BSS) and device tree staged here
 *      trampoline      <- copy of boot.asm's code
 *      boot stack      <- trampoline's stack
 *
 * The trampoline copies the image down to the kernel's load address and
 * jumps to it. That copy overwrites the monitor and this program, which is
 * why the trampoline runs from up here, with its own stack and interrupts off.
 *
 * In addition, MicroMon puts the CompactFlash card into 8-bit transfer mode (SET
 * FEATURES $01).  The card keeps that mode until it's power-cycled or
 * reset, and NetBSD's wdc driver reads the data register 16 bits at a
 * time and can't reset the card itself (that needs /CE2, which the SBC3
 * doesn't wire up).  In 8-bit mode each 16-bit read returns a single
 * byte, so the kernel's IDENTIFY comes back as garbage and wd0 fails.
 * Put the card back into 16-bit mode before starting the kernel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>

#include <bios.h>

#define NUM_BANKS       2

#define BOOT_STACK      1024            // trampoline's stack

#define ALIGN4(x)       (((x) + 3) & ~3UL)

#define MAX_PHDRS       16              // more than any kernel needs
#define FDT_MAGIC       0xd00dfeedUL

// Flattened device tree structure tokens
#define FDT_BEGIN_NODE  1
#define FDT_END_NODE    2
#define FDT_PROP        3
#define FDT_NOP         4
#define FDT_END         9

#define DT_BLOB_SYMBOL  "dt_blob_start"


/*
 * Just enough ELF. The file is big-endian, like the 68030, so the headers
 * can be read straight into these structs.
 */
#define EI_NIDENT       16
#define ELFCLASS32      1
#define ELFDATA2MSB     2
#define ET_EXEC         2
#define EM_68K          4
#define PT_LOAD         1
#define SHT_SYMTAB      2

typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf32_phdr_t;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} elf32_shdr_t;

typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
} elf32_sym_t;

typedef struct {
    uint32_t base;
    uint32_t size;                      // 0 if missing (or failed its test)
} ram_bank_t;

// The header at the start of every device tree blob (all big-endian)
typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header_t;


#define CF_REG(n)       (*(volatile uint8_t *)(0xAE0000 + 2 * (n)))
#define CF_FEATURE      CF_REG(1)
#define CF_DRIVE_HEAD   CF_REG(6)
#define CF_COMMAND      CF_REG(7)
#define CF_STATUS       CF_REG(7)
#define CF_ST_BUSY      0x80
#define CF_ST_ERROR     0x01
#define CF_CMD_SET_FEATURES     0xEF
#define CF_FEATURE_16BIT        0x81    /* disable 8-bit data transfers */


// From boot.asm - treated as bytes to copy, never called in place
extern const uint8_t boot_trampoline[];
extern const uint8_t boot_trampoline_end[];

// End of this program (from the link script)
extern uint8_t _bss_end[];

typedef void (*trampoline_t)(uint32_t len, const void *src, void *dest, void *entry, void *new_sp);


static int cf_wait_not_busy(void) {
    for (long i = 0; i < 1000000; i++) {
        if (!(CF_STATUS & CF_ST_BUSY)) {
            return OK;
        }
    }
    return NOT_OK;
}

static void cf_restore_16bit(void) {
    CF_DRIVE_HEAD = 0xE0;               /* select the master */
    if (cf_wait_not_busy() != OK) {
        printf("CF: card busy, not switching back to 16-bit mode\n");
        return;
    }
    CF_FEATURE = CF_FEATURE_16BIT;
    CF_COMMAND = CF_CMD_SET_FEATURES;
    for (int i = 0; i < 5; i++) {       /* BSY can take 400 ns to appear */
        (void)CF_STATUS;
    }
    if (cf_wait_not_busy() != OK || (CF_STATUS & CF_ST_ERROR)) {
        printf("CF: couldn't switch the card back to 16-bit mode (status $%02x)\n",
            CF_STATUS);
    }
}

static int read_fully(int fd, uint8_t *dest, uint32_t num_bytes) {
    while (num_bytes > 0) {
        int got = read(fd, dest, num_bytes);

        if (got <= 0) {
            if (got == 0) {
                errno = EIO;            // file shorter than lseek said
            }

            return NOT_OK;
        }

        dest += got;
        num_bytes -= got;
    }

    return OK;
}

static int read_at(int fd, off_t offset, void *dest, uint32_t num_bytes) {
    if (lseek(fd, offset, SEEK_SET) != offset) {
        return NOT_OK;
    }

    return read_fully(fd, (uint8_t *)dest, num_bytes);
}

// Check the ELF header and read the program headers
static int read_elf_headers(int fd, elf32_ehdr_t *eh, elf32_phdr_t *ph) {
    if (read_at(fd, 0, eh, sizeof(*eh)) != OK) {
        printf("Couldn't read the ELF header (errno=%d)\n", errno);
        return NOT_OK;
    }

    if ((memcmp(eh->e_ident, "\177ELF", 4) != 0) ||
        (eh->e_ident[4] != ELFCLASS32) ||
        (eh->e_ident[5] != ELFDATA2MSB)) {
        printf("Not a 32-bit big-endian ELF file\n");
        return NOT_OK;
    }

    if ((eh->e_type != ET_EXEC) || (eh->e_machine != EM_68K)) {
        printf("Not an m68k executable (type %u, machine %u)\n", eh->e_type, eh->e_machine);
        return NOT_OK;
    }

    if ((eh->e_phnum == 0) || (eh->e_phnum > MAX_PHDRS) ||
        (eh->e_phentsize != sizeof(elf32_phdr_t))) {
        printf("Unexpected program headers (%u of %u bytes)\n", eh->e_phnum, eh->e_phentsize);
        return NOT_OK;
    }

    if (read_at(fd, eh->e_phoff, ph, eh->e_phnum * sizeof(elf32_phdr_t)) != OK) {
        printf("Couldn't read the program headers (errno=%d)\n", errno);
        return NOT_OK;
    }

    return OK;
}

/*
 * Find a symbol's value in the ELF symbol table, reading the file a chunk
 * at a time (the tables are far too big to read in one go). First find
 * where the name is in the string table, then the symbol that uses it.
 *
 * The linker shares string table entries between names that end the same
 * way, so "dt_blob_start" may only exist as the tail of "_dt_blob_start".
 * Any place the name appears followed by a NUL is a candidate.
 */
#define MAX_CANDIDATES  8

static int find_symbol(int fd, const elf32_ehdr_t *eh, const char *name, uint32_t *value) {
    static uint8_t chunk[1024];
    elf32_shdr_t symtab, strtab;
    uint32_t candidates[MAX_CANDIDATES];
    int num_candidates = 0;
    size_t plen = strlen(name) + 1;     // including the NUL

    if ((eh->e_shoff == 0) || (eh->e_shentsize != sizeof(elf32_shdr_t)) ||
        (plen > sizeof(chunk) / 2)) {
        return NOT_OK;
    }

    // The section holding the symbol table, and its string table
    for (int i = 0; ; i++) {
        if (i >= eh->e_shnum) {
            return NOT_OK;              // stripped
        }

        if (read_at(fd, eh->e_shoff + i * sizeof(elf32_shdr_t), &symtab, sizeof(symtab)) != OK) {
            return NOT_OK;
        }

        if (symtab.sh_type == SHT_SYMTAB) {
            break;
        }
    }

    if ((symtab.sh_link >= eh->e_shnum) ||
        (symtab.sh_entsize != sizeof(elf32_sym_t)) ||
        (read_at(fd, eh->e_shoff + symtab.sh_link * sizeof(elf32_shdr_t), &strtab, sizeof(strtab)) != OK)) {
        return NOT_OK;
    }

    // Chunks overlap by plen - 1 bytes, so a match can't straddle two
    for (uint32_t pos = 0; pos < strtab.sh_size; pos += sizeof(chunk) - (plen - 1)) {
        uint32_t n = strtab.sh_size - pos;

        if (n > sizeof(chunk)) {
            n = sizeof(chunk);
        }

        if (n < plen) {
            break;
        }

        if (read_at(fd, strtab.sh_offset + pos, chunk, n) != OK) {
            return NOT_OK;
        }

        for (uint32_t i = 0; i + plen <= n; i++) {
            if ((memcmp(&chunk[i], name, plen) == 0) && (num_candidates < MAX_CANDIDATES)) {
                candidates[num_candidates++] = pos + i;
            }
        }

        if (n < sizeof(chunk)) {
            break;                      // that was the last chunk
        }
    }

    if (num_candidates == 0) {
        return NOT_OK;
    }

    // Now the symbol with that name
    for (uint32_t pos = 0; pos < symtab.sh_size; pos += sizeof(chunk)) {
        uint32_t n = symtab.sh_size - pos;

        if (n > sizeof(chunk)) {
            n = sizeof(chunk);
        }

        if (read_at(fd, symtab.sh_offset + pos, chunk, n) != OK) {
            return NOT_OK;
        }

        for (uint32_t i = 0; i + sizeof(elf32_sym_t) <= n; i += sizeof(elf32_sym_t)) {
            const elf32_sym_t *sym = (const elf32_sym_t *)&chunk[i];

            for (int c = 0; c < num_candidates; c++) {
                if (sym->st_name == candidates[c]) {
                    *value = sym->st_value;
                    return OK;
                }
            }
        }
    }

    return NOT_OK;
}

// Is there a plausible device tree here, no bigger than max_size?
static int fdt_check(const uint8_t *blob, uint32_t max_size) {
    const fdt_header_t *h = (const fdt_header_t *)blob;

    if ((max_size < sizeof(fdt_header_t)) || (h->magic != FDT_MAGIC)) {
        return NOT_OK;
    }

    if ((h->version < 17) || (h->totalsize > max_size) ||
        (h->off_dt_struct + h->size_dt_struct > h->totalsize) ||
        (h->off_dt_strings + h->size_dt_strings > h->totalsize) ||
        (h->off_dt_struct & 3)) {
        return NOT_OK;
    }

    return OK;
}

static int is_memory_node(const char *name) {
    return ((strcmp(name, "memory") == 0) || (strncmp(name, "memory@", 7) == 0))?YES:NO;
}

/*
 * Rewrite one /memory node's "reg" property. Each (base, size) entry that
 * starts at a bank's base gets the bank's size; every other entry is
 * dropped. The entries that stay are moved to the front, the property's
 * length is reduced to match, and the freed space (a multiple of 8 bytes)
 * is filled with FDT_NOP tokens, so the blob stays the same size.
 */
static uint32_t patch_reg(uint32_t *len_p, uint32_t *cells, const ram_bank_t *banks) {
    uint32_t n = *len_p / 8;
    uint32_t kept = 0;

    for (uint32_t i = 0; i < n; i++) {
        uint32_t base = cells[2 * i];
        uint32_t size = cells[2 * i + 1];
        int bank;

        for (bank = 0; bank < NUM_BANKS; bank++) {
            if ((banks[bank].size != 0) && (banks[bank].base == base)) {
                break;
            }
        }

        if (bank == NUM_BANKS) {
            printf("  Removed:  $%08x, %u KB (no RAM there)\n", base, (unsigned)(size / 1024));
            continue;
        }

        printf("  Bank %d:   $%08x, %u KB", bank, base, (unsigned)(banks[bank].size / 1024));
        if (size != banks[bank].size) {
            printf(" (was %u KB)", (unsigned)(size / 1024));
        }
        printf("\n");

        cells[2 * kept] = base;
        cells[2 * kept + 1] = banks[bank].size;
        kept++;
    }

    // Two cells (8 bytes) per dropped entry become two NOP tokens
    for (uint32_t i = 2 * kept; i < 2 * n; i++) {
        cells[i] = FDT_NOP;
    }

    *len_p = kept * 8;

    return kept;
}

/*
 * Walk the device tree, patching the "reg" property of each /memory node.
 * A node left with no memory at all is overwritten with NOP tokens, which
 * removes it. Only one address cell and one size cell are supported, which
 * is what the SBC3's device trees use.
 */
static int fdt_patch_memory(uint8_t *blob, const ram_bank_t *banks) {
    const fdt_header_t *h = (const fdt_header_t *)blob;
    uint8_t *p = blob + h->off_dt_struct;
    uint8_t *end = p + h->size_dt_struct;
    const char *strings = (const char *)blob + h->off_dt_strings;
    uint32_t addr_cells = 2, size_cells = 1;        // the defaults, if not given
    uint8_t *node_start = NULL;         // the /memory node we're in, if any
    uint32_t node_kept = 0;
    int depth = 0, kept = 0, reg_found = NO;

    while (p + 4 <= end) {
        uint32_t token = *(uint32_t *)p;

        p += 4;

        switch (token) {
            case FDT_BEGIN_NODE: {
                const char *name = (const char *)p;
                size_t len = strlen(name) + 1;

                depth++;
                // The root is at depth 1; /memory nodes are its children
                if ((depth == 2) && is_memory_node(name)) {
                    node_start = p - 4;
                    node_kept = 0;
                }
                p += ALIGN4(len);
                break;
            }

            case FDT_END_NODE:
                if ((node_start != NULL) && (depth == 2)) {
                    if (node_kept == 0) {
                        // Nothing left in it: NOP out the whole node
                        for (uint32_t *t = (uint32_t *)node_start; t < (uint32_t *)p; t++) {
                            *t = FDT_NOP;
                        }
                    }
                    node_start = NULL;
                }
                depth--;
                break;

            case FDT_PROP: {
                uint32_t *len_p = (uint32_t *)p;
                uint32_t len = len_p[0];
                const char *pname = strings + len_p[1];
                uint32_t *value = (uint32_t *)(p + 8);

                if ((depth == 1) && (len == 4)) {
                    if (strcmp(pname, "#address-cells") == 0) {
                        addr_cells = value[0];
                    }
                    else if (strcmp(pname, "#size-cells") == 0) {
                        size_cells = value[0];
                    }
                }
                else if ((node_start != NULL) && (depth == 2) && (strcmp(pname, "reg") == 0)) {
                    if ((addr_cells != 1) || (size_cells != 1) || (len % 8)) {
                        printf("Device tree: can't patch memory with %u address and %u size cells\n",
                            addr_cells, size_cells);
                        return NOT_OK;
                    }

                    node_kept += patch_reg(len_p, value, banks);
                    kept += node_kept;
                    reg_found = YES;
                }

                p += 8 + ALIGN4(len);
                break;
            }

            case FDT_NOP:
                break;

            case FDT_END:
                if (reg_found == NO) {
                    printf("Device tree: no /memory node to patch\n");
                    return NOT_OK;
                }

                if (kept == 0) {
                    printf("Device tree: none of its memory matches the RAM fitted\n");
                    return NOT_OK;
                }

                return OK;

            default:
                printf("Device tree: bad token $%08x\n", token);
                return NOT_OK;
        }
    }

    printf("Device tree: no end token\n");
    return NOT_OK;
}

int main(int argc, char *argv[]) {
    static elf32_phdr_t ph[MAX_PHDRS];
    elf32_ehdr_t eh;
    uint32_t tramp_size = boot_trampoline_end - boot_trampoline;
    uint32_t lo = 0xffffffffUL, hi = 0;
    uint32_t dtb_size = 0, size, image, tramp, stack_top;
    uint8_t *blob = NULL, *builtin = NULL;
    ram_bank_t banks[NUM_BANKS];
    int fd, dtb_fd = -1;
    char *kernel = "/rom0/boot/netbsd";
    char *dtb = NULL;

    if (argc > 3) {
        printf("usage: netbsd [<kernel ELF file> [<device tree blob>]]\n");
        return 1;
    }

    if (argc >= 2) {
        kernel = argv[1];
    }

    if (argc == 3) {
        dtb = argv[2];
    }

    // What RAM did the monitor find?
    for (int b = 0; b < NUM_BANKS; b++) {
        if (syscall(BIOS_RAM_BANK, b, (int)&banks[b].base, (int)&banks[b].size) != OK) {
            printf("The monitor can't report its RAM: is it out of date?\n");
            return 2;
        }
    }

    printf("Attempting to boot '%s'...\n", kernel);
    fd = open(kernel, O_RDONLY);
    if (fd < 0) {
        printf("Couldn't open '%s' (errno=%d)\n", kernel, errno);
        return 3;
    }

    if (read_elf_headers(fd, &eh, ph) != OK) {
        close(fd);
        return 3;
    }

    // Find the span of memory the loadable segments cover
    for (int i = 0; i < eh.e_phnum; i++) {
        if ((ph[i].p_type != PT_LOAD) || (ph[i].p_memsz == 0)) {
            continue;
        }

        if (ph[i].p_filesz > ph[i].p_memsz) {
            printf("Segment %d is bigger in the file than in memory\n", i);
            close(fd);
            return 3;
        }

        if (ph[i].p_paddr < lo) {
            lo = ph[i].p_paddr;
        }

        if (ph[i].p_paddr + ph[i].p_memsz > hi) {
            hi = ph[i].p_paddr + ph[i].p_memsz;
        }
    }

    if (hi == 0) {
        printf("No loadable segments in '%s'\n", kernel);
        close(fd);
        return 3;
    }

    if ((eh.e_entry < lo) || (eh.e_entry >= hi)) {
        printf("Entry point $%08x is outside the kernel ($%08x-$%08x)\n", eh.e_entry, lo, hi);
        close(fd);
        return 3;
    }

    // The device tree goes at end[], straight after the kernel's BSS
    if (dtb != NULL) {
        off_t end;

        dtb_fd = open(dtb, O_RDONLY);
        if (dtb_fd < 0) {
            printf("Couldn't open '%s' (errno=%d)\n", dtb, errno);
            close(fd);
            return 3;
        }

        end = lseek(dtb_fd, 0, SEEK_END);
        if ((end <= 0) || (lseek(dtb_fd, 0, SEEK_SET) != 0)) {
            printf("Couldn't find the size of '%s'\n", dtb);
            close(dtb_fd);
            close(fd);
            return 3;
        }

        dtb_size = (uint32_t)end;

        if (hi & 7) {
            printf("Warning: the kernel ends at $%08x, which isn't 8-byte aligned;\n"
                   "         the kernel may not accept a device tree there\n", hi);
        }
    }

    size = (hi - lo) + dtb_size;

    printf("Kernel: $%08x-$%08x, entry $%08x\n", lo, hi, eh.e_entry);

    /*
     * One block for the image, the trampoline and its stack. The image is
     * 8-byte aligned (the device tree at its end may need it), and the
     * block isn't freed: if we get as far as using it, we never come back,
     * and if we don't, the monitor frees it when this program exits.
     */
    {
        uint32_t want = 8 + ALIGN4(size) + ALIGN4(tramp_size) + BOOT_STACK;
        uint8_t *block = malloc(want);

        if (block == NULL) {
            printf("Not enough free memory to stage the kernel (%u bytes needed)\n", want);
            if (dtb_fd >= 0) {
                close(dtb_fd);
            }
            close(fd);
            return 4;
        }

        image = ((uint32_t)block + 7) & ~7UL;
        tramp = image + ALIGN4(size);
        stack_top = (tramp + ALIGN4(tramp_size) + BOOT_STACK) & ~3UL;
    }

    // It must clear both this program and where the kernel will be copied to
    if ((image < (uint32_t)_bss_end) || (image < lo + size)) {
        printf("The staging area ($%08x) overlaps this program or the kernel's\n"
               "destination ($%08x-$%08x): not enough free memory\n", image, lo, lo + size);
        if (dtb_fd >= 0) {
            close(dtb_fd);
        }
        close(fd);
        return 4;
    }

    // Zero everything first: that covers the BSS and any gaps between segments
    memset((void *)image, 0, hi - lo);

    for (int i = 0; i < eh.e_phnum; i++) {
        if ((ph[i].p_type != PT_LOAD) || (ph[i].p_filesz == 0)) {
            continue;
        }

        if (read_at(fd, ph[i].p_offset, (void *)(image + (ph[i].p_paddr - lo)), ph[i].p_filesz) != OK) {
            printf("Failed to read segment %d (errno=%d)\n", i, errno);
            if (dtb_fd >= 0) {
                close(dtb_fd);
            }
            close(fd);
            return 5;
        }
    }

    if (dtb != NULL) {
        // The device tree given on the command line, at end[]
        blob = (uint8_t *)(image + (hi - lo));

        if (read_fully(dtb_fd, blob, dtb_size) != OK) {
            printf("Failed to read the device tree (errno=%d)\n", errno);
            close(dtb_fd);
            close(fd);
            return 5;
        }

        close(dtb_fd);

        if (fdt_check(blob, dtb_size) != OK) {
            printf("'%s' isn't a device tree blob\n", dtb);
            close(fd);
            return 5;
        }
    }

    /*
     * The device tree built into the kernel, if it has one. A kernel built
     * with CONFIG_STATIC_DEVICE_TREE uses that one even if a blob was given,
     * so patch both.
     */
    {
        uint32_t addr;

        if (find_symbol(fd, &eh, DT_BLOB_SYMBOL, &addr) == OK) {
            if ((addr < lo) || (addr >= hi) ||
                (fdt_check((uint8_t *)(image + (addr - lo)), hi - addr) != OK)) {
                printf("%s ($%08x) isn't a device tree\n", DT_BLOB_SYMBOL, addr);
                close(fd);
                return 5;
            }

            builtin = (uint8_t *)(image + (addr - lo));
        }
        else if (blob == NULL) {
            printf("No %s symbol (is the kernel stripped?): not patching its memory\n",
                DT_BLOB_SYMBOL);
        }
    }

    close(fd);

    if (builtin != NULL) {
        printf("Device tree (built in, at $%08x):\n", (unsigned)(lo + (builtin - (uint8_t *)image)));
        if (fdt_patch_memory(builtin, banks) != OK) {
            return 6;
        }
    }

    if (blob != NULL) {
        printf("Device tree (%s, at $%08x):\n", dtb, hi);
        if (fdt_patch_memory(blob, banks) != OK) {
            return 6;
        }
    }

    cf_restore_16bit();

    memcpy((void *)tramp, boot_trampoline, tramp_size);

    printf("Starting kernel...\n");

    // Console output is sent under interrupts, which the trampoline turns
    // off: make sure it has all gone before the monitor is overwritten
    fflush(NULL);

    // There's no coming back from here
    ((trampoline_t)tramp)(size, (void *)image, (void *)lo,
                                     (void *)eh.e_entry, (void *)stack_top);

    return 0;                           // not reached
}
