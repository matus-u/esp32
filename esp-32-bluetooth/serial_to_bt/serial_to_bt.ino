//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include <WiFi.h>
#include "MyBluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void setup() {
  String wifiMacString = WiFi.macAddress();
  wifiMacString.remove(0,9);
  wifiMacString.remove(2,1);
  wifiMacString.remove(4,1);
  
  
  Serial.begin(9600);
  Serial.println("");

  Serial.println(wifiMacString);

  SerialBT.begin("ESP32test"); //Bluetooth device name
  SerialBT.setPin(wifiMacString.c_str());
  Serial.println("The device started, now you can pair it with bluetooth!");
}

static int count = 0;
static bool discoverable = true;

void loop() {
  count += 1;
  if (count > 6000)
  {
    if (discoverable) {
       Serial.println("setting undiscoverable ");
       SerialBT.setUndiscoverable();
       count = 0;
       discoverable = false;
    } else {
       Serial.println("setting discoverable ");
       SerialBT.setDiscoverable();
       count = 0;
       discoverable = true;
    }
  }
  
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20);
}
