/* ============================================================
   network.c - AegisOS v4.0 Ag Suruculeri (Stub)
   Ethernet / WiFi / TCP-IP Stack temel cercevesi
   QEMU virtio-net ve e1000 uyumlu altyapi
   ============================================================ */
#include "../include/network.h"
#include "../include/vga.h"
#include "../include/pci.h"
#include <stdint.h>

/* ---- Dahili Durum ---- */
static net_status_t net_state;
static net_stats_t  net_statistics;
static wifi_network_t scanned_networks[NET_MAX_SCAN_RESULTS];
static int scan_count = 0;

/* ---- I/O yardimcilari ---- */
static inline void net_outb(uint16_t p, uint8_t v) {
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static inline uint8_t net_inb(uint16_t p) {
    uint8_t v;
    __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));
    return v;
}
static inline uint32_t net_inl(uint16_t p) {
    uint32_t v;
    __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));
    return v;
}

/* ---- Baslatma ---- */
void network_init(void) {
    net_state.connected   = 0;
    net_state.link_speed  = 0;
    net_state.ip[0] = 0; net_state.ip[1] = 0;
    net_state.ip[2] = 0; net_state.ip[3] = 0;
    net_state.gateway[0] = 0; net_state.gateway[1] = 0;
    net_state.gateway[2] = 0; net_state.gateway[3] = 0;
    net_state.dns1[0] = 1; net_state.dns1[1] = 1;
    net_state.dns1[2] = 1; net_state.dns1[3] = 2;
    net_state.dns2[0] = 9; net_state.dns2[1] = 9;
    net_state.dns2[2] = 9; net_state.dns2[3] = 9;
    net_state.dhcp = 1;
    net_state.vpn_active = 0;
    net_state.firewall_active = 1;
    net_state.mac_spoofed = 0;

    for (int i = 0; i < 6; i++) net_state.mac[i] = 0;

    net_statistics.tx_packets = 0;
    net_statistics.rx_packets = 0;
    net_statistics.tx_bytes   = 0;
    net_statistics.rx_bytes   = 0;
    net_statistics.errors     = 0;

    scan_count = 0;
}

/* ---- PCI NIC Arama ---- */
int network_detect_nic(void) {
    /* PCI uzerinden Intel e1000 (8086:100E) veya 
       Realtek RTL8139 (10EC:8139) ararız */
    /* Su an stub: NIC bulunamadi olarak donelim */
    return 0; /* 0 = NIC bulunamadi, 1 = bulundu */
}

/* ---- Durum Sorgulama ---- */
net_status_t *network_get_status(void) {
    return &net_state;
}

net_stats_t *network_get_stats(void) {
    return &net_statistics;
}

int network_is_connected(void) {
    return net_state.connected;
}

/* ---- WiFi Tarama ---- */
int network_wifi_scan(wifi_network_t *results, int max_results) {
    /* Stub: Ornekler dondur */
    if (max_results < 1) return 0;

    int idx = 0;
    /* Ornek ag 1 */
    if (idx < max_results) {
        const char *n = "SUPERONLINE_5G"; int k = 0;
        while (n[k] && k < 31) { results[idx].ssid[k] = n[k]; k++; }
        results[idx].ssid[k] = 0;
        results[idx].signal_strength = 85;
        results[idx].encrypted = 1;
        results[idx].channel = 36;
        idx++;
    }
    /* Ornek ag 2 */
    if (idx < max_results) {
        const char *n = "TP-LINK_C855"; int k = 0;
        while (n[k] && k < 31) { results[idx].ssid[k] = n[k]; k++; }
        results[idx].ssid[k] = 0;
        results[idx].signal_strength = 62;
        results[idx].encrypted = 1;
        results[idx].channel = 6;
        idx++;
    }
    /* Ornek ag 3 */
    if (idx < max_results) {
        const char *n = "Misafir_Agi"; int k = 0;
        while (n[k] && k < 31) { results[idx].ssid[k] = n[k]; k++; }
        results[idx].ssid[k] = 0;
        results[idx].signal_strength = 40;
        results[idx].encrypted = 0;
        results[idx].channel = 11;
        idx++;
    }
    scan_count = idx;
    return idx;
}

/* ---- Baglan / Kes ---- */
int network_connect(const char *ssid, const char *password) {
    (void)password;
    /* Stub: Sanal baglanti simule et */
    int k = 0;
    while (ssid[k] && k < 31) { net_state.ssid[k] = ssid[k]; k++; }
    net_state.ssid[k] = 0;

    net_state.connected  = 1;
    net_state.link_speed = 100; /* 100 Mbps */
    net_state.ip[0] = 192; net_state.ip[1] = 168;
    net_state.ip[2] = 1;   net_state.ip[3] = 105;
    net_state.gateway[0] = 192; net_state.gateway[1] = 168;
    net_state.gateway[2] = 1;   net_state.gateway[3] = 1;

    /* MAC rastgele ata (simule) */
    net_state.mac[0] = 0xAE; net_state.mac[1] = 0xE1;
    net_state.mac[2] = 0x50; net_state.mac[3] = 0x5F;
    net_state.mac[4] = 0x4A; net_state.mac[5] = 0x01;

    return 0; /* basarili */
}

int network_disconnect(void) {
    net_state.connected  = 0;
    net_state.link_speed = 0;
    net_state.ip[0] = 0; net_state.ip[1] = 0;
    net_state.ip[2] = 0; net_state.ip[3] = 0;
    net_state.ssid[0] = 0;
    return 0;
}

/* ---- VPN ---- */
int network_vpn_toggle(void) {
    net_state.vpn_active = !net_state.vpn_active;
    return net_state.vpn_active;
}

/* ---- Firewall ---- */
int network_firewall_toggle(void) {
    net_state.firewall_active = !net_state.firewall_active;
    return net_state.firewall_active;
}

/* ---- MAC Spoofing ---- */
void network_mac_spoof(void) {
    net_state.mac_spoofed = 1;
    /* Rastgele MAC uret */
    net_state.mac[0] = 0x02; /* Locally administered */
    net_state.mac[1] = 0x42;
    net_state.mac[2] = 0xAE;
    net_state.mac[3] = 0x19;
    net_state.mac[4] = 0x73;
    net_state.mac[5] = 0x55;
}

/* ---- Durum Yazdir ---- */
void network_print_status(void) {
    vga_puts("  Ag Durumu: ", 0x0B);
    if (net_state.connected) {
        vga_puts("BAGLI", 0x0A);
        vga_puts("  SSID: ", 0x07);
        vga_puts(net_state.ssid, 0x0F);
        vga_puts("  Hiz: ", 0x07);
        vga_put_dec(net_state.link_speed, 0x0E);
        vga_puts(" Mbps\n", 0x07);
        vga_puts("  IP: ", 0x07);
        vga_put_dec(net_state.ip[0], 0x0F); vga_putc('.', 0x07);
        vga_put_dec(net_state.ip[1], 0x0F); vga_putc('.', 0x07);
        vga_put_dec(net_state.ip[2], 0x0F); vga_putc('.', 0x07);
        vga_put_dec(net_state.ip[3], 0x0F);
        vga_newline();
        vga_puts("  Firewall: ", 0x07);
        vga_puts(net_state.firewall_active ? "AKTIF" : "KAPALI", 
                 net_state.firewall_active ? 0x0A : 0x0C);
        vga_puts("  VPN: ", 0x07);
        vga_puts(net_state.vpn_active ? "AKTIF" : "KAPALI",
                 net_state.vpn_active ? 0x0A : 0x0C);
        vga_newline();
    } else {
        vga_puts("BAGLI DEGIL (0.0.0.0)\n", 0x0C);
    }
}
