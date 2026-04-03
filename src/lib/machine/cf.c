#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <cf.h>

void cf_init(void) {
   _cf_wait_busy();
	*cf_reg_feature = CF_FEATURE_8BIT;
    *cf_reg_command = CF_CMD_SET_FEATURE;
    _cf_wait_busy();
}

int cf_read(uint8_t drive_num, uint32_t sector, uint8_t *buffer) {
    uint8_t status;

    *cf_reg_lba3 = 0xe0 | ((drive_num & 1)<<4) | (uint8_t) ((sector >> 24) & 0x0f);
    *cf_reg_lba2 = (uint8_t) (sector >> 16);
    *cf_reg_lba1 = (uint8_t) (sector >> 8);
    *cf_reg_lba0 = (uint8_t) sector;

    *cf_reg_sector_count = 1;
    *cf_reg_command = CF_CMD_READ_SECTORS;
	_cf_wait_busy();

    // Check for error
	//dump_regs();
	status = *cf_reg_status;
	if (status & CF_ST_ERROR) {
		return -1;
    }

    // Wait for device to indicate data is ready to read
	//printf("Waiting for data...\n");
	//dump_regs();
    //_cf_wait_data();
	CF_WAIT_FOR_DATA();

	for (int i = 0; i < 512; i++) {
		buffer[i] = *cf_reg_data_byte;
		//_cf_wait_busy();
	}

	return 0;
}