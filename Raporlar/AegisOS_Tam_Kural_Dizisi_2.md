# 🛡️ AEGİS OS — TAM KURAL DİZİSİ VE GELİŞTİRME SPESİFİKASYONU
> **Sürüm:** v4.0 Build 5.1 | **Tarih:** 25.03.2026 | **Yazar:** Aziz Ali Artuç  
> Bu belge, `arayüz/` dizinindeki tüm tasarım ve kural dosyalarının sentezlenmiş tek kaynağıdır.  
> Her bölüm birbiriyle çapraz referanslıdır — bir bileşeni değiştirirken ilgili bölümleri mutlaka okuyun.

---

## İÇİNDEKİLER

1. [Genel Mimari ve Katman Haritası](#1-genel-mimari-ve-katman-haritası)
2. [Boot ve BIOS Sistemi](#2-boot-ve-bios-sistemi)
3. [Çekirdek (Kernel) Kuralları](#3-çekirdek-kernel-kuralları)
4. [Donanım Sürücüleri](#4-donanım-sürücüleri)
5. [Güvenlik Sistemi](#5-güvenlik-sistemi)
6. [Arayüz Katmanı (TUI/GUI)](#6-arayüz-katmanı-tuigui)
7. [Pencere ve Fare Etkileşim Sistemi](#7-pencere-ve-fare-etkileşim-sistemi)
8. [Terminal ve Komut Sistemi](#8-terminal-ve-komut-sistemi)
9. [Dosya Sistemi ve Uzantılar](#9-dosya-sistemi-ve-uzantılar)
10. [Ağ ve İnternet Yönetimi](#10-ağ-ve-internet-yönetimi)
11. [Yapay Zeka Entegrasyonu](#11-yapay-zeka-entegrasyonu)
12. [Ses ve Güvenlik Merkezi](#12-ses-ve-güvenlik-merkezi)
13. [Uygulama Ekosistemi](#13-uygulama-ekosistemi)
14. [Modüller Arası İletişim (IPC)](#14-modüller-arası-i̇letişim-ipc)
15. [Tema ve Görsel Sistem](#15-tema-ve-görsel-sistem)
16. [Sistem İzleme ve Görev Yöneticisi](#16-sistem-i̇zleme-ve-görev-yöneticisi)
17. [Ekran Ayarları ve Görünüm](#17-ekran-ayarları-ve-görünüm)
18. [Kullanıcı Hesapları Yönetimi](#18-kullanıcı-hesapları-yönetimi)
19. [Sistem Güncelleme Servisi](#19-sistem-güncelleme-servisi)
20. [Bağlı Donanımlar ve Sürücü Yönetimi](#20-bağlı-donanımlar-ve-sürücü-yönetimi)
21. [Zaman, Bölge ve Dil Ayarları](#21-zaman-bölge-ve-dil-ayarları)
22. [Uygulama Yöneticisi](#22-uygulama-yöneticisi)
23. [⚠️ DURUM RAPORU — Eksik ve Çalışmayan Bileşenler](#23-️-durum-raporu--eksik-ve-çalışmayan-bileşenler)
24. [Geliştirici Uygulama Kılavuzu — Eksikleri Tamamlama](#24-geliştirici-uygulama-kılavuzu--eksikleri-tamamlama)

---

## 1. GENEL MİMARİ VE KATMAN HARİTASI

> **İlgili dosyalar:** `Aegis_Master_Integration_Rules_v4.md`, `Aegis_System_Arch_Map.md`

### 1.1 Ring Yapısı

```
[ RING 3 — KULLANICI ALANI ]
  ├── Web Tarayıcı (Aegis Navigator)
  ├── E-Posta (Aegis Posta)
  ├── Hava Durumu Analizörü
  ├── Masaüstü & Durum Çubuğu
  └── Kütüphane (Belgeler / Oyunlar)

[ RING 1 & 2 — SERVİS ALANI ]
  ├── D-Bus / Aegis_IPC (Modüller arası iletişim protokolü)
  └── X/Y Koordinat Yayıncısı (Fare sürücüsünden GUI'ye veri taşır)

[ RING 0 — ÇEKİRDEK VE DONANIM ]
  ├── Aegis_Firewall  ←→  NIC / Wi-Fi
  ├── VFS (Sanal Dosya Sistemi)  ←→  NVMe / SSD / RAM Disk
  └── Donanım Sürücüleri  ←→  Klavye, Fare, GPU, Ses
```

### 1.2 Katman Erişim Kuralları

| Kural | Açıklama |
|-------|----------|
| **Doğrudan Donanım Yasağı** | Ring 3 uygulamaları donanıma asla doğrudan erişemez; Firewall ve IPC köprüsü üzerinden geçmek zorundadır. |
| **Ses Yönlendirmesi** | Tarayıcı ses ürettiğinde doğrudan hoparlöre gönderemez; önce `Aegis Ses Denetimi (EQ)` modülüne yollar. |
| **Dosya Kaydetme İzolasyonu** | İndirilen dosyalar önce RAM Disk'te oluşturulur → Antivirüs / Hash kontrolü → VFS aracılığıyla SSD'ye yazılır. |
| **Mutlak Sahiplik** | `/` dizini yalnızca sistem güncellemeleri tarafından değiştirilebilir. Kullanıcı `admin` bile `/home/admin/` dışına yazarken `sudo` şifresi girmek zorundadır. |
| **Çöp Kutusu Yasası** | Silinen dosyalar Geri Dönüşüm Kutusu'na gitmez; üzerine rastgele veri yazılarak donanımsal olarak yok edilir. |

---

## 2. BOOT VE BIOS SİSTEMİ

> **İlgili dosyalar:** `Aegis_Boot_Sequence_v4.md`, `Aegis_Boot_Sistemi_Geliştirme_Güçlendirme_Kural_Dizisi.md`, `Aegis_Recovery_Bootloader_v4.md`

### 2.1 Boot Akış Şeması

```
[BIOS / UEFI — POST]
       ↓
[Bootloader Stage 1]  ← 512 byte, MBR uyumlu, sadece disk okuma
       ↓
[Bootloader Stage 2]  ← Kernel yükleme, donanım bilgisi, grafik modu
       ↓
[Kernel Loader]       ← RAM'e yükleme, memory map, CPU modu ayarı
       ↓
[Kernel]              ← Ring 0 aktif
       ↓
[Init System]         ← Daemon'lar ve GUI başlatma
```

### 2.2 Boot Aşamaları ve İlerleme Çubuğu

| Yüzde | Yapılan İşlem |
|-------|--------------|
| %0 – 25 | Microkernel belleğe yükleniyor |
| %25 – 50 | Donanım sürücüleri (Fare, Ağ, Ses) aktif ediliyor |
| %50 – 75 | Dosya sistemi (VFS) ve şifreli diskler açılıyor |
| %75 – 100 | Kullanıcı arayüzü (Desktop/TUI) başlatılıyor |

Canlı boot logu:
```
[  0.000000] Initializing Aegis Microkernel v4.0.0...
[  0.102340] CPU: 16 Logical Cores detected. Using AVX-512.
[  0.254122] MEM: 32 GB RAM Hardware Integrity Check: [ OK ]
[  0.412009] VFS: Mounting /dev/nvme0n1p1 (Encrypted ZFS)... [ OK ]
[  0.684110] HAL: Loading Mouse Driver (Aegis_HID)... [ OK ]
[  0.712443] NET: Initializing Firewall & WireGuard Interface... [ OK ]
[  0.899121] MATH: Loading Python 3.12 Neural Subsystem... [ OK ]
[  1.102394] GUI: Starting Desktop Environment & Status Bar... [ OK ]
```

### 2.3 Boot Menüsü Seçenekleri

```
> 1. Aegis OS v4.0 (Normal Başlat — Tavsiye Edilen)
  2. Aegis OS v4.0 (Güvenli Mod — Ağ Kapalı)
  3. Kurtarma Konsolu (Terminal Modu)
  4. Donanım Tanılama ve Bellek Testi
```

### 2.4 Boot Operasyon Kuralları

- **Dinamik Bekleme:** Sistem açılışında BIOS 5 saniye bekler; hiç tuşa basılmazsa otomatik olarak "Normal Başlat" tetiklenir.
- **Durdurma (Pause):** Herhangi bir tuşa basılırsa sayaç durur — `DURUM: DURAKLATILDI`.
- **Donanım Güvenlik Taraması:** BIOS, donanım seri numaralarını dijital imzayla karşılaştırır. İzinsiz donanım tespit edilirse boot durur, `[FAILED]` uyarısı verir.
- **Acil Durum Kurtarma:** `aegis-repair` komutu ile fabrika ayarlarına dönüş sağlanır.
- **Boot'ta Fare Aktif:** Boot menüsünde global fare sürücüsü çalışır; seçeneklere tıklanabilir.

### 2.5 Multi-Architecture Destek

| Mimari | Mod | Boot Dizini |
|--------|-----|-------------|
| x86 | Protected / Long Mode | `/boot/x86/` |
| ARM | EL1 | `/boot/arm/` |
| RISC-V | Supervisor Mode | `/boot/riscv/` |

### 2.6 Boot Konfigürasyon Dosyası

Konum: `/boot/aegis.cfg`
```
default_kernel=aegis.krn
timeout=3
graphics=true
```

### 2.7 Kurtarma Modu (Recovery)

- Kurtarma modülü anakart ROM'unda çalışır; virüsler bu menüyü bozamaz.
- **Rollback (Zaman Yolculuğu):** Her güncellemeden önce sistem otomatik anlık görüntü (Snapshot) alır. Çökme durumunda önceki kararlı sürüme saniyeler içinde dönülür.
- **Salt Okunur Terminal:** Şifre kayıplarında disk salt okunur bağlanır, daha fazla hasar önlenir.

---

## 3. ÇEKİRDEK (KERNEL) KURALLARI

> **İlgili dosyalar:** `yapay_zaka_destekli_işletim_sistemi_araştırması_.md`, `AegisOS_Guncel_Durum_Raporu_5.1.md`

### 3.1 Bağımsızlık Prensibi (Freestanding Code)

- Çekirdek içinde `libc`, `libstdc++` veya herhangi bir POSIX kütüphanesine **çağrı yapılamaz**.
- Bellek tahsisat algoritmaları (`malloc` / `new`) fiziksel sayfa yöneticisi üzerinden yerel olarak uygulanır.
- `try/catch` ve RTTI (`dynamic_cast`) kullanmak **yasaktır**; hatalar IDT üzerinden Kernel Panic rutinlerine bağlanır.
- Global C++ nesneleri, `.init_array` bölümündeki fonksiyon işaretçileri `kernel_main` öncesinde Assembly döngüsüyle ilklendirilir.

### 3.2 CPU Optimizasyonu — HLT Tabanlı Güç Yönetimi

```c
// YANLIŞ:
while(1) { for(int i=0; i<10000; i++) { __asm__ volatile("pause"); } }

// DOĞRU:
while(1) { __asm__ volatile("hlt"); }
```

`HLT` komutu CPU'yu bir sonraki donanım kesmesine kadar tamamen durdurur. PIT her 1ms'de IRQ0 ürettiğinden sistem yanıt verme süresi korunur, güç tüketimi minimize edilir.

| Dosya | Değişiklik |
|-------|-----------|
| `kernel/kernel.c` → `os_idle_hook()` | `pause` döngüsü → `HLT` |
| `kernel/drivers/pit.c` → `pit_sleep_ms()` | `pause` → `HLT` |
| `kernel/drivers/pit.c` → `pit_sleep_ticks()` | `pause` → `HLT` |

### 3.3 Dinamik Tema Motoru — `aegis_theme_t`

```c
typedef struct {
    char     name[16];
    uint32_t bg_color;      // Arka plan
    uint32_t fg_color;      // Metin
    uint32_t accent_color;  // Vurgu (Altın / Mor)
    uint32_t border_color;  // Kenarlık
} aegis_theme_t;
```

### 3.4 Bellek Yönetimi

- **RAM-FS:** Mevcut durumda dosyalar RAM üzerindedir; sistem kapanınca silinir.
- **Geçici Çözüm:** Kritik yapılandırmalar `initrd` içinde gömülü gelir.
- **Kalıcı Çözüm Yol Haritası:**  
  1. ATA PIO Sürücüsü — HDD/SSD'ye düşük seviyeli erişim  
  2. Sync Komutu — RAM-FS verilerini periyodik olarak fiziksel sektöre yaz

---

## 4. DONANIM SÜRÜCÜLERİ

> **İlgili dosyalar:** `Aegis_GPU_CPU_Driver_Kural_Dizisi.md`, `Aegis_Ağ_Sürücüleri_Network_Start_Kural_Dizisi.md`, `Aegis_Mouse_Driver_Core_v4.md`, `Aegis_Mouse_Settings_v4.md`, `Aegis_Universal_Scroll_Engine_v4.md`

### 4.1 Grafik Sürücüsü (Framebuffer)

**Strateji:** İlk aşamada gerçek GPU driver yazmak yerine VESA/UEFI GOP Framebuffer driver kullanılır.

```c
typedef struct {
    uint32_t* base;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    uint8_t   bpp;
} framebuffer_t;

// Başlatma
int fb_init(framebuffer_t* fb) {
    fb->base   = (uint32_t*)0xE0000000;
    fb->width  = 1024;
    fb->height = 768;
    fb->pitch  = 1024 * 4;
    fb->bpp    = 32;
    return 0;
}

// Piksel çizme
void put_pixel(framebuffer_t* fb, int x, int y, uint32_t color) {
    uint32_t* pixel = fb->base + (y * fb->pitch / 4) + x;
    *pixel = color;
}
```

**Aktif özellikler:**
- ✅ Double Buffering (çift tamponlama)
- ✅ Dirty Rectangle Rendering (değişen alan takibi)
- ✅ 1024×768×32bpp framebuffer desteği
- ✅ Text mode fallback (VESA/GOP yoksa)

**GPU Abstraction Layer (AegisGL):**
```c
void agl_clear(uint32_t color);
void agl_draw_pixel(int x, int y, uint32_t color);
void agl_draw_rect(int x, int y, int w, int h, uint32_t color);
```

### 4.2 Ağ Sürücüleri

Her ağ sürücüsü şu 5 fonksiyona sahip olmalıdır:
```c
int driver_init();
int driver_send(uint8_t* data, int len);
int driver_receive(uint8_t* buffer);
int driver_interrupt_handler();
int driver_shutdown();
```

**Genel Kurallar:**
- Sürücüler donanıma doğrudan erişir, kernel ile API üzerinden konuşur.
- Interrupt (kesme) desteklemeli, event-driven olmalı (blocking değil).
- DMA varsa kullanılmalıdır.

**Ağ Sürücüsü API'leri (`network.c` / `network.h`):**

| Fonksiyon | Açıklama |
|-----------|----------|
| `network_init()` | Alt sistemi başlatır, varsayılan DNS (1.1.1.2 + 9.9.9.9) ayarlar |
| `network_detect_nic()` | PCI üzerinden Intel e1000 / Realtek RTL8139 arar |
| `network_wifi_scan()` | Çevredeki WiFi ağlarını tarar |
| `network_connect(ssid, pass)` | Belirtilen ağa bağlanır |
| `network_disconnect()` | Mevcut bağlantıyı keser |
| `network_vpn_toggle()` | WireGuard VPN tünelini açar/kapatır |
| `network_firewall_toggle()` | Güvenlik duvarını açar/kapatır |
| `network_mac_spoof()` | MAC adresini rastgele değiştirir |

**Veri Yapıları:**
- `net_status_t` — Bağlantı durumu, IP, MAC, DNS, VPN, firewall
- `net_stats_t` — TX/RX paket ve byte sayaçları
- `wifi_network_t` — SSID, sinyal gücü, şifreleme, kanal

### 4.3 Fare ve Touchpad Sürücüsü

**Fare Veri Yapısı:**
```c
typedef struct {
    int x, y;
    int left, right, middle;
    int scroll;
} mouse_t;
```

**USB Fare Paket Yapısı (3–4 Byte):**

| Byte | Anlam |
|------|-------|
| Byte 0 | Buton durumları (Bit 0=Sol, Bit 1=Sağ, Bit 2=Orta) |
| Byte 1 | X Ekseni Deltası (-127 ile +127) |
| Byte 2 | Y Ekseni Deltası (-127 ile +127) |
| Byte 3 | Tekerlek (Scroll) hareketi |

**Güvenlik — BadUSB Koruması:**  
Sistem, takılan USB farenin saniyede gönderdiği komut sayısını analiz eder. İnsanüstü hızda komut gelirse portun gücünü keser.

**Fare İvme Hesaplaması:**
```
Eğer Delta_X > 50 → hız * 1.5 (hızlandırma)
```

**Sürücü Aşamaları:**
1. **Başlatma:** `Aegis_HID_Core` servisi donanım ağacını tarar; USB HID Class=0x03/Subclass=0x01 tanıması.
2. **Veri Okuma:** Ham 3–4 byte USB paketi ayrıştırılır.
3. **GUI Çevirme:** Ham veri ivme hesabıyla GUI koordinatına dönüştürülür ve masaüstüne iletilir.
4. **Güvenlik İzolasyonu:** BadUSB, ivme limiti aşımı kontrolü.

---

## 5. GÜVENLİK SİSTEMİ

> **İlgili dosyalar:** `Aegis_Firewall_v4.md`, `Aegis_Security_and_Sound_Core.md`, `Aegis_Login_Screen_v4.md`

### 5.1 Firewall — Sıfır Güven Politikası (Zero Trust)

- **Varsayılan Yasak:** Dışarıdan içeriye (Inbound) gelen tüm bağlantılar reddedilir. Yalnızca içeriden başlatılan ve onaylanmış servislerin yanıtlarına izin verilir.
- **Uygulama İmzası:** Port numarasına ek olarak uygulamanın dijital imzası (Hash) kontrol edilir. Bilinmeyenler `UNKNOWN_PROCESS` olarak işaretlenir.
- **Derin Paket İnceleme (DPI):** Paket başlıklarına ek olarak içerik analizi yapılır; şifresiz kritik veri akışları durdurulur.
- **Port Taraması Koruması (IDS/IPS):** Ardışık SYN paketleri algılandığında saldırgan IP'si otomatik kara listeye alınır.
- **Panik Modu (F9 Kill Switch):** Tüm ağ trafiği yazılımsal olarak sıfırlanır, NIC enerjisi kesilmez; sadece lokal işlemler aktif kalır.
- **Manuel Bağlantı Kesimi (F4):** Şüpheli TCP/UDP oturumu seçilerek anında düşürülür.

**Durum Göstergesi:**
- 🟢 `[ KORUMALI ]` — Normal durum
- 🔴 `[ SALDIRI ALTINDA ]` — Tehdit algılandı

### 5.2 Giriş Ekranı (Login) Güvenliği

- Parola düz metin olarak karşılaştırılmaz; donanımsal PGP anahtarıyla tuzlanır (salt) ve hash değeri kontrol edilir.
- Yanlış giriş → 5 saniye klavye kilidi (Brute-force koruması).
- Giriş ekranı aktifken bile Firewall devrededir.
- Başarılı girişte Masaüstü, Hava Durumu, E-posta ve Ağ servisleri otomatik daemon olarak başlar.

### 5.3 Güvenlik Merkezi Modülleri

| Modül | İşlev |
|-------|-------|
| Gerçek Zamanlı Koruma | Sürekli dosya ve işlem taraması |
| Ağ Koruması | Firewall aktif izleme |
| USB Kilidi | USB portlarına erişim kısıtlama |
| Karantina | Zararlı dosya izolasyonu |
| Kullanıcı İzinleri | ROOT / USER / SCRIPT seviye yönetimi |

**Kritik Entegrasyon:** Güvenlik tehditi algılandığında Aegis Sound üzerinden 800Hz uyarı tonu çalar ve ekran kırmızıya döner.

---

## 6. ARAYÜZ KATI (TUI/GUI)

> **İlgili dosyalar:** `Aegis_Desktop_Status_Bar_v4.md`, `Aegis_Start_Menu_v4.md`, `Aegis_Control_Center_v4.md`, `Aegis_Login_Screen_v4.md`

### 6.1 Arayüz Tasarım Standartları

- **TUI Format:** Tüm modüller `30×89` karakter genişliğinde ASCII çerçeve kullanır.
- **Pencere Çerçevesi:** Her pencere `[ _ ] [ # ] [ X ]` (Küçült / Büyüt / Kapat) butonlarıyla açılır.
- **Renk Şeması:** Mor / Altın vurgu renkleri; karanlık arka plan.
- **Çözünürlük:** 1360×768 px temel GUI çözünürlüğü.

### 6.2 Masaüstü ve Durum Çubuğu

**Durum Çubuğu (sağ üst köşe):** Her 1 saniyede bir güncellenen canlı telemetri:
- Wi-Fi sinyal gücü `[#]`
- Ses seviyesi `[( ))]`
- Pil durumu `[ 100% 🔋 ]`
- Saat ve tarih `[ HH:MM | GG.AA.YYYY ]`

**Kritik Durum Kuralı:** Pil %15'in altına düştüğünde simge yanıp söner, çerçeve turuncuya döner.

**Masaüstü Bölümleri:**

| Alan | İçerik |
|------|--------|
| Sol üst | Sistem durumu (CPU, RAM, Disk hızı) |
| Sağ üst | Açık pencereler listesi |
| Sağ orta | Sistem mesajları |
| Sol orta | Hızlı erişim (F2/F3/F4) |
| Alt | Canlı terminal satırı `root@aegis:~#` |

**Canlı Terminal Satırı:** Masaüstündeyken doğrudan terminal komutları yazılabilir.

### 6.3 Başlat Menüsü (F1)

**Açılma:** `[F1]` tuşu veya sol alt köşeye tıklama.

**Kategoriler:**
```
[ SİSTEM & GÜVENLİK ]    [ AĞ & İLETİŞİM ]    [ UYGULAMALAR ]
[ DONANIM & MEDYA ]       [ AYARLAR & KONTROL ] [ OTURUM ]
```

**Operasyon Kuralları:**
1. **Type-to-Search:** Menü açılır açılmaz arama kutusu aktiftir; yazmaya başlandığında liste anında filtrelenir.
2. **Hızlı Kategori:** Sol/Sağ ok tuşlarıyla kategoriler arasında blok halinde geçilir.
3. **Mini Terminal:** Arama kutusuna doğrudan çekirdek komutları (`ping 8.8.8.8`, `kill 1245`) yazılıp çalıştırılabilir.

### 6.4 Kontrol Merkezi

**Kategoriler:** Ağ ve İnternet | Sistem & Donanım | Kişiselleştirme | Ses | Zaman ve Bölge | Fare | Güvenlik

**Operasyon Kuralları:**
1. **Birleştirilmiş Veritabanı:** Tüm `.md` modüllerini köprü gibi çağırır; seçilen alt menü ilgili modülü üst ekrana açar.
2. **Canlı Durum Önizlemesi:** Menüye girmeden ağın o anki durumu alt çizgi bölümünde görünür.
3. **Merkezi Kayıt:** Tüm değişiklikler `/etc/aegis/config` dosyasına yazılır; bu dosyayı kopyalamak tüm ayarları taşır.

### 6.5 TAB Navigasyon Kuralı

| Basış | Sonuç |
|-------|-------|
| 1. basış | Sol "Kategori" paneline odaklan |
| 2. basış | Sağ "Detay / Eylem" paneline geç |

**Focus Yönetimi:**
```
TAB → Current_Focus_Index += 1
Liste sonu → Index = 0 (döngüsel)
Fare tıklaması → Index = tıklanan öğenin numarası
```
Klavye ve fare her zaman aynı noktayı senkronize olarak takip eder.

---

## 7. PENCERE VE FARE ETKİLEŞİM SİSTEMİ

> **İlgili dosyalar:** `Aegis_Pencere_Sistemi_Fare_Etkileşim_Kural_Dizisi.md`, `Aegis_Mouse_Driver_Core_v4.md`

### 7.1 Pencere Veri Yapısı

```c
typedef struct {
    int x, y;
    int width, height;
    int isDragging;
    int isResizing;
    int isMinimized;
    int isMaximized;
} window_t;
```

### 7.2 Pencere Kontrol Alanları (Sağ Üst Butonlar)

```
[ - ] Küçült  →  (width - 60, 0)
[ □ ] Büyüt   →  (width - 40, 0)
[ X ] Kapat   →  (width - 20, 0)
```

### 7.3 Tıklama Algılama

```c
int is_inside(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx <= x+w && my >= y && my <= y+h);
}
```

### 7.4 Fare → Masaüstü Veri Akışı

1. Fare sürücüsü saniyede 1000 kez X/Y verisi üretir.
2. Bu veri `Aegis_TUI_Mouse_Tracker` servisine akar.
3. Masaüstündeki tüm butonların "Bounding Box" koordinatları bu veriyi dinler.
4. Koordinat eşleşmesi → tıklama olayı tetiklenir.

---

## 8. TERMİNAL VE KOMUT SİSTEMİ

> **İlgili dosyalar:** `Aegis_Terminal_UI_Komut_ve_Kural_Dizisi.md`, `Aegis_Terminal_Settings_v4.md`, `Aegis_System_Core_Apps.md`

### 8.1 Genel Felsefe

- CLI ve UI birlikte çalışır; arayüz yalnızca komutları tetikleyen bir katmandır.
- Terminal her zaman en yetkili kontrol noktasıdır.
- **Tüm komutlar büyük/küçük harf duyarsızdır (case-insensitive).**

### 8.2 Komut Yapısı Standardı

```
komut [alt-komut] [parametreler] [--opsiyonlar]
```

Örnekler:
```
file create test.txt --size=1kb
net connect wifi --ssid="EvWifi" --pass="12345678"
```

### 8.3 Komut Tablosu (Türkçe + İngilizce)

**Dosya Sistemi:**

| İngilizce | Türkçe | Açıklama |
|-----------|--------|----------|
| `ls` | `listele` | Dizin içeriğini göster |
| `cd` | `git` | Dizin değiştir |
| `pwd` | `konum` | Mevcut dizini göster |
| `file create` | `dosya oluştur` | Yeni dosya oluştur |
| `file delete` | `dosya sil` | Dosya sil |
| `file read` | `dosya oku` | Dosya içeriğini göster |
| `file write` | `dosya yaz` | Dosyaya yaz |

**Sistem:**

| İngilizce | Türkçe | Açıklama |
|-----------|--------|----------|
| `sys info` | `sistem bilgi` | Sistem bilgisi |
| `sys reboot` | `sistem yenidenbaşlat` | Yeniden başlat |
| `sys shutdown` | `sistem kapat` | Sistemi kapat |
| `sys monitor` | `sistem izleme` | CPU / RAM durumu |

**Ağ:**

| İngilizce | Türkçe | Açıklama |
|-----------|--------|----------|
| `net scan` / `ag tara` | `ağ tara` | WiFi ağlarını bul |
| `net connect` / `ag baglan` | `ağ bağlan` | Ağa bağlan |
| `net disconnect` / `ag kes` | `ağ kes` | Bağlantıyı kes |
| `net status` / `ag durum` | `ağ durum` | Bağlantı durumu |
| `vpn` | — | WireGuard VPN aç/kapat |
| `firewall` | `guvenlik duvari` | Firewall aç/kapat |
| `mac spoof` | `mac degistir` | MAC rastgele değiştir |

**Kullanıcı:**

| İngilizce | Türkçe | Açıklama |
|-----------|--------|----------|
| `user add` | `kullanıcı ekle` | Yeni kullanıcı |
| `user del` | `kullanıcı sil` | Kullanıcı sil |
| `user login` | `giriş yap` | Sisteme giriş |
| `user logout` | `çıkış yap` | Sistemden çıkış |

**Uygulama:**

| İngilizce | Türkçe | Açıklama |
|-----------|--------|----------|
| `app install` | `uygulama yükle` | Uygulama kur |
| `app remove` | `uygulama kaldır` | Sil |
| `app run` | `uygulama çalıştır` | Başlat |
| `app list` | `uygulama liste` | Listele |

**Oyunlar:**

| Komut | Açıklama |
|-------|----------|
| `snake` | Aegis Snake Neon |
| `tetris` | Aegis Tetris Classic |
| `pentomino` / `pento` | Pentomino Challenge |
| `minefield` | Aegis Minefield |
| `xox` | Aegis XOX |

**Global GUI Komutları:**

| Komut | Açıklama |
|-------|----------|
| `aegis-gui` | Masaüstü ortamını başlatır |
| `aegis-net --scan` | WiFi ağlarını terminale listeler |
| `aegis-net --vpn connect` | WireGuard tüneline girer |
| `aegis-weather --now` | Anlık hava durumunu tek satır basar |
| `aegis-mail --check` | Yeni mail varsa bildirim düşürür |
| `killall -9 aegis-browser` | Donmuş tarayıcıyı çekirdek seviyesinde siler |

### 8.4 Case-Insensitive Motor

```
INPUT:  FiLe CrEaTe Test.txt
ENGINE: file create test.txt
```
Tüm komutlar sistemde küçük harfe normalize edilir.

---

## 9. DOSYA SİSTEMİ VE UZANTILAR

> **İlgili dosyalar:** `Aegis_Dosya_Sistemi_Uzantı_Uyumluluk_Kural_Dizisi.md`, `Aegis_File_Explorer_v4.md`, `Aegis_Explorer_Lite.md`

### 9.1 Mimari

```
[ Evrensel Dosyalar (.jpg, .mp3, .pdf) ]
                ↓
[ Uyumluluk Katmanı ]
                ↓
[ AegisOS Native Formatlar (.ae*) ]
                ↓
[ Uygulama Katmanı ]
```

### 9.2 AegisOS Özgün Uzantı Sistemi

**Genel yapı:** `.ae[tip][alt-tip]`

**Metin ve Belgeler:**

| Tür | Uzantı |
|-----|--------|
| Metin | `.aetx` |
| Zengin metin | `.aert` |
| Belge | `.aedoc` |
| PDF alternatifi | `.aepg` |
| Kaynak kodu | `.aesrc` |

**Görseller:**

| Tür | Uzantı |
|-----|--------|
| Genel | `.aeimg` |
| Sıkıştırılmış | `.aeic` |
| Kayıpsız | `.aeil` |
| Katmanlı | `.aepsd` |

**Ses:**

| Tür | Uzantı |
|-----|--------|
| Genel | `.aeau` |
| Müzik | `.aemx` |
| Kayıpsız | `.aeal` |

**Video:**

| Tür | Uzantı |
|-----|--------|
| Video | `.aevi` |
| Streaming | `.aestr` |
| 4K | `.ae4k` |

**Sistem ve Uygulama:**

| Tür | Uzantı |
|-----|--------|
| Uygulama | `.aeapp` |
| Kütüphane | `.aelib` |
| Sürücü | `.aedrv` |
| Kernel modülü | `.aekmod` |
| Paket | `.aepkg` |

**Sistem Dosyaları:**

| Tür | Uzantı |
|-----|--------|
| Konfig | `.aecfg` |
| Log | `.aelog` |
| Cache | `.aecache` |
| Sistem imajı | `.aeimgx` |

### 9.3 Dosya Sistemi Kuralları

- **Tombstone Metodu:** Dev dosyalar silindiğinde fiziksel bloklar taşınmaz; Inode metaverisi güncellenerek alan boş işaretlenir (fragmentasyon önleme).
- **Zero-Copy Akış:** AI model ağırlıkları (`.safetensors`, `.gguf`) NVMe DMA yoluyla doğrudan inference belleğine kopyasız aktarılır.
- **Asenkron NVMe Kuyrukları:** Submission Queue (SQ) + Completion Queue (CQ) yapısıyla donanımla non-blocking iletişim.

---

## 10. AĞ VE İNTERNET YÖNETİMİ

> **İlgili dosyalar:** `Aegis_Network_Settings_v4.md`, `Aegis_Network_Scanner_v4.md`, `Aegis_WiFi_Manager_v4.md`, `Aegis_Internet_Options_v4.md`, `Aegis_Ağ_Sürücüleri_Network_Start_Kural_Dizisi.md`

### 10.1 Ağ Stack Mimarisi

```
[Donanım]
   ↓
[Driver Layer]      ← network.c / network.h
   ↓
[Network Stack]     ← TCP/IP, DNS, DHCP
   ↓
[Socket API]
   ↓
[Uygulamalar]       ← Tarayıcı, Posta, Hava Durumu
```

### 10.2 Servis Port İzolasyonu

| Servis | İzin Verilen Port | Protokol |
|--------|------------------|----------|
| E-posta alımı | 993 | IMAP/SSL |
| E-posta gönderme | 465 | SMTP/SSL |
| Web tarayıcı | 443 | HTTPS/TLS 1.3 |
| Hava durumu | 443 | TLS 1.3 |
| DNS | 53 | UDP |
| SSH | 22 | SSH |

### 10.3 Hava Durumu Tetikleyici Kuralı

Hava durumu modülü tetiklendiğinde:
1. Masaüstündeki saat 06:00'ı gösterdiğinde **veya** kullanıcı terminal komutu girdiğinde.
2. İsteği doğrudan internete değil, Aegis Firewall'a gönderir.
3. Firewall hedefin meteoroloji sunucusu (Port 443, TLS 1.3) olduğunu doğrular ve NIC'e okuma yetkisi verir.

### 10.4 Boot Entegrasyonu

Boot sequence 9. adım: `"Ağ sürücüleri yükleniyor..."`
- `network_init()` çağrılır.
- `network_detect_nic()` PCI taraması yapar.

---

## 11. YAPAY ZEKA ENTEGRASYONU

> **İlgili dosyalar:** `Aegis_Native_AI_Assistant_v4.md`, `Aegis_Neural_Network_Manager_v4.md`, `yapay_zaka_destekli_işletim_sistemi_araştırması_.md`

### 11.1 Çekirdek Seviyesi AI Entegrasyonu (Ring 0)

- Yapay zeka kullanıcı katmanında değil, **doğrudan Ring 0 çekirdeğine gömülü** olarak çalışır.
- Görev zamanlaması, bellek tahsisi, güç yönetimi statik algoritmalar yerine Deep Learning / Reinforcement Learning ile yönetilir.
- AI motoru `Ollama` veya harici binary'ye bağımlı değildir; C/C++ ile sıfırdan yazılmıştır.

### 11.2 Gömülü LLM Çıkarım Motoru

- Model formatı: `.gguf` (PGP şifreli)
- Hızlandırma: CPU'da AVX/AVX-512, GPU'da Compute Shader
- Motor tamamen Sandbox içinde çalışır; ağa ve rastgele dosyalara erişimi yoktur.

**SIMD / AVX-512 ile Matris Operasyonları:**
```c
// 4 double'ı YMM yazmacına yükle
__m256d a = _mm256_load_pd(&matrix_a[i]);
__m256d b = _mm256_load_pd(&matrix_b[i]);
// Tek döngüde çarp ve topla (Fused Multiply-Add)
result = _mm256_fmadd_pd(a, b, result);
```

**FPU Durumu Koruma (Kesmelerde):**
```c
// Kesme başında
fxsave(fpu_state_buffer);
// Kesme sonunda
fxrstor(fpu_state_buffer);
```

### 11.3 AI Destekli Bellek Yönetimi

- **LSTM Tabanlı Öngörülü Sayfa Getirme:** LRU/FIFO yerine LSTM modeli bellek erişim paternlerini analiz ederek sayfa hatası olmadan prefetch yapar.
- **CXL Bellek Havuzu:** Fiziksel RAM yetersizliğinde Swap yerine CXL protokolüyle ağ belleklerine cache-coherent erişim.

### 11.4 RL Tabanlı CPU Zamanlayıcısı

- **Durum Uzayı:** Sıcaklık, IPC, L1/L2 önbellek ıskalama oranları.
- **Eylem:** P-State artışı/azalması, C-State seçimi.
- **Ödül:** Düşük gecikme + düşük güç tüketimi.
- MSR üzerinden doğrudan frekans kontrolü:
  - `IA32_PERF_CTL` (0x198) → Hedef P-state yaz
  - `IA32_PERF_STATUS` (0x199) → Mevcut durum oku
  - `IA32_APERF` / `IA32_MPERF` → Gerçek frekans ölçümü

### 11.5 Nöral Ağ Yöneticisi Kuralları

1. **Sıcak Değişim (Hot-Swap):** Model değişimi için yeniden başlatma gerekmez; mevcut model RAM'den sılınır, yenisi yüklenir.
2. **OOM Koruması:** Model RAM/VRAM sınırını aşıyorsa sistem donmaz; yüklemeyi reddeder veya quantization uygular.
3. **RAG — Kendi Verin:** PDF/TXT/Log dosyaları vektör veritabanına (.vec) dönüştürülür; AI kendi verilerinden yanıt verir, uydurmaz.
4. **Kaynak Gösterme:** Yanıt verildiğinde `(Kaynak Dosya: dosya_adı - Satır: N)` gösterilir.
5. **Tam İzolasyon:** Firewall bu modülün tek bir byte bile dışarıya veri göndermesini donanım seviyesinde engeller.

### 11.6 AI Asistan Operasyon Kuralları

1. **Sıfır Ağ Verisi:** Bilimsel Asistan çalışırken ağ bağlantısına izin verilmez. Model güncellemeleri yalnızca PGP imzalı USB ile yapılır.
2. **AI Modu:** Analiz başladığında e-posta, hava durumu gibi servisler dondurulur; CPU/GPU gücünün %90'ı LLM motoruna ayrılır.
3. **Güvenli Kayıt (F10):** Formül + analiz birleştirilip PGP şifrelenerek `.asc` / `.gpg` olarak kütüphaneye kaydedilir.

---

## 12. SES VE GÜVENLİK MERKEZİ

> **İlgili dosyalar:** `Aegis_Security_and_Sound_Core.md`

### 12.1 Ses Yönetim Merkezi

**Bölümler:**

| Bölüm | İşlev |
|-------|-------|
| Ses Çıkışı | Cihaz seçimi, master ses seviyesi |
| Ekolayzır (EQ) | 60Hz / 1kHz / 10kHz bantları, Dinamik mod |
| Ses Girişi (Mikrofon) | Cihaz, giriş seviyesi, AI gürültü engelleme |
| Uygulama Ses Mikseri | Explorer, Writer, Arcade, System bağımsız seviyeler |

**Mikrofon Aktifliği Göstergesi:** Mikrofon aktifken terminal ve üst barda turuncu nokta belirir (gizlilik).

**Kritik Entegrasyon:** Tarayıcıda ses açıldığında → `Aegis Ses Denetimi (EQ)` → Ekolayzır → Hoparlör zinciri zorunludur.

---

## 13. UYGULAMA EKOSİSTEMİ

> **İlgili dosyalar:** `Aegis_System_Core_Apps.md`, `Aegis_User_Library_v4.md`, `Aegis_MathLab_Python_IDE_v4.md`, `Aegis_Ultra_IDE.md`, `Aegis_Writer.md`, `Aegis_Commander_Pro.md`, `Aegis_Web_Browser_v4.md`, `Aegis_Posta_İletişim_Merkezi.md`, `Aegis_Hava_Durumu.md`, `Aegis_Kontrol_Paneli.md`, `Aegis_Mount_Manager_v4.md`

### 13.1 Çekirdek Uygulamalar

| Uygulama | Komut | Açıklama |
|----------|-------|----------|
| Aegis Terminal | `aegis-gui` | Komut merkezi, Ultimate Shell |
| Aegis Writer Pro | `edit <dosya>` | Markdown editörü |
| Aegis File Explorer | — | Dosya yöneticisi |
| Aegis Commander Pro | `[F2]` | Çift panel gelişmiş yönetici |
| Aegis Navigator | `[F3]` | Web tarayıcı |
| Aegis Posta | `[F4]` | E-posta istemcisi |
| Aegis Kontrol Paneli | — | Tek uygulama kontrol merkezi |
| Aegis MathLab/IDE | — | Python IDE + Bilimsel Asistan |
| Aegis Ultra IDE | — | Gelişmiş kod editörü |
| Aegis Mount Manager | — | Disk bağlama/çıkarma yönetimi |

### 13.2 Oyun Sistemi (Aegis Arcade)

| Oyun | Zorluk Seviyeleri | Özellik |
|------|------------------|---------|
| **Snake Neon** | Çırak / Amatör / Uzman / Ninja / Efsane | Duvar geçişi, labirent modu |
| **Tetris Classic** | Acemi / Çırak / Uzman / Usta / Grand | 5 seviye hız, kombo sistemi |
| **Minefield** | Kolay (10×10) / Orta (15×15) / Zor (20×20) | Mayın tarlası |
| **XOX** | — | Klasik X/O |
| **Pentomino Challenge** | — | 12 pentomino, 10×20 alan |

**Oyun Modu Kuralı:** Oyun başladığında gereksiz ağ taramaları ve cron görevleri dondurulur; CPU gücünün %100'ü oyuna aktarılır.

**Puanlama (Pentomino):** `puan = cl² × 150`, PENTOMINO bonusu (5 satır birden) = 5000 puan.

### 13.3 Sandboxing Kuralı

Her uygulama çekirdekten izole "kum havuzu" içinde çalışır. İzinler:
- Ağ erişimi: Açık/Kapalı (granüler)
- Disk erişimi: Belirtilen dizinlerle sınırlı
- CPU limiti: Maksimum yüzde atanabilir
- RAM limiti: Megabayt cinsinden sınırlanabilir

**Sessiz Kaldırma (Wipe):** Uygulama kaldırıldığında paket, geçici dosyalar, önbellek ve logların tamamı diskten kazınır.

### 13.4 Kütüphane Operasyon Kuralları

- **Terminal Render Motoru:** Oyunlar grafik pencere açmaz; Ncurses/TUI veya DOSBox emülatörü ile çalışır.
- **Belge Önizleme:** Dosya seçildiğinde `less` mantığıyla sağ panelde anında önizleme.
- **PDF Metin Çıkarımı:** `pdftotext` ile içerik terminale metin olarak basılır.
- **Askeri Şifreleme:** `[F4]` ile belge PGP özel anahtarıyla `.asc`/`.gpg` formatına çevrilir.

---

## 14. MODÜLLER ARASI İLETİŞİM (IPC)

> **İlgili dosyalar:** `Aegis_Master_Integration_Rules_v4.md`

### 14.1 IPC Kuralları

| Senaryo | Akış |
|---------|------|
| **Posta → Tarayıcı** | Postada link tıklandığında `aegis-open-url` sinyali → Navigator başlatılır ve URL iletilir |
| **Dosya Yöneticisi → Müzik** | `.mp3` tıklandığında `Müzik Çalar` servisi uyandırılır → Durum çubuğuna bildirim gider |
| **Fare → Masaüstü** | 1000 Hz veri → `Aegis_TUI_Mouse_Tracker` → Bounding Box koordinat dinleyicileri |
| **Nöral Ağ → Firewall** | Zararlı IP tespit edilince IPC üzerinden Firewall'a kural yazdırılır |

### 14.2 Güvenlik Entegrasyon Zinciri

```
Güvenlik Tehditi
     ↓
Aegis Security Merkezi (algılama)
     ↓
Aegis Sound (800Hz uyarı tonu)
     ↓
Ekran (kırmızıya döner)
     ↓
Firewall (saldırgan IP kara listeye)
     ↓
Durum Çubuğu ([ SALDIRI ALTINDA ] göstergesi)
```

### 14.3 Global Terminal Entegrasyonu

Tüm grafiksel modüller terminal komutlarıyla tetiklenebilir:
- `aegis-gui` → Masaüstü başlatır
- `aegis-net --scan` → WiFi listesi
- `aegis-net --vpn connect` → VPN tüneli
- `aegis-weather --now` → Tek satır hava durumu
- `aegis-mail --check` → Posta bildirimi

---

## 15. TEMA VE GÖRSEL SİSTEM

> **İlgili dosyalar:** `AegisOS_Guncel_Durum_Raporu_5.1.md`, `Aegis_Cursor_Design_v4.md`

### 15.1 Varsayılan Renk Paleti

| Alan | Renk | Açıklama |
|------|------|----------|
| Arka Plan | Siyah / Koyu Gri | `bg_color` |
| Metin | Beyaz | `fg_color` |
| Vurgu | Mor + Altın | `accent_color` |
| Kenarlık | Koyu Mor | `border_color` |
| Uyarı | Turuncu | Pil düşük, mikrofon aktif |
| Tehdit | Kırmızı | Güvenlik alarmı |
| Güvenli | Yeşil | Normal firewall durumu |
| Terminal Font | Neon Yeşil Monospace | `%85 saydamlık` |

### 15.2 Tema Yapısı

```c
typedef struct {
    char     name[16];
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t accent_color;
    uint32_t border_color;
} aegis_theme_t;
```

Yeni tema eklemek için bu struct'ı doldurun ve `Kontrol Merkezi → Kişiselleştirme` altına kaydedin. Yapılan değişiklik otomatik olarak `/etc/aegis/config` dosyasına yazılır.

### 15.3 TUI Çerçeve Standartları

- Tüm modüller `#` karakteriyle çerçevelenir.
- Başlık satırı: `### [ MODÜL ADI ] [ _ ] [ # ] [ X ] ###`
- Alt kısayol satırı: `### [TUŞLAR VE İŞLEVLER] ###`
- Minimum genişlik: 89 karakter
- Minimum yükseklik: 30 satır

---

## 16. SİSTEM İZLEME VE GÖREV YÖNETİCİSİ

> **İlgili dosyalar:** `Sistem_Izleme_Paneli.md`, `Aegis_Kontrol_Paneli.md`

### 16.1 Arayüz Bölümleri

| Panel | İçerik |
|-------|--------|
| Kaynak Kullanımı | CPU %, RAM %, GPU %, ASCII bar grafikleri |
| Sıcaklık & Fan | CPU/GPU °C, Fan RPM; 80°C üzerinde yanıp söner |
| Depolama & Ağ | Disk %, SWAP %, anlık Down/Up hızı |
| Çekirdek Yükü | C1–C4 çekirdek bazlı kullanım barları |
| Bellek Detayı | Kullanılan / Toplam GB |
| Görev Yöneticisi | PID, İşlem adı, CPU %, Bellek MB, Durum |

### 16.2 Görev Yöneticisi Operasyon Kuralları

- **Hibrit Görünüm:** Üst panel donanım kaynaklarını, alt panel bu kaynakları kimin tükettiğini PID bazlı gösterir.
- **Öncelikli Uyarı:** CPU kullanımı %20'yi aşan süreç listenin en başına taşınır.
- **Dinamik Sıcaklık:** CPU veya GPU 80°C üzerine çıkarsa o satırdaki değerler yanıp söner.
- **Güç Modu Senkronizasyonu:** `[F8]` ile güç modu değiştiğinde çekirdek yükü ve fan RPM anlık tepki verir.
- **Bellek Eşiği:** RAM %90'ı geçince sistem otomatik SWAP artırır ve kullanıcıyı uyarır.
- **Süreç Durdurma:** `[F1]` ile seçili PID güvenli şekilde sonlandırılır (SIGTERM → SIGKILL).
- **Öncelik Ayarı:** `[F2]` ile süreç önceliği (nice değeri) değiştirilebilir.

### 16.3 Güç Modları

| Mod | Tanım |
|-----|-------|
| Güç Tasarrufu | CPU frekansı minimum, C3/C4 uyku öncelikli |
| Dengeli | RL zamanlayıcısı varsayılan politikası |
| Yüksek Performans | P0 state sabit, HLT devre dışı |
| AI Modu | %90 kaynak LLM motoruna, arka plan servisler dondurulur |
| Oyun Modu | %100 kaynak oyuna, ağ tarama ve cron görevleri askıya alınır |

### 16.4 CPU Optimizasyonu — Çalışma Durumu ve Hedef

> ⚠️ **SON RAPOR SONRASI DURUM:** CPU optimizasyonu (`HLT` mekanizması) çalıştırılmış ancak görev yöneticisi arayüzüyle tam entegrasyon tamamlanmamıştır. Aşağıdaki tablo hedef durumu göstermektedir.

**Hesaplama formülü:**
```
CPU_Usage = 100 × (1 - idle_ticks / total_ticks)
```

**Eklenecek görev yöneticisi kaydı:**
```c
// kernel/apps/taskmanager.c içinde
uint64_t idle_start = rdtsc();
asm volatile("hlt");
uint64_t idle_end   = rdtsc();
idle_ticks += (idle_end - idle_start);
total_ticks += TIMER_INTERVAL_TICKS;
cpu_usage = 100 - (idle_ticks * 100 / total_ticks);
```

---

## 17. EKRAN AYARLARI VE GÖRÜNÜM

> **İlgili dosyalar:** `Ekran_Ayarlari.md`, `Aegis_Desktop_Status_Bar_v4.md`, `Aegis_Cursor_Design_v4.md`

### 17.1 Ayar Kategorileri

| Kategori | Seçenekler |
|----------|-----------|
| Çözünürlük | 1360×768 (varsayılan), 1920×1080, 1280×720, özel |
| Yenileme Hızı | 60 Hz, 75 Hz, 120 Hz, 144 Hz |
| Ölçeklendirme | %100, %125, %150 |
| Yönlendirme | Yatay, Dikey, Ters Yatay, Ters Dikey |
| Tema | Neon Dark (varsayılan), Aegis Classic, High Contrast |
| Parlaklık | %0–%100 (kaydırıcı) |
| Gece Işığı | Otomatik (gün batımı/doğumu) veya manuel saat aralığı |
| Kontrast | %0–%100 |
| Yazı Tipi | Aegis Sans Mono, boyut 11pt, antialiasing aktif |
| Renk Profili | sRGB Standard, Display P3, Greyscale |

### 17.2 Ekran Yönetim Kuralları

- **Güvenli Mod Geri Sayımı:** Çözünürlük değiştiğinde 15 saniye onay penceresi açılır; onaylanmazsa eski ayara dönülür.
- **Dinamik TUI Ölçekleme:** 1360×768 temel çözünürlükte tüm ASCII pencereler karakter kaybı olmadan otomatik ölçeklenir.
- **Mavi Işık Filtresi:** Gece Işığı modu saate göre 6500K → 2700K kademeli geçiş yapar.
- **Çoklu Monitör:** İkinci ekran algılandığında "Genişlet / Yinele / Sadece İkinci" seçeneği sunulur. `[F10]` hızlı geçiş.
- **Seyahat Algılama:** IP veya GPS değişiminde saat dilimi güncelleme önerisi çıkar.

### 17.3 GPU Bilgi Paneli

```
MODEL     : Aegis Graphics X1
VRAM      : 4 GB GDDR6
SICAKLIK  : 48°C (eşik: 80°C)
Framebuffer: 1360×768×32bpp
```

> ⚠️ **SON RAPOR SONRASI DURUM:** Ekran ayarları modülü (`Ekran_Ayarlari.md`) tasarlanmış; kernel'e eklenmemiş. Bkz. Bölüm 23.

---

## 18. KULLANICI HESAPLARI YÖNETİMİ

> **İlgili dosyalar:** `Aegis_Login_Screen_v4.md`, `Aegis_Control_Center_v4.md`, `Aegis_Security_and_Sound_Core.md`

### 18.1 Kullanıcı Seviyeleri

| Seviye | Etiket | Yetkiler |
|--------|--------|----------|
| Root | `[ROOT]` | Tüm sistem yetkisi; `/etc/aegis/config` dahil her dosyaya erişim |
| Admin | `[ADMIN]` | `/home/admin/` tam yetki; sistem dışı alanlara sudo şifresi gerekir |
| User | `[USER]` | Yalnızca kendi home dizini ve atanan uygulamalar |
| Script | `[SCRIPT]` | Otomatik bot/daemon hesabı; ağ ve disk erişimi kısıtlı |
| Guest | `[GUEST]` | Oturum sonunda tüm veriler silinir; kalıcı yazma hakkı yok |

### 18.2 Kimlik Doğrulama Akışı

```
Kullanıcı şifre girer
        ↓
Çekirdek → donanımsal PGP anahtarıyla tuzla (salt)
        ↓
SHA-256 hash hesapla
        ↓
Kayıtlı hash ile karşılaştır
        ↓
Eşleşme yok → [ ERİŞİM REDDEDİLDİ ] + 5 sn klavye kilidi
Eşleşme var → Masaüstü başlatılır
```

### 18.3 Hesap Yönetimi Komutları

| Komut | Türkçe | Açıklama |
|-------|--------|----------|
| `user add <ad> --level=user` | `kullanıcı ekle` | Yeni kullanıcı oluştur |
| `user del <ad>` | `kullanıcı sil` | Kullanıcıyı ve home dizinini kaldır |
| `user passwd <ad>` | `kullanıcı şifre` | Şifre değiştir |
| `user list` | `kullanıcı liste` | Tüm hesapları listele |
| `user lock <ad>` | `kullanıcı kilitle` | Hesabı geçici dondur |

### 18.4 Brute-Force Koruması

- 3 başarısız girişte → 5 saniye bekleme
- 5 başarısız girişte → 30 saniye bekleme
- 10 başarısız girişte → Hesap kilitlenir, sadece ROOT açabilir
- Tüm başarısız girişler Firewall log'una IP ile kaydedilir

> ⚠️ **SON RAPOR SONRASI DURUM:** Login ekranı çalışıyor; ancak kullanıcı ekleme/silme GUI paneli (`user management UI`) kernel'e bağlanmamış. Bkz. Bölüm 23.

---

## 19. SİSTEM GÜNCELLEME SERVİSİ

> **İlgili dosyalar:** `Uygulama_Yoneticisi.md`, `Aegis_Control_Center_v4.md`, `AegisOS_Guncel_Durum_Raporu_5.1.md`

### 19.1 Güncelleme Türleri

| Tür | Açıklama | Öncelik |
|-----|----------|---------|
| Kritik Yama | Güvenlik açığı kapatma | 🔴 Zorunlu |
| Kernel Güncellemesi | Çekirdek sürüm yükseltme | 🟠 Önerilir |
| Sürücü Güncellemesi | Donanım sürücü yamaları | 🟡 İsteğe bağlı |
| Uygulama Güncellemesi | Kullanıcı uygulamaları | 🟢 Arka planda |

### 19.2 Güncelleme Protokolü

```
1. Yeni güncelleme algılandı
        ↓
2. Otomatik Snapshot (anlık görüntü) alınır
        ↓
3. Kullanıcıya bildirim → Onay
        ↓
4. Güncelleme RAM Disk'te hazırlanır
        ↓
5. Antivirüs / Hash doğrulaması
        ↓
6. Kernel modülü güncellenir (canlı patch mümkünse)
        ↓
7. Yeniden başlatma gerekiyorsa bildirim
        ↓
8. Başarısız → Snapshot'tan otomatik rollback
```

### 19.3 Güncelleme Yönetimi Kuralları

- **Geri Alma:** Her güncelleme öncesi otomatik Snapshot alınır; hata durumunda `aegis-repair` ile geri dönülür.
- **Kritik Uygulama:** Güvenlik açığı taşıyan uygulama güncellenene kadar `[PASİF]` moda alınır.
- **Bağımlılık Yönetimi:** Eksik kütüphane/lib varsa kullanıcıya bildirilmeden sistem deposundan otomatik tamamlanır.
- **Akıllı Önbellek:** Uzun süre kullanılmayan uygulamalar "Sıkıştırma Modu"na alınarak disk alanı açılır.
- **Sessiz Kaldırma (Wipe):** Kaldırılan uygulama + tüm geçici dosyalar, önbellek ve log tamamen silinir.

> ⚠️ **SON RAPOR SONRASI DURUM:** Güncelleme servisi tasarımı tamamlanmış; `install_uefi.sh` güncellendi ancak otomatik güncelleme daemon'u ve GUI paneli kernel'e entegre edilmemiş. Bkz. Bölüm 23.

---

## 20. BAĞLI DONANIMLAR VE SÜRÜCÜ YÖNETİMİ

> **İlgili dosyalar:** `Bagli_Donanimlar.md`, `Aegis_GPU_CPU_Driver_Kural_Dizisi.md`, `Aegis_Ağ_Sürücüleri_Network_Start_Kural_Dizisi.md`, `Aegis_Mouse_Driver_Core_v4.md`

### 20.1 Desteklenen Sürücü Kategorileri

| Kategori | Durum | Öncelik |
|----------|-------|---------|
| Fare / Touchpad | ✅ Stub çalışıyor | Tamamlandı |
| Klavye (HID) | ✅ Aktif | Tamamlandı |
| Ses Kartı | ✅ Temel çalışıyor | Tamamlandı |
| GPU / Framebuffer | ✅ VESA/GOP stub | Aktif |
| Ethernet (e1000 / RTL8139) | 🔄 Stub yazıldı | Gerçek aktivasyon bekliyor |
| WiFi Sürücüsü | ⚠️ Simülasyon | Firmware + WPA2 eksik |
| Bluetooth Sürücüsü | ❌ Yok | Geliştirilmedi |
| USB Sürücüsü (Host Controller) | ⚠️ Kısmi | HID mevcut; mass storage eksik |
| SAS Sürücüsü | ❌ Yok | Planlanmadı |
| NVMe DMA | 🔄 Tasarım tamamlandı | Gerçek uygulama bekliyor |

### 20.2 Plug & Play Algılama Kuralları

- Yeni USB cihazı takıldığında Kernel 100ms içinde tanır ve listeye ekler.
- Klavye/fare koptuğunda ekran ortasında `[DONANIM KAYBI]` uyarısı belirir.
- Her donanıma benzersiz `HWID` atanır; porta göre değil, kimliğe göre ayarlar saklanır.
- USB Suspend: Aktif olmayan donanımlar `Idle` moduna çekilir.

### 20.3 Sürücü Güvenlik Kuralları

- Her sürücünün dijital imzası ve versiyonu kontrol edilir.
- Uyumsuz sürücü algılanırsa cihaz güvenli moda alınır; `Hatalı Sürücü` sayacı artar.
- Sürücü güncellemesi `[F8]` → Aegis Server → hash doğrulaması → yükleme.

### 20.4 WiFi Sürücüsü Geliştirme Kuralları

```c
// Basitleştirilmiş WiFi Driver modeli
int wifi_init();
int wifi_scan(wifi_network_t* results, int max);
int wifi_connect(const char* ssid, const char* pass);
int wifi_disconnect();
```

**Gerçek WiFi driver için zorunlu adımlar:**
1. Firmware yükleme (proprietary firmware blob veya açık kaynak alternatif)
2. WPA/WPA2 şifreleme stack entegrasyonu
3. RF kanal yönetimi ve güç kontrolü
4. DHCP istemcisi entegrasyonu

### 20.5 Bluetooth Sürücüsü Tasarım Kuralları

```c
// Gelecek implementasyon için temel yapı
int bt_init();
int bt_scan(bt_device_t* devices, int max);
int bt_pair(const char* device_addr, const char* pin);
int bt_disconnect(const char* device_addr);
int bt_send(bt_device_t* dev, uint8_t* data, int len);
```

**Bağlantı protokol yığını:**
```
HCI (Host Controller Interface)
       ↓
L2CAP (Logical Link Control)
       ↓
RFCOMM / A2DP / HID Profile
       ↓
Uygulama Katmanı
```

### 20.6 USB Host Controller Sürücüsü

```c
// USB Mass Storage (henüz eklenmemiş)
typedef struct {
    uint8_t  class;       // 0x08 = Mass Storage
    uint8_t  subclass;    // 0x06 = SCSI
    uint8_t  protocol;    // 0x50 = Bulk-Only
    uint32_t sector_size;
    uint64_t total_sectors;
} usb_mass_storage_t;
```

**USB Sürücü Öncelik Sırası:**
1. ✅ HID (Fare + Klavye) — Tamamlandı
2. 🔄 Mass Storage (Flash disk, HDD) — Bekliyor
3. ❌ Audio Class — Planlanmadı
4. ❌ CDC (Serial/Modem) — Planlanmadı

### 20.7 SAS Sürücüsü Tasarım Kuralları

> SAS (Serial Attached SCSI) sunucu ortamları için yüksek hızlı disk arabirimidir.

```c
int sas_init();
int sas_detect_devices();
int sas_read(uint64_t lba, uint32_t count, void* buffer);
int sas_write(uint64_t lba, uint32_t count, const void* buffer);
```

**Gereksinimler:**
- SAS HBA (Host Bus Adapter) PCI taraması
- SMP (Serial Management Protocol) protokol desteği
- RAID kontrolcü iletişim protokolü

---

## 21. ZAMAN, BÖLGE VE DİL AYARLARI

> **İlgili dosyalar:** `Tarih_Saat_Ayarlar.md`

### 21.1 Saat Senkronizasyon Kuralları

- **NTP:** Her açılışta ve 24 saatte bir `ntp.aegis.com` ile milisaniye hassasiyetinde eşitleme.
- **Çevrimdışı Mod:** İnternet yoksa donanımsal RTC (Real-Time Clock) kullanılır.
- **Manuel Değişiklik Logu:** Saat değişiklikleri güvenlik protokolü gereği log dosyasına kaydedilir.

### 21.2 Bölgesel Uyumluluk

Bölge seçildiğinde otomatik değişenler:

| Bölge Öğesi | Türkiye Varsayılanı |
|-------------|-------------------|
| Tarih formatı | GG.AA.YYYY |
| Saat dilimi | GMT+03:00 |
| Para birimi | TL (₺) |
| Ölçü sistemi | Metrik (Celsius) |
| Yaz saati | Otomatik (yasal düzenlemeye göre) |
| Dil | Türkçe |

### 21.3 Seyahat Modu

IP adresi veya GPS verisi değiştiğinde: `"Saat dilimini güncelleyeyim mi?"` bildirimi çıkar.

---

## 22. UYGULAMA YÖNETİCİSİ

> **İlgili dosyalar:** `Uygulama_Yoneticisi.md`

### 22.1 Uygulama Durumları

| Durum | Anlam |
|-------|-------|
| `Çalışıyor` | Aktif olarak çalışan süreç |
| `Beklemede` | Yüklenmiş, tetiklenmeyi bekliyor |
| `Hazır` | Kurulu, çalıştırılmamış |
| `Pasif` | Güvenlik açığı nedeniyle devre dışı |
| `Arka Plan` | Daemon olarak çalışıyor |

### 22.2 Yönetim Kuralları

- **Sandbox:** Her uygulama izole korumalı alanda çalışır; biri çökerse diğerleri etkilenmez.
- **Yetki Kontrolü:** Uygulamanın dosya sistemi ve donanım erişim izinleri tek tek kapatılabilir.
- **Kritik Yama:** Güvenlik açığı tespit edilen uygulama güncellenene kadar `[PASİF]` moda alınır.
- **Akıllı Önbellek:** Uzun kullanılmayan uygulamalar sıkıştırma moduna alınır.
- **Sessiz Kaldırma:** `[F8]` → paket + geçici dosyalar + önbellek + log tamamen silinir.
- **Kaynak Limiti:** Her uygulamaya maksimum CPU % ve RAM MB limiti atanabilir.
- **Bağımlılık Otomasyonu:** Eksik kütüphane sistem deposundan otomatik tamamlanır.

---

## 23. ⚠️ DURUM RAPORU — EKSİK VE ÇALIŞMAYAN BİLEŞENLER

> **Temel:** v4.0 Build 5.1 son raporu (25.03.2026) sonrası sistem çalıştırıldığında tespit edilen eksiklikler.  
> Bu bölüm, tasarımı tamamlanmış ancak kernel'e **eklenmemiş veya çalışmayan** bileşenlerin tam listesidir.

---

### 23.1 🔴 KRİTİK — Arayüz Kuralları ve Tasarımlar Kernel'e Eklenmemiş

**Sorun:** `arayüz/` dizinindeki tüm `.md` dosyalarında tasarlanan TUI arayüzleri, operasyon kuralları ve görsel yerleşim planları kaynak koduna (`kernel/apps/`, `kernel/ui/`) yansıtılmamıştır. Sistem çalıştırıldığında bu modüller görünmemektedir.

**Etkilenen modüller:**

| Modül | Tasarım Durumu | Kernel Durumu |
|-------|---------------|---------------|
| Ağ ve İnternet Ayarları | ✅ Tam tasarım | ❌ GUI eklenmemiş |
| Ekran Görünüm Ayarları | ✅ Tam tasarım | ❌ GUI eklenmemiş |
| Kullanıcı Hesapları Yönetimi | ✅ Tam tasarım | ❌ GUI eklenmemiş |
| Güvenlik Duvarı (Firewall UI) | ✅ Tam tasarım | ❌ Bağlantı kopuk |
| Sistem Güncelleme Paneli | ✅ Tasarım var | ❌ Daemon yok |
| Görev Yöneticisi | ✅ Tam tasarım | ❌ CPU verisi bağlı değil |
| İşlemci Optimizasyonu Görselleştirme | ✅ HLT kodu eklendi | ❌ UI yansıması yok |
| Ses Kontrol Paneli (tam UI) | ✅ Tasarım var | ⚠️ Temel çalışıyor |
| Kontrol Merkezi (Ana Hub) | ✅ Tam tasarım | ❌ Modüller bağlı değil |
| Masaüstü ve Durum Çubuğu | ✅ Tam tasarım | ❌ Telemetri bağlı değil |
| Başlat Menüsü | ✅ Tam tasarım | ❌ Eklenmemiş |
| Ekran Ayarları | ✅ Tam tasarım | ❌ Eklenmemiş |

---

### 23.2 🔴 KRİTİK — Sürücüler Çalışmıyor

**Sorun:** Ağ, WiFi, Bluetooth, Ethernet, USB ve SAS sürücüleri ya stub seviyesinde kalmış ya da hiç yazılmamıştır.

| Sürücü | Durum | Sorun |
|--------|-------|-------|
| **WiFi Sürücüsü** | ⚠️ Simülasyon | Gerçek firmware yok; WPA2 stack eksik; `network_wifi_scan()` sahte veri döndürüyor |
| **Ethernet Sürücüsü** | 🔄 Stub | `network_detect_nic()` PCI taraması yazılı; gerçek e1000/RTL8139 register programlaması eksik |
| **Bluetooth Sürücüsü** | ❌ Yok | `Bagli_Donanimlar.md` tasarım var; hiç kod yazılmamış |
| **USB Mass Storage** | ❌ Yok | HID (fare/klavye) çalışıyor; flash disk / harddisk okuma/yazma yok |
| **SAS Sürücüsü** | ❌ Yok | Tasarım bu belgede; kod yok |
| **GPU Sürücüsü (Gerçek)** | ⚠️ Kısmi | VESA/GOP Framebuffer çalışıyor; donanım hızlandırma (AMD/Intel tam driver) yok |

---

### 23.3 🔴 KRİTİK — Güvenlik Duvarı UI Bağlantısı Kopuk

**Sorun:** `Aegis_Firewall_v4.md` tasarımı tamamlanmış ve `network.c` içinde `network_firewall_toggle()` fonksiyonu mevcut; ancak TUI arayüzü (`Aegis_Firewall_v4.md`'deki ASCII ekran) kernel'e eklenmemiştir.

**Çalışmayan özellikler:**
- Aktif bağlantılar listesi (TCP/UDP tablosu)
- Paket istatistikleri (bloklanan / izin verilen)
- `[F9]` Panik Modu tetikleyici
- `[F4]` Manuel bağlantı kesimi
- Güvenlik log görüntüleme
- IP kara liste yönetimi

---

### 23.4 🔴 KRİTİK — Görev Yöneticisi ve İşlemci Optimizasyonu

**Sorun:** HLT tabanlı CPU optimizasyonu kernel koduna eklenmiş (`kernel.c`, `pit.c`) ancak:
1. Görev yöneticisi, `idle_ticks / total_ticks` formülünü kullanarak gerçek CPU %'sini hesaplamıyor; sabit/sahte değer gösteriyor.
2. `Sistem_Izleme_Paneli.md`'deki TUI paneli kaynak koduna eklenmemiş.
3. Çekirdek bazlı yük (C1–C4) gösterimi mevcut değil.

---

### 23.5 🟠 ÖNEMLİ — Ekran Görünüm Ayarları

**Sorun:** `Ekran_Ayarlari.md` dosyasındaki tüm ayarlar (çözünürlük değişimi, tema seçimi, gece ışığı, çoklu monitör) için UI tasarlanmış ancak hiçbir ayar `aegis_theme_t` yapısı veya Framebuffer ile gerçek anlamda bağlanmamış.

**Çalışmayan özellikler:**
- Çalışma zamanında çözünürlük değiştirme
- Tema değiştirme (renk paletinin anlık uygulanması)
- Gece Işığı modu (renk sıcaklığı filtresi)
- İkinci monitör algılama ve yapılandırma
- GPU sıcaklık izleme

---

### 23.6 🟠 ÖNEMLİ — Kullanıcı Hesapları GUI'si

**Sorun:** Kullanıcı giriş ekranı (`Aegis_Login_Screen_v4.md`) çalışıyor; ancak kullanıcı ekleme, silme, şifre değiştirme, hesap kilitleme işlemlerinin grafiksel yönetim paneli eklenmemiş.

---

### 23.7 🟡 ORTA — Sistem Güncelleme Servisi

**Sorun:** `Uygulama_Yoneticisi.md` ve `install_uefi.sh` güncellendi; ancak:
- Arka planda çalışan güncelleme kontrol daemon'u (`aegis-updater`) yok.
- Snapshot alma/geri yükleme otomasyonu kernel'e entegre edilmemiş.
- Güncelleme bildirim sistemi durum çubuğuna bağlı değil.

---

### 23.8 🟡 ORTA — Kontrol Merkezi Modül Bağlantıları

**Sorun:** `Aegis_Control_Center_v4.md` tasarımında her kategori ilgili alt modülü IPC üzerinden çağırmalı; ancak bu köprü bağlantıları kurulmamış. Kontrol Merkezi açıldığında alt kategoriler kendi bağımsız ekranlarını açamıyor.

---

## 24. GELİŞTİRİCİ UYGULAMA KILAVUZU — EKSİKLERİ TAMAMLAMA

> Bu bölüm, Bölüm 23'teki eksiklikleri **öncelik sırasına göre** nasıl tamamlayacağınızı adım adım açıklar.

### 24.1 Adım 1 — Görev Yöneticisi + CPU Optimizasyonu Bağlantısı (KRİTİK)

**Dosya:** `kernel/apps/taskmanager.c` (yeni oluşturulacak)

```c
// 1) İzin verilmiş idle tick sayacı kernel.c'ye ekle
volatile uint64_t aegis_idle_ticks  = 0;
volatile uint64_t aegis_total_ticks = 0;

// 2) os_idle_hook() içinde say
void os_idle_hook() {
    uint64_t t0 = rdtsc();
    asm volatile("hlt");
    uint64_t t1 = rdtsc();
    aegis_idle_ticks  += (t1 - t0);
    aegis_total_ticks += TICK_INTERVAL;
}

// 3) taskmanager.c içinde göster
uint8_t get_cpu_usage() {
    return (uint8_t)(100 - (aegis_idle_ticks * 100 / aegis_total_ticks));
}
```

**Bağlantı:** `sys monitor` komutu → `taskmanager_render_tui()` → bu fonksiyonu çağır.

---

### 24.2 Adım 2 — Firewall TUI Entegrasyonu

**Dosya:** `kernel/apps/firewall_ui.c` (yeni oluşturulacak)

```c
// Shell komutu: firewall veya guvenlik duvari
void firewall_tui_open() {
    // net_status_t'dan bağlantı bilgilerini çek
    net_status_t* s = network_get_status();
    // TUI çerçevesini çiz
    tui_draw_frame("AEGIS OS - GÜVENLİK DUVARI");
    // TCP/UDP bağlantı tablosunu render et
    firewall_render_connection_table(s);
    // F9 = panik modu, F4 = bağlantı kes
    firewall_handle_input();
}
```

---

### 24.3 Adım 3 — WiFi Sürücüsü Gerçek Aktivasyon

**Dosya:** `kernel/drivers/wifi.c`

Öncelik sırası:
1. PCI taramasında WiFi kartı tespit et (Vendor/Device ID tablosu)
2. Firmware blob yükle (örn. Intel iwlwifi veya Realtek rtlwifi)
3. MAC başlatma ve SSID tarama (Probe Request → Probe Response)
4. WPA2 handshake (4-way handshake, EAPOL)
5. DHCP istemcisi ile IP al → `network_connect()` güncelle

---

### 24.4 Adım 4 — Bluetooth Sürücüsü İlk Uygulama

**Dosya:** `kernel/drivers/bluetooth.c` (yeni)

```c
// Minimum viable Bluetooth stack
int bt_init()       { /* HCI init, USB veya UART üzerinden */ }
int bt_scan()       { /* Inquiry scan, GAP protokolü */ }
int bt_pair()       { /* SSP (Secure Simple Pairing) */ }
int bt_hid_attach() { /* HID profile: klavye, fare, gamepad */ }
int bt_a2dp_attach(){ /* A2DP profile: ses */ }
```

---

### 24.5 Adım 5 — USB Mass Storage

**Dosya:** `kernel/drivers/usb_msc.c` (yeni)

```c
// BBB (Bulk-Only Transport) protokolü
int usb_msc_init(usb_device_t* dev);
int usb_msc_read(uint32_t lba, uint8_t* buf, uint32_t count);
int usb_msc_write(uint32_t lba, const uint8_t* buf, uint32_t count);
// VFS'e bağla: dosya yöneticisinde USB aygıt görünsün
```

---

### 24.6 Adım 6 — Arayüz Modüllerini Kernel'e Ekle

Her modül için şu şablonu uygula:

```c
// kernel/apps/<modül_adi>.c
#include "tui.h"
#include "input.h"

void <modül>_open() {
    tui_clear_screen();
    tui_draw_border();
    <modül>_render_content();
    while(1) {
        int key = keyboard_get_key();
        if(key == KEY_ESC) break;
        <modül>_handle_key(key);
        <modül>_render_content(); // yeniden çiz
    }
}
```

**Eklenecek modüller ve öncelikleri:**

| Sıra | Modül | Kaynak Dosya |
|------|-------|-------------|
| 1 | Görev Yöneticisi | `Sistem_Izleme_Paneli.md` |
| 2 | Firewall UI | `Aegis_Firewall_v4.md` |
| 3 | Ses Kontrol Tam UI | `Ses_Denetimi.md` + `Aegis_Security_and_Sound_Core.md` |
| 4 | Kontrol Merkezi | `Aegis_Control_Center_v4.md` |
| 5 | Masaüstü & Durum Çubuğu | `Aegis_Desktop_Status_Bar_v4.md` |
| 6 | Başlat Menüsü | `Aegis_Start_Menu_v4.md` |
| 7 | Ekran Ayarları | `Ekran_Ayarlari.md` |
| 8 | Kullanıcı Hesapları UI | `Aegis_Login_Screen_v4.md` |
| 9 | Ağ Ayarları UI | `Aegis_Network_Settings_v4.md` |
| 10 | Güncelleme Servisi | `Uygulama_Yoneticisi.md` |

---

### 24.7 Makefile Güncellemeleri (Tüm Yeni Modüller)

```makefile
# Mevcut
OBJS = kernel/kernel.o kernel/drivers/network.o kernel/games/pentomino.o ...

# Eklenecekler (öncelik sırasıyla)
OBJS += kernel/apps/taskmanager.o
OBJS += kernel/apps/firewall_ui.o
OBJS += kernel/apps/sound_ui.o
OBJS += kernel/apps/control_center.o
OBJS += kernel/apps/desktop.o
OBJS += kernel/apps/startmenu.o
OBJS += kernel/apps/display_settings.o
OBJS += kernel/apps/user_manager.o
OBJS += kernel/apps/network_settings_ui.o
OBJS += kernel/apps/updater.o
OBJS += kernel/drivers/wifi.o
OBJS += kernel/drivers/bluetooth.o
OBJS += kernel/drivers/usb_msc.o
OBJS += kernel/drivers/sas.o
```

---

### 24.8 Shell Komut Bağlantıları (Eksik Olanlar)

`kernel/shell/shell.c` içine eklenecek komutlar:

| Komut | Çağrılacak Fonksiyon |
|-------|---------------------|
| `izle` / `sys monitor` | `taskmanager_open()` |
| `guvenlik duvari` / `firewall` | `firewall_tui_open()` |
| `ses` / `sound` | `sound_ui_open()` |
| `kontrol` / `control` | `control_center_open()` |
| `ekran` / `display` | `display_settings_open()` |
| `kullanici` / `user manage` | `user_manager_open()` |
| `ag ayar` / `net settings` | `network_settings_ui_open()` |
| `guncelle` / `update` | `updater_check()` |
| `donanimlar` / `devices` | `hardware_manager_open()` |

---

## EK A — DOSYA BAĞIMLILIKLARI (Cross-Reference Tablosu)

| Dosya | Bağımlı Olduğu Modüller |
|-------|------------------------|
| `Aegis_Desktop_Status_Bar_v4.md` | Mouse Driver, Firewall, Network, Sound |
| `Aegis_Start_Menu_v4.md` | Tüm uygulama modülleri |
| `Aegis_Control_Center_v4.md` | Ağ, Ses, Fare, Güvenlik, Zaman modülleri |
| `Aegis_Firewall_v4.md` | Network Stack, Security Core, Sound (uyarı) |
| `Aegis_Native_AI_Assistant_v4.md` | Firewall, User Library, Neural Network Manager |
| `Aegis_Neural_Network_Manager_v4.md` | Firewall, User Library, Posta |
| `Aegis_Boot_Sequence_v4.md` | Mouse Driver, Network, VFS, GUI |
| `Aegis_Security_and_Sound_Core.md` | Firewall, Mikrofon, Durum Çubuğu |
| `Aegis_Master_Integration_Rules_v4.md` | Tüm modüller (entegrasyon merkezi) |

---

## EK B — GELİŞTİRME YOLU HARİTASI

Öncelik sırasıyla:

1. ✅ Kernel yükleme çalışıyor (HLT optimizasyonu dahil)
2. ✅ Grafik boot (Framebuffer)
3. ✅ Boot menü sistemi
4. ✅ Secure boot (hash doğrulama)
5. ✅ Ağ sürücüleri (stub + simülasyon)
6. ✅ Pentomino oyunu entegrasyonu
7. ✅ HLT tabanlı CPU optimizasyonu (kernel.c + pit.c)
8. 🔄 **Sonraki — Kritik:** Görev Yöneticisi TUI → CPU kullanımı gerçek bağlantısı
9. 🔄 **Sonraki — Kritik:** Firewall TUI kernel entegrasyonu
10. 🔄 **Sonraki — Kritik:** WiFi gerçek NIC aktivasyonu (e1000 / RTL8139)
11. 🔄 Ses Kontrol Paneli tam TUI uygulaması
12. 🔄 Kontrol Merkezi modül bağlantıları
13. 🔄 Masaüstü ve Durum Çubuğu telemetri sistemi
14. 🔄 Başlat Menüsü implementasyonu
15. 🔄 Ekran Ayarları modülü
16. 🔄 Kullanıcı Hesapları GUI
17. 🔄 Ağ Ayarları UI
18. 🔜 Bluetooth sürücüsü (HCI + HID + A2DP)
19. 🔜 USB Mass Storage sürücüsü
20. 🔜 SAS sürücüsü
21. 🔜 ATA PIO sürücüsü (disk kalıcılığı)
22. 🔜 CXL bellek havuzu
23. 🔜 RL tabanlı tam zamanlayıcı
24. 🔜 Sistem Güncelleme otomatik daemon'u

---

## EK C — KRİTİK STRATEJİ SIRASI

```
👉 Önce stabil boot
👉 Sonra grafik arayüz
👉 Sonra güvenlik katmanı
👉 Sonra ağ
👉 Sonra AI entegrasyonu
```

> **Not:** Bu belge AegisOS projesinin tek referans kaynağıdır. Herhangi bir modülü değiştirirken ilgili bölümlerdeki entegrasyon notlarını ve cross-reference tablosunu mutlaka kontrol edin.
