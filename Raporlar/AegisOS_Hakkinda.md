# Aegis OS Dokümantasyonu ve Geliştirme Raporu

Bu belge, Aegis OS projesinin amacını, kurulumunu, mimarisini ve geliştirilme sürecindeki 5N1K odaklı temel sorun/çözüm dinamiklerini açıklamaktadır.

## İşletim Sistemi Hakkında
Bu işletim Sistemi Aziz Ali Artuç Tarafından antigravity claude open ai gemini3.1 pro kullanılarak oluşturuldu.

---

### 1. Ne Yaptık?
Aegis OS adında; sıfırdan UEFI ve x86 mimarilerine uygun, herhangi bir harici kütüphaneye (libc vs.) bağımlı olmayan, kendi mikro çekirdeği (kernel) ve RAM tabanlı dosya sistemiyle çalışan bağımsız bir işletim sistemi geliştirdik. Sistem içerisine PS/2 sürücüleri, Framebuffer tabanlı (çift tamponlu) grafik sürücüleri, asenkron oyun motoru döngüleri (Tetris, Pong, Pacman, Yılan vb. oyunlar) ve çeşitli sistem araçları (Hesap makinesi, Sistem Monitörü, Metin Editörü) entegre ettik.

### 2. Hedefimiz Neler?
Hızlı boot olabilen (milisaniyeler içinde), minimum kaynak (RAM/CPU) tüketen, donanımla en alt seviyede (bare-metal) native ve stabil bir biçimde haberleşen, gecikmesiz bir yapı yaratmak ana hedefimizdir. İleri vadede ACPI (İleri Yapılandırma ve Güç Arayüzü) modülleri ile çok daha gelişmiş güç yönetimi ve çok çekirdekli (multicore) işlem destekleri sağlamayı hedefliyoruz.

### 3. Bilgisayara ve Sanal Makineye Nasıl Kurulur?
**Sanal Makineye Kurulum (QEMU, VirtualBox, VMware vs.):**
Proje dizininde derleme sonucu `AegisOS_v3.iso` adında bir imaj dosyası oluşmaktadır. Sanal makinenizin depolama (Storage) ya da CD/DVD okuyucu ayarlarına bu `.iso` dosyasını bağlayıp cihazı başlatmanız yeterlidir. Multiboot sistemi Aegis OS'i otomatik olarak boot edecektir.

**Gerçek Bilgisayara Kurulum (UEFI Boot):**
Projedeki `install_uefi.sh` scripti aracılığıyla derlenen işletim sistemi dosyaları paketlenir.
1. Bilgisayarda FAT32 formatlı (örneğin 2GB boyutunda) bağımsız bir disk bölümü (Örn: `D:\`) oluşturulur.
2. Derlenen `aegis_core.bin` dosyası ile birlikte `EFI/BOOT/BOOTX64.EFI` bootloader dosyası direkt olarak bu diske kopyalanır.
3. Bilgisayar yeniden başlatılır ve BIOS/UEFI arayüzünden ilgili FAT32 diski veya "Aegis OS" boot girdisi seçilerek sistem USB/Disk üzerinden başlatılır.

### 4. Hangi Adımlardan Geçer?
Çekirdek derlendikten ve makine başlatıldıktan sonra sistem şu akıştan geçer:
- **Bootloader Aşaması:** Bilgisayarın UEFI/BIOS'u, GRUB tabanlı Bootloader'ı diskin başından belleğe yükler. Multiboot2 başlıkları taranır.
- **Donanım İlklendirme (Kernel Entry):** `kernel.c` devreye girer. İşlemcinin GDT (Global Descriptor Table) ve IDT (Interrupt Descriptor Table) yapıları kurulur.
- **Sürücüler:** PIC donanımları yeniden programlanır. Klavye (PS/2), VGA/Framebuffer ve PIT (zamanlayıcı) interrupt yapıları (ISR) etkinleştirilir.
- **Dosya Sistemi:** Standart Harddisk (IDE) bulunamazsa, sistem belleği üzerinde çalışan sanal bir FS (RAM-FS) oluşturulup içine varsayılan dizinler eklenir.
- **Arayüz (Shell):** VGA grafik motoru devreye girer ve Aegis OS Shell çalışmaya başlar. Kullanıcı komut girmeye hazır hale gelir.

### 5. Kullandığımız Açık Kaynaklar ve Kütüphaneler Neler?
Aegis OS işletim sistemi hiçbir standart kapalı veya açık kaynak C/C++ (libc vb.) sistem kütüphanesi kullanmaz. Kullanılan yegane açık kaynak parçalar sadece "Derleme ve Boot" aşamalarına yardımcı olan araç zincirleridir:
*   **GNU GCC ve Binutils:** Çapraz derleyici (Cross-compiler) olarak C kodlarının bare-metal x86 kodlarına dönüştürülmesini sağlar.
*   **GNU GRUB (Multiboot 2):** İlk önyükleyici menüsünün hazırlanıp donanım bilgisinin (VGA Framebuffer, Memory Map vb.) çekirdeğe aktarılmasını sağlar.
Tüm font render işlemleri, string kütüphaneleri (`strlen`, `strcmp`), mem kütüphaneleri ve donanım sürücüsü yazılımları sıfırdan baştan yazılmıştır.

### 6. Çalıştırmak İçin Neler Gerekli?
*   Herhangi bir 32-bit veya 64-bit komut setini destekleyen x86/x64 mimarili işlemci.
*   En az 16 MB Sistem Belleği (RAM).
*   Görüntü çıkışı için VGA ya da VESA Framebuffer destekli bir görüntü bağdaştırıcısı.
*   Giriş için PS/2 (veya USB Legacy Mode) destekli bir klavye.

### 7. Çözdüğümüz Problem Ne?
Günümüz işletim sistemlerinin arkasında çalışan, binlerce katmandan ve devasa kütüphane bağımlılıklarından oluşan "bloated" (şişirilmiş) ve hantal yazılım mimarisine karşı; salt donanıma hükmeden, sadece gerekli özellikleri barındırdığı için çökme olasılığı çok düşük, gecikmesiz ve güvenli bir taban sunduk. Klasik "while(1) poll" donanım kitleyici fonksiyonlar yerine tamamen olay güdümlü (event-driven) zamanlayıcı interruptları kullanarak kaynak bloklamalarının önüne geçtik. Pencerelerin titremesini kesmek için çift tamponlu grafik yapısını modüler hale getirdik.
