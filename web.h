#pragma once
/*
  web.h
  Web server (port 80) dan capture server (port 81), handler HTTP,
  serta pencatatan status ke Serial Monitor.
*/

#include "config.h"
#include "handler.h"

// ===================== Variabel global =====================
static httpd_handle_t main_httpd    = NULL;
static httpd_handle_t capture_httpd = NULL;

static volatile uint32_t stat_frames     = 0;
static volatile uint32_t stat_bytes      = 0;
static volatile uint32_t stat_last_len   = 0;
static volatile uint32_t last_capture_ms = 0;

// ===================== Utilitas =====================
static String peerIp(httpd_req_t *req) {
  int fd = httpd_req_to_sockfd(req);
  struct sockaddr_storage ss;
  socklen_t len = sizeof(ss);
  if (getpeername(fd, (struct sockaddr *)&ss, &len) != 0) return "?";
  if (ss.ss_family == AF_INET) {
    uint32_t a = ((struct sockaddr_in *)&ss)->sin_addr.s_addr;
    return IPAddress(a).toString();
  }
  if (ss.ss_family == AF_INET6) {
    struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)&ss;
    return IPAddress(s6->sin6_addr.un.u8_addr[12], s6->sin6_addr.un.u8_addr[13],
                     s6->sin6_addr.un.u8_addr[14], s6->sin6_addr.un.u8_addr[15]).toString();
  }
  return "?";
}

static esp_err_t onSocketOpen(httpd_handle_t hd, int fd) {
  Serial.printf("[%lu ms] [SOCKET] koneksi TCP baru (fd=%d)\n", millis(), fd);
  return ESP_OK;
}

static void onSocketClose(httpd_handle_t hd, int fd) {
  Serial.printf("[%lu ms] [SOCKET] koneksi TCP ditutup (fd=%d)\n", millis(), fd);
  close(fd);
}

static esp_err_t sendPage(httpd_req_t *req, const char *name, const char *page) {
  uint32_t t0 = millis();
  Serial.printf("[%lu ms] [HTTP] halaman %s diminta oleh %s\n", t0, name, peerIp(req).c_str());
  httpd_resp_set_type(req, "text/html");
  esp_err_t r = httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
  Serial.printf("[%lu ms] [HTTP] halaman %s terkirim %u byte dalam %lu ms (%s)\n",
                millis(), name, (unsigned)strlen(page), (unsigned long)(millis() - t0),
                r == ESP_OK ? "OK" : "GAGAL");
  return r;
}

// ===================== Handler HTTP =====================
static esp_err_t config_handler(httpd_req_t *req) { return sendPage(req, "konfigurasi", PAGE_CONFIG); }
static esp_err_t view_handler(httpd_req_t *req)   { return sendPage(req, "video", PAGE_VIEW); }

// Tidak dicatat ke Serial karena dipanggil berkala oleh browser
static esp_err_t status_handler(httpd_req_t *req) {
  sensor_t *s = esp_camera_sensor_get();
  char json[192];
  int n = snprintf(json, sizeof(json),
    "{\"framesize\":%u,\"quality\":%u,\"brightness\":%d,\"contrast\":%d,\"fps\":%d,\"rssi\":%d}",
    s->status.framesize, s->status.quality, s->status.brightness, s->status.contrast,
    TARGET_FPS, getRssi());
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json, n);
}

static esp_err_t control_handler(httpd_req_t *req) {
  char query[64], var[16], val[8];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "var", var, sizeof(var)) != ESP_OK ||
      httpd_query_key_value(query, "val", val, sizeof(val)) != ESP_OK) {
    Serial.printf("[%lu ms] [CTRL] request tidak valid dari %s\n", millis(), peerIp(req).c_str());
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int v = atoi(val);
  sensor_t *s = esp_camera_sensor_get();
  int r = -1;

  if      (!strcmp(var, "framesize"))  r = s->set_framesize(s, (framesize_t)v);
  else if (!strcmp(var, "quality"))    r = s->set_quality(s, v);
  else if (!strcmp(var, "brightness")) r = s->set_brightness(s, v);
  else if (!strcmp(var, "contrast"))   r = s->set_contrast(s, v);
  else if (!strcmp(var, "fps")) {
    TARGET_FPS = constrain(v, 1, 30);
    v = TARGET_FPS;
    r = 0;
  }

  if (r < 0) {
    Serial.printf("[%lu ms] [CTRL] gagal set %s = %d\n", millis(), var, v);
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  Serial.printf("[%lu ms] [CTRL] %s = %d (dari %s)\n", millis(), var, v, peerIp(req).c_str());
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, "ok", 2);
}

// Satu foto JPEG per permintaan. Dengan GRAB_LATEST, frame yang dikirim selalu yang terbaru.
static esp_err_t capture_handler(httpd_req_t *req) {
  uint32_t t0 = millis();
  camera_fb_t *fb = esp_camera_fb_get();
  uint32_t t1 = millis();
  if (!fb) {
    Serial.printf("[%lu ms] [CAPTURE] gagal mengambil frame dari kamera\n", millis());
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");

  size_t len = fb->len;
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, len);
  uint32_t t2 = millis();
  esp_camera_fb_return(fb);

  if (t2 - t0 > 80) {
    Serial.printf("[%lu ms] [CAPTURE] lambat, ambil frame %lu ms, kirim %lu ms\n",
                  t2, (unsigned long)(t1 - t0), (unsigned long)(t2 - t1));
  }

  if (res == ESP_OK) {
    uint32_t now = millis();
    if (now - last_capture_ms > 3000) {
      Serial.printf("[%lu ms] [CAPTURE] klien mulai mengambil gambar: %s\n", now, peerIp(req).c_str());
    }
    last_capture_ms = now;
    stat_frames++;
    stat_bytes += len;
    stat_last_len = len;
  } else {
    Serial.printf("[%lu ms] [CAPTURE] gagal mengirim frame (kode %d)\n", millis(), (int)res);
  }
  return res;
}

// ===================== Start server =====================
void startServers() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers  = 8;
  config.max_open_sockets  = 4;      // dua server, total tetap muat dalam batas soket LWIP
  config.lru_purge_enable  = true;
  config.send_wait_timeout = 3;
  config.recv_wait_timeout = 3;
  config.open_fn  = onSocketOpen;
  config.close_fn = onSocketClose;

  httpd_uri_t uri_config  = { "/",        HTTP_GET, config_handler,  NULL };
  httpd_uri_t uri_view    = { "/view",    HTTP_GET, view_handler,    NULL };
  httpd_uri_t uri_status  = { "/status",  HTTP_GET, status_handler,  NULL };
  httpd_uri_t uri_control = { "/control", HTTP_GET, control_handler, NULL };
  httpd_uri_t uri_capture = { "/capture", HTTP_GET, capture_handler, NULL };

  config.server_port = 80;
  if (httpd_start(&main_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(main_httpd, &uri_config);
    httpd_register_uri_handler(main_httpd, &uri_view);
    httpd_register_uri_handler(main_httpd, &uri_status);
    httpd_register_uri_handler(main_httpd, &uri_control);
    Serial.println("[SERVER] web server aktif di port 80");
  } else {
    Serial.println("[SERVER] GAGAL memulai web server port 80");
  }

  // Server terpisah untuk foto agar halaman dan kontrol tidak ikut terblokir saat WiFi macet
  config.server_port = 81;
  config.ctrl_port  += 1;
  if (httpd_start(&capture_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(capture_httpd, &uri_capture);
    Serial.println("[SERVER] capture server aktif di port 81");
  } else {
    Serial.println("[SERVER] GAGAL memulai capture server port 81");
  }
}

// ===================== Log status berkala =====================
// Dipanggil dari loop(). Mencetak fps tiap detik dan info WiFi dan memori tiap 5 detik.
void logStatus() {
  static uint32_t lastStat = 0;
  static uint32_t lastInfo = 0;
  static bool wasActive = false;
  uint32_t now = millis();

  if (now - lastStat >= 1000) {
    lastStat = now;
    uint32_t f = stat_frames;
    uint32_t b = stat_bytes;
    stat_frames = 0;
    stat_bytes  = 0;

    bool active = (last_capture_ms != 0) && (now - last_capture_ms < 2000);
    if (active) {
      Serial.printf("[%lu ms] [CAPTURE] %u fps | %u KB/s | frame terakhir %u KB\n",
                    now, (unsigned)f, (unsigned)(b / 1024), (unsigned)(stat_last_len / 1024));
    } else if (wasActive) {
      Serial.printf("[%lu ms] [CAPTURE] klien berhenti mengambil gambar\n", now);
    }
    wasActive = active;
  }

  if (now - lastInfo >= 5000) {
    lastInfo = now;
    Serial.printf("[%lu ms] [INFO] klien WiFi: %d | RSSI: %d dBm | heap bebas: %u KB | min heap: %u KB\n",
                  now, WiFi.softAPgetStationNum(), getRssi(),
                  (unsigned)(ESP.getFreeHeap() / 1024), (unsigned)(ESP.getMinFreeHeap() / 1024));
  }
}
