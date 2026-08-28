🔌 STM32 Composite USB: Otonom Telemetri Monitörü (HID + CDC)
STM32USBPythonC

Bu proje, bilgisayara takıldığı anda otonom olarak klavye gibi davranarak bir Python betiği yazan, ardından bilgisayarın RAM ve CPU verilerini okuyarak yine aynı USB kablosu üzerinden seri port (CDC) aracılığıyla mikrodenetleyiciye geri gönderen çift yönlü bir Composite USB (HID Klavye + Sanal Seri Port) projesidir.

📖 1. Projenin Amacı ve Özeti
Bir donanımın (mikrodenetleyici) bilgisayara takıldığı anda hiçbir harici yazılıma veya sürücüye ihtiyaç duymadan (Plug & Play) bilgisayarın donanım sensörlerine ulaşıp ulaşamayacağını kanıtlamak amacıyla geliştirilmiştir.

Sistem şu döngüyü icra eder:

STM32 bilgisayara takıldığında kendini hem bir Klavye (HID) hem de Seri Port (CDC) olarak tanıtır.
Otomatik olarak Windows + R tuşlarına basar ve PowerShell'i açar.
Arka planda psutil ve pyserial kütüphanelerini indirir.
Terminale satır satır bir Python kodu yazar (Yazdığı kod, bilgisayarın CPU/RAM verilerini okuyup COM portuna gönderen bir döngüdür).
Bilgisayar bu verileri STM32'ye geri gönderir, STM32 ise bu verileri alıp harici bir LCD ekranda anlık olarak gösterir.

🛠️ 2. Kullanılan Donanım ve "Çift USB" Bağlantı Mimarisi
Mikrodenetleyici: STM32 Nucleo-144 (NUCLEO-F439ZI)
Gösterge: 16x2 Karakter LCD ve PCF8574 I2C Genişletici.
<br>
<p align="center">
  <img src="https://github.com/IlkeAsan/STM32-USB-Telemetry-Monitor/blob/main/images/Ekran%20g%C3%B6r%C3%BCnt%C3%BCs%C3%BC%202026-08-28%20093037.png" width="46%" />
  <img src="https://github.com/IlkeAsan/STM32-USB-Telemetry-Monitor/blob/main/images/WhatsApp%20Image%202026-08-26%20at%2010.47.24%20(1).jpeg?raw=true" width="46%" />
  <br><br><br>
</p>
<br>
⚠️ Kritik Donanım Detayı: Çift USB Portu Kullanımı
STM32 Nucleo-144 kartları üzerinde iki adet USB portu bulunur ve bu proje için her ikisinin de rolü farklıdır:

Üstteki USB Portu (ST-LINK): Bilgisayardan karta kod yüklemek (Debug/Flash) ve karta güç (VCC) vermek için kullanılır.
Alttaki USB Portu (USB OTG FS / User USB): Mikrodenetleyicinin içindeki USB çevre birimine (Peripheral) doğrudan bağlıdır. Kartımızın bilgisayara bir "Klavye" ve "COM Port" olarak görünmesini sağlayan asıl veri yolu burasıdır.
Çalıştırma Esnasında: Kartın çalışabilmesi için üstteki porttan güç alması, alttaki portun ise bilgisayara takılarak veri iletişimini sağlaması gerekmektedir.

🧠 3. Karşılaşılan Mühendislik Problemleri ve Çözümleri
3.1. Composite Device Sürücü Çökmesi (IAD Çözümü)
Windows işletim sistemi, tek bir USB kablosu üzerinden hem klavye hem de seri port cihazı bağlandığında sürücüleri karıştırıp "Kod 10 (Device Cannot Start)" hatası vermektedir. Bu sorunu çözmek için USB tanımlayıcılarının (Descriptor) arasına IAD (Interface Association Descriptor) modülleri yerleştirilerek Windows'a bu iki farklı donanımın sınırları kesin olarak belirtilmiştir.

3.2. TR-Q (Türkçe Q) Klavye Uyumsuzluğu
Standart USB HID protokolü donanımsal olarak Amerikan (US) klavye scancode'ları gönderir. Örneğin kodda : (iki nokta) yollamak istediğinizde, bilgisayarın dili Türkçe ise ekrana Ş harfi basılır ve Python kodu çöker. Çözüm: keyboard_helper.c içerisine devasa bir karakter haritası (CharToKeycode) yazılarak, Türkçe Q klavyede şifrelenmiş karakterler tersine mühendislikle US scancode'larına çevrilmiş ve %100 uyumluluk sağlanmıştır.

⏱️ 4. ÖNEMLİ: Zamanlama (Delay) Ayarlarının Yapılandırılması
STM32 klavye tuşlarını gönderirken bilgisayarın tepki süresine (pencerelerin açılması, programların inmesi) saygı duymak zorundadır. Aksi takdirde harfleri boşluğa veya yanlış sekmeye yazar.

Özellikle Python kütüphanelerinin inme süresi, sizin bilgisayarınızın internet hızına bağlıdır. Kütüphane indirme işleminin zamanlamasını değiştirmek isterseniz keyboard_helper.c dosyasına girerek şu satırı bulmalısınız:

c

/* Core/Src/keyboard_helper.c - Satır ~195 */
REPL_Line("pip install psutil pyserial", 12000); // 12000 ms (12 Saniye)

Kullanıcı Geliştirme Notu: Eğer internetiniz yavaşsa ve siyah PowerShell ekranında indirme işlemi bitmeden STM32 diğer kodları yazmaya başlıyorsa, buradaki 12000 değerini örneğin 25000 (25 saniye) yaparak indirme işlemine ekstra pay (süre) verebilirsiniz. Eğer kütüphaneler zaten bilgisayarınızda yüklüyse bu süreyi 3000'e düşürerek süreci hızlandırabilirsiniz.
------------------------------------------
Aynı şekilde harflerin yazılma hızını artırmak isterseniz Keyboard_SendKey içindeki HAL_Delay(3) değerini 1 veya 2 yaparak şimşek hızında yazım elde edebilirsiniz.

🚀 5. Kurulum ve Çalıştırma Adımları
Donanım Bağlantılarını Yapın:
Birinci Micro-USB kablosunu STM32'nin Üst (ST-LINK) portuna takarak bilgisayara bağlayın (Güç ve kod yükleme için).
İkinci Micro-USB kablosunu STM32'nin Alt (USER USB) portuna takarak bilgisayara bağlayın (Klavye/Seri Port iletişimi için).
I2C LCD ekranı PB8 (SCL) ve PB9 (SDA) pinlerine takın.
Kodu Derleyin ve Yükleyin:
Projeyi STM32CubeIDE ile açın.
Eğer internet hızınıza göre pip install süresini artırmak isterseniz keyboard_helper.c dosyasındaki ayarı yapın.
Projeyi derleyin ve karta flashlayın (Run).
İzleyin!
Kod karta yüklendiği an STM32 kendini resetler, alt USB portu üzerinden bilgisayarınıza bir klavye olarak bağlanır.
Ekranda hiçbir şeye dokunmayın (Farenizi veya klavyenizi kullanmayın).
STM32 otomatik olarak Win+R yapacak, PowerShell'i açacak, gerekli eklentileri indirecek ve "Designed by Ilke" imzalı Python kodunu yazarak çalıştıracaktır.
Ekranda anlık RAM/CPU telemetrisini ve LCD ekranınızda sonuçları görebilirsiniz.

🛠️ 6. Geliştiriciler İçin: Neler Eklenebilir?
Zararlı Yazılım (BadUSB) Güvenlik Araştırmaları: Bu mimari (HID Payload Injection), siber güvenlik alanında sistemlerin zafiyetlerini test etmek (Penetration Testing) amacıyla USB Rubber Ducky alternatifi olarak incelenebilir ve geliştirilebilir.
Bu proje, donanım-yazılım senkronizasyonu ve USB yığıtlarının (Stack) derinlemesine incelenmesi amacıyla açık kaynak olarak sunulmuştur.
