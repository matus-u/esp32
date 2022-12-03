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

bool checkAddress(String myAddr, String clientAddr)
{
  clientAddr.toLowerCase();
  myAddr.toLowerCase();
  if (myAddr == "30" && clientAddr == "00")
    return true;
  return false;
}

void serialPrint(String data){
  Serial.println(data+countCheckSumAsString(data));
}


bool handleBtDisc(const String& addr1, const String& addr2, const String& rest)
{
  if (rest.startsWith("Q")) {
   lastDiscoverableTime = millis();
   SerialBT.setDiscoverable();
   serialPrint("@" + addr2 + addr1 + "OK");
   
   return true;
  }
  return false;
}

String readPin() {
  String pin = "";
  while (pin == "") {
    delay(20);
    serialPrint("@0030W?");
    String code = Serial.readStringUntil('\n');

    if ((code.length() >= 15) && (code.startsWith("@3000W="))) {
      if (!checkCheckSum(code)) 
      {
        serialPrint("@0030ERR-CHECKSUM");
      } else {
        pin = code.substring(7,13);
      }
    }
  }
  return pin;
}

void cmdProtocolFunc() {
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

    if (!checkAddress(addr1, addr2)) 
    {
      SerialBT.write(reinterpret_cast<const unsigned char *>(code.c_str()), code.length());
      SerialBT.write('\n');
      return;
    }

    handleBtDisc(addr1, addr2, rest);
  }
}

void setup() {

  Serial.begin(9600);

  String pin = readPin();
  SerialBT.begin("ESP32"); //Bluetooth device name
  SerialBT.setPin(pin.c_str());
  SerialBT.setUndiscoverable();
}

long count = 1;

void loop() {

  count++;
  
  if (((lastDiscoverableTime > 0) && (lastDiscoverableTime + 120000 < millis())) || (lastDiscoverableTime < 0 && ((count % 500) == 0))) {
    lastDiscoverableTime = -1;
    SerialBT.setUndiscoverable();
  }


  if (Serial.available()) {
    cmdProtocolFunc();
  }
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20);
}
