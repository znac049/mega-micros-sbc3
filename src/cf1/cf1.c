#include "stdlib.h"
#include "printf.h"
#include "machine.h"

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

#define ATA_REG_BASE		    0xae0001
#define ATA_REG_DATA		    ((volatile uint16_t *) (ATA_REG_BASE + 0x0))
#define ATA_REG_DATA_BYTE	    ((volatile uint8_t *) (ATA_REG_BASE + 0x0))

#define ATA_REG_FEATURE		    ((volatile uint8_t *) (ATA_REG_BASE + 0x2))
#define ATA_REG_ERROR		    ((volatile uint8_t *) (ATA_REG_BASE + 0x2))

#define ATA_REG_SECTOR_COUNT    ((volatile uint8_t *) (ATA_REG_BASE + 0x4))

#define ATA_REG_SECTOR_NUM	    ((volatile uint8_t *) (ATA_REG_BASE + 0x6))

#define ATA_REG_CYL_LOW		    ((volatile uint8_t *) (ATA_REG_BASE + 0x8))

#define ATA_REG_CYL_HIGH	    ((volatile uint8_t *) (ATA_REG_BASE + 0xa))

#define ATA_REG_DRIVE_HEAD	    ((volatile uint8_t *) (ATA_REG_BASE + 0xc))

#define ATA_REG_STATUS		    ((volatile uint8_t *) (ATA_REG_BASE + 0xe))
#define ATA_REG_COMMAND		    ((volatile uint8_t *) (ATA_REG_BASE + 0xe))

#define ATA_CMD_READ_SECTORS	0x20
#define ATA_CMD_WRITE_SECTORS	0x30
#define ATA_CMD_IDENTIFY	    0xec
#define ATA_CMD_SET_FEATURE	    0xef

#define ATA_FEATURE_8BIT        0x01

#define ATA_ST_BUSY		        0x80
#define ATA_ST_DATA_READY	    0x08
#define ATA_ST_ERROR		    0x01

#define ATA_DELAY(x)		{ for (int delay = 0; delay < (x); delay++) { __asm volatile(""); } }
#define ATA_WAIT()		{ while (*ATA_REG_STATUS & ATA_ST_BUSY) { } }
#define ATA_WAIT_FOR_DATA()	{ while (!((*ATA_REG_STATUS) & ATA_ST_DATA_READY)) { } }

struct cf_info {
	uint16_t signature;
	uint16_t num_cylinders;
	uint16_t reserved1;
	uint16_t num_heads;
	uint32_t obsolete1;
	uint16_t sectors_per_track;
	uint32_t sectors_per_card;
	uint16_t obsolete2;
	char serial_num[20];
	uint32_t obsolete3;
	uint16_t num_ecc_bytes;
	char firmware_revision[8];
	char model_number[40];
};

void _ata_wait_busy(void);
void _ata_wait_data(void);

void ata_wait()
{
	ATA_DELAY(10);
	ATA_WAIT();
	ATA_DELAY(10);
}

static inline char hexchar(uint8_t byte)
{
	if (byte < 10) {
		return byte + 0x30;
	} else {
		return byte + 0x37;
	}
}

int ata_read_sector(int sector, char *buffer)
{
	// Set 8-bit mode
	(*ATA_REG_FEATURE) = 0x01;
	(*ATA_REG_COMMAND) = ATA_CMD_SET_FEATURE;
	_ata_wait_busy();

	// Read a sector
	(*ATA_REG_DRIVE_HEAD) = 0xE0;
	//(*ATA_REG_DRIVE_HEAD) = 0xE0 | (uint8_t) ((sector >> 24) & 0x0F);
	(*ATA_REG_SECTOR_NUM) = (uint8_t) sector;
	(*ATA_REG_CYL_LOW) = (uint8_t) (sector >> 8);
	(*ATA_REG_CYL_HIGH) = (uint8_t) (sector >> 16);
	(*ATA_REG_SECTOR_COUNT) = 2;
	(*ATA_REG_COMMAND) = ATA_CMD_READ_SECTORS;
	_ata_wait_busy();

	char status = (*ATA_REG_STATUS);
	if (status & 0x01)
		return 0;

	_ata_wait_busy();
	ATA_WAIT_FOR_DATA();

	for (int i = 0; i < 1024; i++) {
		buffer[i] = (*ATA_REG_DATA_BYTE);
		ata_wait();
	}

	for (int i = 0; i < 1024; i++) {
		putchar(hexchar((buffer[i] >> 4) & 0xF));
		putchar(hexchar(buffer[i] & 0xF));
		if ((i & 0x1F) == 0x1F)
			putchar('\n');
	}

	return 0;
}

int ata_detect()
{
	uint8_t status;

	status = *ATA_REG_STATUS;
	// If the busy bit is already set, or the two bits that are always 0, then perhaps nothing is connected
	if (status & (ATA_ST_BUSY | 0x06))
		return 0;
	ATA_DELAY(10);

	// Reset the IDE bus
	(*ATA_REG_COMMAND) = ATA_CMD_IDENTIFY;

	for (int i = 0; i < 1000; i++) {
		ATA_DELAY(10);

		status = *ATA_REG_STATUS;
		// If it becomes unbusy within the timeout then a drive is connected
		if (!(status & ATA_ST_BUSY)) {
			if (status & ATA_ST_DATA_READY) {
				ATA_DELAY(100);
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
    byte_t info[1024];
	struct cf_info *inf = (struct cf_info *) info;
	byte_t status;

    printf("Reading CF info...\r\n");

    _ata_wait_busy();
    *ATA_REG_COMMAND = ATA_CMD_IDENTIFY;
    _ata_wait_busy();

	for (int i=0; i<1024; i++) {
		status = *ATA_REG_STATUS;
		while (!(status & 0x08)) {
			status = *ATA_REG_STATUS;
		}
		info[i] = *ATA_REG_DATA_BYTE;
		printf("i=%d\r", i);
	}

	printf("\r\nSignature: %04x\r\n", inf->signature);
    printf("  Serial: "); pr_info(inf->serial_num, 20);
    printf("\r\n");

/*
	for (int i=0; i<1024; i++) {
		printf("%02x ", info[i]);
	}
*/
	printf("\r\n");
}

void cf_init(void) {
    // Put the device into 8-bit mode
    _ata_wait_busy();
    *ATA_REG_FEATURE = ATA_FEATURE_8BIT;
    *ATA_REG_COMMAND = ATA_CMD_SET_FEATURE;
    _ata_wait_busy();
}

void main() {
	printf("cf_init()\r\n");
	cf_init();

	if (!ata_detect()) {
        printf("No drive found\r\n");
        return;
    }

	printf("cf_info()\r\n");
    cf_info();
}