/*
 * WT32-ETH01 PPP-to-Ethernet router
 *
 * UART2 (TX=IO17, RX=IO5) runs a PPP *server*. The peer (e.g. NetBSD pppd)
 * gets PPP_PEER_ADDR; traffic is NATed out of the LAN8720 Ethernet port,
 * which takes its own address via DHCP.
 *
 * Target: ESP-IDF v5.3 - v5.5.
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include "lwip/ip4_addr.h"
#include "lwip/lwip_napt.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"

static const char *TAG = "pppr";

/* ---- Configuration ---------------------------------------------------- */

#define PPP_UART        UART_NUM_2
#define PPP_TX_GPIO     17          /* header pin "TXD" -> 68k RxD */
#define PPP_RX_GPIO     5           /* header pin "RXD" <- 68k TxD (3.3V only!) */
#define PPP_BAUD        115200

/* Hardware flow control. Strongly recommended at 115200 into a 68681.
 * Set to 1 and wire RTS/CTS, then use "crtscts" in pppd. */
#define PPP_USE_RTSCTS  0
#define PPP_RTS_GPIO    14          /* ESP RTS out -> 68k CTS in */
#define PPP_CTS_GPIO    15          /* ESP CTS in  <- 68k RTS out (3.3V only!) */

/* Point-to-point addresses: pick a subnet not used on your LAN. */
#define PPP_LOCAL_ADDR  "10.42.0.1"
#define PPP_PEER_ADDR   "10.42.0.2"
#define PPP_FALLBACK_DNS "1.1.1.1"

/* WT32-ETH01 Ethernet wiring */
#define ETH_PHY_ADDR    1
#define ETH_MDC_GPIO    23
#define ETH_MDIO_GPIO   18
#define ETH_OSC_EN_GPIO 16          /* enables the 50 MHz oscillator feeding GPIO0 */

/* ---------------------------------------------------------------------- */

static ppp_pcb *ppp;
static struct netif ppp_if;
static esp_netif_t *eth_netif;
static EventGroupHandle_t ev;
#define EV_PPP_DOWN BIT0

/* lwIP -> UART */
static u32_t ppp_output_cb(ppp_pcb *pcb, const void *data, u32_t len, void *ctx)
{
    int n = uart_write_bytes(PPP_UART, data, len);
    return n < 0 ? 0 : (u32_t)n;
}

/* Runs in the lwIP tcpip thread. */
static void ppp_status_cb(ppp_pcb *pcb, int err, void *ctx)
{
    struct netif *nif = ppp_netif(pcb);

    if (err == PPPERR_NONE) {
        ESP_LOGI(TAG, "PPP up: local %s", ip4addr_ntoa(netif_ip4_addr(nif)));
        ESP_LOGI(TAG, "        peer  %s", ip4addr_ntoa(netif_ip4_gw(nif)));
        /* NAT everything that arrives on the PPP interface. */
        ip_napt_enable(netif_ip4_addr(nif)->addr, 1);
        return;
    }

    ESP_LOGW(TAG, "PPP down (err %d), will listen again", err);
    xEventGroupSetBits(ev, EV_PPP_DOWN);
}

/* UART -> lwIP */
static void uart_rx_task(void *arg)
{
    static uint8_t buf[512];
    for (;;) {
        int n = uart_read_bytes(PPP_UART, buf, sizeof(buf), pdMS_TO_TICKS(10));
        if (n > 0) {
            pppos_input_tcpip(ppp, buf, n);
        }
    }
}

static void uart_init(void)
{
    uart_config_t cfg = {
        .baud_rate = PPP_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if PPP_USE_RTSCTS
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 100,
#else
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#endif
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(PPP_UART, 4096, 4096, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(PPP_UART, &cfg));
#if PPP_USE_RTSCTS
    ESP_ERROR_CHECK(uart_set_pin(PPP_UART, PPP_TX_GPIO, PPP_RX_GPIO,
                                 PPP_RTS_GPIO, PPP_CTS_GPIO));
#else
    ESP_ERROR_CHECK(uart_set_pin(PPP_UART, PPP_TX_GPIO, PPP_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
#endif
}

static void eth_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == ETH_EVENT && id == ETHERNET_EVENT_CONNECTED)    ESP_LOGI(TAG, "Ethernet link up");
    if (base == ETH_EVENT && id == ETHERNET_EVENT_DISCONNECTED) ESP_LOGW(TAG, "Ethernet link down");
    if (base == IP_EVENT && id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *e = data;
        ESP_LOGI(TAG, "Ethernet IP " IPSTR, IP2STR(&e->ip_info.ip));
    }
}

static void eth_init(void)
{
    /* Power up the external 50 MHz RMII clock oscillator. */
    gpio_reset_pin(ETH_OSC_EN_GPIO);
    gpio_set_direction(ETH_OSC_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ETH_OSC_EN_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    eth_esp32_emac_config_t emac_cfg = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    emac_cfg.smi_gpio.mdc_num  = ETH_MDC_GPIO;
    emac_cfg.smi_gpio.mdio_num = ETH_MDIO_GPIO;
    emac_cfg.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    emac_cfg.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;   /* GPIO0 */
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&emac_cfg, &mac_cfg);

    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    phy_cfg.phy_addr = ETH_PHY_ADDR;
    phy_cfg.reset_gpio_num = -1;
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_cfg);

    esp_eth_config_t cfg = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth;
    ESP_ERROR_CHECK(esp_eth_driver_install(&cfg, &eth));

    esp_netif_config_t ncfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&ncfg);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth)));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_eth_start(eth));
}

/* Must be called while the PPP link is dead. */
static void ppp_configure(void)
{
    ip4_addr_t a;

    ip4addr_aton(PPP_LOCAL_ADDR, &a);
    ppp_set_ipcp_ouraddr(ppp, &a);
    ip4addr_aton(PPP_PEER_ADDR, &a);
    ppp_set_ipcp_hisaddr(ppp, &a);

    /* Hand the peer the DNS server we learned from DHCP, if any. */
    esp_netif_dns_info_t dns = {0};
    if (esp_netif_get_dns_info(eth_netif, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK &&
        dns.ip.u_addr.ip4.addr != 0) {
        a.addr = dns.ip.u_addr.ip4.addr;
    } else {
        ip4addr_aton(PPP_FALLBACK_DNS, &a);
    }
    ppp_set_ipcp_dnsaddr(ppp, 0, &a);

#if PPP_AUTH_SUPPORT
    ppp_set_auth(ppp, PPPAUTHTYPE_NONE, NULL, NULL);
    ppp_set_auth_required(ppp, 0);
#endif
}

void app_main(void)
{
    ev = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    eth_init();
    uart_init();

    ppp = pppapi_pppos_create(&ppp_if, ppp_output_cb, ppp_status_cb, NULL);
    if (!ppp) {
        ESP_LOGE(TAG, "pppapi_pppos_create failed");
        return;
    }

    xTaskCreate(uart_rx_task, "ppp_rx", 4096, NULL, 12, NULL);

    for (;;) {
        ppp_configure();
        ESP_LOGI(TAG, "PPP listening on UART%d @ %d", PPP_UART, PPP_BAUD);
        pppapi_listen(ppp);

        xEventGroupWaitBits(ev, EV_PPP_DOWN, pdTRUE, pdFALSE, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
