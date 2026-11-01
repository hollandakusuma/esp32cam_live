/*
  wifi.ino
  Hotspot (AP) atau router (STA), event WiFi, dan pembacaan RSSI.
*/

int getRssi() {
  if (WiFi.getMode() == WIFI_STA) return WiFi.RSSI();
  wifi_sta_list_t sl;
  if (esp_wifi_ap_get_sta_list(&sl) == ESP_OK && sl.num > 0) return sl.sta[0].rssi;
  return 0;
}

static void onWiFiEvent(WiFiEvent_t e, WiFiEventInfo_t info) {
  switch (e) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.printf("[%lu ms] [WIFI] hotspot aktif\n", millis());
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
      const uint8_t *m = info.wifi_ap_staconnected.mac;
      Serial.printf("[%lu ms] [WIFI] klien tersambung, MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                    millis(), m[0], m[1], m[2], m[3], m[4], m[5]);
      break;
    }
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.printf("[%lu ms] [WIFI] IP diberikan ke klien: %s\n", millis(),
                    IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
      const uint8_t *m = info.wifi_ap_stadisconnected.mac;
      Serial.printf("[%lu ms] [WIFI] klien terputus, MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                    millis(), m[0], m[1], m[2], m[3], m[4], m[5]);
      break;
    }
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("[%lu ms] [WIFI] tersambung ke router, IP %s\n", millis(),
                    WiFi.localIP().toString().c_str());
      break;
    default:
      break;
  }
}

void startWiFi() {
  WiFi.onEvent(onWiFiEvent);

  if (strlen(ROUTER_SSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.config(STA_IP, STA_GW, STA_MASK);
    WiFi.begin(ROUTER_SSID, ROUTER_PASS);
    Serial.print("Menyambung ke router");
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
      delay(300);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.setSleep(false);
      WiFi.setTxPower(WIFI_TX_POWER);
      Serial.print("Mode router, buka http://");
      Serial.println(WiFi.localIP());
      return;
    }
    Serial.println("Router gagal, pindah ke mode hotspot");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, 0, 2);   // maksimal 2 klien
  WiFi.setSleep(false);                              // harus setelah WiFi menyala
  WiFi.setTxPower(WIFI_TX_POWER);
  Serial.print("Mode hotspot, SSID ");
  Serial.print(AP_SSID);
  Serial.print(", channel ");
  Serial.print(AP_CHANNEL);
  Serial.print(", buka http://");
  Serial.println(WiFi.softAPIP());
}
