/**
 * hd63484.c - Hitachi HD63484 ACRTC driver (single device, 16-bit bus)
 * See hd63484.h for the assumptions and documentation.
 */
#include "hd63484.h"

#include <stdio.h>
#include <string.h>

/* Shadow copies of the write-mostly registers (the only driver state). */
static struct {
    uint16_t ccr;   /* ABT bit always stored as 0 */
    uint16_t omr;
    uint16_t dcr;
} acrtc;

const char *hd63484_strerror(int err)
{
    switch (err) {
        case HD63484_OK:              return "ok";
        case HD63484_ERR_PARAM:       return "invalid parameter";
        case HD63484_ERR_TIMEOUT:     return "timeout waiting for status";
        case HD63484_ERR_NO_DEVICE:   return "device did not respond as expected";
        case HD63484_ERR_CMD:         return "ACRTC reported a command error";
        case HD63484_ERR_UNSUPPORTED: return "unsupported combination";
        default:                      return "unknown error";
    }
}

void hd63484_bus_write(int reg, int val) {
    uint16_t *p = (uint16_t *)(HD63484_BASE_ADDR + (reg<<1));

    printf("WR(%d) -> %04x\n", reg, val);

    *p = val;
}

uint16_t hd63484_bus_read(int reg) {
    uint16_t *p = (uint16_t *)(HD63484_BASE_ADDR + (reg<<1));
    uint16_t val = *p;

    if (reg == 0) {
        val &= 0xff;
    }
    
    printf("rd(%d) <- %04x\n", reg, val);

    return val;
}

/* ------------------------------------------------------------------------- */
/* Direct-access registers                                                   */
/* ------------------------------------------------------------------------- */
uint8_t hd63484_read_status(void)
{
    return (uint8_t)(hd63484_bus_read(0) & 0xFFu);
}

/* AR (LSB ignored in 16-bit mode), then one word transfer. */
void hd63484_write_reg(uint8_t reg, uint16_t value)
{
    hd63484_bus_write(0, reg & 0xFEu);
    hd63484_bus_write(1, value);
}

uint16_t hd63484_read_reg(uint8_t reg)
{
    hd63484_bus_write(0, reg & 0xFEu);
    return hd63484_bus_read(1);
}

/* ------------------------------------------------------------------------- */
/* Status polling and FIFO                                                   */
/* ------------------------------------------------------------------------- */
static int wait_sr(uint8_t mask, uint8_t value)
{
    unsigned long n = HD63484_POLL_LIMIT;

    while (n--) {
        if ((hd63484_read_status() & mask) == value)
            return HD63484_OK;
    }
    return HD63484_ERR_TIMEOUT;
}

static int fifo_write_word(uint16_t w)
{
    int rc = wait_sr(HD63484_SR_WFR, HD63484_SR_WFR);

    if (rc)
        return rc;
    hd63484_bus_write(0, HD63484_REG_FIFO);   /* AR = 0 selects the FIFOs */
    hd63484_bus_write(1, w);
    return HD63484_OK;
}

static int fifo_read_word(uint16_t *w)
{
    int rc = wait_sr(HD63484_SR_RFR, HD63484_SR_RFR);

    if (rc)
        return rc;
    hd63484_bus_write(0, HD63484_REG_FIFO);
    *w = hd63484_bus_read(1);
    return HD63484_OK;
}

int hd63484_send_cmd(uint16_t opcode, const uint16_t *params, size_t nparams)
{
    int rc = fifo_write_word(opcode);
    size_t i;

    for (i = 0; rc == HD63484_OK && i < nparams; i++)
        rc = fifo_write_word(params[i]);
    return rc;
}

int hd63484_wait_idle(void)
{
    unsigned long n = HD63484_POLL_LIMIT;

    while (n--) {
        uint8_t sr = hd63484_read_status();

        if (sr & HD63484_SR_CER)
            return HD63484_ERR_CMD;
        /* FIFO drained and last command finished */
        if ((sr & (HD63484_SR_WFE | HD63484_SR_CED)) ==
            (HD63484_SR_WFE | HD63484_SR_CED))
            return HD63484_OK;
    }
    return HD63484_ERR_TIMEOUT;
}

/* ------------------------------------------------------------------------- */
/* Register-access commands                                                  */
/* ------------------------------------------------------------------------- */
int hd63484_wpr(uint8_t reg, uint16_t value)
{
    if (reg > 0x1Fu)
        return HD63484_ERR_PARAM;
    return hd63484_send_cmd((uint16_t)(HD63484_CMD_WPR | reg), &value, 1);
}

int hd63484_rpr(uint8_t reg, uint16_t *value)
{
    int rc;

    if (reg > 0x1Fu || !value)
        return HD63484_ERR_PARAM;
    rc = hd63484_send_cmd((uint16_t)(HD63484_CMD_RPR | reg), NULL, 0);
    return rc ? rc : fifo_read_word(value);
}

/* 24-bit pointer format shared by ORG and RWP:
 *   high word: DN[15:14] | address[19:12]
 *   low  word: address[11:0] << 4 | dot[3:0]                                */
static void pack_ptr(uint8_t dn, uint32_t addr, uint8_t dot,
                     uint16_t *hi, uint16_t *lo)
{
    *hi = (uint16_t)(((uint16_t)(dn & 3u) << 14) | ((addr >> 12) & 0xFFu));
    *lo = (uint16_t)(((addr & 0xFFFu) << 4) | (dot & 0xFu));
}

int hd63484_org(uint8_t dn, uint32_t addr, uint8_t dot)
{
    uint16_t p[2];

    if (dn > 3 || addr > 0xFFFFFu)
        return HD63484_ERR_PARAM;
    pack_ptr(dn, addr, dot, &p[0], &p[1]);
    return hd63484_send_cmd(HD63484_CMD_ORG, p, 2);
}

int hd63484_set_colors(uint16_t cl0, uint16_t cl1)
{
    int rc = hd63484_wpr(HD63484_PR_CL0, cl0);

    return rc ? rc : hd63484_wpr(HD63484_PR_CL1, cl1);
}

int hd63484_move_abs(int16_t x, int16_t y)
{
    uint16_t p[2];

    p[0] = (uint16_t)x;
    p[1] = (uint16_t)y;
    return hd63484_send_cmd(HD63484_CMD_AMOVE, p, 2);
}

/* ------------------------------------------------------------------------- */
/* Frame buffer access                                                       */
/* ------------------------------------------------------------------------- */
int hd63484_set_rwp(uint8_t dn, uint32_t addr, uint8_t dot)
{
    uint16_t hi, lo;
    int rc;

    if (dn > 3 || addr > 0xFFFFFu)
        return HD63484_ERR_PARAM;
    pack_ptr(dn, addr, dot, &hi, &lo);
    rc = hd63484_wpr(HD63484_PR_RWPH, hi);
    return rc ? rc : hd63484_wpr(HD63484_PR_RWPL, lo);
}

int hd63484_write_words(uint8_t dn, uint32_t addr,
                        const uint16_t *data, size_t count)
{
    int rc = hd63484_set_rwp(dn, addr, 0);
    size_t i;

    for (i = 0; rc == HD63484_OK && i < count; i++)
        rc = hd63484_send_cmd(HD63484_CMD_WT, &data[i], 1);
    return rc ? rc : hd63484_wait_idle();
}

int hd63484_read_words(uint8_t dn, uint32_t addr, uint16_t *data, size_t count)
{
    int rc = hd63484_set_rwp(dn, addr, 0);
    size_t i;

    for (i = 0; rc == HD63484_OK && i < count; i++) {
        rc = hd63484_send_cmd(HD63484_CMD_RD, NULL, 0);
        if (rc == HD63484_OK)
            rc = fifo_read_word(&data[i]);
    }
    return rc;
}

int hd63484_fill_words(uint8_t dn, uint32_t addr, uint16_t value, size_t count)
{
    int rc = hd63484_set_rwp(dn, addr, 0);
    size_t i;

    for (i = 0; rc == HD63484_OK && i < count; i++)
        rc = hd63484_send_cmd(HD63484_CMD_WT, &value, 1);
    return rc ? rc : hd63484_wait_idle();
}

int hd63484_clear_area(uint8_t dn, uint32_t addr, uint16_t words_per_line,
                       uint16_t lines, uint16_t value)
{
    uint16_t p[3];
    int rc;

    if (!words_per_line || !lines)
        return HD63484_ERR_PARAM;
    rc = hd63484_set_rwp(dn, addr, 0);
    if (rc)
        return rc;

    p[0] = value;
    p[1] = (uint16_t)(words_per_line - 1u);
    p[2] = (uint16_t)(-(int16_t)(lines - 1u));
    rc = hd63484_send_cmd(HD63484_CMD_CLR, p, 3);
    return rc ? rc : hd63484_wait_idle();
}

/* ------------------------------------------------------------------------- */
/* Configuration helpers                                                     */
/* ------------------------------------------------------------------------- */
void hd63484_config_default(hd63484_config_t *c)
{
    memset(c, 0, sizeof *c);
    c->master            = true;
    c->gbm               = HD63484_GBM_1BPP;
    c->gai               = HD63484_GAI_1;
    c->acm               = HD63484_ACM_SINGLE;
    c->rsm               = HD63484_RSM_NON_INTERLACE;
    c->static_ram        = false;
    c->zero_unused_regs  = true;
    c->init_drawing_regs = true;

    c->screen[HD63484_SCREEN_BASE].enable  = true;
    c->screen[HD63484_SCREEN_BASE].display = true;
}

unsigned hd63484_pixels_per_mcycle(hd63484_gbm_t gbm, hd63484_gai_t gai,
                                   hd63484_acm_t acm)
{
    unsigned ppw, ppmc;

    if ((unsigned)gbm > HD63484_GBM_16BPP)
        return 0;
    ppw = 16u >> (unsigned)gbm;           /* pixels per 16-bit word */

    switch (gai) {
    case HD63484_GAI_1:
    case HD63484_GAI_2:
    case HD63484_GAI_4:
    case HD63484_GAI_8:
        ppmc = ppw << (unsigned)gai;
        break;
    case HD63484_GAI_HALF:
        ppmc = ppw / 2u;
        break;
    default:
        return 0;
    }

    /* Dual access modes use two memory cycles per display cycle. */
    if (acm != HD63484_ACM_SINGLE)
        ppmc /= 2u;
    return ppmc;
}

uint16_t hd63484_mem_width_words(unsigned pixels, hd63484_gbm_t gbm)
{
    unsigned long bits = (unsigned long)pixels << (unsigned)gbm;

    return (uint16_t)((bits + 15ul) / 16ul);
}

int hd63484_config_set_timing(hd63484_config_t *c,
                              const hd63484_crt_timing_t *t)
{
    unsigned ppmc, v_avail;

    if (c->rsm != HD63484_RSM_NON_INTERLACE)
        return HD63484_ERR_UNSUPPORTED;

    ppmc = hd63484_pixels_per_mcycle(c->gbm, c->gai, c->acm);
    if (!ppmc)
        return HD63484_ERR_PARAM;

    if ((t->h_active % ppmc) || (t->h_front % ppmc) ||
        (t->h_sync   % ppmc) || (t->h_back  % ppmc))
        return HD63484_ERR_PARAM;   /* not representable in memory cycles */

    c->hc  = (uint16_t)((t->h_active + t->h_front + t->h_sync + t->h_back) / ppmc);
    c->hsw = (uint16_t)(t->h_sync   / ppmc);
    c->hds = (uint16_t)(t->h_back   / ppmc);
    c->hdw = (uint16_t)(t->h_active / ppmc);

    c->vc  = (uint16_t)(t->v_active + t->v_front + t->v_sync + t->v_back);
    c->vsw = t->v_sync;
    c->vds = t->v_back;

    v_avail = t->v_active;
    if (c->screen[HD63484_SCREEN_UPPER].enable)
        v_avail -= (v_avail >= c->screen[HD63484_SCREEN_UPPER].height)
                       ? c->screen[HD63484_SCREEN_UPPER].height : v_avail;
    if (c->screen[HD63484_SCREEN_LOWER].enable)
        v_avail -= (v_avail >= c->screen[HD63484_SCREEN_LOWER].height)
                       ? c->screen[HD63484_SCREEN_LOWER].height : v_avail;
    if (!v_avail)
        return HD63484_ERR_PARAM;
    c->screen[HD63484_SCREEN_BASE].height = (uint16_t)v_avail;
    return HD63484_OK;
}

int hd63484_config_validate(const hd63484_config_t *c)
{
    unsigned n, total;

    if (!c)
        return HD63484_ERR_PARAM;
    if ((unsigned)c->gbm > HD63484_GBM_16BPP)
        return HD63484_ERR_PARAM;
    if (c->cursor_skew > 3 || c->disp_skew > 3)
        return HD63484_ERR_PARAM;
    if (c->rsm != HD63484_RSM_NON_INTERLACE &&
        c->rsm != HD63484_RSM_INTERLACE_SYNC &&
        c->rsm != HD63484_RSM_INTERLACE_SYNC_VIDEO)
        return HD63484_ERR_PARAM;
    if (c->acm != HD63484_ACM_SINGLE && c->acm != HD63484_ACM_INTERLEAVED &&
        c->acm != HD63484_ACM_SUPERIMPOSED)
        return HD63484_ERR_PARAM;
    if (!(c->gai <= HD63484_GAI_8 || c->gai == HD63484_GAI_NONE ||
          c->gai == HD63484_GAI_HALF))
        return HD63484_ERR_PARAM;

    /* Horizontal timing */
    if (c->hc  < 1 || c->hc  > 256) return HD63484_ERR_PARAM;
    if (c->hsw < 2 || c->hsw > 31)  return HD63484_ERR_PARAM;
    if (c->hds < 1 || c->hds > 256) return HD63484_ERR_PARAM;
    if (c->hdw < 1 || c->hdw > 256) return HD63484_ERR_PARAM;
    total = (unsigned)c->hsw + c->hds + c->hdw;
    if (total > c->hc)              return HD63484_ERR_PARAM;

    /* Vertical timing */
    if (c->vc  < 1 || c->vc  > 4095) return HD63484_ERR_PARAM;
    if (c->vsw < 1 || c->vsw > 31)   return HD63484_ERR_PARAM;
    if (c->vds < 1 || c->vds > 256)  return HD63484_ERR_PARAM;

    /* Dual access modes need an even display width */
    if (c->acm != HD63484_ACM_SINGLE && (c->hdw & 1u))
        return HD63484_ERR_PARAM;

    /* Screens */
    if (!c->screen[HD63484_SCREEN_BASE].enable)
        return HD63484_ERR_PARAM;               /* Base is mandatory */

    total = 0;                                  /* now: sum of split heights */
    for (n = 0; n < 4; n++) {
        const hd63484_screen_t *s = &c->screen[n];

        if (!s->enable)
            continue;
        if (s->mem_width < 1 || s->mem_width > 4095) return HD63484_ERR_PARAM;
        if (s->start_addr > 0xFFFFFu)                return HD63484_ERR_PARAM;
        if (s->start_dot > 15)                       return HD63484_ERR_PARAM;
        if (s->character && (s->first_raster > 31 || s->last_raster > 31))
            return HD63484_ERR_PARAM;
        if (n != HD63484_SCREEN_WINDOW) {
            if (s->height < 1 || s->height > 4095)   return HD63484_ERR_PARAM;
            total += s->height;
        }
    }
    if ((unsigned long)c->vsw + c->vds + total > c->vc)
        return HD63484_ERR_PARAM;

    if (c->screen[HD63484_SCREEN_WINDOW].enable) {
        if (c->hws < 1 || c->hws > 256 || c->hww < 1 || c->hww > 256)
            return HD63484_ERR_PARAM;
        if (c->vws < 1 || c->vws > 4096 || c->vww < 1 || c->vww > 4095)
            return HD63484_ERR_PARAM;
        if (c->vws < c->vds)                    /* manual: VWS >= VDS */
            return HD63484_ERR_PARAM;
        if (c->acm != HD63484_ACM_SINGLE) {
            if (c->hww & 1u)                            return HD63484_ERR_PARAM;
            if ((c->hws & 1u) != (c->hds & 1u))         return HD63484_ERR_PARAM;
        }
    }
    return HD63484_OK;
}

/* ------------------------------------------------------------------------- */
/* Register value builders                                                   */
/* ------------------------------------------------------------------------- */
static uint16_t build_omr(const hd63484_config_t *c)
{
    uint16_t v = 0;

    if (c->master)               v |= HD63484_OMR_MIS;
    if (c->drawing_priority)     v |= HD63484_OMR_ACP;
    if (c->window_smooth_scroll) v |= HD63484_OMR_WSS;
    if (c->static_ram)           v |= HD63484_OMR_RAM;
    v |= (uint16_t)((c->cursor_skew & 3u) << HD63484_OMR_CSK_SHIFT);
    v |= (uint16_t)((c->disp_skew   & 3u) << HD63484_OMR_DSK_SHIFT);
    v |= (uint16_t)(((unsigned)c->gai & 7u) << HD63484_OMR_GAI_SHIFT);
    v |= (uint16_t)(((unsigned)c->acm & 3u) << HD63484_OMR_ACM_SHIFT);
    v |= (uint16_t)(((unsigned)c->rsm & 3u) << HD63484_OMR_RSM_SHIFT);
    return v;                                   /* STR left clear */
}

static uint16_t build_dcr(const hd63484_config_t *c)
{
    const hd63484_screen_t *s = c->screen;
    uint16_t v = c->attr;

    if (c->dsp_mode)
        v |= HD63484_DCR_DSP;
    if (s[HD63484_SCREEN_BASE].display)
        v |= HD63484_DCR_SE1;
    if (s[HD63484_SCREEN_UPPER].enable) {
        v |= HD63484_DCR_SE0_EN;
        if (s[HD63484_SCREEN_UPPER].display)  v |= HD63484_DCR_SE0_DSP;
    }
    if (s[HD63484_SCREEN_LOWER].enable) {
        v |= HD63484_DCR_SE2_EN;
        if (s[HD63484_SCREEN_LOWER].display)  v |= HD63484_DCR_SE2_DSP;
    }
    if (s[HD63484_SCREEN_WINDOW].enable) {
        v |= HD63484_DCR_SE3_EN;
        if (s[HD63484_SCREEN_WINDOW].display) v |= HD63484_DCR_SE3_DSP;
    }
    return v;
}

static void write_sar(unsigned n, uint32_t addr, uint8_t dot)
{
    hd63484_write_reg((uint8_t)HD63484_REG_SARH(n),
                      (uint16_t)(((uint16_t)(dot & 0xFu) << 8) |
                                 ((addr >> 16) & 0xFu)));
    hd63484_write_reg((uint8_t)HD63484_REG_SARL(n), (uint16_t)(addr & 0xFFFFu));
}

/* ------------------------------------------------------------------------- */
/* Control                                                                   */
/* ------------------------------------------------------------------------- */
int hd63484_start(void)
{
    acrtc.omr |= HD63484_OMR_STR;
    hd63484_write_reg(HD63484_REG_OMR, acrtc.omr);
    return HD63484_OK;
}

int hd63484_stop(void)
{
    acrtc.omr &= (uint16_t)~HD63484_OMR_STR;
    hd63484_write_reg(HD63484_REG_OMR, acrtc.omr);
    return HD63484_OK;
}

int hd63484_abort(void)
{
    hd63484_write_reg(HD63484_REG_CCR, (uint16_t)(acrtc.ccr | HD63484_CCR_ABT));
    hd63484_write_reg(HD63484_REG_CCR, acrtc.ccr);   /* ABT back to 0 */
    return HD63484_OK;
}

int hd63484_set_screen_visibility(unsigned screen, bool enable, bool display)
{
    uint16_t en = 0, dsp = 0;

    switch (screen) {
    case HD63484_SCREEN_UPPER:  en = HD63484_DCR_SE0_EN; dsp = HD63484_DCR_SE0_DSP; break;
    case HD63484_SCREEN_BASE:   en = 0;                  dsp = HD63484_DCR_SE1;     break;
    case HD63484_SCREEN_LOWER:  en = HD63484_DCR_SE2_EN; dsp = HD63484_DCR_SE2_DSP; break;
    case HD63484_SCREEN_WINDOW: en = HD63484_DCR_SE3_EN; dsp = HD63484_DCR_SE3_DSP; break;
    default: return HD63484_ERR_PARAM;
    }

    acrtc.dcr &= (uint16_t)~(en | dsp);
    if (en && enable)  acrtc.dcr |= en;
    if (display)       acrtc.dcr |= dsp;
    hd63484_write_reg(HD63484_REG_DCR, acrtc.dcr);
    return HD63484_OK;
}

int hd63484_set_screen_start(unsigned screen, uint32_t addr, uint8_t dot)
{
    if (screen > 3 || addr > 0xFFFFFu || dot > 15)
        return HD63484_ERR_PARAM;
    write_sar(screen, addr, dot);
    return HD63484_OK;
}

uint16_t hd63484_read_raster(void)
{
    return (uint16_t)(hd63484_read_reg(HD63484_REG_RCR) & 0x0FFFu);
}

/* ------------------------------------------------------------------------- */
/* Initialisation                                                            */
/* ------------------------------------------------------------------------- */
int hd63484_init(const hd63484_config_t *c)
{
    int rc;
    unsigned n;
    uint8_t sr;

    rc = hd63484_config_validate(c);
    if (rc)
        return rc;

    memset(&acrtc, 0, sizeof acrtc);

    /* 1. Abort: flushes the FIFOs and forces SR to $23. Doubles as a presence
     *    check. (After a hardware /RES the chip is already in this state: SR =
     *    $23, CCR.ABT = 1, OMR.MIS/STR = 0, everything else undefined, and
     *    HSYNC held low until STR is set.) */
    hd63484_write_reg(HD63484_REG_CCR, HD63484_CCR_ABT);
    sr = hd63484_read_status();
    if ((sr & (HD63484_SR_CED | HD63484_SR_WFR | HD63484_SR_WFE)) !=
        (HD63484_SR_CED | HD63484_SR_WFR | HD63484_SR_WFE))
        return HD63484_ERR_NO_DEVICE;

    /* 2. Stop the timing generator while we program it (OMR.STR = 0). */
    acrtc.omr = build_omr(c);
    hd63484_write_reg(HD63484_REG_OMR, acrtc.omr);

    /* 3. Timing control RAM */
    hd63484_write_reg(HD63484_REG_HSR,
                      (uint16_t)(((c->hc - 1u) << 8) | (c->hsw & 0x1Fu)));
    hd63484_write_reg(HD63484_REG_HDR,
                      (uint16_t)(((c->hds - 1u) << 8) | (c->hdw - 1u)));
    hd63484_write_reg(HD63484_REG_VSR, (uint16_t)(c->vc & 0x0FFFu));
    hd63484_write_reg(HD63484_REG_VDR,
                      (uint16_t)(((c->vds - 1u) << 8) | (c->vsw & 0x1Fu)));

    hd63484_write_reg(HD63484_REG_SP1,
                      c->screen[HD63484_SCREEN_BASE].height & 0x0FFFu);
    if (c->screen[HD63484_SCREEN_UPPER].enable)
        hd63484_write_reg(HD63484_REG_SP0,
                          c->screen[HD63484_SCREEN_UPPER].height & 0x0FFFu);
    if (c->screen[HD63484_SCREEN_LOWER].enable)
        hd63484_write_reg(HD63484_REG_SP2,
                          c->screen[HD63484_SCREEN_LOWER].height & 0x0FFFu);

    hd63484_write_reg(HD63484_REG_BCR, c->blink_control);

    if (c->screen[HD63484_SCREEN_WINDOW].enable) {
        hd63484_write_reg(HD63484_REG_HWR,
                          (uint16_t)(((c->hws - 1u) << 8) | (c->hww - 1u)));
        hd63484_write_reg(HD63484_REG_VWS, (uint16_t)((c->vws - 1u) & 0x0FFFu));
        hd63484_write_reg(HD63484_REG_VWW, (uint16_t)(c->vww & 0x0FFFu));
    }

    /* 4. Display control RAM: RAR / MWR / SAR per enabled screen */
    for (n = 0; n < 4; n++) {
        const hd63484_screen_t *s = &c->screen[n];

        if (!s->enable)
            continue;
        if (s->character)
            hd63484_write_reg((uint8_t)HD63484_REG_RAR(n),
                              (uint16_t)(((s->last_raster & 0x1Fu) << 8) |
                                         (s->first_raster & 0x1Fu)));
        hd63484_write_reg((uint8_t)HD63484_REG_MWR(n),
                          (uint16_t)((s->character ? 0x8000u : 0u) |
                                     (s->mem_width & 0x0FFFu)));
        write_sar(n, s->start_addr, s->start_dot);
    }

    /* 5. Put registers with undefined power-up state into a benign state. */
    if (c->zero_unused_regs) {
        hd63484_write_reg(HD63484_REG_GCR_X,  0);
        hd63484_write_reg(HD63484_REG_GCR_YS, 0);
        hd63484_write_reg(HD63484_REG_GCR_YE, 0);
        hd63484_write_reg(HD63484_REG_BCUR1,  0);
        hd63484_write_reg(HD63484_REG_BCA1,   0);
        hd63484_write_reg(HD63484_REG_BCUR2,  0);
        hd63484_write_reg(HD63484_REG_BCA2,   0);
        hd63484_write_reg(HD63484_REG_CDR,    0);
        hd63484_write_reg(HD63484_REG_ZFR,    0);   /* 1x / 1x zoom */
    }

    /* 6. Display Control Register */
    acrtc.dcr = build_dcr(c);
    hd63484_write_reg(HD63484_REG_DCR, acrtc.dcr);

    /* 7. Command Control Register: GBM + IRQ mask, ABT = 0 enables the FIFOs */
    acrtc.ccr = (uint16_t)((((unsigned)c->gbm & 7u) << HD63484_CCR_GBM_SHIFT) |
                           c->irq_mask);
    hd63484_write_reg(HD63484_REG_CCR, acrtc.ccr);

    /* 8. Go */
    rc = hd63484_start();
    if (rc)
        return rc;

    /* 9. Drawing parameter defaults */
    if (c->init_drawing_regs) {
        const uint16_t all_ones = 0xFFFFu;

        if ((rc = hd63484_wpr(HD63484_PR_CL0,  0)))         return rc;
        if ((rc = hd63484_wpr(HD63484_PR_CL1,  all_ones)))  return rc;
        if ((rc = hd63484_wpr(HD63484_PR_CCMP, 0)))         return rc;
        if ((rc = hd63484_wpr(HD63484_PR_EDG,  0)))         return rc;
        if ((rc = hd63484_wpr(HD63484_PR_MASK, all_ones)))  return rc;
        if ((rc = hd63484_org(HD63484_SCREEN_BASE,
                              c->screen[HD63484_SCREEN_BASE].start_addr,
                              c->screen[HD63484_SCREEN_BASE].start_dot)))
            return rc;
        if ((rc = hd63484_wait_idle()))
            return rc;
    }
    return HD63484_OK;
}