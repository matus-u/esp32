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
   //digitalWrite(16, HIGH);
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
    handleBtDisc();
  } else if (code.startsWith("@Q0")) {
    ESP.restart();
  } else {
    SerialBT.write(reinterpret_cast<const unsigned char *>(code.c_str()), code.length());
    SerialBT.write('\n');
  }
}

void setup() {

  Serial.begin(9600);

  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  initializeBluetooth();

  SerialBT.setUndiscoverable();
}

long count = 1;
long delayCount = 0;

void loop() {

  //digitalWrite(2, LOW);
  delayCount++;
  if (delayCount == 100)
  {
    digitalWrite(2,LOW);
  }
  if( delayCount == 200)
  {
    digitalWrite(2,HIGH);
    delayCount = 0;
  }
  
  count++;
  
  if (((lastDiscoverableTime > 0) && (lastDiscoverableTime + 120000 < millis())) || (lastDiscoverableTime < 0 && ((count % 10000) == 0))) {
    lastDiscoverableTime = -1;
    SerialBT.setUndiscoverable();
    //digitalWrite(16, HIGH);
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
