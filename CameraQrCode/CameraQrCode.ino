#include "ESP32QRCodeReader.h"
#include <WiFi.h>

#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void startCameraServer();
void qrCodeDetectTask();
static camera_config_t config;

static ESP32QRCodeReader *reader = NULL;

void loopA(void* taskData)
{

  reader = new ESP32QRCodeReader();

  while (true)
  {
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
      Serial.println("Camera capture failed");
      return;
    }
    
    if (reader->qrCodeDetectTask(&config, fb))
    {
      Serial.println("DETECTED:");
      Serial.println((const char *)reader->buffer);
    }

    esp_camera_fb_return(fb);
    fb = NULL;

    delay(100);
  }
}


void setup()
{
  Serial.begin(115200);
  Serial.println();

  if (!psramFound())
  {
    Serial.println("No psram");
    return;
  }

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  WiFi.begin("TP-LINK_B2A4", "910426mativa");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  xTaskCreatePinnedToCore(loopA, "qrCodeDetectTask", 40 * 1024, NULL, 5, NULL, 1);

}

void loop()
{
  delay(100);
}
