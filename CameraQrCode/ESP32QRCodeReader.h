#ifndef ESP32_QR_CODE_ARDUINO_H_
#define ESP32_QR_CODE_ARDUINO_H_

#include "quirc.h"
#include "esp_camera.h"

class ESP32QRCodeReader
{
public:

  // Constructor
  ESP32QRCodeReader();
  ~ESP32QRCodeReader() { 
    quirc_destroy(this->q);
  }

  uint8_t buffer[1024];

  bool qrCodeDetectTask(camera_config_t* camera_config, camera_fb_t *fb);

private:
  struct quirc *q = NULL;
  uint16_t old_width = 0;
  uint16_t old_height = 0;

};

#endif // ESP32_QR_CODE_ARDUINO_H_
