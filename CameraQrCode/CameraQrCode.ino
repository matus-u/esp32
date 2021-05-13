#include <Arduino.h>
#include "ESP32QRCodeReader.h"
#include <WiFi.h>

ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
//ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER, FRAMESIZE_SVGA);

void startCameraServer();

void onQrCodeTask(void *pvParameters)
{
  struct QRCodeData qrCodeData;

  while (true)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))
    {
      Serial.println("Found QRCode");
      if (qrCodeData.valid)
      {
        Serial.print("Payload: ");
        Serial.println((const char *)qrCodeData.payload);
      }
      else
      {
        Serial.print("Invalid: ");
        Serial.println((const char *)qrCodeData.payload);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  /*
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
  */

  reader.setup();

  Serial.println("Setup QRCode Reader");

  //reader.setDebug(true);
  reader.beginOnCore(1);

  Serial.println("Begin on Core 1");

  xTaskCreate(onQrCodeTask, "onQrCode", 4 * 1024, NULL, 4, NULL);

  
}

void loop()
{
  delay(100);
}
