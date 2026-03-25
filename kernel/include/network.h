#ifndef NETWORK_H
#define NETWORK_H
#include <stdint.h>

/* ============================================================
   network.h - AegisOS v4.0 Ag Altyapisi Tanimlari
   Ethernet / WiFi / TCP-IP Stack cercevesi
   ============================================================ */

#define NET_MAX_SCAN_RESULTS 16

/* WiFi tarama sonucu */
typedef struct {
    char    ssid[32];
    int     signal_strength;  /* 0-100 */
    int     encrypted;        /* 0=acik, 1=sifrelenmis */
    int     channel;
} wifi_network_t;

/* Ag durumu */
typedef struct {
    int     connected;
    char    ssid[32];
    uint8_t ip[4];
    uint8_t gateway[4];
    uint8_t subnet[4];
    uint8_t dns1[4];
    uint8_t dns2[4];
    uint8_t mac[6];
    int     link_speed;       /* Mbps */
    int     dhcp;             /* 1=DHCP, 0=Statik */
    int     vpn_active;
    int     firewall_active;
    int     mac_spoofed;
} net_status_t;

/* Ag istatistikleri */
typedef struct {
    uint64_t tx_packets;
    uint64_t rx_packets;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint32_t errors;
} net_stats_t;

/* API */
void          network_init(void);
int           network_detect_nic(void);
net_status_t *network_get_status(void);
net_stats_t  *network_get_stats(void);
int           network_is_connected(void);
int           network_wifi_scan(wifi_network_t *results, int max_results);
int           network_connect(const char *ssid, const char *password);
int           network_disconnect(void);
int           network_vpn_toggle(void);
int           network_firewall_toggle(void);
void          network_mac_spoof(void);
void          network_print_status(void);

#endif /* NETWORK_H */
