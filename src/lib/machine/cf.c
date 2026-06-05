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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <machine.h>

void cf_init(void) {
   _cf_wait_busy();
	*cf_reg_feature = CF_FEATURE_8BIT;
    *cf_reg_command = CF_CMD_SET_FEATURE;
    _cf_wait_busy();
}

int cf_read(uint8_t drive_num, uint32_t sector, uint8_t *buffer) {
    uint8_t status;

    // printf("cf_read(%d, %d,...)\n", drive_num, sector);

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

	for (int i = 0; i < CF_SECTOR_SIZE; i++) {
		buffer[i] = *cf_reg_data_byte;
		//_cf_wait_busy();
	}

	return 0;
}

static void sanitize_string(char *str, int max_len) {
    // Endianness!
    for (int i=0; i<max_len; i+=2) {
        char c = str[i];

        str[i] = str[i+1];
        str[i+1] = c;
    }

    str[max_len-1] = EOS;

    // Remove any trailing spaces
    for (int i=max_len-2; i<=0; i--) {
        if (str[i] == ' ') {
            str[i] = EOS;
        }
        else {
            return;
        }
    }
}

int cf_identify(uint8_t drive_num, cf_info_t *info) {
    uint8_t buf[CF_SECTOR_SIZE];
    cf_info_t *inf = (cf_info_t *) buf;

    if (drive_num > 1) {
        return -1;
    }

    _cf_wait_busy();
    *cf_reg_lba3 = 0xe0 | (drive_num?0x10:0);
    *cf_reg_command = CF_CMD_IDENTIFY;
    _cf_wait_busy();

	for (int i=0; i<512; i++) {
		uint8_t status = *cf_reg_status;

        while (!(status & 0x08)) {
			status = *cf_reg_status;
		}
		buf[i] = *cf_reg_data_byte;
	}

    // Endianness!
    info->signature = __builtin_bswap16(inf->signature);
    info->num_cylinders = __builtin_bswap16(inf->num_cylinders);
    info->num_heads = __builtin_bswap16(inf->num_heads);
    info->sectors_per_track = __builtin_bswap16(inf->sectors_per_track);
    info->sectors_per_card = __builtin_bswap32(inf->sectors_per_card);
    info->num_ecc_bytes = __builtin_bswap16(inf->num_ecc_bytes);
    info->max_multiple_sectors = __builtin_bswap16(inf->max_multiple_sectors);
    info->capabilities = __builtin_bswap16(inf->capabilities);
    info->field_validity = __builtin_bswap16(inf->field_validity);
    info->current_num_cylinders = __builtin_bswap16(inf->current_num_cylinders);
    info->current_num_heads = __builtin_bswap16(inf->current_num_heads);
    info->current_sectors_per_track = __builtin_bswap16(inf->current_sectors_per_track);
    info->current_capacity_in_sectors = __builtin_bswap32(inf->current_capacity_in_sectors);

    memcpy(&info->serial_num, &inf->serial_num, 20);
    memcpy(&info->firmware_revision, &inf->firmware_revision, 8);
    memcpy(&(info->model_number), &(inf->model_number), 40);
    
    sanitize_string(info->serial_num, 20);
    sanitize_string(info->firmware_revision, 8);
    sanitize_string(info->model_number, 40);

    return 0;
}