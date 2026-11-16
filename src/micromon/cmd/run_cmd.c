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
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <machine.h>

#include "micromon.h"

/*
 * Load a big-endian m68k ELF executable from a file and run it. The
 * program gets argv[0] = <file>, followed by any further arguments.
 */
void handle_run_command(int argc, char *argv[]) {
    uint32_t entry;
    int fd;
    int res;

    if (argc < 2) {
        printk("usage: run <file> [<args> ...]\n");
        return;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == NOT_OK) {
        printk("%s: can't open file. errno=%d\n", argv[1], errno);
        return;
    }

    res = load_elf(fd, &entry);
    close(fd);

    if (res == NOT_OK) {
        printk("%s: not loaded\n", argv[1]);
        return;
    }

    // 'go' on its own will now re-run this program
    snprintf(program_name, sizeof(program_name), "%s", argv[1]);
    go_address = entry;

    // Tweak argv so it contains program name plus all args after the filename
    argv[0] = program_name;
    for (int i=1; i<argc-1; i++) {
        argv[i] = argv[i+1];
    }
    argc--;

    go_run(argc, argv);
}
