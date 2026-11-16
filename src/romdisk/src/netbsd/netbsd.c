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
 * netbsd <kernel image>
 *
 * Loads a raw kernel image, copies it to KERNEL_LOAD and jumps to it.
 *
 * Memory is tight, so the image isn't malloc'd: the heap's first free block
 * starts just above the monitor's data, so a big malloc() would cover the
 * monitor's stack and this program (at $40000), and reading the kernel into
 * it would overwrite the code doing the reading.
 *
 * Instead, top of RAM is laid out as:
 *
 *      image           <- kernel file is read here
 *      trampoline      <- copy of boot.asm's code
 *      boot stack      <- trampoline's stack, ending at RAM_TOP
 *
 * The trampoline copies the image down to KERNEL_LOAD and jumps to it. That
 * copy overwrites the monitor and this program, which is why the trampoline
 * runs from up here, with its own stack and interrupts off.
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

#ifndef RAM_TOP
#define RAM_TOP         0x400000        // first address past the end of RAM (4MB)
#endif

#ifndef KERNEL_LOAD
#define KERNEL_LOAD     0x2000          // where the kernel is linked to run
#endif

#define KERNEL_ENTRY    KERNEL_LOAD
#define BOOT_STACK      1024            // trampoline's stack

#define ALIGN4(x)       (((x) + 3) & ~3UL)


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

int main(int argc, char *argv[]) {
    uint32_t tramp_size = boot_trampoline_end - boot_trampoline;
    uint32_t size, image, tramp;
    off_t end;
    int fd;

    if (argc != 2) {
        printf("usage: netbsd <kernel image>\n");
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("Couldn't open '%s' (errno=%d)\n", argv[1], errno);
        return 3;
    }

    end = lseek(fd, 0, SEEK_END);
    if ((end <= 0) || (lseek(fd, 0, SEEK_SET) != 0)) {
        printf("Couldn't find the size of '%s'\n", argv[1]);
        close(fd);
        return 3;
    }

    size = (uint32_t)end;

    // Work down from the top of RAM
    tramp = RAM_TOP - BOOT_STACK - ALIGN4(tramp_size);
    image = (tramp - size) & ~3UL;

    printf("Kernel image: %u bytes, staged at $%08x, will run at $%08x\n", size, image, KERNEL_LOAD);

    // The staged image must clear both this program and the kernel's destination
    if ((size > tramp) || (image < (uint32_t)_bss_end) || (image < KERNEL_LOAD + size)) {
        printf("Kernel image is too big to stage (largest is about %u bytes)\n",
            (unsigned)((tramp - KERNEL_LOAD) / 2));
        close(fd);
        return 4;
    }

    if (read_fully(fd, (uint8_t *)image, size) != OK) {
        printf("Failed to read the kernel image (errno=%d)\n", errno);
        close(fd);
        return 5;
    }

    close(fd);

    cf_restore_16bit();

    memcpy((void *)tramp, boot_trampoline, tramp_size);

    printf("Starting kernel...\n");

    // There's no coming back from here
    ((trampoline_t)tramp)(size, (void *)image, (void *)KERNEL_LOAD,
                                     (void *)KERNEL_ENTRY, (void *)RAM_TOP);

    return 0;                           // not reached
}
