#include "ESP32QRCodeReader.h"
#include <WiFi.h>

#include "esp_camera.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void startCameraServer();
void stopCameraServer();
void qrCodeDetectTask();
static camera_config_t config;

static ESP32QRCodeReader *reader = NULL;

void serialPrint(String data){
  digitalWrite(2, HIGH);
  delayMicroseconds(100);
  Serial.println(data);
  delayMicroseconds(10);
  digitalWrite(2, LOW);
}

void loopQrCodeDetect(void* taskData)
{

  reader = new ESP32QRCodeReader();

  while (true)
  {
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
      //serialPrint("Err: Camera capture failed");
      return;
    }
    
    if (reader->qrCodeDetectTask(&config, fb))
    {
      //serialPrint("DETECTED:");
      serialPrint((const char *)reader->buffer);
    }

    esp_camera_fb_return(fb);
    fb = NULL;

    delay(100);
  }
}

static int wifi_enabled = 0;

void cmdProtocolFunc(bool (*handler_func)(const String&, const String&, const String&)) {
  String code = Serial.readStringUntil('\n');
  if (code.startsWith("@")) {
    String addr1 = code.substring(1,3);
    String addr2 = code.substring(3,5);
    String rest = code.substring(5);
    serialPrint("@" + rest + "OKSS");
    if (handler_func(addr1, addr2, rest)) {
      return;
    } else {
        serialPrint("@" + addr2 + addr1 + "NOKSS");
    }
  }
}

static int progNum = 0;

bool wifiConnect(const String& ssid, const String password) {
  if (progNum == 1) {
    WiFi.begin(ssid.c_str(), password.c_str());
    int i =0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      i++;
      if (i == 10) {
        return false;
      }
    } 
  } else {
    WiFi.softAP(ssid.c_str(), password.c_str());
  }
  return true;
}

void wifiDisconnect() {
  if (progNum == 1)
    WiFi.disconnect(true, true);
  else
    WiFi.softAPdisconnect(true);
}

IPAddress wifiIp() {
  if (progNum == 1)
    return  WiFi.localIP();
  return WiFi.softAPIP();
}

bool wifiCommands(const String& addr1, const String& addr2, const String& rest) {
  if ((!rest.startsWith("WIFI=")) && (!rest.startsWith("WIOFF")) && (!rest.startsWith("WSTAT?"))) {
    return false;
  }

  if (rest.startsWith("WIFI=")) {
    if (wifi_enabled == 1) {
      stopCameraServer();
      wifiDisconnect();
    }

    String ssid = rest.substring(5,15);
    String password = rest.substring(16,26);
    if (!wifiConnect(ssid, password)) {
      serialPrint("@" + addr2 + addr1 + "NOKSS");
    } else {
      startCameraServer();
      delay(100);
      wifi_enabled = 1;
      serialPrint("@" + addr2 + addr1 + "OKSS");
    }
  } else if (rest.startsWith("WIOFF")) {
    if (wifi_enabled == 1) {
      stopCameraServer();
      wifiDisconnect();
    }
    serialPrint("@" + addr2 + addr1 + "OKSS");
    wifi_enabled = 0;
  } else if (rest.startsWith("WSTAT?")) {
    if (wifi_enabled == 1) {
      IPAddress IP = wifiIp();
      serialPrint("@" + addr2 + addr1 + "WSTAT=" + IP.toString() + "SS");
    } else {
      serialPrint("@" + addr2 + addr1 + "WSTAT=NOKSS");
    }
  }
  return true;
}

bool loopProgSelection(const String& addr1, const String& addr2, const String& rest) {
  if ((!rest.startsWith("PRGQR")) && (!rest.startsWith("PRGBX"))) {
    return false;
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

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  if (rest.startsWith("PRGQR")) {

    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    progNum = 1;

  } else if (rest.startsWith("PRGBX")) {
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    progNum = 2;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    serialPrint("@" + addr2 + addr1 + "NOKSS");
    return true;
  }

  if (rest.startsWith("PRGQR")) {
    xTaskCreatePinnedToCore(loopQrCodeDetect, "qrCodeDetectTask", 40 * 1024, NULL, 5, NULL, 1);
  }

  serialPrint("@" + addr2 + addr1 + "OKSS");
  return true;
}

bool loopNoProg(const String& addr1, const String& addr2, const String& rest) {
  return false;
}

void setup()
{
  Serial.begin(9600);

  if (!psramFound())
  {
    Serial.println("No psram");
    return;
  }

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);

  serialPrint("READY");

  while ((progNum != 1) && (progNum !=2))
    cmdProtocolFunc(loopProgSelection);
  serialPrint("AFTER SELECTION");
}

void loop()
{
  if ((progNum == 1) or (progNum == 2)) {
    cmdProtocolFunc(wifiCommands);
  } else {
    cmdProtocolFunc(loopNoProg);
  }
  delay(100);
}
