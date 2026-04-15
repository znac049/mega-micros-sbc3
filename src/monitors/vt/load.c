#include <stdio.h>

#include "cli.h"

int do_load(int argc, char **argv) {
	char *filename = "a.srec";

	if (argc >= 2) {
		filename = argv[1];
	}

	if (has_mbr(0)) {
		printf("MBR record found\n");
	}
	else {
		printf("No MBR found\n");
		return -1;
	}

	printf("loading '%s'\n", filename);

	for (int i=0; i<4; i++) {
		switch (get_partition_type(0, i)) {
			case PART_FAT16:
				printf("Partition %d is FAT16\n", i);
				break;

			case PART_MINIXV1:
				printf("Partition %d is FAT16\n", i);
				break;

			default:
				printf("Partition %d has an unknown partition type\n", i);
				break;
		}

	}

	return 0;
}
