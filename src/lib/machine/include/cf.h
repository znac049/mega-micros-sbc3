#pragma once

#include <ctype.h>

#define cf_base	((volatile uint8_t *) 0xae0000)

#define cf_reg_data			    ((volatile uint16_t *) cf_base)
#define cf_reg_data_byte	    ((volatile uint8_t *)  cf_base)
#define cf_reg_feature			((volatile uint8_t *)  cf_base+2)
#define cf_reg_error			((volatile uint8_t *)  cf_base+2)
#define cf_reg_sector_count	    ((volatile uint8_t *)  cf_base+4)
#define cf_reg_sector_num		((volatile uint8_t *)  cf_base+6)
#define cf_reg_lba0				((volatile uint8_t *)  cf_base+6)
#define cf_reg_cyl_low			((volatile uint8_t *)  cf_base+8)
#define cf_reg_lba1				((volatile uint8_t *)  cf_base+8)
#define cf_reg_cyl_high		    ((volatile uint8_t *)  cf_base+10)
#define cf_reg_lba2  		    ((volatile uint8_t *)  cf_base+10)
#define cf_reg_drive_head		((volatile uint8_t *)  cf_base+12)
#define cf_reg_lba3				((volatile uint8_t *)  cf_base+12)
#define cf_reg_status			((volatile uint8_t *)  cf_base+14)
#define cf_reg_command			((volatile uint8_t *)  cf_base+14)

#define CF_CMD_READ_SECTORS	    0x20
#define CF_CMD_WRITE_SECTORS	0x30
#define CF_CMD_IDENTIFY	        0xEC
#define CF_CMD_SET_FEATURE	    0xEF

#define CF_FEATURE_8BIT         1

#define CF_ST_ERROR		        0x01
#define CF_ST_CORR				0x04
#define CF_ST_DRQ	    		0x08
#define CF_ST_DSC				0x10
#define CF_ST_DWF				0x20
#define CF_ST_RDY				0x40
#define CF_ST_BUSY		        0x80

#define CF_DRIVE_MASTER			0
#define CF_DRIVE_SLAVE			1

#define CF_DELAY(x)		{ for (int delay = 0; delay < (x); delay++) { __asm volatile(""); } }
#define CF_WAIT_FOR_DATA()	{ while (!((*cf_reg_status) & CF_ST_DRQ)) { } }

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
	uint16_t max_multiple_sectors;
	uint16_t reserved2;
	uint16_t capabilities;
	uint16_t reserved3;
	uint16_t unused1;
	uint16_t obsolete4;
	uint16_t field_validity;
	uint16_t current_num_cylinders;
	uint16_t current_num_heads;
	uint16_t current_sectors_per_track;
	uint32_t current_capacity_in_sectors;
};

void _cf_wait_busy(void);
void _cf_wait_data(void);
void cf_init(void);
int cf_read(uint8_t drive_num, uint32_t sector, uint8_t *buffer);
