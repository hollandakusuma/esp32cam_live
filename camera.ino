/*
  camera.ino
  Inisialisasi kamera OV2640 pada board AI-Thinker.
*/

bool initCamera() {
  camera_config_t c;
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;  c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM;  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;  c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM;  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;
  c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM;
  c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM;
  c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;
  c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = CAMERA_XCLK_HZ;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size   = DEFAULT_FRAMESIZE;
  c.jpeg_quality = DEFAULT_QUALITY;
  c.fb_count     = 2;             // dua buffer + GRAB_LATEST, selalu ambil frame terbaru
  c.fb_location  = CAMERA_FB_IN_PSRAM;
  c.grab_mode    = CAMERA_GRAB_LATEST;

  if (!psramFound()) {
    c.fb_count    = 1;
    c.fb_location = CAMERA_FB_IN_DRAM;
  }

  return esp_camera_init(&c) == ESP_OK;
}
