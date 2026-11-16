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
#include <malloc.h>
#include <errno.h>
#include <stddef.h>
#include <fcntl.h>

#define STACK_SIZE 65536

extern void setSSP(uint8_t *new_ssp);
extern void invoke_kernel(void);

int gobble_file(int fd, void *dest, off_t num_bytes) {
    char *p = (char *)dest;

    lseek(fd, 0, SEEK_SET);

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

// This function is running under the new stack
void do_the_magic(uint8_t *src, off_t num_bytes) {
    uint8_t *mem = (uint8_t *)0x2000;

    for (int i=0; i<num_bytes; i++) {
        *mem++ = *src++;
    }

    invoke_kernel();
}


// These must be statics or globals so they still work when we switch stacks!
uint8_t *kernel;
off_t size;

void main(int argc, char *argv[]) {
    uint8_t *stack_area = malloc(STACK_SIZE);
    uint8_t *ssp;
    int fd;

    if (stack_area == NULL) {
        printf("Error - couldn't allocate space for temporary stack\n");
        exit(2);
    }

    if (argc != 2) {
        printf("usage: netbsd <filename>\n");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("Couldn't open file '%s'\n", argv[1]);
        exit(3);
    }

    size = lseek(fd, 0, SEEK_END);
    printf("Kernel image size: %d\n", size);
    kernel = malloc(size);
    if (kernel == NULL) {
        printf("Coiuldn't allocate enough memory to hold kernel");
        exit(4);
    }

    if (gobble_file(fd, kernel, size) != OK) {
        printf("Failed to read the kernel into memory");
        exit(5);
    }

    close(fd);

    ssp = &stack_area[STACK_SIZE];
    printf("Switching system stack to $%08x\n", ssp);

    // There's no going back from here in...
    setSSP(ssp);
    do_the_magic(kernel, size);
}