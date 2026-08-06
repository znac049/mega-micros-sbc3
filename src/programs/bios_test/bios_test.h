
#pragma once

#define RAM_MAX ((1024*1024*8)-1)
#define ONE_MEG (1024*1024)

// #define DS1307_ADDR   0x68
// #define SH1107_ADDR   0x3C

// #define PIT_BASE   0xAF0001UL

// #define PGCR       (*(volatile uint8_t *)(PIT_BASE + 0x00))
// #define PBDDR      (*(volatile uint8_t *)(PIT_BASE + 0x06))
// #define PBDR       (*(volatile uint8_t *)(PIT_BASE + 0x12))

// #define RES_BIT    0
// #define RES_MASK   (1u << RES_BIT)

// #define SH1107_WIDTH    128
// #define SH1107_HEIGHT   128
// #define SH1107_PAGES    (SH1107_HEIGHT / 8)

/* 
 * Some 128x128 SH1107 modules need a display-offset and/or a column-offset
 * to line up correctly. The following values work for my generic baord
 * but If your image is shifted or wrapped, try adjusting these two first.
 */
// #define SH1107_DISPLAY_OFFSET   0x60
// #define SH1107_COLUMN_OFFSET    0x60

// struct ds1307_time {
//     uint8_t seconds;   /* 0-59 */
//     uint8_t minutes;   /* 0-59 */
//     uint8_t hours;     /* 0-23 (24-hour mode assumed) */
//     uint8_t day;       /* 1-7, day of week (chip-defined numbering) */
//     uint8_t date;      /* 1-31 */
//     uint8_t month;     /* 1-12 */
//     uint8_t year;      /* 0-99, add 2000 */
// };

// typedef struct ds1307_time ds1307_time_t;

// struct font {
//     const uint8_t width;
//     const uint8_t height;
//     const uint16_t *font_chars;
//     const uint8_t *char_widths;
// };

// typedef struct font font_t;

