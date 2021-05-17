#include "ESP32QRCodeReader.h"
#include "Arduino.h"

ESP32QRCodeReader::ESP32QRCodeReader()
{
  this->old_width = 0;
  this->old_height = 0;
  this->q = quirc_new();
  if (this->q == NULL)
  {
      Serial.println("can't create quirc object");
  }
}

bool ESP32QRCodeReader::qrCodeDetectTask(camera_config_t* camera_config, camera_fb_t *fb)
{
  if (camera_config->frame_size > FRAMESIZE_SVGA)
  {
    Serial.println("Camera Size err");
    return false;
  }

  uint8_t *image = NULL;

  if (old_width != fb->width || old_height != fb->height)
  {
    Serial.printf("Recognizer size change w h len: %d, %d, %d \r\n", fb->width, fb->height, fb->len);
    Serial.println("Resize the QR-code recognizer.");
    // Resize the QR-code recognizer.
    if (quirc_resize(this->q, fb->width, fb->height) < 0)
    {
      Serial.println("Resize the QR-code recognizer err (cannot allocate memory).");
      return false;
    }
    else
    {
      Serial.println("Resize the QR-code success");
      this->old_width = fb->width;
      this->old_height = fb->height;
    }
  }

  image = quirc_begin(this->q, NULL, NULL);
  memcpy(image, fb->buf, fb->len);
  quirc_end(this->q);

  int count = quirc_count(this->q);
  if (count == 0)
  {
    Serial.printf("Error: not a valid qrcode\n");
    return false;
  }

  struct quirc_code code;
  struct quirc_data data;
  quirc_decode_error_t err;

  quirc_extract(this->q, 0, &code);
  err = quirc_decode(&code, &data);

  if (err)
  {
    const char *error = quirc_strerror(err);
    Serial.printf("Decoding FAILED: %s\n", error);
    return false;
  }

  Serial.printf("Decoding successful:\n");

  for (int i = 0; i < data.payload_len; i++)
  {
    this->buffer[i] = data.payload[i];
  }

  this->buffer[data.payload_len] = '\0';

  return true;
}
