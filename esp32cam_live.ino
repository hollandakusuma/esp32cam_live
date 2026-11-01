/*
  ESP32-CAM Live Monitor, model tarik (pull) untuk latensi rendah
  Board : AI Thinker ESP32-CAM, PSRAM Enabled

  File utama hanya berisi setup() dan loop().
    config.h   pengaturan, pin, dan deklarasi fungsi
    camera.ino inisialisasi kamera
    wifi.ino   hotspot atau router, event WiFi, RSSI
    handler.h  string HTML (halaman konfigurasi dan video)
    web.h      web server, handler HTTP, log status
*/

#include "config.h"
#include "handler.h"
#include "web.h"

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32-CAM Live Monitor (model tarik) ===");

  if (CAMERA_ENABLED) {
    if (!initCamera()) {
      Serial.println("Kamera gagal diinisialisasi, restart...");
      delay(1000);
      ESP.restart();
    }
    Serial.println("[CAM] kamera siap");
  } else {
    Serial.println("[CAM] kamera DIMATIKAN (mode tes WiFi)");
  }

  startWiFi();

  if (CAMERA_ENABLED) {
    startServers();
    Serial.printf("Siap. Target %d fps. Menunggu klien...\n", TARGET_FPS);
  } else {
    Serial.println("Siap. Server web tidak dijalankan, gunakan ping untuk menguji WiFi.");
  }
}

void loop() {
  logStatus();
  delay(1);
}
