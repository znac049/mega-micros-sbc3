#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <machine.h>

#include "cli.h"

#define NL() putchar('\n')

typedef struct dos_partition {
    unsigned char boot_flag;
    unsigned char chs_begin[3];
    unsigned char sys_type;
    unsigned char chs_end[3];
    unsigned char start_sector[4];
    unsigned char nr_sector[4];
} dos_partition_t;


static uint16_t swap16(uint16_t val) {
	return (val>>8) | ((val&0xff)<<8);
}

static uint32_t swap32(uint32_t val) {
	return (val>>24) | 
			((val&0xff00)>>8) |
			((val&0xff00)<<8) |
			((val&0xff)<<24);
}

void pr_info(char *buff, int len) {
    for (int i=0; i<len; i++) {
        _putchar(buff[i]);
    }
}

void cf_info(void) {
    uint8_t info[512];
	struct cf_info *inf = (struct cf_info *) info;
	uint8_t status;
	uint16_t *cp;

    printf("Reading CF info...\n");

    _cf_wait_busy();
    *cf_reg_command = CF_CMD_IDENTIFY;
    _cf_wait_busy();

	for (int i=0; i<512; i++) {
		status = *cf_reg_status;
		while (!(status & 0x08)) {
			status = *cf_reg_status;
		}
		info[i] = *cf_reg_data_byte;
		//printf("i=%d\r", i);
	}

	// Fix byte order
	inf->signature = swap16(inf->signature);
	inf->num_cylinders = swap16(inf->num_cylinders);
	inf->num_heads = swap16(inf->num_heads);
	inf->sectors_per_track = swap16(inf->sectors_per_track);
	inf->sectors_per_card = swap32(inf->sectors_per_card);
	inf->num_ecc_bytes = swap16(inf->num_ecc_bytes);
	inf->max_multiple_sectors = swap16(inf->max_multiple_sectors);
	inf->capabilities = swap16(inf->capabilities);
	inf->field_validity = swap16(inf->field_validity);
	inf->current_num_cylinders = swap16(inf->current_num_cylinders);
	inf->current_num_heads = swap16(inf->current_num_heads);
	inf->current_sectors_per_track = swap16(inf->current_sectors_per_track);
	inf->current_capacity_in_sectors = swap32(inf->current_capacity_in_sectors);

	cp = (uint16_t *)&inf->serial_num;
	for (int i=0; i<10; i++) {
		cp[i] = swap16(cp[i]);
	}

	cp = (uint16_t *)&inf->firmware_revision;
	for (int i=0; i<4; i++) {
		cp[i] = swap16(cp[i]);
	}

	cp = (uint16_t *)&inf->model_number;
	for (int i=0; i<20; i++) {
		cp[i] = swap16(cp[i]);
	}

	printf("\nSignature          : %04x\n", inf->signature);
    printf("   Serial          : "); pr_info(inf->serial_num, 20); NL();
	printf("  Model #          : ");  pr_info(inf->model_number, 40); NL();
	printf(" Num Cylinders     : %d\n", inf->current_num_cylinders);
	printf(" Num Heads         : %d\n", inf->current_num_heads);
	printf("Capacity in sectors: %d\n", inf->current_capacity_in_sectors);
    printf("\n");
}

void init_cf(void) {
	// Set 8-bit mode
	printf("Set 8-bit mode\n");
	*cf_reg_feature = 0x01;
	*cf_reg_command = CF_CMD_SET_FEATURE;
	_cf_wait_busy();
}

char *to_binary(unsigned int val, int size) {
	static char to_binary_buffer[33];
	unsigned int mask = 1<<(size-1);

	if ((size < 0) || (size > 32)) {
		return "Blargh!!";
	}

	for (int i=0; i<size; i++) {
		to_binary_buffer[i] = (val&mask)?'1':'0';
		mask = mask >> 1;
	}
	to_binary_buffer[size] = EOS;

	return to_binary_buffer;

}

void dump_regs(void) {
	static char *names[] = {"   Data","  Error","Sec cnt","   LBA0","   LBA1","   LBA2","   LBA3"," Status"};
	printf("CF registers:\n");
	for (int i=1; i<8; i++) {
		uint8_t reg = cf_reg_data_byte[i<<1];

		printf(" %8s: $%02x  %s\n", names[i], reg, to_binary(reg, 8));
	}
}

int dump_dos_partition(dos_partition_t *part, int partition_number) {
	uint32_t start_sector = swap32(*((uint32_t *) &part->start_sector));
	uint32_t num_sectors = swap32(*((uint32_t *) &part->nr_sector));

	printf(" %d: %02x %02x %08x %08x\n", partition_number + 1, 
			part->boot_flag,
			part->sys_type,
			start_sector,
			num_sectors
		);

	return start_sector;
}


void dump_sector(uint32_t sector_num) {
	uint8_t buffer[512];

	if (cf_read(0, sector_num, buffer) != 0) {
		printf("CF read failed on sector %d\n", sector_num);
		return;
	}

	printf("\nSector %d\n", sector_num);

	for (int i=0; i<512; i+= 32) {
		printf("  %04x: ", i);

		for (int j=0; j<32; j++) {
			printf("%02x ", buffer[i+j]);
		}

		printf("   ");

		for (int j=0; j<32; j++) {
			printf("%c", is_printable(buffer[i+j])?buffer[i+j]:'.');
		}

		putchar('\n');
	}
}

int do_partitions(int argc, char **argv) {
	uint8_t buffer[1024];
	uint16_t signature;
	uint16_t *p;
	uint32_t start_sector;

	init_cf();

	// See if it looks like a MSDOS partition table
	if (cf_read(0, 0, buffer) != 0) {
		printf("CF read failed\n");
		return -1;
	}

	p = (uint16_t *)&buffer[0x1fe];
	signature = *p;

	printf("Sector signature: %04x\n", signature);
	if (signature != 0x55aa) {
		printf("No '0x55aa' signature found at end of sector\n");
		return -2;
	}

	for (int i=0; i<4; i++) {
		if ((start_sector = dump_dos_partition((dos_partition_t *) &buffer[446+(i*16)], i)) > 0) {
			dump_sector(start_sector);
		}
	}

	return 0;
}

int do_ata(int argc, char **args) {
	init_cf();
	cf_info();
	dump_sector(0);

	return 0;
}

