#pragma once
/*
  config.h
  Semua pengaturan yang sering diubah ada di file ini.
  Jangan menyertakan file ini dari file .cpp lain, cukup dari esp32cam_live.ino.
*/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_http_server.h"
#include <lwip/sockets.h>

// ===================== Pengaturan utama =====================
int TARGET_FPS = 10;                       // fps tampilan, 1 sampai 30. Disarankan 5 sampai 12.
                                           // Bisa diubah saat berjalan: /control?var=fps&val=5

// Mode hotspot (ESP32-CAM menjadi access point)
const char* AP_SSID    = "ESP32CAM-Live";
const char* AP_PASS    = "12345678";       // minimal 8 karakter
const int   AP_CHANNEL = 11;               // pilih 1, 6, atau 11 yang paling sepi

// Mode router (opsional). Kosongkan ROUTER_SSID jika hanya memakai hotspot.
// Jangan commit SSID dan password asli ke GitHub.
const char* ROUTER_SSID = "";
const char* ROUTER_PASS = "";
IPAddress STA_IP(192, 168, 1, 50);         // satu subnet dengan router, di luar rentang DHCP
IPAddress STA_GW(192, 168, 1, 1);          // alamat router
IPAddress STA_MASK(255, 255, 255, 0);

// Default kamera saat boot (paling ringan)
const framesize_t DEFAULT_FRAMESIZE = FRAMESIZE_QVGA;   // 320x240
const int         DEFAULT_QUALITY   = 63;               // 10 terbaik, 63 terendah
const int         CAMERA_XCLK_HZ    = 10000000;         // 10 MHz, naikkan ke 20000000 jika perlu

// Parameter untuk pengujian WiFi
const wifi_power_t WIFI_TX_POWER = WIFI_POWER_19_5dBm;  // uji WIFI_POWER_11dBm jika curiga daya kurang
const bool CAMERA_ENABLED = true;                       // false = hanya WiFi, untuk tes ping tanpa kamera

// ===================== Pin AI-Thinker =====================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===================== Deklarasi fungsi =====================
// Fungsi ini didefinisikan di file lain (camera.ino, wifi.ino, web.h).
// Deklarasi di sini mencegah error urutan saat file digabung oleh Arduino.
bool initCamera();
void startWiFi();
int  getRssi();
void startServers();
void logStatus();
