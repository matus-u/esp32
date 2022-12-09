//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial
#include "MyBluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


BluetoothSerial SerialBT;

long lastDiscoverableTime = -1;

void serialPrint(String data) {
  Serial.println(data);
}


void handleBtDisc()
{
   lastDiscoverableTime = millis();
   SerialBT.setDiscoverable();
   digitalWrite(16, HIGH);
}

String readPin() {
  String pin = "";
  while (pin == "") {
    delay(20);
    serialPrint("@W?");
    String code = Serial.readStringUntil('\n');

    if ((code.length() >= 9) && (code.startsWith("@W="))) {
        pin = code.substring(3,9);
    }
  }
  return pin;
}

void cmdProtocolFunc() {
  String code = Serial.readStringUntil('\n');
  if (code.startsWith("@Q")) {
    handleBtDisc();
  } else {
    SerialBT.write(reinterpret_cast<const unsigned char *>(code.c_str()), code.length());
    SerialBT.write('\n');
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  String pin = readPin();
  SerialBT.begin("ESP32"); //Bluetooth device name
  SerialBT.setPin(pin.c_str());
  SerialBT.setUndiscoverable();
}

long count = 1;

void loop() {

  digitalWrite(16, LOW);
  count++;
  
  if (((lastDiscoverableTime > 0) && (lastDiscoverableTime + 120000 < millis())) || (lastDiscoverableTime < 0 && ((count % 1000) == 0))) {
    lastDiscoverableTime = -1;
    SerialBT.setUndiscoverable();
    digitalWrite(16, HIGH);
    count = 1;
  }


  if (Serial.available()) {
    cmdProtocolFunc();
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(10);
}
