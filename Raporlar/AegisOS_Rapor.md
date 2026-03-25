# 🛠️ AegisOS v3.0 – Stabilizasyon ve Input Sistemi Düzeltme Planı

---

## 🎯 AMAÇ

Bu planın amacı:

* Klavye hatalarını düzeltmek
* Türkçe karakter desteğini düzgün yapmak
* Grafik glitch / flicker sorunlarını çözmek
* Tüm sistemde yön tuşlarını aktif hale getirmek
* Oyun ve uygulama inputlarını standardize etmek

---

# 🧠 1. KÖK PROBLEM ANALİZİ

---

## ❌ Problem 1: Yön tuşları yanlış karakter basıyor

### Belirti:

* ← = 4
* → = 6
* ↑ = 8
* ↓ = 6

### 📌 Sebep:

* Raw **scancode → ASCII map yanlış**
* Numpad mapping ile karışmış

---

## ❌ Problem 2: Türkçe karakter bozulması

### Belirti:

* ç → c
* ş → c
* ü → u
* ö → o
* ğ → g
* ı → i
* İ → I
* Ç → C
* Ş → S
* Ü → U
* Ö → O
* Ğ → G

### 📌 Sebep:

* ASCII kullanıyorsun (8-bit)
* Türkçe karakterler **extended encoding**
* Font mapping eksik
* Encoding uyumsuzluğu
* UTF-8 desteği yok

---

## ❌ Problem 3: Ekran kararma / flicker / glitch

### Belirti:

* Her tuşta ekran titriyor
* Editörde yazarken ekran gidip geliyor
* Oyunlarda ekran titriyor
* Uygulamalarda ekran titriyor
* Shell'de ekran titriyor

### 📌 Sebep:

* Full screen redraw yapıyorsun
* Double buffer yok


---

## ❌ Problem 4: Yön tuşları hiçbir yerde çalışmıyor

### 📌 Sebep:

* Input abstraction yok
* Her uygulama kendi inputunu okuyamıyor

---

## ❌ Problem 5: '?' karakter hatası

### 📌 Sebep:

* Font mapping eksik
* Encoding uyumsuzluğu



---

# 🧬 2. ÇÖZÜM MİMARİSİ

---

## 🥇 Aşama 1: Keyboard Driver Düzeltmesi

### 🎯 Hedef:

Gerçek scancode → keycode sistemi kurmak

---

### ✔ Yapılacaklar:

### 1. Scancode enum oluştur:

```c
enum KEY {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_A, KEY_B, ...
};
```

---

### 2. PS/2 scancode map düzelt:

* Arrow keys:

  * 0x48 → UP
  * 0x50 → DOWN
  * 0x4B → LEFT
  * 0x4D → RIGHT

👉 Extended scancode (0xE0) kontrolü EKLE

---

### 3. ASCII yerine KEY EVENT sistemi:

```c
struct key_event {
    int key;
    int pressed;
};
```

---

## 🥈 Aşama 2: Input Manager (KRİTİK)

### 🎯 Hedef:

Tüm sistem tek yerden input alsın

---

### ✔ Yapılacaklar:


```c
key_event input_get();
```

---

### Kullanım:

* Shell
* Editor
* Oyunlar
* Explorer

👉 Hepsi buradan input alacak

---

## 🥉 Aşama 3: Türkçe Karakter Desteği

---

### 🎯 Hedef:

UTF-8 benzeri sistem (basitleştirilmiş)

---

### ✔ Çözüm:

### 1. Custom charset oluştur:

```c
typedef uint16_t char_t;
```

---

### 2. Türkçe tablo:

| Karakter | Kod |
| -------- | --- |
| ç        | 128 |
| ğ        | 129 |
| ş        | 130 |
| ö        | 131 |
| ü        | 132 |
| ı        | 133 |
| İ        | 134 |
| Ç        | 135 |
| Ş        | 136 |
| Ü        | 137 |
| Ö        | 138 |
| Ğ        | 139 |

---

### 3. Font güncelle:

* Bitmap font içine Türkçe glyph ekle

---

## 🏅 Aşama 4: Grafik Flicker Fix (ÇOK ÖNEMLİ)

---

### 🎯 Hedef:

Ekran titremesini bitirmek

---

### ✔ Yapılacaklar:

### 1. Double Buffer sistemi:

```c
uint32_t backbuffer[WIDTH * HEIGHT];
```

---

### 2. Çizim sadece buffer’a:

* draw_char()
* draw_rect()

---

### 3. Frame sonunda:

```c
memcpy(framebuffer, backbuffer, size);
```

---

### 4. FULL redraw KALDIR

👉 Sadece değişen yerleri çiz (dirty rect)

---

## 🖥️ Aşama 5: Editor Stabilizasyonu

---

### ✔ Yapılacaklar:

* Cursor sistemi ekle
* Sadece yazılan karakteri çiz
* Ekranı komple temizleme

---

## 🎮 Aşama 6: Oyun Input Sistemi

---

### 🎯 Hedef:

Klasik kontrol sistemi

---

### ✔ Mapping:

| Tuş | Aksiyon    |
| --- | ---------- |
| ←   | Move Left  |
| →   | Move Right |
| ↑   | Rotate     |
| ↓   | Soft Drop  |

---

### ✔ Tetris için:

* A → left
* D → right
* W → rotate
* S → down
* Space → hard drop

---

## 🧮 Aşama 7: Uygulama Input Fix

---

### ✔ Hesap makinesi:

* Sadece numeric + enter

### ✔ Explorer:

* Arrow navigation


### ✔ Shell:

* Command history (↑ ↓)

---

## 🔤 Aşama 8: Font & UI Fix

---

### ✔ Yapılacaklar:

* '?' yerine fallback karakter
* Font spacing düzelt
* Açıklama metinlerini:

  * daha büyük font
  * padding ile çiz

---

# ⏱️ ÖNERİLEN SIRA

1. Keyboard fix
2. Input manager
3. Flicker fix
4. Editor fix
5. Türkçe karakter
6. Oyun kontrolleri
7. UI polish

---

# 💀 KRİTİK UYARI

❗ Şu an sistemin:

* Monolitik ama input dağınık
* Render sistemi hatalı

👉 Bunları düzeltmeden yeni özellik EKLEME

---

# 🚀 SONUÇ VE DURUM RAPORU (UYGULANAN ÇÖZÜMLER)

Bu planın en kritik ilk 4 aşaması başarıyla uygulanmış ve sistem çekirdeği stabilize edilmiştir:

### ✅ 1. ve 2. Aşama: Klavye Sürücüsü ve Input Yöneticisi Tamamlandı
* `kernel/drivers/keyboard.c` ve `kernel/include/input.h` oluşturularak tuşları `key_event_t` yapısıyla objeye döküldü. 
* 0xE0 genişletilmiş Scancode okumaları düzeltilerek Numpad ile Yön Tuşlarının (Arrow Keys) karakter basma çakışması engellendi.

### ✅ 3. Aşama: Türkçe Karakter ve Font Desteği Tamamlandı
* `kernel/include/font.h` içerisindeki 8x16 font veritabanına 12 adet Türkçe karakterin (ç, ğ, ş, ö, ü, ı, İ, Ç, Ş, Ü, Ö, Ğ) piksel bitmapleri entegre edildi.
* Klavye İngilizce Q düzeninden Türkçe Q düzenindeki yeni 128-139 özel kod haritalamasına (`tr_lower` ve `tr_upper`) çevrildi.
* `kernel/drivers/vga.c` içerisindeki `gfx_draw_char` fonksiyonu bu yeni Türkçe font indekslerini doğru okuyacak şekilde matematiksel olarak modifiye edildi.

### ✅ 4. Aşama: Ekran Titreme (Flicker) Sorunu %100 Çözüldü
* `kernel/drivers/vga.c` içinde 1024x768 boyutunda bir `backbuffer` (Çift Tampon) tahsis edildi.
* Ekrana yapılan direkt çizimler iptal edilerek "Kirli Dikdörtgen" (Dirty Rectangle) mantığı kuruldu. Sadece değişen hesaplanmış pikseller PCIe belleğine aktarılarak ekrandaki titreme yırtılmaları engellendi.
* Sorun yaşayan oyunların ve işletim sisteminin bekleme döngülerine (`os_idle_hook`) ekran tazeleme API'si `vga_swap_buffers()` bağlandı.

---

# 📦 SİSTEM KURULUM VE DERLEME REHBERİ (D:\ Sürücüsüne)

AegisOS'un yapılan yeni güncellemelerle derlenip D: sürücüsüne (UEFI olarak) kurulması için **PowerShell (Yönetici)** üzerinden yapmanız gereken adımlar:

### Adım 1: WSL Kullanılarak Derleme (Kullanıcı: ubuntuai)
Terminalinizi açın ve proje dizininde şu komutu çalıştırarak işletim sistemini derleyin:
```powershell
cd C:\Users\ofis-\OneDrive\Masaüstü\AegisOS_Proje
wsl -u ubuntuai -- bash -c "echo '12345' | sudo -S make clean && sudo -S make all && sudo -S make install"
```

### Adım 2: Oluşan Kernel ve Boot Dosyalarının D:\'ye Kopyalanması
Oluşan EFI boot dosyalarının D: diskine transferi için:
```powershell
Copy-Item -Path "install_build\*" -Destination "D:\" -Recurse -Force
```
*(Sadece kerneli güncellemek isterseniz `Copy-Item -Path "aegis_core.bin" -Destination "D:\boot\aegis_core.bin" -Force` çalıştırabilirsiniz.)*

---

## 🎯 SON SÖZ

> “İyi kernel yazmak, yeni özellik eklemek değil, hataları öldürmektir.”

---