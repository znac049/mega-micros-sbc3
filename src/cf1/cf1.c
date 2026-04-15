#include <stdlib.h>
#include <stdio.h>
#include <machine.h>

#define NL() putchar('\n')

static uint16_t swap16(uint16_t val) {
	return (val>>8) | ((val&0xff)<<8);
}

static uint32_t swap32(uint32_t val) {
	return (val>>24) | 
			((val&0xff00)>>8) |
			((val&0xff00)<<8) |
			((val&0xff)<<24);
}

static inline char hexchar(uint8_t byte)
{
	if (byte < 10) {
		return byte + 0x30;
	} else {
		return byte + 0x37;
	}
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

static inline void report_status(void) {
	uint8_t status = *cf_reg_status;

	printf("CF Status: $%02x - %s\n", status, to_binary(status, 8));
}

int cf_detect()
{
	uint8_t status;
	
	report_status();
	status = *cf_reg_status;

	// If the busy bit is already set, or the two bits that are always 0, then perhaps nothing is connected
	printf("status=%02x\n", status);
	if (status & (CF_ST_BUSY | 0x06)) {
		printf("It's busy and shouldn't be!\n");
		return 0;
	}

	CF_DELAY(10);

	// Reset the IDE bus
	(*cf_reg_command) = CF_CMD_IDENTIFY;

	for (int i = 0; i < 1000; i++) {
		CF_DELAY(10);

		status = *cf_reg_status;
		// If it becomes unbusy within the timeout then a drive is connected
		if (!(status & CF_ST_BUSY)) {
			if (status & CF_ST_RDY) {
				CF_DELAY(100);
				return 1;
			} else {
				return 0;
			}
		}
	}
	return 0;
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

    printf("Reading CF info...\r\n");

    _cf_wait_busy();
	//CF_WAIT();
    *cf_reg_command = CF_CMD_IDENTIFY;
    _cf_wait_busy();
	//CF_WAIT();

	for (int i=0; i<512; i++) {
		status = *cf_reg_status;
		while (!(status & 0x08)) {
			status = *cf_reg_status;
		}
		info[i] = *cf_reg_data_byte;
		printf("i=%d\r", i);
	}

	printf("\nSignature          : %04x\n", swap16(inf->signature));
    printf("   Serial          : "); pr_info(inf->serial_num, 20); NL();
	printf("  Model #          : ");  pr_info(inf->model_number, 40); NL();
	printf(" Num Cylinders     : %d\n", swap16(inf->current_num_cylinders));
	printf(" Num Heads         : %d\n", swap16(inf->current_num_heads));
	printf("Capacity in sectors: %d\n", swap32(inf->current_capacity_in_sectors));
    printf("\n");

	printf("\n");
}

void cf_init(void) {
	report_status();

    // Put the device into 8-bit mode
    _cf_wait_busy();
	//CF_WAIT();
	*cf_reg_feature = CF_FEATURE_8BIT;
    *cf_reg_command = CF_CMD_SET_FEATURE;
    _cf_wait_busy();
	//CF_WAIT();
}

void main() {
	printf("cf_init()\r\n");
	cf_init();

	if (!cf_detect()) {
        printf("No drive found\r\n");
		exit(42);
    }

	printf("cf_info()\r\n");
    cf_info();
}