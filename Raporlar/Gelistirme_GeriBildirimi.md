# AegisOS v3.0 - Gelistirme GeriBildirimi

Tarih: 24.03.2026

Hazirlanma temeli:
- `AegisOS_Guncel_Durum_Raporu2.md`
- `pacman.md`
- Mevcut kaynak kod analizi

Bu belge, yalnizca kullanici tarafindan acikca vurgulanan alanlar icin hazirlanmistir. Bu alanlar disindaki calisan kodlarin davranisini degistirecek genis capli bir mudahale bu raporun disindadir. Entegrasyon icin zorunlu olan noktalar haricinde saglam modullere dokunulmamalidir.

## 1. Kapsam ve kesin sinir

Bu rapor sadece su basliklara odaklanir:
- Pacman oyununun stabil hale getirilmesi ve `pacman.md` gorunumu/kurallari ile hizalanmasi
- Pong oyununun akici hale getirilmesi
- Clock uygulamasi acildiginda shell tarafinda olusan donma algisinin giderilmesi
- RAM, CPU, GPU durumunu canli gosteren grafiksel bir sistem izlencesi eklenmesi
- Bilgisayari kapatma, yeniden baslatma, oturum acma ve oturum kapatma akislarinin eklenmesi
- On yukleme ekraninin duzeltilmesi, performansa bagli ilerleme cubugu ve tus ile durdurma davranisi eklenmesi
- Fontlarin hizali gorunmesi, satir araliginin 1.0 olmasi ve genel tipografik duzen 
- Editor penceresinin ekrandan tasma sorununun giderilmesi 

Bu raporun disinda tutulacak alanlar:
- Dosya sistemi mantiginin yeniden yazilmasi
- Ses, USB, fare, Tetris, Snake, XOX, Hesap Makinesi gibi calisan alanlara gereksiz mudahale
- Sadece estetik sebeple yapilan genel refaktorler
- Gercek veri uretmeyen sahte CPU/GPU yuzdeleri

Bu raporda "zorunlu entegrasyon" olarak kabul edilen sinirli dosyalar:
- `kernel/games/pacman.c`
- `kernel/games/pong.c`
- `kernel/apps/clock.c`
- `kernel/apps/editor.c`
- `kernel/kernel.c`
- `kernel/drivers/vga.c`
- `kernel/include/font.h`
- `grub.cfg`
- Yeni eklenecek sistem izlencesi / guc / oturum modulleri
- Yalnizca erisim noktasi icin gerekiyorsa `kernel/shell/shell.c`, `kernel/apps/explorer.c`, `kernel/apps/appmanager.c`

## 2. Mevcut teknik taban

`AegisOS_Guncel_Durum_Raporu2.md` icindeki tespit dogrudur: cekirdek tarafinda `key_state`, `input_available()` ve `input_get()` tabanli daha modern bir input modeli kurulmustur. Bu, altyapida onemli bir ilerlemedir.

Ancak mevcut kaynak kod analizi su sonucu vermektedir:
- Altyapi modernlesmis olsa da uygulama seviyesinde bazi moduller hala eski veya yetersiz oyun/arayuz mantigi ile calismaktadir.
- Oyun akiciligi ve pencere duzeni tarafinda sorunlarin ana kaynagi, input altyapisindan cok zamanlama, cizim yenileme, sabit boyut kullanimi ve eksik durum makinesidir.
- Ozellikle Pacman, Pong, Clock, boot akis, editor ve yeni istenen sistem izlencesi alanlari uygulama mimarisi duzeyinde hedefli bir revizyon istemektedir.

## 3. Dogrudan kaynak kod tespitleri

Bu bolum, raporun sadece genel oneriler degil, mevcut dosyalardan cikmis teknik tespitlere dayandigini gostermek icin eklenmistir.

### 3.1 Pacman

Kaynak bazli tespitler:
- `kernel/games/pacman.c:107-122` araliginda harita sabit bir Pacman haritasi degil, rastgele uretilen bir duvar/yem dagilimidir.
- `kernel/games/pacman.c:22` satirinda sadece `3` hayalet vardir. `pacman.md` icindeki klasik duzende `4` hayalet ve farkli davranis kimlikleri vardir.
- `kernel/games/pacman.c:176-199` araliginda istenen yon sadece o dongude gelen tusa baglidir. Yon kaliciligi yoktur. Bu nedenle Pacman surekli akmak yerine tusa basildikca tek adimlik ilerleme davranisi sergiler.
- `kernel/games/pacman.c:227-266` araliginda hayalet mantigi yalnizca rastgele yurume davranisidir. Chase, Scatter, Frightened, ghost-house, goz olarak merkeze donus ve yeniden dogus mantigi yoktur.
- `kernel/games/pacman.c:268-269` araliginda hiz, `for (volatile int d = 0; d < 2000000; d++)` tipi donanim bagimli bir bekleme ile kontrol edilmektedir. Bu, oyunun hizini makinenin performansina gore bozabilir.
- `kernel/games/pacman.c` icinde `vga_swap_buffers()` cagrisi yoktur. Grafik modunda cizim yenilemesinin duzenli ve kontrollu akmasi garanti edilmemektedir.

Sonuc:
- Mevcut Pacman, `pacman.md` tarafinda tarif edilen oyun degil; "Pacman temali basit hareketli labirent" duzeyindedir.
- Stabilite sorununun cekirdek input katmanindan cok, oyun modelinin eksikliginden ve zamanlama/cizim mimarisinden geldigi gorulmektedir.

### 3.2 Pong

Kaynak bazli tespitler:
- `kernel/games/pong.c:245-260` araliginda oyuncu hareketleri `1333` sayac esigi ile kontrol edilmektedir.
- `kernel/games/pong.c:263-274` araliginda CPU raket hareketi yine `1333` sayac esigine baglidir.
- `kernel/games/pong.c:276-314` araliginda top hareketi `2000` sayac esigine baglidir.
- `kernel/games/pong.c:317-318` araliginda her dongu sonunda bos islem tipi bir gecikme vardir.
- Bu model gercek zaman yerine dongu sayisina gore calistigi icin ayni oyun farkli donanimlarda farkli akicilik verir.

Sonuc:
- Pong'daki yavaslik, input sisteminden degil; zaman bazli olmayan bir oyun dongusunden kaynaklanmaktadir.

### 3.3 Clock ve shell donma algisi

Kaynak bazli tespitler:
- `kernel/apps/clock.c:169-285` araliginda uygulama sonsuz modal dongu icinde calismaktadir.
- `kernel/apps/clock.c:178-188` araliginda zaman yenilemesi `++timer > 100000` tipi yapay bir sayacla yapilmaktadir.
- `kernel/apps/clock.c:190` satirinda `keyboard_check_key()` kullanilmaktadir.
- `kernel/apps/clock.c` icinde `vga_swap_buffers()` yoktur.
- `kernel/shell/shell.c:242` satirinda `clock` komutu dogrudan `app_clock()` cagirir. Yani clock acildiginda shell akisindan cikilip modal uygulamaya gecilmektedir.

Sonuc:
- Eger beklenti "clock acikken shell de ayni anda komut alsin" ise, sorun sadece `clock.c` ile cozulemez; bu beklenti pencere yonetimi veya kooperatif gorev sistemi ister.
- Eger beklenti "clock acildiginda sistem kilitlenmis gibi hissettirmesin, saat akici cizilsin, ESC ile temiz donsun" ise, bu problem `clock.c` icinde hedefli bicimde cozulebilir.

### 3.4 Sistem izlencesi

Kaynak bazli tespitler:
- `kernel/lib/memory.c:41-47` sadece heap kullanimini metin olarak basan `memory_print_stats()` fonksiyonu vardir.
- `kernel/drivers/pci.c:28-49` PCI taramasi yapar ama canli grafiksel performans izlemesi sunmaz.
- Kod tabaninda GPU yuk yuzdesi uretecek bir telemetri katmani gorulmemistir.
- `kernel/apps/appmanager.c` ve `kernel/apps/explorer.c` icinde sistem izlencesi icin bir uygulama kaydi yoktur.

Sonuc:
- "Task Manager benzeri canli sistem izlencesi" su anda yoktur ve sifirdan ama sinirli kapsamla eklenmelidir.
- CPU ve RAM icin canli metrik uretmek mumkundur.
- GPU icin gercek yuzde verisi mevcut kod tabaninda hazir degildir. Bu alan sahte veriyle degil, gercek destek varsa gercek veriyle ele alinmalidir.

### 3.5 Boot ve on yukleme

Kaynak bazli tespitler:
- `kernel/kernel.c:66-116` araliginda mevcut boot akis sadece metinsel ilerleme satirlari yazmaktadir.
- `grub.cfg:1` satirinda `set timeout=5` bulunmaktadir. Bu, boot oncesi tus ile otomatik acilisi durdurma davranisi icin mevcut bir temel saglar.
- Boot sirasinda grafiksel ilerleme cubugu, performansa gore ilerleme, pauseli bekleme ekrani veya "herhangi bir tusa basarak durdur" mantigi su an yoktur.

Sonuc:
- On yukleme ekrani kullanici istegindeki kalite seviyesine cikmak icin iki asamali tasarlanmalidir: bootloader asamasi ve kernel preload asamasi.

### 3.6 Font ve editor yerlesimi

Kaynak bazli tespitler:
- `kernel/drivers/vga.c:23-24` sabit `8x16` karakter hucre sistemi kullanmaktadir.
- `kernel/include/font.h:10` satirinda font tablosu sinirli karakter setiyle tutulmaktadir.
- `kernel/apps/editor.c:7-10` satirlarinda editor penceresi sabit `121x45` boyutla tanimlidir.
- `kernel/apps/editor.c` ekran boyutunu `vga_get_width()` ve `vga_get_height()` ile dinamik hesaba katmamaktadir.

Sonuc:
- Editor ve tipografi duzeni mevcut 1024x768 varsayimiyla calisiyor gibi gorunse de, sabit geometri kullandigi icin tasma riski tasimaktadir.
- Tipografik tutarlilik sadece fonttan degil, satir ilerleme mantigi, desteklenmeyen glifler ve pencere geometri hesabindan da etkilenmektedir.

## 4. Ayrintili gelistirme geribildirimi

## 4.1 Pacman oyunu stabil degil, guncellenmesi gerek

Bu baslik altindaki revizyon, sadece `kernel/games/pacman.c` ve gerekiyorsa buna eslik eden yeni bir `pacman_map.h` / `pacman_rules.h` duzeyinde sinirli tutulmalidir.

Mevcut durumun ana sorunu:
- Oyun `pacman.md` ile tanimli gorunum ve kurallari tasimiyor.
- Harita rastgele oldugu icin her acilista farkli ve Pacman kimliginden uzak.
- Hareket surekliligi olmadigi icin oynanis "stabil" degil, kesik kesik ve tusa bagimli.
- Hayalet davranislari zayif oldugu icin oyun denge kuramiyor.
- Oyun hizi donanim bagimli beklemeler yuzunden tutarsiz.
- Cizim yenilemesi acik bir frame swap cagrisi ile sabitlenmemis.

Zorunlu duzeltme maddeleri:
- Harita, `pacman.md` icindeki gorunume sabit ve birebir ya da birebire cok yakin bir metin tabanli labirent olarak tanimlanmalidir.
- `SEVIYE`, `SKOR`, alt yardim satiri ve oyun alani yerlesimi `pacman.md` ile uyumlu hale getirilmelidir.
- Pacman'in son yonu korunmali, oyuncu yeni yon girdiginde "queued turn" mantigi uygulanmalidir.
- Pacman bir tusa her kare yeniden basilmadan hareket etmeye devam etmelidir.
- Duvara carpan istek yonu saklanmali, uygun anda otomatik donus yapilmalidir.
- Klasik 4 hayalet eklenmelidir: Blinky, Pinky, Inky, Clyde.
- Hayaletler tek tip rastgele yurume yerine durum makinesi ile calismalidir: `scatter`, `chase`, `frightened`, `eyes-return`.
- `pacman.md` icindeki buyuk guc toplari eklenmeli ve gecici frightened suresi uygulanmalidir.
- Meyve bonusu ve seviye ilerleme mantigi eklenmelidir.
- Skor sistemi guc topu ve hayalet yeme katsayilarini desteklemelidir.
- Oyun hizi, `pit_get_millis()` veya benzeri zaman tabanli kontrolle yonetilmelidir.
- Her update adimindan sonra kontrollu `vga_swap_buffers()` cagrisi ile goruntu akisi netlestirilmelidir.

Ozellikle dikkat edilmesi gerekenler:
- Hayalet AI gercekten kurala dayali olmali, ama bu revizyonun disinda kalan sistemlere dokunmak icin bahane haline gelmemelidir.
- Pacman icin gereken yeni veri yapilari sadece bu oyun dosyasi ve yakin yardimci basliklarla sinirli tutulmalidir.
- Input sistemi cekirdekte zaten var; bu oyunda input altyapisi yeniden yazilmamalidir.

Bu alan icin kabul kriterleri:
- Oyun her acilista ayni ana harita ile acilmali.
- Pacman tus birakildiktan sonra da gecerli yonde akmaya devam etmeli.
- Hayaletler 4 farkli kimlik ve durumla davranmali.
- Guc topu yenince hayaletler korkmus moda gecmeli.
- Skor, seviye, bonus, yeniden baslatma ve cikis davranisi stabil calismali.
- Oyun hizi farkli donanimlarda "asiri hizli / asiri yavas" davranis vermemeli.

## 4.2 Pong oyunu oynanisi yavas, akici degil

Bu baslik altindaki revizyon yalnizca `kernel/games/pong.c` icinde cozulmelidir.

Mevcut durumun ana sorunu:
- Dongu sayaci tabanli hareket hesaplari var.
- Top ve raketler gercek zaman yerine islemci dongu sayisina gore ilerliyor.
- Bu nedenle akicilik hissi zayif, tepki suresi gecikmeli ve makineye bagli.

Zorunlu duzeltme maddeleri:
- `1333` ve `2000` gibi buyuk sayac esikleri kaldirilmalidir.
- Oyun, `pit_get_millis()` veya tick tabanli sabit zaman adimiyla calismalidir.
- Input orneklendirme hizi ile fizik guncelleme hizlari ayrilmalidir.
- Raket hareketi, tusa basili tutma halinde akici ve surekli olmali; her adimda tek tek "sayac doldu mu" hissi vermemelidir.
- Top hareketi sabit update periyodu ile ilerlemeli; skor oldugunda kisa ve kontrollu yeniden servis beklemesi yapilmalidir.
- CPU raketi, oyunu haksiz yere bozmayan ama tepkili bir takip algoritmasina gecmelidir.
- Kare sonu bos `for` gecikmesi yerine PIT tabanli ritim veya sabit frame pacing kullanilmalidir.

Ek kalite notlari:
- `vga_swap_buffers()` cagrisi Pong'da zaten vardir; bu avantaj korunmalidir.
- Kullanilmayan hiz degiskenleri ya anlamli hale getirilmeli ya da temizlenmelidir.
- Oyun akiciligi, top hizi ile input tepkisini ayri ayri ayarlayan bir modelle kurulmalidir.

Bu alan icin kabul kriterleri:
- Raketler basili tusa surekli ve akici cevap vermeli.
- Top hareketsiz, takila takila veya gecikmeli gitmemeli.
- Tek kisilik ve iki kisilik mod ayni akicilik standartinda olmali.
- Ayni surum farkli donanimlarda benzer oynanis hissi vermeli.

## 4.3 Clock acilinca shell ekrani donuyor

Bu madde iki farkli anlama gelebilir. Bu raporda esas alinan yorum sudur:
- Clock acildiginda sistem "kilitlenmis gibi" hissettirmemeli.
- Saat arayuzu akici cizilmeli.
- ESC ile cikildiginda shell temiz sekilde geri calismaya devam etmelidir.

Sinir notu:
- Eger kullanici clock penceresi acikken shell'in arka planda ayni anda komut almasini istiyorsa, bu artik sadece `clock.c` duzeltmesi degildir. Bu, pencere/gorev yonetimi mimarisi ister.
- Bu rapor, gereksiz genel refaktor yapmamak icin minimum yorumlu cozumu onerir.

Zorunlu duzeltme maddeleri:
- `keyboard_check_key()` ile donen modal busy-loop yerine `input_available()` / `input_get()` uyumlu bir event akisi kurulmalidir.
- Saat yenilemesi yapay `timer++` sayaci yerine `pit_get_millis()` uzerinden gercek zaman araligiyla yapilmalidir.
- Arayuz ciziminden sonra `vga_swap_buffers()` duzenli cagirilmalidir.
- Uygulama acildiginda ve kapanirken shell framebuffer durumunu kirletmeyecek temiz gecis uygulanmalidir.
- Cikis davranisi `vga_clear()` sonrasi shell promptuna net donus vermelidir.
- Kaydetme ve secim akislarinda gereksiz uzun bos beklemeler azaltulmalidir.

Opsiyonel ama dogru yon:
- Shell zaten sag ustte saat gosteren bir yapiya sahip oldugu icin (`shell.c` icindeki `update_clock_display()`), buyuk clock penceresi zorunluysa modal app olarak kalabilir.
- Eger "shell donmasin" talebi daha guclu bir canli saat paneli beklentisi tasiyorsa, clock buyuk pencere yerine shell ust barina veya sistem izlencesine tasinmalidir.

Bu alan icin kabul kriterleri:
- Clock ekrani acildiginda guncel saniye akisi takilmadan gorunmeli.
- Tus tepkisi gecikmemeli.
- ESC ile cikista shell tekrar komut almaya devam etmeli.
- Uygulama acikken "hard lock" hissi olusmamali.

## 4.4 Sistem izlencesi olacak, canli RAM CPU GPU gosterecek

Bu madde mevcut kod tabaninda yeni bir uygulama gerektirir. En temiz cozum, yeni bir uygulama dosyasi eklemek ve sadece giris noktalarinda entegrasyon yapmaktir.

Onerilen dosya siniri:
- Yeni: `kernel/apps/system_monitor.c`
- Yeni: `kernel/include/system_monitor.h`
- Minimal entegrasyon: `kernel/shell/shell.c`, `kernel/apps/explorer.c`, `kernel/apps/appmanager.c`

Mimari hedef:
- "Task Manager benzeri" ama AegisOS'un mevcut text-grid / framebuffer dogasina uygun grafiksel panel
- Ustte ozet kutular
- Ortada canli cizgi veya blok grafikler
- Altta sistem butonlari / kisayollar

RAM tarafinda zorunlu davranis:
- Mevcut bump allocator kullanim miktari `heap_ptr / HEAP_SIZE` mantigiyla canli gosterilmelidir.
- Ekranda hem sayisal deger hem grafik olmalidir.
- "Gercek fiziksel RAM kullanimi" yoksa bu alan acikca "heap kullanimi" olarak etiketlenmelidir.

CPU tarafinda zorunlu davranis:
- CPU kullanim verisi, idle zaman ve toplam tick farki uzerinden uretilmelidir.
- Bunun icin `os_idle_hook()` tarafinda idle sayac mantigi eklenebilir.
- Son 1-5 saniyelik kayan ortalama veya history buffer ile grafik cizilmelidir.
- Sahte sabit deger veya rastgele yuzde kesinlikle kullanilmamalidir.

GPU tarafinda zorunlu davranis:
- Mevcut kod tabaninda gercek GPU yuzdesi verecek telemetri yoktur.
- Bu nedenle iki asamali bir politika izlenmelidir.
- Asama 1: PCI uzerinden bulunan goruntu aygiti bilgisi, framebuffer modu, cizim yenileme/fps benzeri render saglik metrikleri gosterilir.
- Asama 2: Eger GPU surucu telemetrisi eklenirse, gercek kullanim yuzdesi gosterilir.
- GPU tarafinda gercek veri yokken "GPU %37" gibi sahte bir deger uretilmemelidir.

### Sistem izlencesi icin donanim veri kaynagi katmani

Bu sistem izlencesi yalnizca ekran ustunde oran gosteren bir uygulama olarak degil, alttaki donanim veri yollarini bilen bir izleme katmani olarak tasarlanmalidir. Ozellikle pil, sicaklik, fan ve parlaklik verileri CPU'dan degil, anakart/firmware katmanlarindan alinacagi varsayimi ile ilerlenmelidir.

ACPI (Advanced Configuration and Power Interface) katmani icin kurallar:
- Pil durumu, sicaklik, fan ve guc yonetimi gibi alanlarda birincil kaynak ACPI olmalidir.
- Bu veriler pratikte anakart uzerindeki Embedded Controller (EC) ile iliskili oldugu icin sistem izlencesi bu verileri dogrudan CPU verisi gibi ele almamalidir.
- Kernel, BIOS/UEFI tarafindan birakilan ACPI tablolarini okumayi hedeflemelidir: `RSDP`, `FADT`, `DSDT`.
- Uzun vadeli ve tam uyumlu bir cozum icin bir `AML Interpreter` katmani gerekecegi raporda kabul edilmelidir.
- Pil yuzdesi ve pil bilgisi icin ACPI icindeki `_BST` (Battery Status) ve `_BIF` (Battery Information) metodlari hedef veri kaynagi olarak ele alinmalidir.
- Parlaklik kontrolu gerekiyorsa ACPI tarafinda `_BCL` ve `_BCM` metodlari desteklenmelidir.

PCI / PCIe veri yolu taramasi icin kurallar:
- GPU, ag karti ve diger cevre birimlerinin varligi sistem izlencesinde PCI taramasi ile belirlenmelidir.
- Temel cihaz kimligi tespiti icin PCI konfigurasyon alanina `0xCF8` ve `0xCFC` I/O portlari uzerinden erisilmesi hedeflenmelidir.
- Her aygit icin en az `Vendor ID` ve `Device ID` bilgisi toplanmalidir.
- GPU bolumunde gercek kullanim yuzdesi yoksa en azindan bulunan goruntu denetleyicisinin kimligi, veri yolu ve temel sinifi gosterilmelidir.

CPUID ve islemci verileri icin kurallar:
- Islemci model bilgisi, ozellikler ve bazi termal yetenekler icin `CPUID` talimati kullanilmalidir.
- CPU bolumunde yalnizca kullanim yuzdesi degil, desteklenen ozellikler de gerekirse ozet bilgi olarak gosterilmelidir.
- Termal yetenekler icin `CPUID` leaf degerleri referans alinmali, gerekirse `EAX=0x06` benzeri sorgularla termal kapasite bilgisi okunmalidir.
- Sicaklik verisi gerekiyorsa sadece `CPUID` yeterli kabul edilmemeli; gerekli yerlerde `MSR` veya `EC` veri kaynaklari ile birlikte dusunulmelidir.

RAM, disk ve diger sistem verileri icin izleme kurallari:
- Toplam RAM miktari icin en dogru erken kaynak `UEFI Memory Map` olmalidir.
- Sistem izlencesi yalnizca heap kullanimini degil, mumkunse toplam bellek ve kullanilabilir bellek sinirlarini da gostermelidir.
- Disk bilgisi gerekiyorsa veri kaynagi `AHCI` veya `NVMe` denetleyicisi olmalidir.
- SATA tarafinda `IDENTIFY` benzeri sorgularla disk kimligi ve kapasitesi alinmasi hedeflenmelidir.

GPU ve ekran kontrolu icin kurallar:
- Modern UEFI sistemlerde temel goruntu cikisi icin `GOP` (Graphics Output Protocol) esas alinmalidir.
- Eski uyumluluk senaryolarinda `VBE` sadece sinirli bir geri donus secenegi olarak dusunulmelidir.
- Parlaklik kontrolu, GPU surucusu yoksa once ACPI tarafindan ele alinmalidir.
- Gercek panel arka isik kontrolu gerekiyorsa uzun vadede `PWM` register kontrolu veya surucu seviyesinde panel kontrolu gerekecegi raporda kabul edilmelidir.
- Sistem izlencesi, parlaklik gibi alanlarda destek yoksa sahte kontrol sunmamali; destek durumu acikca belirtilmelidir.

Sistem izlencesi icin donanim veri haritasi:

| Veri Tipi | Oncelikli Kaynak | Teknik Kural |
| --- | --- | --- |
| Pil ve guc | ACPI | `_BST`, `_BIF`, DSDT ve EC tabanli veri akisi |
| Ekran parlakligi | ACPI / GOP / PWM | Oncelik ACPI, surucu varsa PWM tabanli fiziksel kontrol |
| Sicaklik | MSR / EC / ACPI | CPU ve anakart sicakligi ayni kaynak gibi ele alinmamalidir |
| Fan durumu | ACPI / EC | Fan telemetrisi anakart denetleyicisi uzerinden okunmalidir |
| Disk bilgisi | AHCI / NVMe | Denetleyici tabanli kimlik ve kapasite sorgulari |
| Toplam RAM | UEFI Memory Map | Boot asamasinda alinan bellek haritasi esas alinmalidir |
| GPU kimligi | PCI / PCIe | Vendor ID, Device ID, class bilgisi ve goruntu denetleyicisi tipi |

Bu veriler sistem izlencesi tasarimina su sekilde yansitilmalidir:
- CPU paneli: kullanim, model, desteklenen temel ozellikler, mumkunse termal durum
- RAM paneli: toplam bellek, kullanilan bellek, heap kullanim orani
- GPU paneli: cihaz kimligi, framebuffer/GOP durumu, destek varsa parlaklik ve telemetri
- Guc paneli: pil durumu, sarj durumu, AC baglanti durumu, sicaklik/fan bilgisi
- Depolama paneli: denetleyici tipi, disk kimligi, kapasite ve temel durum bilgisi

Bu alt katman icin temel ilke:
- Sistem izlencesi veri kaynagini bilmeden sahte yuzdeler gosteren bir pencere olmamalidir.
- Her veri alani icin hangi donanim veya firmware yolundan beslendigi acikca tanimlanmalidir.
- Desteklenmeyen telemetri alanlari, uydurma degerlerle degil `desteklenmiyor` veya `surucu gerekli` gibi net durum mesajlariyla gosterilmelidir.
Arayuz zorunluluklari:
- Ayrik paneller: CPU, RAM, GPU
- En az son 60 ornek icin canli tarihce grafigi
- Renk kodlu durum: normal, yuksek, kritik
- ESC ile cikis
- F5 veya R ile manuel yenileme
- Eger istenirse alt kisimda sistem eylemleri: `oturumu kapat`, `yeniden baslat`, `kapat`

Bu alan icin kabul kriterleri:
- Uygulama shell, explorer ve app manager uzerinden acilabilmeli.
- RAM ve CPU alanlari gercek canli veri gosteriyor olmali.
- GPU bolumu sahte veri kullanmadan durum gostermeli.
- Grafikler sabit degil, zamanla degisen history yapisina sahip olmali.

## 4.5 Bilgisayari acma/kapatma, oturum acma/kapatma olacak

Bu madde oyun veya basit app duzeltmesi degil; cekirdek baslangic akisi ile shell arasinda sinirli bir oturum/guc katmani eklenmesini gerektirir.

Onerilen dosya siniri:
- Yeni: `kernel/system/session.c`
- Yeni: `kernel/include/session.h`
- Yeni: `kernel/system/power.c`
- Yeni: `kernel/include/power.h`
- Entegrasyon: `kernel/kernel.c`, `kernel/shell/shell.c`

Oturum akisinda zorunlu davranis:
- Sistem boot ettikten sonra dogrudan shell yerine bir giris/oturum ekrani gosterebilir.
- Basarili giris sonrasi shell acilir.
- `logout` komutu shell'den cikis verip tekrar oturum ekranina dondurur.

Guc akisinda zorunlu davranis:
- `shutdown` komutu: destek varsa sistemi kapatir.
- `reboot` komutu: sistemi yeniden baslatir.
- `logout` komutu: aktif oturumu kapatir.
- `login` ekrani: yeniden giris akisina dondurur.

Kritik teknik not:
- Gercek donanimda guvenilir `shutdown` davranisi icin ACPI/guclu guc yonetimi gerekir.
- Mevcut kod tabaninda ACPI guc yonetimi gorunmedigi icin, kapatma ozelligi donanima gore degisebilir.
- Bu nedenle uygulama sirasinda "desteklenen ortamda gercek kapatma, aksi halde guvenli bilgi mesaji" yaklasimi benimsenmelidir.
- `reboot` daha kolay saglanabilir; klavye denetleyicisi reset ya da guvenli alternatifler kullanilabilir.

Arayuz tarafinda zorunlu davranis:
- Bu eylemler shell komutlari ile erisilebilir olmali.
- Istenirse sistem izlencesi veya yeni bir sistem menusu icinden de cagrilabilmeli.

Bu alan icin kabul kriterleri:
- Oturum kapatma shell'i sonlandirmadan giris ekranina donmeli.
- Yeniden baslat komutu test ortaminda calismali.
- Kapatma destegi varsa gercek kapatma yapmali, yoksa kontrollu bilgi vermeli.
- Bu akislar baska uygulamalari bozacak sekilde daginik kod ekleyerek degil, merkezi modulle kurulmalidir.

## 4.6 On yukleme ekrani duzeltilmeli, ilerleme cubugu olacak, performansa gore ilerleyecek

Mevcut `kernel/kernel.c` sadece satir satir boot log basiyor. Kullanici istegi ise daha duzgun bir preload ekranidir.

Bu alan icin zorunlu mimari:
- Boot sureci iki seviyeye ayrilmalidir.
- Seviye A: Bootloader bekleme ve otomatik baslatma kontrolu
- Seviye B: Kernel preload ekrani ve grafiksel ilerleme

Seviye A - bootloader davranisi:
- `grub.cfg` icindeki timeout davranisi korunmali ve kullanici bir tusa basarsa otomatik boot akisi durdurulabilmelidir.
- Bu, "sistem acilirken herhangi bir tusa basilarak on yukleme islemi durdurulabilecek" talebinin en erken asamadaki dogru karsiligidir.

Seviye B - kernel preload davranisi:
- `kernel/kernel.c` icinde metin satirlari yerine merkezi bir preload arayuzu cizilmelidir.
- Logo / sistem adi
- Altinda ilerleme cubugu
- Altinda anlik durum mesaji: `Bellek baslatiliyor`, `IDT kuruluyor`, `Klavye hazirlaniyor` gibi

Ilerleme cubugu mantigi:
- Ilerleme, sabit sureli sahte animasyon olmamalidir.
- Cubuk yalnizca gercek init adimlari tamamlandikca ilerlemelidir.
- Daha yavas sistemde adim gec tamamlanacagi icin cubuk da daha yavas ilerleyecek; boylece kullanici istegindeki "performansa gore ilerleme" davranisi dogal yoldan saglanmis olur.
- Adimlara agirlik verilebilir. Ornek:
- Bellek: %10
- IDT/PIT: %15
- Dosya sistemi: %10
- Klavye: %10
- Fare: %10
- Ses: %10
- USB/PCI: %20
- RTC ve shell'e gecis: %15

Pause davranisi:
- Kullanici bir tusa basinca preload pause moduna gecmeli.
- Pause davranisinin en temiz hali, klavye init edildikten sonra kernel preload ekraninda uygulanir.
- Klavye init oncesi bekleme gereksinimi bootloader timeout ile cozulmelidir.
- Pause modunda ekranda acik metin olmalidir: `Boot duraklatildi - devam etmek icin herhangi bir tusa basin`

Bu alan icin kabul kriterleri:
- Boot sirasinda gorsel olarak temiz bir ekran gorulmeli.
- Ilerleme cubugu init adimlariyla birlikte ilerlemeli.
- Herhangi bir tusa basinca uygun asamada duraklama davranisi olmali.
- Tus basilmadiginda boot otomatik devam etmeli.

## 4.7 Fontlar hizali olacak, satir araligi 1.0 olacak

Bu madde yalnizca "font dosyasini degistir" meselesi degildir. Tipografi, render hucreleri ve uygulama yerlesimi birlikte ele alinmalidir.

Mevcut durum:
- `vga.c` tarafinda `8x16` hucre sistemi var. Bu iyi bir tabandir.
- Ancak butun uygulamalar bu hucre sistemini dinamik layout ile kullanmiyor.
- Font tablosu sinirli oldugu icin bazi glifler grafik modunda dogru gorunmeyebilir.

Zorunlu duzeltme maddeleri:
- Satir ilerleme mesafesi `F_H = 16` uzerinden net ve sabit tutulmalidir. Bu, 1.0 satir araliginin teknik karsiligidir.
- Fazladan ust/alt bos piksel veya uygulama bazli keyfi bos satir kullanimi azaltulmalidir.
- Butun ana ekranlar monospaced grid uzerinde hizalanmalidir.
- Grafik modunda desteklenmeyen kutu/glif karakterleri ya font tablosuna eklenmeli ya da ASCII tabanli alternatiflerle degistirilmelidir.
- Turkce karakterler ve baslik satirlari icin tekil glif hizasi kontrol edilmelidir.

Kalite hedefi:
- Satirlar birbirine ne sikisik ne de gereksiz acik gorunmeli.
- Ayni satirdaki panellerin baslangic ve bitis hatti kaymamalidir.
- Baslik, govde ve alt bilgi satirlari hucre gridinden tasmamalidir.

Bu alan icin kabul kriterleri:
- Tum ana uygulamalarda metinler soldan/sagdan hizali gorunmeli.
- Satir araligi tum uygulamalarda tek tip olmali.
- Desteklenmeyen glif yuzunden bozuk goruntu kalmamalidir.

## 4.8 Editor penceresinin bir kismi ekrandan tasiyor

Bu madde sadece `kernel/apps/editor.c` icinde cozulmelidir. Editorun dosya acma/kaydetme mantigi gereksiz yere dagitilmamalidir.

Mevcut durumun ana sorunu:
- Pencere boyutu sabit tanimli.
- Ekran kapasitesi dinamik okunmuyor.
- Farkli framebuffer kosullarinda veya text-mode fallback durumunda tasma riski var.
- Ic satirlar ve alt bilgi satirlari sabit uzunlukte.

Zorunlu duzeltme maddeleri:
- Editor geometri hesabi `vga_get_width()/8` ve `vga_get_height()/16` degerlerinden runtime hesaplanmalidir.
- `WX`, `WY`, `WW`, `WH` sabitleri yerine clamp edilen dinamik pencere boyutu kullanilmalidir.
- Pencere ekran icine sigmiyorsa otomatik kucultulmelidir.
- Status line, text area ve footer genislikleri pencere genisligine gore yeniden hesaplanmalidir.
- Sabit uzunluktaki footer/status metinleri gerekli yerde kesilmeli veya adapte edilmelidir.
- Gorsel tasma olmamasi icin sag ve alt marj guvenceleri eklenmelidir.

Ozellikle dokunulmamasi gereken sey:
- Editorun ac/kaydet mantigi kapsam disi degil ama bu madde esas olarak pencere geometri duzeltmesidir.
- Editore yeni ozellik eklemek bu maddeyi gereksiz buyutmemelidir.

Bu alan icin kabul kriterleri:
- Pencerenin hicbir bolumu ekran disina tasmaz.
- Baslik, metin alani ve footer butun olarak gorunur.
- 1024x768 ve daha dar kosullarda bozulma yerine kontrollu kuculme olur.

## 5. Oncelik sirasi

Bu islerin uygulanma sirasi asagidaki gibi olmalidir:

1. Boot/preload ve zaman tabani duzeltmeleri
2. Clock donma algisinin giderilmesi
3. Pong zamanlama revizyonu
4. Pacman'in `pacman.md` ile uyumlu yeniden kurulmasi
5. Sistem izlencesi uygulamasi
6. Guc ve oturum akislarinin eklenmesi
7. Font hizasi ve editor geometri duzeltmesi

Bu siralamanin nedeni:
- Boot, clock ve oyun akiciligi icin once zaman tabanli ve cizim tabanli davranis oturmalidir.
- Pong ve Pacman ayni zamanlama problemini farkli siddetlerde yasamaktadir.
- Sistem izlencesi ve guc/oturum katmani cekirdek seviyesinde daha planli eklenmelidir.
- Editor ve tipografi duzeltmeleri en sona birakilabilir ama kapsam disina itilmemelidir.

## 6. Uygulama siniri matrisi

Pacman icin:
- Dokunulacak ana dosya: `kernel/games/pacman.c`
- Gerekirse yeni yardimci dosya: `kernel/games/pacman_map.h`
- Dokunulmayacak alanlar: diger oyunlar

Pong icin:
- Dokunulacak ana dosya: `kernel/games/pong.c`
- Dokunulmayacak alanlar: diger oyunlar

Clock icin:
- Dokunulacak ana dosya: `kernel/apps/clock.c`
- Zorunlu entegrasyon: gerekirse `kernel/shell/shell.c`
- Dokunulmayacak alanlar: ilgisiz sistem uygulamalari

Sistem izlencesi icin:
- Yeni dosya: `kernel/apps/system_monitor.c`
- Yeni baslik: `kernel/include/system_monitor.h`
- Zorunlu erisim noktasi: `kernel/shell/shell.c`, `kernel/apps/explorer.c`, `kernel/apps/appmanager.c`

Guc/oturum icin:
- Yeni dosyalar: `kernel/system/power.c`, `kernel/system/session.c`
- Yeni basliklar: `kernel/include/power.h`, `kernel/include/session.h`
- Zorunlu entegrasyon: `kernel/kernel.c`, `kernel/shell/shell.c`

Boot/preload icin:
- Dokunulacak dosyalar: `kernel/kernel.c`, `grub.cfg`

Font/editor icin:
- Dokunulacak dosyalar: `kernel/drivers/vga.c`, `kernel/include/font.h`, `kernel/apps/editor.c`

## 7. Kabuk komutlari ve isleme kurallari

Bu bolum, yalnizca kullanicinin ek talebine gore mevcut rapora ilave edilmistir. Amac, `cd`, `make`, `rm`, `del` ve bunlarin Turkce karsiliklarinin sistemde nasil ele alinacagini tanimlamaktir. Bu bolum yeni bir proje analizi degil, yalnizca rapora eklenecek davranis kurallaridir.

### 7.1 Komut esleme tablosu

- `cd`
  Turkce karsiliklari: `git`, `konum`
  Temel gorev: aktif calisma konumunu degistirmek

- `make`
  Turkce karsiliklari: `derle`, `kur`
  Temel gorev: derleme, kurulum veya paketleme akislarini baslatmak

- `rm`
  Turkce karsiliklari: `sil`
  Temel gorev: dosya silmek

- `del`
  Turkce karsiliklari: `sil`
  Temel gorev: dosya silmek

### 7.2 `cd` / `git` / `konum` komutu icin kurallar

- Bu komut grubu gercek dizin degistirme komutu olarak tanimlanmalidir.
- Komut hem kok dizine, hem alt dizine, hem de ust dizine gecebilmelidir.
- `cd ..` ust dizine cikis anlamina gelmelidir.
- `cd /` kok dizine donus anlamina gelmelidir.
- `cd <klasor_adi>` yalnizca mevcut dizin altinda boyle bir klasor varsa basarili olmalidir.
- Hedef klasor yoksa acik hata mesaji verilmelidir.
- Basarili gecis sonrasi shell promptu mevcut konumu gostermelidir.
- Explorer ve shell ayni dizin mantigini paylasmali; iki farkli konum gercegi olusturulmamalidir.
- Bu komut dizin destegi gelmeden sahte calisiyor gibi gosterilmemelidir.
- Eger dosya sistemi halen duz yapi kullaniyorsa, raporda ve uygulamada bu durum gecici sinir olarak acik belirtilmelidir.

### 7.3 `make` / `derle` / `kur` komutu icin kurallar

- Bu komut grubu bos cevap veren bir metin komutu olmaktan cikarilmalidir.
- `make` temel derleme giris komutu olarak kabul edilmelidir.
- `derle` komutu `make` ile ayni davranisi gostermelidir.
- `kur` komutu sadece derleme degil, hedefe gore kurulum asamasini da tetikleyebilmelidir.
- Komutlar kontrolsuz sekilde tum sistemi etkilememeli; sadece tanimli ve guvenli bir is akisina baglanmalidir.
- Derleme hedefi yoksa acik hata verilmelidir: ornek `derlenecek hedef bulunamadi`.
- Islem sirasinda durum satirlari gosterilmelidir: `hazirlaniyor`, `derleniyor`, `baglaniyor`, `kuruluyor`, `tamamlandi` gibi.
- Hata oldugunda kullaniciya yalnizca `basarisiz` denmemeli; hangi asamada durdugu belirtilmelidir.
- Bu komutlar gercek bir derleyici zincirine baglanana kadar sahte basarili sonuc uretmemelidir.
- `kur` komutu, ilgili hedef kurulabilir degilse bunu acikca belirtmelidir; zorla kurulum yapmaya calismamalidir.

### 7.4 `rm` / `del` / `sil` komutu icin kurallar

- Bu komutlar ayni silme davranisini paylasmalidir.
- Varsayilan davranis dosya silmek olmalidir.
- Klasor silme destegi ayrica tasarlanmadikca bu komutla klasor silinmemelidir.
- Silme islemi geri alinamaz olarak kabul edilmeli ve riskli alanlarda onay istenmelidir.
- Sistem dosyalari, cekirdek dosyalari, boot dosyalari ve kritik uygulama kayitlari korumali alan olarak tanimlanmalidir.
- Korumali alandaki bir hedef icin dogrudan silme yerine engelleme ve acik uyari verilmelidir.
- Hedef dosya yoksa sessiz gecilmemeli, hata donulmelidir.
- Basarili silme sonrasi kullaniciya hangi dosyanin silindigi net bicimde yazilmalidir.
- `del` ve `rm` arasinda farkli davranis olusturulmamali; iki komut ayni cekirdek silme yordamina baglanmalidir.
- Explorer icindeki silme mantigi ile shell silme mantigi ayni guvenlik kurallarini kullanmalidir.

### 7.5 Ortak davranis ilkeleri

- Komutun Ingilizce ve Turkce karsiligi ayni sonuc uretmelidir.
- Alias mantigi sadece isim duzeyinde olmali, alt tarafta farkli kod yollari olusturulmamalidir.
- Her komut icin `basarili`, `hata`, `gecersiz hedef`, `yetkisiz/korumali alan` durumlari ayri ele alinmalidir.
- Komut yardim metinlerinde hem Ingilizce hem Turkce karsiliklar birlikte listelenmelidir.
- Shell yardim ekrani ve rapor ayni komut sozlugunu kullanmalidir.
- Bu komutlar eklenirken mevcut calisan oyunlar, uygulamalar ve boot akislarina gereksiz mudahale yapilmamalidir.

## 8. Nihai yonlendirme

Bu rapordan sonra uygulanacak calisma su prensiple ilerlemelidir:
- Sadece kullanici tarafindan belirtilen alanlar duzeltilmeli
- Bunun disindaki calisan kodlar refaktor bahanesiyle degistirilmemeli
- Yeni ozellikler merkezi ama dar kapsamli modullerle eklenmeli
- Sahte performans metrikleri uretilmemeli
- Oyunlar ve uygulamalar donanimdan bagimsiz daha tutarli zaman tabanina alinmali

Net sonuc:
- Pacman, artik `pacman.md` ile uyumlu gorunum ve kurallara sahip stabil bir oyun olmalidir.
- Pong, akici ve tepkisel bir oynanis sunmalidir.
- Clock, shell'i kilitlenmis gibi gostermemelidir.
- Sistem izlencesi, canli ve grafiksel bir AegisOS uygulamasi olmalidir.
- Boot, guc, oturum, font ve editor alanlari isletim sistemi hissini tamamlayan duzeyde toparlanmalidir.