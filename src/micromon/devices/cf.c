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

#include "micromon.h"

int cf_init(void) {
    uint8_t status;

    /*
     * Select the master explicitly. The card isn't reset by a warm
     * reset, so it may still have drive 1 selected from the last
     * cf_drive_ready() call - and commands sent to a missing slave
     * are aborted.
     */
    *cf_reg_lba3 = 0xe0;
    CF_DELAY_400NS();

    /*
     * Diable IRQs - harmless on boards that don't have
     * /CS2 connected - it will write cf_reg_lba3
     * instead
     */
    *cf_reg_dev_ctrl = CF_CTRL_NIEN;

    if (_cf_wait_busy() == NOT_OK) {
        printk("CF: timed out waiting for card (status=$%02X)\n", *cf_reg_status);
        return NOT_OK;
    }

	*cf_reg_feature = CF_FEATURE_16BIT;
    *cf_reg_command = CF_CMD_SET_FEATURE;

      /*
     * The card may take up to 400nS to set BSY after a command is
     * written - reading status before then gives a stale result.
     */
    CF_DELAY_400NS();

    if (_cf_wait_busy() == NOT_OK) {
        printk("CF: timed out after SET FEATURES (status=$%02X)\n", *cf_reg_status);
        return NOT_OK;
    }

    if (*cf_reg_status & CF_ST_ERROR) {
        return NOT_OK;
    }

    /*
     * Report, but don't fail, on an error here - the old code ignored
     * it and detection still worked. cf_drive_ready() decides whether
     * a card is really there.
     */
    status = *cf_reg_status;
    if (status & CF_ST_ERROR) {
        printk("CF: SET FEATURES (16-bit) rejected (status=$%02X, error=$%02X)\n",
            status, *cf_reg_error);
     }

    return OK;
}

/*
 * Wait for DRQ (the card is ready to transfer data), with a timeout.
 * Returns NOT_OK if the card reports an error or never gets there.
 */
static int cf_wait_drq(void) {
    for (uint32_t i=0; i<CF_DRQ_TIMEOUT; i++) {
        uint8_t status = *cf_reg_status;

        if (status & (CF_ST_ERROR | CF_ST_DWF)) {
            return NOT_OK;
        }

        if ((status & (CF_ST_BUSY | CF_ST_DRQ)) == CF_ST_DRQ) {
            return OK;
        }
    }

    return NOT_OK;
}

// Read a sector's worth of data, 16 bits at a time
static void cf_read_data(uint8_t *buffer) {
    for (int i = 0; i < CF_WORDS_PER_SECTOR; i++) {
        uint16_t word = *cf_reg_data;

        *buffer++ = (uint8_t)(word >> 8);
        *buffer++ = (uint8_t)word;
    }
}

// Write a sector's worth of data, 16 bits at a time
static void cf_write_data(const uint8_t *buffer) {
    for (int i = 0; i < CF_WORDS_PER_SECTOR; i++) {
        *cf_reg_data = (uint16_t)((buffer[0] << 8) | buffer[1]);
        buffer += 2;
    }
}

static void cf_set_lba(uint8_t drive_num, uint32_t sector) {
    *cf_reg_lba0 = (uint8_t) sector;
    *cf_reg_lba1 = (uint8_t) (sector >> 8);
    *cf_reg_lba2 = (uint8_t) (sector >> 16);
    *cf_reg_lba3 = 0xe0 | ((drive_num & 1)<<4) | (uint8_t) ((sector >> 24) & 0x0f);
}

int cf_read(uint8_t drive_num, uint32_t sector, uint8_t *buffer) {
    uint8_t status;

    // printk("cf_read(%d, %d,...)\n", drive_num, sector);

    cf_set_lba(drive_num, sector);

    *cf_reg_sector_count = 1;
    *cf_reg_command = CF_CMD_READ_SECTORS;

    // BSY may take up to 400nS to appear after the command
    CF_DELAY_400NS();

	if (_cf_wait_busy() == NOT_OK) {
        // Timed out
        return NOT_OK;
    }

    // Check for error
	//dump_regs();
	status = *cf_reg_status;
	if (status & CF_ST_ERROR) {
		return NOT_OK;
    }

    // Wait for device to indicate data is ready to read
	//printk("Waiting for data...\n");
	//dump_regs();
    //_cf_wait_data();
    if (cf_wait_drq() == NOT_OK) {
        return NOT_OK;
    }

    cf_read_data(buffer);

	return OK;
}

/*
 * Write one 512-byte sector. Returns OK once the card says the data is
 * stored, or NOT_OK on an error or timeout.
 */
int cf_write(uint8_t drive_num, uint32_t sector, const uint8_t *buffer) {
    uint8_t status;

    // The card must be idle before it'll take a command
    if (_cf_wait_busy() == NOT_OK) {
        return NOT_OK;
    }

    cf_set_lba(drive_num, sector);
    *cf_reg_sector_count = 1;
    *cf_reg_command = CF_CMD_WRITE_SECTORS;

    // BSY may take up to 400nS to appear after the command
    CF_DELAY_400NS();

    if (_cf_wait_busy() == NOT_OK) {
        return NOT_OK;
    }

    // Wait for the card to ask for the data
    if (cf_wait_drq() == NOT_OK) {
        printk("cf_write: sector %d refused (status=$%02X, error=$%02X)\n",
            sector, *cf_reg_status, *cf_reg_error);
        return NOT_OK;
    }

    cf_write_data(buffer);

    // The card goes busy while it stores the sector
    CF_DELAY_400NS();

    if (_cf_wait_busy() == NOT_OK) {
        printk("cf_write: sector %d timed out\n", sector);
        return NOT_OK;
    }

    status = *cf_reg_status;
    if (status & (CF_ST_ERROR | CF_ST_DWF)) {
        printk("cf_write: sector %d failed (status=$%02X, error=$%02X)\n",
            sector, status, *cf_reg_error);
        return NOT_OK;
    }

    return OK;
}

uint8_t cf_drive_ready(uint8_t drive_num) {
    /* 
     * Poll the status register: if no card is present, this will
     * consistently return 0. If a card is present, DRDY and DSC
     * should be set.
     */ 
    register uint8_t cf_good = CF_ST_RDY | CF_ST_DSC;

    if (drive_num > 1) {
        return NO;
    }

    // Select drive
    *cf_reg_lba3 = 0xe0 | (drive_num?0x10:0);

    for (int i=0; i<1000; i++) {
        if ((*cf_reg_status & cf_good) == cf_good) {
            return YES;
        }
    }

    return NO;
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

    // DRQ is set once, for the whole sector
    if (cf_wait_drq() == NOT_OK) {
        return -1;
    }

    cf_read_data(buf);

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