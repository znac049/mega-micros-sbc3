#include "ctype.h"

#define ATA_REG_BASE		CONFIG_ATA_BASE
#define ATA_REG_DATA		((volatile uint16_t *) (ATA_REG_BASE + 0x0))
#define ATA_REG_DATA_BYTE	((volatile uint8_t *) (ATA_REG_BASE + 0x0))
#define ATA_REG_FEATURE		((volatile uint8_t *) (ATA_REG_BASE + 0x2))
#define ATA_REG_ERROR		((volatile uint8_t *) (ATA_REG_BASE + 0x2))
#define ATA_REG_SECTOR_COUNT	((volatile uint8_t *) (ATA_REG_BASE + 0x4))
#define ATA_REG_SECTOR_NUM	((volatile uint8_t *) (ATA_REG_BASE + 0x6))
#define ATA_REG_CYL_LOW		((volatile uint8_t *) (ATA_REG_BASE + 0x8))
#define ATA_REG_CYL_HIGH	((volatile uint8_t *) (ATA_REG_BASE + 0xa))
#define ATA_REG_DRIVE_HEAD	((volatile uint8_t *) (ATA_REG_BASE + 0xc))
#define ATA_REG_STATUS		((volatile uint8_t *) (ATA_REG_BASE + 0xe))
#define ATA_REG_COMMAND		((volatile uint8_t *) (ATA_REG_BASE + 0xe))


#define ATA_CMD_READ_SECTORS	0x20
#define ATA_CMD_WRITE_SECTORS	0x30
#define ATA_CMD_IDENTIFY	0xEC
#define ATA_CMD_SET_FEATURE	0xEF

#define ATA_ST_BUSY		0x80
#define ATA_ST_DATA_READY	0x08
#define ATA_ST_ERROR		0x01


#define ATA_DELAY(x)		{ for (int delay = 0; delay < (x); delay++) { asm volatile(""); } }
#define ATA_WAIT()		{ ATA_DELAY(4); while (*ATA_REG_STATUS & ATA_ST_BUSY) { } }
#define ATA_WAIT_FOR_DATA()	{ while (!((*ATA_REG_STATUS) & ATA_ST_DATA_READY)) { } }

void command_atatest(int argc, char **args)
{
	uint32_t sector = 46842;
	char buffer[512];

	if (argc >= 2)
		sector = (uint32_t) strtol(args[1], NULL, 10);

	//printf("Set COMET CompactFlash async mode\n");
	//*COMET_VME_CF_CONTROL = 0x00;
	//ATA_DELAY(10);
	//*COMET_VME_CF_CONTROL = 0xb8;
	//ATA_DELAY(20);

	// Set 8-bit mode
	//printf("Set 8-bit mode\n");
	(*ATA_REG_FEATURE) = 0x01;
	(*ATA_REG_COMMAND) = ATA_CMD_SET_FEATURE;
	ATA_WAIT();

	// Read a sector
	//printf("Setup read\n");
	(*ATA_REG_DRIVE_HEAD) = 0xE0;
	//(*ATA_REG_DRIVE_HEAD) = 0xE0 | (uint8_t) ((sector >> 24) & 0x0F);
	(*ATA_REG_CYL_HIGH) = (uint8_t) (sector >> 16);
	(*ATA_REG_CYL_LOW) = (uint8_t) (sector >> 8);
	(*ATA_REG_SECTOR_NUM) = (uint8_t) sector;
	(*ATA_REG_SECTOR_COUNT) = 1;
	(*ATA_REG_COMMAND) = ATA_CMD_READ_SECTORS;
	ATA_WAIT();

	//printf("Read status\n");
	char status = (*ATA_REG_STATUS);
	if (status & 0x01) {
		printf("Error while reading ata: %x\n", (*ATA_REG_ERROR));
		return;
	}

	//printf("Wait for data\n");
	ATA_WAIT();
	ATA_WAIT_FOR_DATA();

	//printf("Read data\n");
	for (int i = 0; i < 512; i++) {
		//((uint16_t *) buffer)[i] = (*ATA_REG_DATA);
		//asm volatile("rol.w	#8, %0\n" : "+g" (((uint16_t *) buffer)[i]));
		buffer[i] = (*ATA_REG_DATA_BYTE);
		//printf("%x ", 0xff & buffer[i]);

		//ATA_WAIT_FOR_DATA();
		ATA_WAIT();
		//ATA_DELAY(10);
	}

	printf("Mem %x:\n", sector);
	for (int i = 0; i < 512; i++) {
		printf("%02x ", 0xff & buffer[i]);
		if ((i & 0x1F) == 0x1F)
			printf("\n");
	}

	return;
}
