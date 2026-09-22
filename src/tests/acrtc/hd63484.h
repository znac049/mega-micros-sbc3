/**
 * hd63484.h - Hitachi HD63484 ACRTC driver (single device, 16-bit bus)
 *
 * Assumptions built into this version:
 *   1. Exactly one ACRTC. All state is static; no device handle is passed.
 *   2. The chip is memory-mapped at HD63484_BASE_ADDR (default 0xAA0000).
 *   3. The chip is in 16-bit host-bus mode (/DACK high while /RES is low).
 *
 * Bus access: RS is assumed to be wired to A1, so
 *     base + 0   RS=0   write: Address Register    read: Status Register
 *     base + 2   RS=1   read/write: register selected by AR (or the FIFOs)
 * Every access is a 16-bit word at an even address. The manual forbids 8-bit
 * transfers in 16-bit mode, so never touch the chip with byte accesses.
 *
 * Overridable at compile time (-D...):
 *     HD63484_BASE_ADDR   chip base address              (0x00AA0000)
 *     HD63484_RS_STRIDE   byte distance RS=0 -> RS=1     (2)
 *     HD63484_POLL_LIMIT  status polls before a timeout  (2000000)
 * Bus access itself can be replaced (e.g. for a simulator) by defining the
 * function-like macros HD63484_BUS_WRITE(rs, value) / HD63484_BUS_READ(rs).
 *
 * Hardware reset (/RES) is the board's job; init() also aborts the chip, so
 * calling it again without a reset is safe.
 *
 * Sources for register maps and bit layouts: Hitachi HD63484 ACRTC User's
 * Manual (May 1985), sections 4-5, cross-checked against MAME's emulation for
 * command opcodes and drawing-parameter register numbers. Items marked
 * [VERIFY] were not confirmed against the manual text.
 *
 * Typical use:
 *     hd63484_config_t cfg;
 *     hd63484_config_default(&cfg);
 *     cfg.gbm = HD63484_GBM_1BPP;
 *     hd63484_config_set_timing(&cfg, &crt_timing);
 *     cfg.screen[HD63484_SCREEN_BASE].mem_width =
 *             hd63484_mem_width_words(640, cfg.gbm);
 *     if (hd63484_init(&cfg) != HD63484_OK) { ... }
 */
#ifndef HD63484_H
#define HD63484_H

#include <ctype.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include <stdint.h>

typedef bool_t bool;
#define true ((bool) ~0)
#define false ((bool) 0)


#ifdef __cplusplus
extern "C" {
#endif

#ifndef HD63484_BASE_ADDR
#define HD63484_BASE_ADDR   0x00AA0000
#endif
#ifndef HD63484_RS_STRIDE
#define HD63484_RS_STRIDE   2u
#endif
#ifndef HD63484_POLL_LIMIT
#define HD63484_POLL_LIMIT  2000000
#endif

/* ========================================================================= */
/* Error codes                                                               */
/* ========================================================================= */
typedef enum {
    HD63484_OK              =  0,
    HD63484_ERR_PARAM       = -1,  /* bad argument / config value            */
    HD63484_ERR_TIMEOUT     = -2,  /* status polling timed out               */
    HD63484_ERR_NO_DEVICE   = -3,  /* SR did not read back as expected       */
    HD63484_ERR_CMD         = -4,  /* CER (command error) flag set           */
    HD63484_ERR_UNSUPPORTED = -5   /* combination not handled by this driver */
} hd63484_err_t;

const char *hd63484_strerror(int err);

/* ========================================================================= */
/* Direct-access register addresses (value loaded into AR)                   */
/* ========================================================================= */
#define HD63484_REG_FIFO   0x00u  /* write/read FIFO entry                   */
#define HD63484_REG_CCR    0x02u  /* Command Control Register                */
#define HD63484_REG_OMR    0x04u  /* Operation Mode Register                 */
#define HD63484_REG_DCR    0x06u  /* Display Control Register                */

/* Timing control RAM */
#define HD63484_REG_RCR    0x80u  /* Raster Count (read only)                */
#define HD63484_REG_HSR    0x82u  /* Horizontal Sync: HC[15:8], HSW[4:0]     */
#define HD63484_REG_HDR    0x84u  /* Horizontal Display: HDS[15:8], HDW[7:0] */
#define HD63484_REG_VSR    0x86u  /* Vertical Sync: VC[11:0]                 */
#define HD63484_REG_VDR    0x88u  /* Vertical Display: VDS[15:8], VSW[4:0]   */
#define HD63484_REG_SP1    0x8Au  /* Split screen 1 (Base)  height, 12 bit   */
#define HD63484_REG_SP0    0x8Cu  /* Split screen 0 (Upper) height, 12 bit   */
#define HD63484_REG_SP2    0x8Eu  /* Split screen 2 (Lower) height, 12 bit   */
#define HD63484_REG_BCR    0x90u  /* Blink control                           */
#define HD63484_REG_HWR    0x92u  /* Horizontal Window: HWS[15:8], HWW[7:0]  */
#define HD63484_REG_VWS    0x94u  /* Vertical Window Start (12 bit)          */
#define HD63484_REG_VWW    0x96u  /* Vertical Window Width (12 bit)          */
#define HD63484_REG_GCR_X  0x98u  /* Graphic cursor: CXE[15:8], CXS[7:0]     */
#define HD63484_REG_GCR_YS 0x9Au  /* Graphic cursor Y start (12 bit)         */
#define HD63484_REG_GCR_YE 0x9Cu  /* Graphic cursor Y end   (12 bit)         */
/* 0x9E-0x9F: ACRTC internal work area - never access */

/* Display control RAM: four screens, 8 bytes apart */
#define HD63484_REG_RAR(n)   (0xC0u + 8u * (n)) /* Raster Address: LRA[12:8] FRA[4:0] */
#define HD63484_REG_MWR(n)   (0xC2u + 8u * (n)) /* Memory Width: CHR[15], MW[11:0]    */
#define HD63484_REG_SARH(n)  (0xC4u + 8u * (n)) /* Start addr high: SDA[11:8], SAH[3:0] [VERIFY bit positions] */
#define HD63484_REG_SARL(n)  (0xC6u + 8u * (n)) /* Start addr low 16 bits             */

#define HD63484_REG_BCUR1  0xE0u  /* Block cursor 1                          */
#define HD63484_REG_BCA1   0xE2u
#define HD63484_REG_BCUR2  0xE4u  /* Block cursor 2                          */
#define HD63484_REG_BCA2   0xE6u
#define HD63484_REG_CDR    0xE8u  /* Cursor definition                       */
#define HD63484_REG_ZFR    0xEAu  /* Zoom factor                             */
#define HD63484_REG_LPAH   0xECu  /* Light pen address                       */
#define HD63484_REG_LPAL   0xEEu

/* Screen numbers */
#define HD63484_SCREEN_UPPER   0u
#define HD63484_SCREEN_BASE    1u
#define HD63484_SCREEN_LOWER   2u
#define HD63484_SCREEN_WINDOW  3u

/* ========================================================================= */
/* Status register bits (read with RS=0)                                     */
/* ========================================================================= */
#define HD63484_SR_CER   0x80u  /* Command Error (cleared by ABT)            */
#define HD63484_SR_ARD   0x40u  /* Area Detect (cleared by RPR or ABT)       */
#define HD63484_SR_CED   0x20u  /* Command End: able to accept a command     */
#define HD63484_SR_LPD   0x10u  /* Light Pen Strobe Detect                   */
#define HD63484_SR_RFF   0x08u  /* Read FIFO Full                            */
#define HD63484_SR_RFR   0x04u  /* Read FIFO Ready (has data)                */
#define HD63484_SR_WFR   0x02u  /* Write FIFO Ready (not full)               */
#define HD63484_SR_WFE   0x01u  /* Write FIFO Empty                          */

/* ========================================================================= */
/* Command Control Register (r02)                                            */
/* ========================================================================= */
#define HD63484_CCR_ABT       (1u << 15)  /* Abort (1 = abort, FIFOs cleared) */
#define HD63484_CCR_PSE       (1u << 14)  /* Pause                            */
#define HD63484_CCR_DDM       (1u << 13)  /* Data DMA mode                    */
#define HD63484_CCR_CDM       (1u << 12)  /* Command/parameter DMA mode       */
#define HD63484_CCR_DRC       (1u << 11)  /* DMA request: 1 = cycle steal     */
#define HD63484_CCR_GBM_SHIFT 8u          /* Graphic Bit Mode [10:8]          */
#define HD63484_CCR_GBM_MASK  (7u << 8)
/* CCR[7:0] = interrupt enables, bit-for-bit aligned with SR[7:0] */

/* Graphic bit mode: bits per logical pixel = 1 << GBM */
typedef enum {
    HD63484_GBM_1BPP  = 0,
    HD63484_GBM_2BPP  = 1,
    HD63484_GBM_4BPP  = 2,
    HD63484_GBM_8BPP  = 3,
    HD63484_GBM_16BPP = 4
} hd63484_gbm_t;

/* ========================================================================= */
/* Operation Mode Register (r04)                                             */
/* ========================================================================= */
#define HD63484_OMR_MIS        (1u << 15) /* 1 = master (set for a single ACRTC) */
#define HD63484_OMR_STR        (1u << 14) /* Start: 1 = run display + drawing    */
#define HD63484_OMR_ACP        (1u << 13) /* 1 = drawing priority                */
#define HD63484_OMR_WSS        (1u << 12) /* Window smooth scroll                */
#define HD63484_OMR_CSK_SHIFT  10u        /* Cursor skew [11:10]                 */
#define HD63484_OMR_DSK_SHIFT  8u         /* DISP skew   [9:8]                   */
#define HD63484_OMR_RAM        (1u << 7)  /* 1 = static RAM (no refresh addr)    */
#define HD63484_OMR_GAI_SHIFT  4u         /* Graphic address increment [6:4]     */
#define HD63484_OMR_ACM_SHIFT  2u         /* Access mode [3:2]                   */
#define HD63484_OMR_RSM_SHIFT  0u         /* Raster scan mode [1:0]              */

/* Graphic Address Increment (words fetched per display cycle) */
typedef enum {
    HD63484_GAI_1    = 0,  /* +1 word                                         */
    HD63484_GAI_2    = 1,  /* +2 words                                        */
    HD63484_GAI_4    = 2,  /* +4 words                                        */
    HD63484_GAI_8    = 3,  /* +8 words                                        */
    HD63484_GAI_NONE = 5,  /* no increment (codes 4, 5 and 6 all mean this)   */
    HD63484_GAI_HALF = 7   /* +1 word every two display cycles                */
} hd63484_gai_t;

typedef enum {
    HD63484_ACM_SINGLE        = 0,  /* single access                          */
    HD63484_ACM_INTERLEAVED   = 2,  /* dual access mode 0                     */
    HD63484_ACM_SUPERIMPOSED  = 3   /* dual access mode 1 (window superimposed) */
} hd63484_acm_t;

typedef enum {
    HD63484_RSM_NON_INTERLACE        = 0,
    HD63484_RSM_INTERLACE_SYNC       = 2,
    HD63484_RSM_INTERLACE_SYNC_VIDEO = 3
} hd63484_rsm_t;

/* ========================================================================= */
/* Display Control Register (r06)                                            */
/* ========================================================================= */
#define HD63484_DCR_DSP     (1u << 15)  /* DISP1/2 output mode                */
#define HD63484_DCR_SE1     (1u << 14)  /* Base screen: 1 = displayed         */
#define HD63484_DCR_SE0_EN  (1u << 13)  /* Upper screen enabled               */
#define HD63484_DCR_SE0_DSP (1u << 12)  /* Upper screen displayed (not blank) */
#define HD63484_DCR_SE2_EN  (1u << 11)  /* Lower screen enabled               */
#define HD63484_DCR_SE2_DSP (1u << 10)  /* Lower screen displayed             */
#define HD63484_DCR_SE3_EN  (1u << 9)   /* Window screen enabled              */
#define HD63484_DCR_SE3_DSP (1u << 8)   /* Window screen displayed            */
/* DCR[7:0] = ATR: user-defined video attribute bits */

/* ========================================================================= */
/* FIFO commands (opcode word, followed by parameter words)                  */
/* ========================================================================= */
#define HD63484_CMD_ORG     0x0400u  /* 2 params: DN|DPAH, DPAL|DPD           */
#define HD63484_CMD_WPR     0x0800u  /* | reg#(0-31), 1 param                 */
#define HD63484_CMD_RPR     0x0C00u  /* | reg#(0-31), 0 params, returns word  */
#define HD63484_CMD_WPTN    0x1800u  /* | pattern RAM addr(0-15), 1+n params  */
#define HD63484_CMD_RPTN    0x1C00u
#define HD63484_CMD_DRD     0x2400u
#define HD63484_CMD_DWT     0x2800u
#define HD63484_CMD_DMOD    0x2C00u
#define HD63484_CMD_RD      0x4400u  /* read word at RWP, RWP++               */
#define HD63484_CMD_WT      0x4800u  /* 1 param: write word at RWP, RWP++     */
#define HD63484_CMD_MOD     0x4C00u
#define HD63484_CMD_CLR     0x5800u  /* 3 params: data, dx, dy                */
#define HD63484_CMD_SCLR    0x5C00u
#define HD63484_CMD_CPY     0x6000u
#define HD63484_CMD_SCPY    0x7000u
#define HD63484_CMD_AMOVE   0x8000u  /* 2 params: x, y                        */
#define HD63484_CMD_RMOVE   0x8400u
#define HD63484_CMD_ALINE   0x8800u  /* | drawing attribute byte              */
#define HD63484_CMD_RLINE   0x8C00u
#define HD63484_CMD_ARCT    0x9000u
#define HD63484_CMD_RRCT    0x9400u
#define HD63484_CMD_APLL    0x9800u
#define HD63484_CMD_RPLL    0x9C00u
#define HD63484_CMD_APLG    0xA000u
#define HD63484_CMD_RPLG    0xA400u
#define HD63484_CMD_CRCL    0xA800u
#define HD63484_CMD_ELPS    0xAC00u
#define HD63484_CMD_AARC    0xB000u
#define HD63484_CMD_RARC    0xB400u
#define HD63484_CMD_AEARC   0xB800u
#define HD63484_CMD_REARC   0xBC00u
#define HD63484_CMD_AFRCT   0xC000u
#define HD63484_CMD_RFRCT   0xC400u
#define HD63484_CMD_PAINT   0xC800u
#define HD63484_CMD_DOT     0xCC00u
#define HD63484_CMD_PTN     0xD000u
#define HD63484_CMD_AGCPY   0xE000u
#define HD63484_CMD_RGCPY   0xF000u

/* Drawing parameter register numbers (for WPR/RPR). Pr02 = CCMP is confirmed
 * by the manual; the others follow the manual's register order and MAME. */
#define HD63484_PR_CL0   0x00u  /* colour 0                                   */
#define HD63484_PR_CL1   0x01u  /* colour 1                                   */
#define HD63484_PR_CCMP  0x02u  /* colour comparison                          */
#define HD63484_PR_EDG   0x03u  /* edge colour                                */
#define HD63484_PR_MASK  0x04u  /* write mask                                 */
#define HD63484_PR_PRC1  0x05u  /* pattern RAM control 1                      */
#define HD63484_PR_PRC2  0x06u
#define HD63484_PR_PRC3  0x07u
#define HD63484_PR_XMIN  0x08u  /* area definition                            */
#define HD63484_PR_YMIN  0x09u
#define HD63484_PR_XMAX  0x0Au
#define HD63484_PR_YMAX  0x0Bu
#define HD63484_PR_RWPH  0x0Cu  /* read/write pointer high (incl. DN)         */
#define HD63484_PR_RWPL  0x0Du  /* read/write pointer low  (incl. dot)        */
#define HD63484_PR_DPH   0x0Eu  /* drawing pointer         [VERIFY numbering] */
#define HD63484_PR_DPL   0x0Fu
#define HD63484_PR_CPH   0x10u  /* current pointer         [VERIFY numbering] */
#define HD63484_PR_CPL   0x11u

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */
typedef struct {
    bool     enable;        /* screens 0/2/3: enabled. Base (1) is always on  */
    bool     display;       /* not blanked (set false to blank but keep area) */
    uint16_t height;        /* rasters: SP0/SP1/SP2 (screens 0..2 only, 1-4095) */
    uint32_t start_addr;    /* 20-bit word address in frame buffer            */
    uint8_t  start_dot;     /* SDA: start dot address 0-15 (horizontal scroll) */
    uint16_t mem_width;     /* 16-bit words per raster (1-4095)               */
    bool     character;     /* CHR: character screen instead of graphic       */
    uint8_t  first_raster;  /* FRA 0-31, character screens only               */
    uint8_t  last_raster;   /* LRA 0-31, character screens only               */
} hd63484_screen_t;

typedef struct {
    /* --- Operation Mode Register --- */
    bool     master;            /* MIS: true for single/master ACRTC           */
    bool     drawing_priority;  /* ACP                                          */
    bool     window_smooth_scroll; /* WSS (superimposed access only)            */
    bool     static_ram;        /* RAM bit. false = DRAM refresh addr on MAD   */
    uint8_t  cursor_skew;       /* CSK 0-3 memory cycles                        */
    uint8_t  disp_skew;         /* DSK 0-3 memory cycles                        */
    hd63484_gai_t gai;
    hd63484_acm_t acm;
    hd63484_rsm_t rsm;

    /* --- Command Control Register --- */
    hd63484_gbm_t gbm;          /* bits per logical pixel                       */
    uint8_t  irq_mask;          /* CCR[7:0] interrupt enables (SR bit layout)   */

    /* --- CRT timing (real counts; driver writes N-1 where the chip needs it) ---
     * Horizontal units are memory cycles, vertical units are rasters.         */
    uint16_t hc;                /* total memory cycles per line (1-256)         */
    uint16_t hsw;               /* HSYNC width (2-31)                           */
    uint16_t hds;               /* HSYNC rising edge -> display start (1-256)   */
    uint16_t hdw;               /* display width (1-256)                        */
    uint16_t vc;                /* total rasters per frame (1-4095)             */
    uint16_t vsw;               /* VSYNC width (1-31)                           */
    uint16_t vds;               /* VSYNC rising edge -> display start (1-256)   */

    /* --- Window screen geometry (used if screen[3].enable) --- */
    uint16_t hws, hww;          /* horizontal start / width, memory cycles      */
    uint16_t vws, vww;          /* vertical start / width, rasters              */

    /* --- Screens 0..3 = Upper, Base, Lower, Window --- */
    hd63484_screen_t screen[4];

    /* --- Display Control Register extras --- */
    bool     dsp_mode;          /* DCR.DSP                                      */
    uint8_t  attr;              /* DCR.ATR user video attribute bits            */

    /* --- Misc --- */
    uint16_t blink_control;     /* raw BCR value (0 = blink outputs off)        */
    bool     zero_unused_regs;  /* write 0 to cursor/zoom registers at init.    *
                                 * Power-up contents are undefined; 0 is assumed
                                 * to mean "off / 1x" [VERIFY]                  */
    bool     init_drawing_regs; /* after start: CL0/CL1/CCMP/EDG/MASK + ORG(base) */
} hd63484_config_t;

/** CRT timing expressed in pixels (non-interlaced). */
typedef struct {
    uint16_t h_active, h_front, h_sync, h_back;
    uint16_t v_active, v_front, v_sync, v_back;
} hd63484_crt_timing_t;

/** Sensible starting point: master, 1 bpp, GAI +1, single access, DRAM
 *  refresh on, base screen only. Timing/screen sizes still need filling. */
void hd63484_config_default(hd63484_config_t *cfg);

/** Pixels output per memory cycle for a GBM/GAI/ACM combination
 *  (0 if the combination has no fixed relationship). */
unsigned hd63484_pixels_per_mcycle(hd63484_gbm_t gbm, hd63484_gai_t gai,
                                   hd63484_acm_t acm);

/** Words per raster needed for `pixels` logical pixels at `gbm`. */
uint16_t hd63484_mem_width_words(unsigned pixels, hd63484_gbm_t gbm);

/**
 * Convert pixel-based CRT timing into HC/HSW/HDS/HDW/VC/VSW/VDS and set the
 * base screen height. Requires gbm/gai/acm/rsm to be set first. Every
 * horizontal value must be a multiple of pixels_per_mcycle(). Non-interlaced
 * only. If Upper/Lower screens are enabled their heights are subtracted from
 * the base screen height.
 */
int hd63484_config_set_timing(hd63484_config_t *cfg,
                              const hd63484_crt_timing_t *t);

/** Range/consistency check. Called by hd63484_init(). */
int hd63484_config_validate(const hd63484_config_t *cfg);

/* ========================================================================= */
/* Initialisation and control                                                */
/* ========================================================================= */

/**
 * Full bring-up:
 *   1. ABT: flush FIFOs, force SR to $23, verify the chip answers
 *   2. OMR written with STR = 0 (timing must only be changed while stopped)
 *   3. timing RAM (HSR, HDR, VSR, VDR, SSW, HWR, VWR) + blink
 *   4. display control RAM (RAR/MWR/SAR) for each enabled screen
 *   5. optional zeroing of cursor/zoom registers
 *   6. DCR (screen enables)
 *   7. CCR (GBM, interrupt mask, ABT = 0 so the FIFOs run)
 *   8. OMR with STR = 1 (display + drawing start)
 *   9. optional drawing parameter defaults and ORG on the base screen
 */
int hd63484_init(const hd63484_config_t *cfg);

int hd63484_start(void);   /* OMR.STR = 1 */
int hd63484_stop(void);    /* OMR.STR = 0 */

/** Abort the current command and flush both FIFOs (also clears CER). */
int hd63484_abort(void);

/* --- Raw access --- */
uint8_t  hd63484_read_status(void);
void     hd63484_write_reg(uint8_t reg, uint16_t value);
uint16_t hd63484_read_reg(uint8_t reg);

/* --- Display control --- */
/** Change a screen's enable/display bits in DCR (Base ignores `enable`). */
int hd63484_set_screen_visibility(unsigned screen, bool enable, bool display);
/** Rewrite a screen's start address / start dot (scrolling). Do this in
 *  vertical blanking if you want to avoid tearing. */
int hd63484_set_screen_start(unsigned screen, uint32_t addr, uint8_t dot);
/** Current raster (RCR). Only valid when HSYNC is high, and HSW must be >= 3. */
uint16_t hd63484_read_raster(void);

/* --- FIFO command interface --- */
/** Queue opcode + n parameter words in the write FIFO (blocks on WFR). */
int hd63484_send_cmd(uint16_t opcode, const uint16_t *params, size_t nparams);
/** Wait until the FIFO is drained and the last command has finished.
 *  Returns HD63484_ERR_CMD if CER is set. */
int hd63484_wait_idle(void);

int hd63484_wpr(uint8_t reg, uint16_t value);
int hd63484_rpr(uint8_t reg, uint16_t *value);

/** ORG: set logical origin/drawing screen `dn` (0-3), word address, dot. */
int hd63484_org(uint8_t dn, uint32_t addr, uint8_t dot);
int hd63484_set_colors(uint16_t cl0, uint16_t cl1);
int hd63484_move_abs(int16_t x, int16_t y);

/* --- Frame buffer access via the Read/Write Pointer --- */
int hd63484_set_rwp(uint8_t dn, uint32_t addr, uint8_t dot);
/** Write `count` words starting at `addr` (uses WT, one command per word). */
int hd63484_write_words(uint8_t dn, uint32_t addr,
                        const uint16_t *data, size_t count);
/** Read `count` words starting at `addr` (uses RD). */
int hd63484_read_words(uint8_t dn, uint32_t addr, uint16_t *data, size_t count);
/** Fill `count` words with `value` using WT (slow but simple). */
int hd63484_fill_words(uint8_t dn, uint32_t addr, uint16_t value, size_t count);
/**
 * Fast rectangular fill using CLR. Starts at `addr` on screen `dn`, covers
 * `words_per_line` words by `lines` rasters, top to bottom. Parameter and
 * direction conventions follow MAME's CLR implementation (dx = words-1,
 * dy = -(lines-1) walks toward higher addresses) [VERIFY against manual
 * section 6.5 before relying on it].
 */
int hd63484_clear_area(uint8_t dn, uint32_t addr, uint16_t words_per_line,
                       uint16_t lines, uint16_t value);

#ifdef __cplusplus
}
#endif
#endif /* HD63484_H */