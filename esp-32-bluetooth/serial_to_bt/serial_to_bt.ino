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

const int DISCOVERABLE_LOGIC_PIN = 5;
const int LED_PIN = 2;

void setBtUndis()
{
  if (digitalRead(DISCOVERABLE_LOGIC_PIN) == LOW)
  {
    SerialBT.setUndiscoverable();
    digitalWrite(LED_PIN, HIGH);
  }
}
void setBtDis()
{
   lastDiscoverableTime = millis();
   if (digitalRead(DISCOVERABLE_LOGIC_PIN) == LOW) {
     SerialBT.setDiscoverable();
     digitalWrite(LED_PIN, LOW);
   }
}

void initializeBluetooth() {
  String pin = "";
  String blName = "";
  while (pin == "" || blName == "") {
    delay(20);
    serialPrint("@W?");
    String code = Serial.readStringUntil('\n');

    if ((code.length() > 5) && (code.startsWith("@W="))) {
        int firstIndex = code.indexOf(",");
        int lastIndex = code.lastIndexOf(",");
        if ((firstIndex > 3) && (firstIndex < lastIndex)) {
          blName = code.substring(3,firstIndex);
          pin = code.substring(firstIndex+1, lastIndex);
        }

    }
  }
  
  SerialBT.begin(blName.c_str()); //Bluetooth device name
  SerialBT.setPin(pin.c_str());
}

void cmdProtocolFunc() {
  String code = Serial.readStringUntil('\n');
  if (code.startsWith("@Q0")) {
    setBtDis();
  } else if (code.startsWith("@Q1")) {
    ESP.restart();
  } else {
    SerialBT.write(reinterpret_cast<const unsigned char *>(code.c_str()), code.length());
    SerialBT.write('\n');
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(DISCOVERABLE_LOGIC_PIN, INPUT);
  
  digitalWrite(LED_PIN, LOW);
  initializeBluetooth();

  setBtUndis();
  
}

long count = 1;

void loop() {
  
  count++;
  
  if (((lastDiscoverableTime > 0) && (lastDiscoverableTime + 180000 < millis())) || (lastDiscoverableTime < 0 && ((count % 10000) == 0))) {
    lastDiscoverableTime = -1;
    setBtUndis();
    count = 1;
  }


  if (Serial.available()) {
    cmdProtocolFunc();
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(2);
}
