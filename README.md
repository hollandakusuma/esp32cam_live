# ESP32-CAM Live Monitor

Firmware ESP32-CAM (AI-Thinker) untuk menampilkan video langsung di browser smartphone atau laptop dengan latensi rendah. Proyek ini dibuat sebagai mata untuk robot yang bergerak melewati rintangan. Halaman web disimpan di dalam firmware, jadi cukup menyambung ke WiFi ESP32-CAM lalu membuka satu alamat di browser, tanpa aplikasi tambahan.

## Fitur

- ESP32-CAM menjadi hotspot sendiri (mode AP), tanpa router dan tanpa internet.
- Mode router (STA) opsional dengan static IP. Jika router gagal tersambung dalam 10 detik, otomatis kembali ke mode hotspot.
- Halaman konfigurasi sebelum video: resolusi, kualitas JPEG, brightness, dan contrast.
- Halaman video yang pas untuk smartphone landscape. Semua teks kecil dan berada di dalam frame kamera.
- Pengaturan langsung di halaman video: resolusi (kiri bawah), brightness (kiri tengah), contrast (kanan tengah).
- Indikator latensi (kanan bawah) dan kekuatan sinyal RSSI (kanan atas) dengan warna hijau, kuning, atau merah.
- Batas fps sebagai variabel global yang bisa diubah di kode maupun saat berjalan.
- Log lengkap di Serial Monitor: klien tersambung, halaman diminta, foto dikirim, fps, RSSI, dan memori.

## Cara kerja

Video tidak memakai stream MJPEG, melainkan model tarik. Browser meminta satu foto lewat `/capture`, menampilkannya, menunggu sisa interval sesuai `TARGET_FPS`, lalu meminta lagi. Dengan begitu tidak ada antrean frame lama saat WiFi tersendat, dan umur gambar yang tampil dibatasi satu kali round-trip. Jika permintaan macet lebih dari 3 detik, permintaan dibatalkan, gambar terakhir tetap tampil, dan browser mencoba lagi otomatis.

Angka LATENCY di layar adalah waktu dari meminta foto sampai foto diterima browser. Angka ini belum termasuk waktu sensor menangkap gambar.

## Perangkat keras

- ESP32-CAM board AI-Thinker (kamera OV2640, dengan PSRAM).
- Catu daya 5V minimal 2A langsung ke pin 5V dan GND, dengan kabel pendek. Kapasitor sekitar 470 sampai 1000 uF di dekat pin 5V sangat membantu.
- Untuk upload firmware: adaptor USB ke serial (FTDI) atau board pendukung ESP32-CAM-MB.

Catatan untuk robot. Jangan satu sumber daya dengan motor tanpa filter, karena lonjakan arus motor bisa menjatuhkan tegangan kamera dan WiFi. Idealnya pakai regulator step down terpisah untuk ESP32-CAM.

## Struktur proyek

```
esp32cam-live/
├── README.md
├── .gitignore
└── esp32cam_live/
    ├── esp32cam_live.ino   setup() dan loop()
    ├── config.h            pengaturan, pin, deklarasi fungsi
    ├── camera.ino          inisialisasi kamera
    ├── wifi.ino            hotspot atau router, event WiFi, RSSI
    ├── handler.h           string HTML halaman konfigurasi dan video
    └── web.h               web server, handler HTTP, log status
```

Nama folder sketch harus sama dengan nama file utama (`esp32cam_live`), karena itu aturan Arduino IDE.

## Instalasi

1. Pasang Arduino IDE.
2. Tambahkan paket board ESP32 dari Espressif lewat Boards Manager. Proyek ini diuji dengan versi 2.0.6.
3. Pilih board **AI Thinker ESP32-CAM**.
4. Aktifkan **PSRAM** (Tools, PSRAM, Enabled).
5. Buka `esp32cam_live/esp32cam_live.ino`, lalu upload. Pastikan GPIO0 dihubungkan ke GND saat upload, dan dilepas setelahnya.
6. Buka Serial Monitor pada 115200 baud untuk melihat log.

Pesan `Multiple libraries were found for "WiFi.h"` saat kompilasi hanya informasi dan bisa diabaikan.

## Cara pakai

Mode hotspot (bawaan)

1. Sambungkan smartphone atau laptop ke WiFi `ESP32CAM-Live` dengan password `12345678`.
2. Buka `http://192.168.4.1` di browser.
3. Atur konfigurasi, lalu tekan **Mulai**.

Jika Android memutus WiFi karena tidak ada internet, matikan data seluler selama dipakai, matikan fitur pindah otomatis ke data seluler (Smart Network Switch, Wi-Fi+, atau sejenisnya), dan pilih tetap terhubung saat muncul peringatan.

Mode router

1. Isi `ROUTER_SSID`, `ROUTER_PASS`, dan alamat static IP di `config.h`. Pastikan subnet dan gateway sesuai router.
2. Upload ulang. IP akan tercetak di Serial Monitor.
3. Sambungkan perangkat ke router yang sama, lalu buka alamat static IP tersebut, misalnya `http://192.168.1.50`.

Disarankan mengunci channel router di 1, 6, atau 11 pada band 2.4 GHz.

## Pengaturan

Semua pengaturan ada di `esp32cam_live/config.h`.

| Pengaturan | Bawaan | Keterangan |
|---|---|---|
| `TARGET_FPS` | 10 | fps tampilan, 1 sampai 30. Disarankan 5 sampai 12. |
| `AP_SSID`, `AP_PASS` | ESP32CAM-Live, 12345678 | Nama dan password hotspot. Minimal 8 karakter. |
| `AP_CHANNEL` | 11 | Pilih 1, 6, atau 11 yang paling sepi. |
| `ROUTER_SSID`, `ROUTER_PASS` | kosong | Isi untuk memakai mode router. |
| `DEFAULT_FRAMESIZE` | QVGA | Resolusi awal saat boot. |
| `DEFAULT_QUALITY` | 63 | Kualitas JPEG awal. 10 terbaik, 63 terendah. |
| `CAMERA_XCLK_HZ` | 10 MHz | Naikkan ke 20 MHz jika perlu. |
| `WIFI_TX_POWER` | 19,5 dBm | Daya pancar WiFi. Turunkan hanya untuk pengujian. |
| `CAMERA_ENABLED` | true | Isi false untuk tes ping tanpa kamera. |

`TARGET_FPS` juga bisa diubah saat berjalan tanpa upload ulang:

```
http://192.168.4.1/control?var=fps&val=5
```

## Endpoint

| Alamat | Port | Fungsi |
|---|---|---|
| `/` | 80 | Halaman konfigurasi |
| `/view` | 80 | Halaman video |
| `/status` | 80 | Status kamera, fps, dan RSSI dalam format JSON |
| `/control?var=NAMA&val=NILAI` | 80 | Mengubah `framesize`, `quality`, `brightness`, `contrast`, atau `fps` |
| `/capture` | 81 | Satu foto JPEG |

Foto dilayani di port terpisah supaya halaman dan kontrol tetap responsif saat pengiriman foto tersendat.

## Pemecahan masalah

Video tersendat atau latensi melonjak. Perhatikan angka RSSI di layar. Hijau (lebih baik dari -65 dBm) berarti sinyal bagus, kuning (-65 sampai -75) batas, merah (lebih buruk dari -75) lemah. Jika RSSI bagus tetapi masih tersendat, penyebabnya biasanya daya, derau, atau interferensi channel. Beberapa langkah uji:

1. Jalankan `ping -t 192.168.4.1` di laptop saat video berjalan. Paket timeout menandakan masalah di jalur WiFi, bukan di kode.
2. Ganti catu daya ke adaptor 5V 2A dengan kabel pendek, dan tambahkan kapasitor dekat pin 5V.
3. Ganti `AP_CHANNEL` ke 1 atau 6.
4. Set `CAMERA_ENABLED = false`, upload, lalu ping. Jika ping bersih, penyebabnya kamera atau catu daya.
5. Coba mode router. Radio dan antena router umumnya lebih baik daripada hotspot ESP32.
6. Di laptop Windows, matikan penghematan daya pada adaptor WiFi dan setel Roaming Aggressiveness ke Lowest.

Gambar hitam atau rusak. Turunkan `CAMERA_XCLK_HZ` ke 10 MHz, dan periksa catu daya.

Kamera gagal diinisialisasi. Periksa kabel pita kamera dan pastikan PSRAM aktif.

Halaman lambat dibuka. Ketik alamat lengkap `http://192.168.4.1` dan lihat selisih waktu di Serial Monitor, apakah jeda terjadi sebelum atau sesudah permintaan sampai ke ESP32.

## Keamanan

Jangan commit SSID dan password router asli ke repositori publik. Kosongkan `ROUTER_SSID` dan `ROUTER_PASS` di `config.h` sebelum commit. Password hotspot bawaan sebaiknya diganti jika perangkat dipakai di tempat umum.

## Lisensi

Tambahkan berkas `LICENSE` sesuai pilihanmu, misalnya MIT.
