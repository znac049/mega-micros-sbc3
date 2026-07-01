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
#include <errno.h>
#include <machine.h>

#define WITH_DISKS

uint8_t cpu_type = CPU_68000;
uint8_t cpu_speed_mhz = 33;

FILE *stdin;
FILE *stdout;
FILE *stderr;

filesystem_t _mounted_filesystems[MAX_FILESYSTEMS];
uint8_t _num_mounted_filesystems = 0;

static const char *cpus[] = {
    "68000/68008",
    "68010",
    "68020",
    "68030 Enhanced 32-bit"
};

static void _init_streams(void) {
	stdin = stdout = fopen("CON:", "a+");
	stderr = fopen("AUX:", "w");
}

void pre_main(void) {
    _claim_pit();
    _claim_duart(NO, NO);
    _init_heap();
    _init_streams();

    cpu_speed_mhz = measure_cpu_clock();

#ifdef WITH_DISKS
    cf_init();
#endif

    if (1 /*running_in_rom*/) {
        printf("%c[2J", 27);
        printf("%c[H", 27);
        printf("Mega-680x0 Computer System\n");
        printf("Code is running in %s\n", running_in_rom?"ROM":"RAM");
        
        // Detect CPU variant
        cpu_type = (uint8_t) detect_cpu_type();
        if (cpu_type > 3) {
            printf("UNKOWN processor type detected\n");
        }
        else {
            printf("%s Processor running at ~%dMHz\n", cpus[cpu_type], cpu_speed_mhz);
        }
    }

    // Attempt to mount all ext2 filesystems we can...
    if (cf_drive_ready(0)) {
        cf_info_t info;

        if (cf_identify(0, &info) == 0) {
            int res = read_partition_table(0);

            printf("CF%d: %s\n", 0, info.model_number);

            if (res != 0 && res != ENOMEM) {
                printf("CF%d: no partitions found\n", 0);
            }
            else {
                uint8_t num_parts = get_partition_count();

                // Check each partition for an ext2 filesystem
                for (uint8_t part=0; part<num_parts; part++) {
                    disk_partition_t *dp = get_partition(part);
                    ext2_fs_t *fs = ext2_mount(part);

                    if (fs != NULL) {
                        _mounted_filesystems[_num_mounted_filesystems].dp = dp;
                        _mounted_filesystems[_num_mounted_filesystems].fs = fs;
                        _num_mounted_filesystems++;
                        printf("%s: ext2\n", dp->name);
                    }
                }
            }
        }
    }
}

void post_main(int status) {
    if (status) {
        printf("exited with status: %d\n", status);
    }

    // There may be data in the uart TX buffers. Force flush
    fflush(stdout);
    fflush(stderr);

    for (int i=0; i<_num_mounted_filesystems; i++) {
        ext2_umount(_mounted_filesystems[i].fs);
    }

    _release_duart();
    _release_pit();
}