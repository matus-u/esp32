#include "ESP32QRCodeReader.h"
#include <WiFi.h>
#include "Arduino.h"
#include <Preferences.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void startCameraServer(QueueHandle_t codeQueue);
void stopCameraServer();
void qrCodeDetectTask();
static camera_config_t config;

static ESP32QRCodeReader *reader = NULL;
static QueueHandle_t qrCodeQueue = NULL;

Preferences preferences;
static int address485 = 0x30;

String countCheckSumAsString(const String& str)
{
  int sum = 0;
  for (int i = 0; i< str.length(); i++)
  {
    sum+= (int) str.charAt(i);
  }
  sum = sum % 256;
  char buf [3];
  sprintf(buf, "%02X", sum);
  String countedStrChecksum;
  countedStrChecksum += buf[0];
  countedStrChecksum += buf[1];
  return countedStrChecksum;
}

bool checkCheckSum(const String &cmd)
{
  if (cmd.length()<=2)
    return false;

  String checkSum = cmd.substring(cmd.length() -3, cmd.length() -1);
  if (checkSum == "SS")
    return true;

  return countCheckSumAsString(cmd.substring(0, cmd.length() - 3)) == checkSum;
}

bool checkAddress(String addrs)
{
  addrs.toLowerCase();
  if (addrs == "fe")
    return true;
  return ((int) strtol(addrs.c_str(), 0, 16)) == address485;
}

bool setAddress(const String &restValue)
{
  if (restValue.length() <= 4)
    return false;

  String stringAddress = restValue.substring(2, restValue.length()-3);

  int a = stringAddress.toInt();
  if (a == 0)
    return false;

  address485 = preferences.putInt("485address", a);
  address485 = a;
  return true;
}

void serialPrint(String data){
  digitalWrite(2, HIGH);
  delayMicroseconds(100);
  Serial.println(data+countCheckSumAsString(data));
  delayMicroseconds(10);
  digitalWrite(2, LOW);
}

void loopQrCodeDetect(void* taskData)
{

  reader = new ESP32QRCodeReader();

  camera_fb_t *fb = NULL;
  while (true)
  {
    if (xQueueReceive(qrCodeQueue, &fb, (TickType_t)pdMS_TO_TICKS(100)))
    {
      Serial.println("DEQUEUE");
      if (reader->qrCodeDetectTask(&config, fb))
      {
        //serialPrint("DETECTED:");
        serialPrint((const char *)reader->buffer);
      }
      free(fb->buf);
      free(fb);
      fb = NULL;
    }
  }
}

static int wifi_enabled = 0;

void cmdProtocolFunc(bool (*handler_func)(const String&, const String&, const String&)) {
  String code = Serial.readStringUntil('\n');
  if (code.startsWith("@")) {
    String addr1 = code.substring(1,3);
    String addr2 = code.substring(3,5);

    if (!checkCheckSum(code)) 
    {
      serialPrint("@" + addr2 + addr1 + "ERR-CHECKSUM");
      return;
    }

    String rest = code.substring(5);

    if (!checkAddress(addr2)) 
    {
      return;
    }

    if (rest.startsWith("?")) {
      serialPrint("@" + addr2 + addr1 + "=ESP32C_1.0");
    } else if (rest.startsWith("A=") && setAddress(rest)) {
      serialPrint("@" + addr2 + addr1 + "OK");
    } else if (rest.startsWith("A?")) {
      serialPrint("@" + addr2 + addr1 + "A=" + String(address485, DEC));
    } else if (handler_func(addr1, addr2, rest)) {
      return;
    } else {
      serialPrint("@" + addr2 + addr1 + "NOK");
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
      if (i == 40) {
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

String parseSSID(const String& rest)
{
  int indexOf = rest.indexOf("|");
  if (indexOf >= 5)
    return rest.substring(5,indexOf);
  return "";
}

String parsePassword(const String& rest)
{
  int indexOf = rest.indexOf("|");
  int lastIndex = rest.lastIndexOf("|");
  if ((indexOf >= 5) && (indexOf < lastIndex)) {
    return rest.substring(indexOf+1, lastIndex);
  }
  return "";
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

    String ssid = parseSSID(rest);
    String password = parsePassword(rest);
    if (ssid == "" || !wifiConnect(ssid, password)) {
      serialPrint("@" + addr2 + addr1 + "NOK");
    } else {
      startCameraServer(qrCodeQueue);
      delay(100);
      wifi_enabled = 1;
      serialPrint("@" + addr2 + addr1 + "OK");
    }
  } else if (rest.startsWith("WIOFF")) {
    if (wifi_enabled == 1) {
      stopCameraServer();
      wifiDisconnect();
    }
    serialPrint("@" + addr2 + addr1 + "OK");
    wifi_enabled = 0;
  } else if (rest.startsWith("WSTAT?")) {
    if (wifi_enabled == 1) {
      IPAddress IP = wifiIp();
      serialPrint("@" + addr2 + addr1 + "WSTAT=" + IP.toString());
    } else {
      serialPrint("@" + addr2 + addr1 + "WSTAT=NOK");
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
    config.jpeg_quality = 0;
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
    serialPrint("@" + addr2 + addr1 + "NOK");
    return true;
  }

  if (rest.startsWith("PRGQR")) {
    qrCodeQueue = xQueueCreate(10, sizeof(camera_fb_t*));
    xTaskCreatePinnedToCore(loopQrCodeDetect, "qrCodeDetectTask", 40 * 1024, NULL, 5, NULL, 1);
  }

  serialPrint("@" + addr2 + addr1 + "OK");
  return true;
}

bool loopNoProg(const String& addr1, const String& addr2, const String& rest) {
  return false;
}

void setup()
{
  Serial.begin(9600);

  preferences.begin("qr-app", false);
  address485 = preferences.getInt("485address", 0x30);

  if (!psramFound())
  {
    Serial.println("No psram");
    return;
  }

  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);

  while ((progNum != 1) && (progNum !=2))
    cmdProtocolFunc(loopProgSelection);
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
